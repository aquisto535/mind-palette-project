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
