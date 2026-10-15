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
/* fs_module.cpp: fs.readFile veya fs.writeFileSync gibi temel 
   dosya okuma/yazma işlemlerinin C++ (POSIX / Win32) karşılıklarını işletim sistemine iletir. */