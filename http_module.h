/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Declares small synchronous HTTP client bindings backed by libcurl.
 */
#pragma once

#include <string>

#include <v8.h>

namespace mininode::HttpModule {

struct Response {
   long status_code = 0;
   std::string body;
   std::string error;
};

Response PerformRequest(const std::string& url, const std::string& method,
                       const std::string& body, bool require_tls);
v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context,
                                    bool require_tls = false);
void Install(v8::Isolate* isolate, v8::Local<v8::Context> context);

}