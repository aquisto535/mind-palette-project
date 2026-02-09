#pragma once

#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

/**
 * @brief ITaskQueue - Producer-Consumer Pattern Interface
 * 
 * Abstract interface for task queues.
 * Prepares for Week 4 Thread Pool implementation.
 */
template<typename T>
class ITaskQueue {
public:
    virtual ~ITaskQueue() = default;
    
    virtual void push(T task) = 0;
    virtual std::optional<T> pop() = 0;
    virtual bool empty() const = 0;
    virtual size_t size() const = 0;
};

/**
 * @brief SyncTaskQueue - Single-threaded mock implementation
 * 
 * Simple queue for testing without multithreading.
 * Will be replaced with AsyncTaskQueue in Week 4.
 */
template<typename T>
class SyncTaskQueue : public ITaskQueue<T> {
public:
    void push(T task) override {
        queue_.push(std::move(task));
    }
    
    std::optional<T> pop() override {
        if (queue_.empty()) {
            return std::nullopt;
        }
        T task = std::move(queue_.front());
        queue_.pop();
        return task;
    }
    
    bool empty() const override {
        return queue_.empty();
    }
    
    size_t size() const override {
        return queue_.size();
    }

private:
    std::queue<T> queue_;
};

/**
 * @brief AsyncTaskQueue - Thread-safe blocking queue (Week 4)
 * 
 * Uses mutex and condition_variable for synchronization.
 * Supports blocking pop and graceful shutdown.
 */
template<typename T>
class AsyncTaskQueue : public ITaskQueue<T> {
public:
    void push(T task) override {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (closed_) return;
            queue_.push(std::move(task));
        }
        cv_.notify_one();
    }
    
    std::optional<T> pop() override {
        std::unique_lock<std::mutex> lock(mutex_);
        cv_.wait(lock, [this]() { return closed_ || !queue_.empty(); });
        
        if (queue_.empty()) {
            return std::nullopt;  // Closed and empty
        }
        
        T task = std::move(queue_.front());
        queue_.pop();
        return task;
    }
    
    /**
     * @brief Non-blocking pop
     */
    std::optional<T> tryPop() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return std::nullopt;
        }
        T task = std::move(queue_.front());
        queue_.pop();
        return task;
    }
    
    bool empty() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.empty();
    }
    
    size_t size() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
    
    /**
     * @brief Close the queue (unblock waiting threads)
     */
    void close() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
        }
        cv_.notify_all();
    }
    
    bool isClosed() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

private:
    std::queue<T> queue_;
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    bool closed_ = false;
};
