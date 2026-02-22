/**
 * Canny Threshold Benchmark
 * 
 * Tests different threshold combinations for Canny Edge Detection
 * to validate the choice of 50/150 (1:3 ratio).
 * 
 * Metrics:
 * 1. Edge Pixel Count: Quantitative measure of "how much edge" is detected.
 * 2. Processing Time: Performance impact of different thresholds.
 */
#include <opencv2/opencv.hpp>
#include <chrono>
#include <iostream>
#include <vector>
#include <numeric>
#include <string>
#include <iomanip>

using namespace std::chrono;

// Threshold configuration struct
struct Config {
    double low;
    double high;
    std::string desc;
};

// Run fn() N times, return median elapsed ms
template<typename Fn>
double measure_ms(Fn fn, int N = 20) {
    std::vector<double> times;
    times.reserve(N);
    for (int i = 0; i < N; ++i) {
        auto t0 = high_resolution_clock::now();
        fn();
        auto t1 = high_resolution_clock::now();
        times.push_back(duration<double, std::milli>(t1 - t0).count());
    }
    std::sort(times.begin(), times.end());
    return times[N / 2]; // median
}

int main(int argc, char* argv[]) {
    // Default image path (same as previous benchmark)
    std::string img_path = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\uploads\\IMG_1294.jpg";
    if (argc > 1) img_path = argv[1];

    cv::Mat original = cv::imread(img_path);
    if (original.empty()) {
        std::cerr << "To run this test, please provide a valid image path.\n";
        std::cerr << "Usage: canny_benchmark.exe <path_to_image>\n";
        // Fallback to a black image if file not found, just to run the code structure (will result in 0 edges)
        original = cv::Mat::zeros(512, 512, CV_8UC3);
    }

    // Preprocessing (Same conditions as production)
    cv::Mat processed, grayscale;
    cv::resize(original, processed, cv::Size(512, 512));
    cv::GaussianBlur(processed, processed, cv::Size(5, 5), 0);
    cv::medianBlur(processed, processed, 3);
    cv::cvtColor(processed, grayscale, cv::COLOR_BGR2GRAY);

    std::cout << "=== Canny Threshold Benchmark ===\n";
    std::cout << "Image Size: " << grayscale.cols << "x" << grayscale.rows << "\n";
    std::cout << "Total Pixels: " << grayscale.total() << "\n\n";

    std::vector<Config> configs = {
        {50, 150, "Default (1:3) - Current"},
        {100, 200, "High/Strict (1:2)"},
        {30, 90, "Low/Sensitive (1:3)"},
        {100, 100, "1:1 Ratio (No Hysteresis)"},
        {10, 30, "Very Low (Noise prone)"}
    };

    std::cout << "| Low | High | Ratio | Description | Edge Pixels | Edge % | Time (ms) |\n";
    std::cout << "|-----|------|-------|-------------|-------------|--------|-----------|\n";

    for (const auto& cfg : configs) {
        cv::Mat edges;
        long long pixelCount = 0;

        double t = measure_ms([&]{
            cv::Canny(grayscale, edges, cfg.low, cfg.high, 3, true);
        });

        // Count non-zero pixels (edges)
        pixelCount = cv::countNonZero(edges);
        double edgePercent = 100.0 * pixelCount / grayscale.total();

        std::cout << "| " << std::setw(3) << cfg.low << " | " 
                  << std::setw(4) << cfg.high << " | "
                  << "1:" << (int)(cfg.high/cfg.low) << "   | "
                  << std::setw(20) << std::left << cfg.desc << " | "
                  << std::setw(11) << std::right << pixelCount << " | "
                  << std::fixed << std::setprecision(2) << edgePercent << "% | "
                  << t << " |\n";
        
        // Save result image for visual inspection (optional)
        std::string outName = "benchmark_canny_" + std::to_string((int)cfg.low) + "_" + std::to_string((int)cfg.high) + ".jpg";
        cv::imwrite(outName, edges);
    }
    
    std::cout << "\n=== DONE ===\n";
    std::cout << "Check generated images (benchmark_canny_*.jpg) for visual comparison.\n";

    return 0;
}
