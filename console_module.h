/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Declares installation and creation functions for the console object.
 */
#pragma once

#include <v8.h>

namespace mininode::ConsoleModule {

void Install(v8::Isolate* isolate, v8::Local<v8::Context> context);
v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context);

}