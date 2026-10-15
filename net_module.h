/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Declares the basic TCP networking module exposed to JavaScript.
 */
#pragma once

#include <v8.h>

namespace mininode::NetModule {

void Install(v8::Isolate* isolate, v8::Local<v8::Context> context);
v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context);

}