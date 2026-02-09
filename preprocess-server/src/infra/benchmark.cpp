#include "infra/benchmark.h"
#include "infra/thread_pool.h"
#include <iostream>
#include <iomanip>
#include <sstream>

double Benchmark::measure(std::function<void()> fn) {
    auto start = std::chrono::high_resolution_clock::now();
    fn();
    auto end = std::chrono::high_resolution_clock::now();
    
    return std::chrono::duration<double, std::milli>(end - start).count();
}

double Benchmark::measureAverage(std::function<void()> fn, int iterations) {
    double total = 0.0;
    for (int i = 0; i < iterations; ++i) {
        total += measure(fn);
    }
    return total / iterations;
}

void Benchmark::runScalabilityTest(int minThreads, int maxThreads, 
                                    std::function<void()> taskFn, int taskCount) {
    std::cout << "\n=== Scalability Test ===" << std::endl;
    std::cout << "Tasks: " << taskCount << std::endl;
    std::cout << std::setw(10) << "Threads" 
              << std::setw(15) << "Time (ms)" 
              << std::setw(15) << "Throughput"
              << std::endl;
    std::cout << std::string(40, '-') << std::endl;
    
    for (int threads = minThreads; threads <= maxThreads; threads *= 2) {
        ThreadPool pool(threads);
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < taskCount; ++i) {
            pool.enqueue(taskFn);
        }
        
        pool.shutdown();
        
        auto end = std::chrono::high_resolution_clock::now();
        double duration = std::chrono::duration<double, std::milli>(end - start).count();
        double throughput = taskCount / (duration / 1000.0);
        
        std::cout << std::setw(10) << threads 
                  << std::setw(15) << std::fixed << std::setprecision(2) << duration
                  << std::setw(15) << std::fixed << std::setprecision(1) << throughput << "/s"
                  << std::endl;
    }
}

std::string Benchmark::formatDuration(double ms) {
    std::stringstream ss;
    if (ms < 1.0) {
        ss << std::fixed << std::setprecision(2) << (ms * 1000) << " 關s";
    } else if (ms < 1000.0) {
        ss << std::fixed << std::setprecision(2) << ms << " ms";
    } else {
        ss << std::fixed << std::setprecision(2) << (ms / 1000.0) << " s";
    }
    return ss.str();
}

