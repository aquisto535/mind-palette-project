#include "infra/thread_pool.h"

ThreadPool::ThreadPool(size_t numThreads) {
    // Default to hardware concurrency, minimum 1
    if (numThreads == 0) {
        numThreads = std::thread::hardware_concurrency();
        if (numThreads == 0) numThreads = 1;
    }
    
    workers_.reserve(numThreads);
    
    for (size_t i = 0; i < numThreads; ++i) {
        workers_.emplace_back([this]() 
        {
            while (true) 
            {
                std::function<void()> task;
                
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    
                    // Wait until there's a task or we're stopping
                    cv_.wait(lock, [this]() {
                        return stop_ || !tasks_.empty();
                    });
                    
                    // Exit if stopped and no more tasks
                    if (stop_ && tasks_.empty()) {
                        return;
                    }
                    
                    // Get the next task
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
                
                // Execute task outside the lock
                task();
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    if (!shutdown_) {
        shutdown();
    }
}

void ThreadPool::shutdown() {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        stop_ = true;
    }
    
    cv_.notify_all();
    
    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    shutdown_ = true;
}

