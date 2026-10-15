#pragma once

#include <v8.h>

namespace mininode::ConsoleModule {

void Install(v8::Isolate* isolate, v8::Local<v8::Context> context);
v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context);

}
/* console_module.cpp: JavaScript'ten çağrilan console.log(), 
   console.error() gibi komutları yakalayıp C++ standard çıktısına (std::cout) yazar. */