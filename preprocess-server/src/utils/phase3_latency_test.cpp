#include "core/image_processor.h"
#include "core/pipeline_factory.h"
#include "core/filter_pipeline.h"
#include <iostream>
#include <chrono>

int main(int argc, char** argv) {
    if (argc < 2) return 1;
    ImageProcessor processor;
    cv::Mat image = processor.Load(argv[1]);
    if (image.empty()) return 1;

    FilterPipeline pipeline = PipelineFactory::createHybridPipeline();

    // Warmup
    pipeline.execute(image);
    
    auto start = std::chrono::high_resolution_clock::now();
    cv::Mat processed = pipeline.execute(image);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    std::cout << "PipelineFactory Latency: " << duration << " ms" << std::endl;

    return 0;
}
