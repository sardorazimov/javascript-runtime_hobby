/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Exposes the TCP request primitive under an explicit socket module.
 */
#include "socket_module.h"

#include "binding.h"
#include "net_module.h"

namespace mininode::SocketModule {

v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context) {
   auto* isolate = v8::Isolate::GetCurrent();
   auto module = v8::Object::New(isolate);
   auto request = v8::Function::New(context, NetModule::TcpRequest).ToLocalChecked();
   module->Set(context, ToV8String(isolate, "requestSync"), request).Check();
   return module;
}

void Install(v8::Isolate*, v8::Local<v8::Context> context) {
   auto module = CreateObject(context);
   context->Global()->Set(context,
                          ToV8String(v8::Isolate::GetCurrent(), "socket"), module)
      .Check();
}

}