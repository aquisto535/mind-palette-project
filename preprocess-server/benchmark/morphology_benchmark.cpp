/**
 * Morphology (EnhanceContours) Benchmark
 * 
 * Tests different kernel sizes for Morphological CLOSE operation
 * to validate the choice of kernel size 3.
 * 
 * Metrics:
 * 1. Added Pixels: How many pixels were added to fill gaps?
 * 2. Processing Time: Performance impact.
 */
#include <opencv2/opencv.hpp>
#include <chrono>
#include <iostream>
#include <vector>
#include <numeric>
#include <string>
#include <iomanip>

using namespace std::chrono;

struct Config {
    int kernelSize;
    std::string desc;
};

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
    return times[N / 2];
}

int main(int argc, char* argv[]) {
    std::string img_path = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\uploads\\IMG_1294.jpg";
    if (argc > 1) img_path = argv[1];

    cv::Mat original = cv::imread(img_path);
    if (original.empty()) {
        std::cerr << "Image not found, using dummy.\n";
        original = cv::Mat::zeros(512, 512, CV_8UC3);
    }

    // 1. Preprocess until Canny (Input for Morphology)
    cv::Mat processed, grayscale, edges;
    cv::resize(original, processed, cv::Size(512, 512));
    cv::GaussianBlur(processed, processed, cv::Size(5, 5), 0);
    cv::medianBlur(processed, processed, 3);
    cv::cvtColor(processed, grayscale, cv::COLOR_BGR2GRAY);
    cv::Canny(grayscale, edges, 50, 150, 3, true);

    long long originalPixels = cv::countNonZero(edges);

    std::cout << "=== Morphology (Close) Benchmark ===\n";
    std::cout << "Input Edge Pixels: " << originalPixels << "\n\n";

    std::vector<Config> configs = {
        {1, "Identity (No-op equivalent)"},
        {3, "Kernel 3x3 (Default)"},
        {5, "Kernel 5x5 (Strong)"},
        {7, "Kernel 7x7 (Very Strong)"}
    };

    std::cout << "| Kernel | Description | Post-Close Pixels | Added Pixels | Increase % | Time (ms) |\n";
    std::cout << "|--------|-------------|-------------------|--------------|------------|-----------|\n";

    for (const auto& cfg : configs) {
        cv::Mat result;
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(cfg.kernelSize, cfg.kernelSize));
        
        double t = measure_ms([&]{
            cv::morphologyEx(edges, result, cv::MORPH_CLOSE, kernel);
        });

        long long finalPixels = cv::countNonZero(result);
        long long added = finalPixels - originalPixels;
        double increasePercent = 100.0 * added / originalPixels;

        std::cout << "| " << std::setw(6) << cfg.kernelSize << " | " 
                  << std::setw(11) << std::left << cfg.desc << " | "
                  << std::setw(17) << std::right << finalPixels << " | "
                  << std::setw(12) << added << " | "
                  << std::fixed << std::setprecision(2) << increasePercent << "% | "
                  << std::setw(9) << t << " |\n";
                  
        // Save result
        std::string outName = "benchmark_morph_" + std::to_string(cfg.kernelSize) + ".jpg";
        cv::imwrite(outName, result);
    }
    
    std::cout << "\n=== DONE ===\n";
    return 0;
}
