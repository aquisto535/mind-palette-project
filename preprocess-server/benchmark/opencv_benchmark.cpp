/**
 * OpenCV Parameter Benchmark
 * 
 * Tests:
 * 1. Resize: large image (before) vs 512x512 (after) ─ GaussianBlur timing
 * 2. GaussianBlur kernel comparison: 3x3, 5x5, 7x7
 * 3. MedianBlur kernel comparison: 3x3, 5x5
 * 4. Full pipeline timing
 */
#include <opencv2/opencv.hpp>
#include <chrono>
#include <iostream>
#include <vector>
#include <numeric>
#include <string>

using namespace std::chrono;

// Run fn() N times, return median elapsed ms
template<typename Fn>
double measure_ms(Fn fn, int N = 30) {
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
    std::string img_path = "C:\\Users\\user\\Documents\\GitHub\\mind-palette-project\\shared_volume\\uploads\\1764993122673_562738870.jpg";
    if (argc > 1) img_path = argv[1];

    cv::Mat original = cv::imread(img_path);
    if (original.empty()) {
        std::cerr << "Failed to load: " << img_path << "\n";
        return 1;
    }

    std::cout << "=== OpenCV Parameter Benchmark ===\n";
    std::cout << "Image: " << original.cols << "x" << original.rows << " (" 
              << original.cols * original.rows / 1000000.0 << " MP)\n\n";

    // --- Test 1: Resize overhead ---
    std::cout << "--- Test 1: Resize Strategy ---\n";

    // 1a: GaussianBlur on ORIGINAL size (before resize)
    double t_blur_large = measure_ms([&]{
        cv::Mat tmp;
        cv::GaussianBlur(original, tmp, cv::Size(5,5), 0);
    });

    // 1b: Resize to 512x512 FIRST, then blur
    cv::Mat resized512;
    cv::resize(original, resized512, cv::Size(512, 512));

    double t_blur_small = measure_ms([&]{
        cv::Mat tmp;
        cv::GaussianBlur(resized512, tmp, cv::Size(5,5), 0);
    });

    long long ops_large = (long long)original.cols * original.rows;
    long long ops_small = 512LL * 512;
    double ratio = (double)ops_large / ops_small;

    std::cout << "GaussianBlur on original (" << original.cols << "x" << original.rows << "): "
              << t_blur_large << " ms\n";
    std::cout << "GaussianBlur on 512x512:              " << t_blur_small << " ms\n";
    std::cout << "Pixel count ratio:                    " << ratio << "x\n";
    std::cout << "Actual speedup (resize first):        " << t_blur_large / t_blur_small << "x faster\n\n";

    // --- Test 2: GaussianBlur kernel comparison ---
    std::cout << "--- Test 2: GaussianBlur Kernel Size (on 512x512) ---\n";
    for (int k : {3, 5, 7}) {
        double t = measure_ms([&]{
            cv::Mat tmp;
            cv::GaussianBlur(resized512, tmp, cv::Size(k,k), 0);
        });
        std::cout << "  " << k << "x" << k << ": " << t << " ms\n";
    }
    std::cout << "\n";

    // --- Test 3: MedianBlur kernel comparison ---
    std::cout << "--- Test 3: MedianBlur Kernel Size (on 512x512) ---\n";
    for (int k : {3, 5}) {
        double t = measure_ms([&]{
            cv::Mat tmp;
            cv::medianBlur(resized512, tmp, k);
        });
        std::cout << "  " << k << "x" << k << ": " << t << " ms\n";
    }
    std::cout << "\n";

    // --- Test 4: Full pipeline ---
    std::cout << "--- Test 4: Full Preprocess Pipeline ---\n";
    double t_full = measure_ms([&]{
        cv::Mat p;
        original.copyTo(p);
        cv::resize(p, p, cv::Size(512, 512));
        cv::GaussianBlur(p, p, cv::Size(5,5), 0);
        cv::medianBlur(p, p, 3);
        cv::Mat g;
        cv::cvtColor(p, g, cv::COLOR_BGR2GRAY);
    });
    std::cout << "Full pipeline (resize→gaussian→median→gray): " << t_full << " ms\n\n";

    std::cout << "=== DONE ===\n";
    return 0;
}
