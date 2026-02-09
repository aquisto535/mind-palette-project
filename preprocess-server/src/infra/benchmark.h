#pragma once

#include <functional>
#include <chrono>
#include <vector>
#include <string>

/**
 * @brief Benchmark - Performance measurement utility
 * 
 * Measures execution time and throughput.
 */
class Benchmark {
public:
    /**
     * @brief Measure execution time of a function
     * @param fn Function to measure
     * @return Duration in milliseconds
     */
    static double measure(std::function<void()> fn);
    
    /**
     * @brief Measure average execution time over multiple iterations
     * @param fn Function to measure
     * @param iterations Number of iterations
     * @return Average duration in milliseconds
     */
    static double measureAverage(std::function<void()> fn, int iterations);
    
    /**
     * @brief Run scalability test with different thread counts
     * @param minThreads Minimum thread count
     * @param maxThreads Maximum thread count
     * @param taskFn Task function to execute
     * @param taskCount Number of tasks to run
     */
    static void runScalabilityTest(int minThreads, int maxThreads, 
                                    std::function<void()> taskFn, int taskCount);
    
    /**
     * @brief Format duration for display
     */
    static std::string formatDuration(double ms);
};
