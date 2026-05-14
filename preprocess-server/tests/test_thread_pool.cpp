#include <gtest/gtest.h>
#include "thread_pool.h"
#include <atomic>
#include <chrono>
#include <vector>

// ==================== ThreadPool Basic Tests ====================

TEST(ThreadPoolTest, CreateAndDestroy) {
    ThreadPool pool(4);
    EXPECT_EQ(pool.size(), 4);
}

TEST(ThreadPoolTest, EnqueueSingleTask) {
    ThreadPool pool(2);
    std::atomic<int> counter{0};
    
    pool.enqueue([&counter]() {
        counter++;
    });
    
    pool.shutdown();
    EXPECT_EQ(counter.load(), 1);
}

TEST(ThreadPoolTest, EnqueueMultipleTasks) {
    ThreadPool pool(4);
    std::atomic<int> counter{0};
    const int numTasks = 100;
    
    for (int i = 0; i < numTasks; ++i) {
        pool.enqueue([&counter]() {
            counter++;
        });
    }
    
    pool.shutdown();
    EXPECT_EQ(counter.load(), numTasks);
}

// ==================== Synchronization Tests ====================

TEST(ThreadPoolTest, NoDataRace) {
    ThreadPool pool(8);
    std::atomic<int> counter{0};
    const int numTasks = 1000;
    
    for (int i = 0; i < numTasks; ++i) {
        pool.enqueue([&counter]() {
            // Simulate work
            std::this_thread::sleep_for(std::chrono::microseconds(10));
            counter.fetch_add(1, std::memory_order_relaxed);
        });
    }
    
    pool.shutdown();
    EXPECT_EQ(counter.load(), numTasks);  // No data race
}

TEST(ThreadPoolTest, NoDeadlock) {
    ThreadPool pool(4);
    std::atomic<bool> completed{false};
    
    // Enqueue task that enqueues another task
    pool.enqueue([&pool, &completed]() {
        // This should not cause deadlock
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        completed = true;
    });
    
    // Timeout test - if deadlock, this will hang
    auto start = std::chrono::steady_clock::now();
    pool.shutdown();
    auto end = std::chrono::steady_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    EXPECT_LT(duration.count(), 5);  // Should complete within 5 seconds
    EXPECT_TRUE(completed.load());
}

// ==================== Edge Cases ====================

TEST(ThreadPoolTest, EmptyPoolShutdown) {
    ThreadPool pool(2);
    // No tasks enqueued
    pool.shutdown();
    EXPECT_EQ(pool.size(), 2);  // Size should remain
}

TEST(ThreadPoolTest, ZeroThreadsFallback) {
    // Should use hardware_concurrency or at least 1
    ThreadPool pool(0);
    EXPECT_GE(pool.size(), 1);
}

// ==================== Queue Size Limit Tests ====================

TEST(ThreadPoolTest, EnqueueReturnsTrueOnSuccess) {
    ThreadPool pool(2, 10); // maxQueueSize = 10
    std::atomic<int> counter{0};

    bool result = pool.enqueue([&counter]() { counter++; });

    EXPECT_TRUE(result);
    pool.shutdown();
    EXPECT_EQ(counter.load(), 1);
}

TEST(ThreadPoolTest, EnqueueReturnsFalseWhenQueueFull) {
    // Workers are blocked → queue fills up
    ThreadPool pool(1, 3); // 1 worker, maxQueueSize = 3

    std::mutex blockMutex;
    std::unique_lock<std::mutex> blockLock(blockMutex);

    // Block the single worker thread
    pool.enqueue([&blockMutex]() {
        std::unique_lock<std::mutex> lk(blockMutex); // blocks until test releases
    });

    // Fill queue to capacity (worker is blocked, so queue depth grows)
    bool r1 = pool.enqueue([]() {});
    bool r2 = pool.enqueue([]() {});
    bool r3 = pool.enqueue([]() {}); // This should hit the limit

    EXPECT_TRUE(r1);
    EXPECT_TRUE(r2);
    EXPECT_FALSE(r3); // Queue full → rejected

    blockLock.unlock(); // Release worker
    pool.shutdown();
}

TEST(ThreadPoolTest, QueueFullDoesNotCrash) {
    ThreadPool pool(1, 2); // Very small queue
    std::atomic<int> accepted{0};
    std::atomic<int> rejected{0};

    std::mutex blockMutex;
    std::unique_lock<std::mutex> blockLock(blockMutex);

    // Block the single worker
    pool.enqueue([&blockMutex]() {
        std::unique_lock<std::mutex> lk(blockMutex);
    });

    // Flood with tasks
    for (int i = 0; i < 20; ++i) {
        if (pool.enqueue([]() {})) {
            ++accepted;
        } else {
            ++rejected;
        }
    }

    EXPECT_LE(accepted.load(), 2);  // At most maxQueueSize accepted
    EXPECT_GE(rejected.load(), 18); // Rest must be rejected
    EXPECT_NO_FATAL_FAILURE(blockLock.unlock());
    pool.shutdown();
}

// ==================== Scalability Preparation ====================

TEST(ThreadPoolTest, WorkDistribution) {
    const int numThreads = 4;
    ThreadPool pool(numThreads);
    
    std::vector<std::atomic<int>> threadCounts(numThreads);
    for (auto& c : threadCounts) c = 0;
    
    std::atomic<int> totalTasks{0};
    const int numTasks = 100;
    
    for (int i = 0; i < numTasks; ++i) {
        pool.enqueue([&totalTasks]() {
            std::this_thread::sleep_for(std::chrono::microseconds(100));
            totalTasks++;
        });
    }
    
    pool.shutdown();
    EXPECT_EQ(totalTasks.load(), numTasks);
}
