/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Declares the native Buffer module exposed to JavaScript.
 */
#pragma once

#include <v8.h>

namespace mininode::BufferModule {

void Install(v8::Isolate* isolate, v8::Local<v8::Context> context);
v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context);

}