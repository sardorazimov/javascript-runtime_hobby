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

private:
   void Worker();

   std::mutex mutex_;
   std::condition_variable condition_;
   std::queue<std::function<void()>> tasks_;
   std::vector<std::thread> workers_;
   bool stopping_ = false;
};

}