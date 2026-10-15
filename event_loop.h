#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>

#include <v8.h>

namespace mininode {

class EventLoop {
public:
   explicit EventLoop(v8::Isolate* isolate);
   ~EventLoop();

   void Install(v8::Local<v8::Context> context);
   void Run(v8::Local<v8::Context> context);
   void Post(std::function<void()> task);
   void Ref();
   void Unref();

private:
   struct Timer {
      int id;
      std::chrono::steady_clock::time_point due;
      v8::Global<v8::Function> callback;
   };

   static void SetTimeout(const v8::FunctionCallbackInfo<v8::Value>& info);
   static void ClearTimeout(const v8::FunctionCallbackInfo<v8::Value>& info);
   int AddTimer(v8::Local<v8::Function> callback, int delay_ms);
   void RemoveTimer(int id);

   v8::Isolate* isolate_;
   int next_timer_id_ = 1;
   std::vector<Timer> timers_;
   std::mutex mutex_;
   std::condition_variable condition_;
   std::queue<std::function<void()>> tasks_;
   std::atomic<int> active_references_{0};
};

}