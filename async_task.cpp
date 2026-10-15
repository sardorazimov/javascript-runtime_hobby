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
   {
      std::lock_guard lock(mutex_);
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