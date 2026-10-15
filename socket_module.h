/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Declares the raw TCP socket experiment exposed to JavaScript.
 */
#pragma once

#include <v8.h>

namespace mininode::SocketModule {

v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context);
void Install(v8::Isolate* isolate, v8::Local<v8::Context> context);

}