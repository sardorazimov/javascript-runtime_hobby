/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Implements bounded synchronous HTTP requests and response objects.
 */
#include "http_module.h"

#include <algorithm>
#include <mutex>

#include <curl/curl.h>

#include "binding.h"

namespace mininode::HttpModule {
namespace {

constexpr std::size_t kMaximumResponseBytes = 2 * 1024 * 1024;
std::once_flag curl_initialization;
CURLcode curl_initialization_result = CURLE_FAILED_INIT;

struct ResponseBuffer {
   std::string body;
   bool exceeded_limit = false;
};

std::size_t ReceiveBody(char* data, std::size_t size, std::size_t count,
                        void* user_data) {
   const auto bytes = size * count;
   auto* response = static_cast<ResponseBuffer*>(user_data);
   if (bytes > kMaximumResponseBytes - response->body.size()) {
      response->exceeded_limit = true;
      return 0;
   }
   response->body.append(data, bytes);
   return bytes;
}

void Throw(v8::Isolate* isolate, const std::string& message) {
   isolate->ThrowException(v8::Exception::Error(ToV8String(isolate, message)));
}

void SetResponse(v8::Local<v8::Context> context,
                 const v8::FunctionCallbackInfo<v8::Value>& info,
                 bool require_tls, const std::string& method) {
   auto* isolate = info.GetIsolate();
   if (info.Length() < 1 || !info[0]->IsString()) {
      Throw(isolate, "request expects a URL string");
      return;
   }
   const auto url = ToStdString(isolate, info[0]);
   const bool is_https = url.starts_with("https://");
   const bool is_http = url.starts_with("http://");
   if ((!is_http && !is_https) || (require_tls && !is_https)) {
      Throw(isolate, require_tls ? "TLS requests require an https:// URL"
                                : "Only http:// and https:// URLs are supported");
      return;
   }

   const auto body = info.Length() > 2 ? ToStdString(isolate, info[2]) : "";
   const auto response = PerformRequest(url, method, body, require_tls);
   if (!response.error.empty()) {
      Throw(isolate, response.error);
      return;
   }

   auto result = v8::Object::New(isolate);
   result->Set(context, ToV8String(isolate, "statusCode"),
               v8::Integer::New(isolate, static_cast<int>(response.status_code)))
      .Check();
   result->Set(context, ToV8String(isolate, "ok"),
               v8::Boolean::New(isolate, response.status_code >= 200 &&
                                            response.status_code < 300))
      .Check();
   result->Set(context, ToV8String(isolate, "body"),
               ToV8String(isolate, response.body))
      .Check();
   info.GetReturnValue().Set(result);
}

void GetSync(const v8::FunctionCallbackInfo<v8::Value>& info) {
   SetResponse(info.GetIsolate()->GetCurrentContext(), info,
               info.Data().As<v8::Boolean>()->Value(), "GET");
}

void RequestSync(const v8::FunctionCallbackInfo<v8::Value>& info) {
   auto* isolate = info.GetIsolate();
   const auto method = info.Length() > 1 ? ToStdString(isolate, info[1]) : "GET";
   SetResponse(isolate->GetCurrentContext(), info,
               info.Data().As<v8::Boolean>()->Value(), method);
}

}

Response PerformRequest(const std::string& url, const std::string& method,
                        const std::string& body, bool require_tls) {
   std::call_once(curl_initialization, [] {
      curl_initialization_result = curl_global_init(CURL_GLOBAL_DEFAULT);
   });
   if (curl_initialization_result != CURLE_OK) {
      return {0, {}, "Could not initialize libcurl"};
   }

   CURL* curl = curl_easy_init();
   if (curl == nullptr) {
      return {0, {}, "Could not create an HTTP request"};
   }

   ResponseBuffer response_buffer;
   char error_buffer[CURL_ERROR_SIZE]{};
   curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
   curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, require_tls ? "https" : "http,https");
   curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR,
                    require_tls ? "https" : "http,https");
   curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
   curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 3L);
   curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 5000L);
   curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 10000L);
   curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
   curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, error_buffer);
   curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, ReceiveBody);
   curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_buffer);
   curl_easy_setopt(curl, CURLOPT_USERAGENT, "MiniNode-Learning-Runtime/0.1");
   if (method != "GET") {
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());
      if (!body.empty()) {
         curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.data());
         curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE_LARGE,
                          static_cast<curl_off_t>(body.size()));
      }
   }

   const auto result = curl_easy_perform(curl);
   long status_code = 0;
   curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);
   std::string error;
   if (response_buffer.exceeded_limit) {
      error = "HTTP response exceeded the 2 MiB limit";
   } else if (result != CURLE_OK) {
      error = error_buffer[0] != '\0' ? error_buffer : curl_easy_strerror(result);
   }
   curl_easy_cleanup(curl);
   return {status_code, std::move(response_buffer.body), std::move(error)};
}

v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context,
                                    bool require_tls) {
   auto* isolate = v8::Isolate::GetCurrent();
   auto module = v8::Object::New(isolate);
   auto mode = v8::Boolean::New(isolate, require_tls);
   auto get = v8::Function::New(context, GetSync, mode).ToLocalChecked();
   auto request = v8::Function::New(context, RequestSync, mode).ToLocalChecked();
   module->Set(context, ToV8String(isolate, "getSync"), get).Check();
   module->Set(context, ToV8String(isolate, "requestSync"), request).Check();
   return module;
}

void Install(v8::Isolate*, v8::Local<v8::Context> context) {
   auto module = CreateObject(context);
   context->Global()->Set(context, ToV8String(v8::Isolate::GetCurrent(), "http"),
                          module)
      .Check();
}

}