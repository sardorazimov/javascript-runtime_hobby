#pragma once

#include <iostream>

#include <v8.h>

#include "binding.h"

namespace mininode::PromiseTracker {

inline void Install(v8::Isolate* isolate) {
   isolate->SetPromiseRejectCallback([](v8::PromiseRejectMessage message) {
      if (message.GetEvent() != v8::kPromiseRejectWithNoHandler) {
         return;
      }
      auto* current_isolate = message.GetPromise()->GetIsolate();
      v8::String::Utf8Value reason(current_isolate, message.GetValue());
      std::cerr << "MiniNode unhandled promise rejection: "
              << (*reason == nullptr ? "unknown rejection" : *reason) << '\n';
   });
}

}