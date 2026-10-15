/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Exposes HTTPS-only GET and request helpers through the HTTP client.
 */
#include "tls_module.h"

#include "binding.h"
#include "http_module.h"

namespace mininode::TlsModule {

v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context) {
   return HttpModule::CreateObject(context, true);
}

void Install(v8::Isolate*, v8::Local<v8::Context> context) {
   auto module = CreateObject(context);
   context->Global()->Set(context, ToV8String(v8::Isolate::GetCurrent(), "tls"),
                          module)
      .Check();
}

}