/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Implements the background worker pool used for blocking I/O tasks.
 */
#include "async_task.h"

#include <stdexcept>

namespace mininode {

AsyncTaskPool::AsyncTaskPool(std::size_t worker_count) {
   if (worker_count == 0) {
      worker_count = 1;
   }
   workers_.reserve(worker_count);
   for (std::size_t index = 0; index < worker_count; ++index) {
      workers_.emplace_back([this] { Worker(); });
   }
}

AsyncTaskPool::~AsyncTaskPool() {
   Shutdown();
}

void AsyncTaskPool::Shutdown() {
   {
      std::lock_guard lock(mutex_);
      if (stopping_) {
         return;
      }
      stopping_ = true;
   }
   condition_.notify_all();
   for (auto& worker : workers_) {
      if (worker.joinable()) {
         worker.join();
      }
   }
}

void AsyncTaskPool::Submit(std::function<void()> task) {
   {
      std::lock_guard lock(mutex_);
      if (stopping_) {
         throw std::runtime_error("Task pool is stopping");
      }
      tasks_.push(std::move(task));
   }
   condition_.notify_one();
}
void AsyncTaskPool::Worker() {
   while (true) {
      std::function<void()> task;
      {
         std::unique_lock lock(mutex_);
         condition_.wait(lock, [this] { return stopping_ || !tasks_.empty(); });
         if (stopping_ && tasks_.empty()) {
            return;
         }
         task = std::move(tasks_.front());
         tasks_.pop();
      }
      try {
         task();
      } catch (...) {
      }
   }
}

}

/*
Destruction sequence (most critical issue): The task pool may still be active while `event_loop_.reset()` is executing within the `~Runtime` destructor. `fs_async_context_.event_loop` is a raw pointer. If a script encounters an error and `RunFile` returns early—meaning `succeeded == false` and the event loop never runs—there could still be an `fs.readFile` operation pending or in progress within the pool. Once that operation completes, it attempts to post a result to an already-destroyed event loop, leading to a "use-after-free" error. The solution is to shut down the pool first:
cpp
void AsyncTaskPool::Shutdown() {
{
std::lock_guard lock(mutex_); 
stopping_ = true; 
}
condition_.notify_all(); 
for (auto& w : workers_) if (w.joinable()) w.join(); 
}
// At the beginning of ~Runtime:
async_tasks_.Shutdown(); 
event_loop_.reset();

Move the logic currently inside the destructor into `Shutdown()`, and have the destructor call `Shutdown()`. 
`Shutdown()` must be safe to call multiple times (subsequent calls are inherently safe because `joinable()` will return `false`).

`v8::Global` must not be destroyed on a worker thread: Within the `Worker()` 
loop, a task is destroyed at the end of each iteration.
 If a lambda in `fs_module.cpp` captures a `v8::Global` (or a `shared_ptr` 
 holding one), the final reference will be dropped on the worker thread; 
this causes the `v8::Global` destructor to access the `v8::Isolate` from a thread other than the main thread,
resulting in a data race. Rule: Lambdas passed to the worker should only capture plain data (e.g., path, buffer, error code).
The callback itself should be stored in a table on the main thread (mapping an ID to a `v8::Global<v8::Function>`),
with the worker thread only passing the ID. Review the calls to `Submit` in `fs_module.cpp` accordingly.
`catch (...) {}` swallows exceptions silently: If a task throws an exception on a worker thread, 
it goes unnoticed, the callback is never invoked, and the program may end up waiting indefinitely within the event loop.
 At the very least, log using `std::cerr`; a better approach would be for the task itself to catch the error and report it back to the main thread as an "error result."
The outer `TryCatch` swallows errors from the event loop: `RunFile` defines a `TryCatch`, and `event_loop_->Run` resides within that scope. 
Since `TryCatch` is not set to "verbose," any JS exception thrown within a timer or filesystem callback is caught but never reported. 
Instead, place a `TryCatch` around `RunMain` and move the event loop outside of it. Furthermore, 
ensure that every callback invocation within the `EventLoop` has its own `TryCatch` to capture and log any errors.
Accumulation of `HandleScope` objects: The entire program runs within a single, outer 
`HandleScope`. If `EventLoop::Run` does not open a new `HandleScope` for each callback invocation,
 handles will accumulate until the program terminates—which, in a long-running server, would manifest as a memory leak.
Microtasks: The policy is set to `kExplicit`. Currently, `PerformMicrotaskCheckpoint` 
is called only once, after `RunMain`. If the event loop does not invoke it after every callback, any
 `Promise.then` chains resolved within those callbacks will stall. Check whether `EventLoop::Run` triggers a checkpoint after each callback.
Constructor exception safety: If an exception is thrown during the "install" phase—after `isolate_`
 has been initialized—the destructor will not execute (because the object was never fully constructed), leading to a leak of the `isolate` and V8 resources.
  While not critical for a learning project, it is worth noting. A simple solution is to wrap the initialization in a `try/catch` block, perform the necessary cleanup in the `catch` clause, and then re-throw the exception.
*/