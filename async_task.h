/*
 * Copyright (c) 2026 Sardor Azimov. Hobby project for learning runtime internals.
 * Purpose: Declares the worker pool that runs blocking tasks off the JS thread.
 */
#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

namespace mininode {

class AsyncTaskPool {
public:
   explicit AsyncTaskPool(std::size_t worker_count = 2);
   ~AsyncTaskPool();

   AsyncTaskPool(const AsyncTaskPool&) = delete;
   AsyncTaskPool& operator=(const AsyncTaskPool&) = delete;

   void Submit(std::function<void()> task);
   void Shutdown();

private:
   void Worker();

   std::mutex mutex_;
   std::condition_variable condition_;
   std::queue<std::function<void()>> tasks_;
   std::vector<std::thread> workers_;
   bool stopping_ = false;
};

}