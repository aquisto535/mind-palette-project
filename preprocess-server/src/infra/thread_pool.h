#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

/**
 * @brief ThreadPool - Standard C++17 Thread Pool
 * 
 * Producer-Consumer pattern implementation.
 * Uses std::thread, std::mutex, std::condition_variable.
 */
class ThreadPool {
public:
    /**
     * @brief Construct thread pool
     * @param numThreads Number of worker threads (0 = hardware_concurrency)
     */
    explicit ThreadPool(size_t numThreads = 0);
    
    /**
     * @brief Destructor - waits for all tasks to complete
     */
    ~ThreadPool();

    // Disable copy
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    /**
     * @brief Enqueue a task for execution
     * @param task Function to execute
     */
    template<class F>
    void enqueue(F&& task);

    /**
     * @brief Get number of worker threads
     */
    size_t size() const { return workers_.size(); }

    /**
     * @brief Shutdown the pool and wait for completion
     */
    void shutdown();

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<bool> stop_{false};
    std::atomic<bool> shutdown_{false};
};

// Template implementation (must be in header)
template<class F>
void ThreadPool::enqueue(F&& task) {
    {
        std::lock_guard<std::mutex> lock(mutex_);  // 큐 보호 잠금
        if (stop_) return;  // Don't accept new tasks after stop
        tasks_.emplace(std::forward<F>(task)); // 람다를 큐에 넣음
    }
    cv_.notify_one(); // 워커 스레드에게 작업이 왔음을 알림
}
