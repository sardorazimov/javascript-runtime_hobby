/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Declares filesystem bindings and state for asynchronous callbacks.
 */
#pragma once

#include <v8.h>

namespace mininode {
class AsyncTaskPool;
class EventLoop;
}

namespace mininode::FsModule {

struct AsyncContext {
   v8::Isolate* isolate = nullptr;
   v8::Global<v8::Context> context;
   AsyncTaskPool* task_pool = nullptr;
   EventLoop* event_loop = nullptr;
};

void Install(v8::Isolate* isolate, v8::Local<v8::Context> context,
          AsyncContext* async_context);
v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context);
v8::Local<v8::Object> CreateObject(v8::Local<v8::Context> context,
                                   AsyncContext* async_context);

}