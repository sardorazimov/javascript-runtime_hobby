/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Implements timers and dispatches queued callbacks on the JS thread.
 */
#include "event_loop.h"

#include <algorithm>
#include <iostream>
#include <thread>

#include "binding.h"

namespace mininode {
namespace {

EventLoop* GetLoop(const v8::FunctionCallbackInfo<v8::Value>& info) {
   return static_cast<EventLoop*>(info.Data().As<v8::External>()->Value(
      v8::kExternalPointerTypeTagDefault));
}

}

EventLoop::EventLoop(v8::Isolate* isolate) : isolate_(isolate) {}

EventLoop::~EventLoop() {
   timers_.clear();
}

void EventLoop::Install(v8::Local<v8::Context> context) {
   auto data = v8::External::New(
      isolate_, this, v8::kExternalPointerTypeTagDefault);
   auto set_timeout = v8::Function::New(context, SetTimeout, data).ToLocalChecked();
   auto clear_timeout =
      v8::Function::New(context, ClearTimeout, data).ToLocalChecked();
   context->Global()->Set(context, ToV8String(isolate_, "setTimeout"), set_timeout)
      .Check();
   context->Global()
      ->Set(context, ToV8String(isolate_, "clearTimeout"), clear_timeout)
      .Check();
}

void EventLoop::SetTimeout(const v8::FunctionCallbackInfo<v8::Value>& info) {
   auto* isolate = info.GetIsolate();
   if (info.Length() == 0 || !info[0]->IsFunction()) {
      isolate->ThrowException(v8::Exception::TypeError(
         ToV8String(isolate, "setTimeout expects a callback")));
      return;
   }
   const auto delay = info.Length() > 1
                     ? info[1]->Int32Value(isolate->GetCurrentContext()).FromMaybe(0)
                     : 0;
   const int timer_id = GetLoop(info)->AddTimer(
      info[0].As<v8::Function>(), std::max(0, delay));
   info.GetReturnValue().Set(v8::Integer::New(isolate, timer_id));
}

void EventLoop::ClearTimeout(const v8::FunctionCallbackInfo<v8::Value>& info) {
   if (info.Length() > 0) {
      GetLoop(info)->RemoveTimer(
         info[0]->Int32Value(info.GetIsolate()->GetCurrentContext()).FromMaybe(0));
   }
}

int EventLoop::AddTimer(v8::Local<v8::Function> callback, int delay_ms) {
   const int id = next_timer_id_++;
   timers_.push_back({id, std::chrono::steady_clock::now() +
                         std::chrono::milliseconds(delay_ms),
                  v8::Global<v8::Function>(isolate_, callback)});
   return id;
}

void EventLoop::RemoveTimer(int id) {
   const auto timer = std::find_if(timers_.begin(), timers_.end(),
                           [id](const Timer& item) {
                              return item.id == id;
                           });
   if (timer != timers_.end()) {
      timers_.erase(timer);
   }
}

void EventLoop::Post(std::function<void()> task) {
   {
      std::lock_guard lock(mutex_);
      tasks_.push(std::move(task));
   }
   condition_.notify_one();
}

void EventLoop::Ref() {
   active_references_.fetch_add(1, std::memory_order_relaxed);
   condition_.notify_one();
}

void EventLoop::Unref() {
   active_references_.fetch_sub(1, std::memory_order_relaxed);
   condition_.notify_one();
}

void EventLoop::Run(v8::Local<v8::Context> context) {
   while (true) {
      std::function<void()> task;
      {
         std::unique_lock lock(mutex_);
         if (tasks_.empty()) {
            const auto next = std::min_element(
               timers_.begin(), timers_.end(),
               [](const Timer& left, const Timer& right) {
                  return left.due < right.due;
               });
            if (next == timers_.end() &&
               active_references_.load(std::memory_order_relaxed) == 0) {
               return;
            }
            if (next == timers_.end()) {
               condition_.wait(lock, [this] {
                  return !tasks_.empty() ||
                        active_references_.load(std::memory_order_relaxed) == 0;
               });
            } else {
               condition_.wait_until(lock, next->due,
                                [this] { return !tasks_.empty(); });
            }
         }
         if (!tasks_.empty()) {
            task = std::move(tasks_.front());
            tasks_.pop();
         }
      }

      if (task) {
         v8::HandleScope handle_scope(isolate_);
         v8::TryCatch try_catch(isolate_);
         task();
         isolate_->PerformMicrotaskCheckpoint();
         if (try_catch.HasCaught()) {
            v8::String::Utf8Value message(isolate_, try_catch.Exception());
            std::cerr << "MiniNode callback error: "
                    << (*message == nullptr ? "JavaScript error" : *message)
                    << '\n';
         }
         continue;
      }

      const auto next = std::min_element(
         timers_.begin(), timers_.end(),
         [](const Timer& left, const Timer& right) { return left.due < right.due; });
      if (next == timers_.end() || next->due > std::chrono::steady_clock::now()) {
         continue;
      }
      auto callback = next->callback.Get(isolate_);
      timers_.erase(next);

      v8::HandleScope handle_scope(isolate_);
      v8::TryCatch try_catch(isolate_);
      v8::Local<v8::Value> result;
      const bool callback_succeeded =
         callback->Call(context, context->Global(), 0, nullptr).ToLocal(&result);
      isolate_->PerformMicrotaskCheckpoint();
      if (try_catch.HasCaught()) {
         v8::String::Utf8Value message(isolate_, try_catch.Exception());
         std::cerr << "MiniNode timer error: "
                 << (*message == nullptr ? "JavaScript error" : *message)
                 << '\n';
      }
            static_cast<void>(callback_succeeded);
   }
}

}