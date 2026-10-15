/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Provides small conversion helpers between C++ strings and V8 values.
 */
#pragma once

#include <string>
#include <string_view>

#include <v8.h>

namespace mininode {

inline v8::Local<v8::String> ToV8String(v8::Isolate* isolate,
                              std::string_view value) {
   const char* data = value.empty() ? "" : value.data();
   return v8::String::NewFromUtf8(isolate, data,
                           v8::NewStringType::kNormal,
                           static_cast<int>(value.size()))
      .ToLocalChecked();
}

inline std::string ToStdString(v8::Isolate* isolate,
                        v8::Local<v8::Value> value) {
   v8::String::Utf8Value text(isolate, value);
   return *text == nullptr ? std::string{} : std::string(*text, text.length());
}

}