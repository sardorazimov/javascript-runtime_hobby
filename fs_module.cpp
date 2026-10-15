#include "fs_module.h"

#include <filesystem>
#include <exception>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

#include "async_task.h"
#include "binding.h"
#include "event_loop.h"

namespace mininode::FsModule {
namespace {

void ThrowError(v8::Isolate* isolate, const std::string& message) {
   isolate->ThrowException(v8::Exception::Error(ToV8String(isolate, message)));
}

void ReadFileSync(const v8::FunctionCallbackInfo<v8::Value>& info) {
   auto* isolate = info.GetIsolate();
   if (info.Length() < 1) {
      ThrowError(isolate, "readFileSync requires a path");
      return;
   }

   const auto file_path = ToStdString(isolate, info[0]);
   std::ifstream file(file_path, std::ios::binary);
   if (!file) {
      ThrowError(isolate, "Could not read file: " + file_path);
      return;
   }

   std::ostringstream contents;
   contents << file.rdbuf();
   auto result = ToV8String(isolate, contents.str());
   info.GetReturnValue().Set(result);
}

void WriteFileSync(const v8::FunctionCallbackInfo<v8::Value>& info) {
   auto* isolate = info.GetIsolate();
   if (info.Length() < 2) {
      ThrowError(isolate, "writeFileSync requires a path and data");
      return;
   }

   const auto file_path = ToStdString(isolate, info[0]);
   const auto contents = ToStdString(isolate, info[1]);
   std::ofstream file(file_path, std::ios::binary | std::ios::trunc);
   if (!file || !file.write(contents.data(),
                      static_cast<std::streamsize>(contents.size()))) {
      ThrowError(isolate, "Could not write file: " + file_path);
   }
}

void ReadFile(const v8::FunctionCallbackInfo<v8::Value>& info) {
   auto* isolate = info.GetIsolate();
   if (info.Length() < 2 || !info[1]->IsFunction()) {
      ThrowError(isolate, "readFile expects a path and callback");
      return;
   }

   auto* async_context = static_cast<AsyncContext*>(
      info.Data().As<v8::External>()->Value());
   const auto file_path = ToStdString(isolate, info[0]);
   auto callback = std::make_shared<v8::Global<v8::Function>>(
      isolate, info[1].As<v8::Function>());
   async_context->event_loop->Ref();

   try {
      async_context->task_pool->Submit(
         [async_context, callback, file_path] {
            std::ifstream file(file_path, std::ios::binary);
            std::string contents;
            std::string error;
            if (!file) {
               error = "Could not read file: " + file_path;
            } else {
               std::ostringstream stream;
               stream << file.rdbuf();
               contents = stream.str();
               if (file.bad()) {
                  error = "Failed while reading file: " + file_path;
               }
            }

            async_context->event_loop->Post(
               [async_context, callback, contents = std::move(contents),
                error = std::move(error)] {
                  auto* current_isolate = async_context->isolate;
                  v8::HandleScope handle_scope(current_isolate);
                  auto context = async_context->context.Get(current_isolate);
                  v8::Context::Scope context_scope(context);
                  v8::Local<v8::Value> arguments[2];
                  if (error.empty()) {
                     arguments[0] = v8::Null(current_isolate);
                     arguments[1] = ToV8String(current_isolate, contents);
                  } else {
                     arguments[0] = v8::Exception::Error(
                        ToV8String(current_isolate, error));
                     arguments[1] = v8::Undefined(current_isolate);
                  }
                  v8::TryCatch try_catch(current_isolate);
                  callback->Get(current_isolate)
                     ->Call(context, v8::Undefined(current_isolate), 2,
                           arguments);
                  current_isolate->PerformMicrotaskCheckpoint();
                  if (try_catch.HasCaught()) {
                     v8::String::Utf8Value message(
                        current_isolate, try_catch.Exception());
                     std::cerr << "MiniNode fs callback error: "
                             << (*message == nullptr ? "JavaScript error"
                                               : *message)
                             << '\n';
                  }
                  callback->Reset();
                  async_context->event_loop->Unref();
               });
         });
   } catch (const std::exception& error) {
      async_context->event_loop->Unref();
      callback->Reset();
      ThrowError(isolate, error.what());
   }
}

void ExistsSync(const v8::FunctionCallbackInfo<v8::Value>& info) {
   auto* isolate = info.GetIsolate();
   if (info.Length() < 1) {
      info.GetReturnValue().Set(false);
      return;
   }
   std::error_code error;
   const bool exists = std::filesystem::exists(
      ToStdString(isolate, info[0]), error);
   info.GetReturnValue().Set(exists && !error);
}

void AddMethod(v8::Isolate* isolate, v8::Local<v8::Context> context,
            v8::Local<v8::Object> object, const char* name,
            v8::FunctionCallback callback) {
   auto function = v8::Function::New(context, callback).ToLocalChecked();
   object->Set(context, ToV8String(isolate, name), function).Check();
}

}

v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context,
                                    AsyncContext* async_context) {
   auto* isolate = context->GetIsolate();
   auto fs = v8::Object::New(isolate);
   AddMethod(isolate, context, fs, "readFileSync", ReadFileSync);
   AddMethod(isolate, context, fs, "writeFileSync", WriteFileSync);
   AddMethod(isolate, context, fs, "existsSync", ExistsSync);
   auto read_file = v8::Function::New(
                   context, ReadFile, v8::External::New(isolate, async_context))
                   .ToLocalChecked();
   fs->Set(context, ToV8String(isolate, "readFile"), read_file).Check();
   return fs;
}

void Install(v8::Isolate*, v8::Local<v8::Context> context,
          AsyncContext* async_context) {
   auto fs = CreateObject(context, async_context);
   context->Global()->Set(context, ToV8String(context->GetIsolate(), "fs"), fs)
      .Check();
}

}
/* fs_module.cpp: fs.readFile veya fs.writeFileSync gibi temel 
   dosya okuma/yazma işlemlerinin C++ (POSIX / Win32) karşılıklarını işletim sistemine iletir. */