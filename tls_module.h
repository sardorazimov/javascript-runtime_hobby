/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Declares the HTTPS-only request API with certificate verification.
 */
#pragma once

#include <v8.h>

namespace mininode::TlsModule {

v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context);
void Install(v8::Isolate* isolate, v8::Local<v8::Context> context);

}