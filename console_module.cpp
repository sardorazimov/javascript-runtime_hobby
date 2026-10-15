#include "console_module.h"

#include <iostream>

#include "binding.h"

namespace mininode::ConsoleModule {
namespace {

void Write(const v8::FunctionCallbackInfo<v8::Value>& info) {
   const auto severity = info.Data().As<v8::Integer>()->Value();
   auto* output = severity > 0 ? &std::cerr : &std::cout;
   for (int index = 0; index < info.Length(); ++index) {
      if (index != 0) {
         *output << ' ';
      }
      *output << ToStdString(info.GetIsolate(), info[index]);
   }
   *output << '\n';
}

void AddMethod(v8::Isolate* isolate, v8::Local<v8::Context> context,
            v8::Local<v8::Object> object, const char* name, int severity) {
   auto data = v8::Integer::New(isolate, severity);
   auto function = v8::Function::New(context, Write, data).ToLocalChecked();
   object->Set(context, ToV8String(isolate, name), function).Check();
}

}

v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context) {
   auto* isolate = context->GetIsolate();
   auto console = v8::Object::New(isolate);
   AddMethod(isolate, context, console, "log", 0);
   AddMethod(isolate, context, console, "info", 0);
   AddMethod(isolate, context, console, "warn", 1);
   AddMethod(isolate, context, console, "error", 2);
   return console;
}

void Install(v8::Isolate*, v8::Local<v8::Context> context) {
   auto console = CreateObject(context);
   context->Global()->Set(context, ToV8String(context->GetIsolate(), "console"),
                     console)
      .Check();
}

}
/* console_module.cpp: JavaScript'ten çağrilan console.log(), 
   console.error() gibi komutları yakalayıp C++ standard çıktısına (std::cout) yazar. */