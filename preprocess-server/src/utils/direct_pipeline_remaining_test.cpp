#include <opencv2/opencv.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct BinaryStats {
    int foregroundPixels;
    int components;
    int smallComponents;
};

static cv::Mat resizeKeepingAspectRatio(const cv::Mat& input, int targetSize) {
    const double scale = std::min(
        static_cast<double>(targetSize) / input.cols,
        static_cast<double>(targetSize) / input.rows
    );
    const int newWidth = static_cast<int>(input.cols * scale);
    const int newHeight = static_cast<int>(input.rows * scale);

    cv::Mat resized;
    cv::resize(input, resized, cv::Size(newWidth, newHeight), 0, 0, cv::INTER_AREA);
    return resized;
}

static cv::Mat denoise(const cv::Mat& input) {
    cv::Mat output;
    cv::GaussianBlur(input, output, cv::Size(3, 3), 0);
    return output;
}

static cv::Mat toGray(const cv::Mat& input) {
    cv::Mat gray;
    cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    return gray;
}

static cv::Mat adaptiveBinary(const cv::Mat& gray, int blockSize, int c) {
    cv::Mat binary;
    cv::adaptiveThreshold(
        gray,
        binary,
        255,
        cv::ADAPTIVE_THRESH_GAUSSIAN_C,
        cv::THRESH_BINARY_INV,
        blockSize,
        c
    );
    return binary;
}

static cv::Mat otsuBinary(const cv::Mat& gray) {
    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    return binary;
}

static cv::Mat addLightingGradient(const cv::Mat& gray) {
    cv::Mat shaded(gray.size(), gray.type());
    for (int y = 0; y < gray.rows; ++y) {
        for (int x = 0; x < gray.cols; ++x) {
            const double ratio = static_cast<double>(x) / std::max(1, gray.cols - 1);
            const double factor = 0.62 + 0.38 * ratio;
            shaded.at<uchar>(y, x) = cv::saturate_cast<uchar>(gray.at<uchar>(y, x) * factor);
        }
    }
    return shaded;
}

static BinaryStats getBinaryStats(const cv::Mat& binary) {
    cv::Mat labels;
    cv::Mat stats;
    cv::Mat centroids;
    const int labelCount = cv::connectedComponentsWithStats(binary, labels, stats, centroids, 8, CV_32S);

    int smallComponents = 0;
    for (int label = 1; label < labelCount; ++label) {
        const int area = stats.at<int>(label, cv::CC_STAT_AREA);
        if (area >= 1 && area <= 8) {
            ++smallComponents;
        }
    }

    return {
        cv::countNonZero(binary),
        std::max(0, labelCount - 1),
        smallComponents,
    };
}

static cv::Mat morph(const cv::Mat& binary, int operation, int kernelSize) {
    cv::Mat output;
    const cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(kernelSize, kernelSize));
    cv::morphologyEx(binary, output, operation, kernel);
    return output;
}

static double benchmarkMorph(const cv::Mat& binary, int operation, int kernelSize, int iterations) {
    cv::Mat output;
    morph(binary, operation, kernelSize);

    const auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < iterations; ++i) {
        output = morph(binary, operation, kernelSize);
    }
    const auto end = std::chrono::steady_clock::now();
    const auto elapsed = std::chrono::duration<double, std::milli>(end - start).count();
    return elapsed / iterations;
}

static cv::Rect getContentRoi(const cv::Mat& binary) {
    if (binary.empty()) {
        return cv::Rect(0, 0, 0, 0);
    }

    cv::Mat closed = morph(binary, cv::MORPH_CLOSE, 5);
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(closed, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty()) {
        return cv::Rect(0, 0, binary.cols, binary.rows);
    }

    const double totalArea = static_cast<double>(binary.cols) * binary.rows;
    std::vector<std::pair<double, cv::Rect>> validRects;
    for (const auto& contour : contours) {
        const double area = cv::contourArea(contour);
        if (area < totalArea * 0.001) {
            continue;
        }
        validRects.push_back({area, cv::boundingRect(contour)});
    }

    if (validRects.empty()) {
        return cv::Rect(0, 0, binary.cols, binary.rows);
    }

    auto dominant = std::max_element(
        validRects.begin(),
        validRects.end(),
        [](const auto& a, const auto& b) { return a.first < b.first; }
    );

    cv::Rect roi = dominant->second;
    const int padding = 10;
    roi.x = std::max(0, roi.x - padding);
    roi.y = std::max(0, roi.y - padding);
    roi.width = std::min(binary.cols - roi.x, roi.width + 2 * padding);
    roi.height = std::min(binary.rows - roi.y, roi.height + 2 * padding);
    return roi;
}

static cv::Mat drawRoi(const cv::Mat& base, const cv::Rect& roi) {
    cv::Mat color;
    if (base.channels() == 1) {
        cv::cvtColor(base, color, cv::COLOR_GRAY2BGR);
    } else {
        color = base.clone();
    }
    cv::rectangle(color, roi, cv::Scalar(0, 0, 255), 2);
    return color;
}

static cv::Mat letterbox(const cv::Mat& input, int targetSize, int interpolation, uchar padValue, cv::Rect* placedRoi) {
    const double scale = std::min(
        static_cast<double>(targetSize) / input.cols,
        static_cast<double>(targetSize) / input.rows
    );
    const int newWidth = static_cast<int>(input.cols * scale);
    const int newHeight = static_cast<int>(input.rows * scale);

    cv::Mat resized;
    cv::resize(input, resized, cv::Size(newWidth, newHeight), 0, 0, interpolation);

    cv::Mat canvas(targetSize, targetSize, input.type(), cv::Scalar(padValue));
    const int x = (targetSize - newWidth) / 2;
    const int y = (targetSize - newHeight) / 2;
    resized.copyTo(canvas(cv::Rect(x, y, newWidth, newHeight)));

    if (placedRoi != nullptr) {
        *placedRoi = cv::Rect(x, y, newWidth, newHeight);
    }
    return canvas;
}

static void saveChannelSet(
    const std::string& outputDir,
    const std::string& prefix,
    const cv::Mat& grayCanvas,
    const cv::Mat& binaryCanvas
) {
    cv::Mat chR = grayCanvas;

    cv::Mat chG;
    cv::bitwise_not(binaryCanvas, chG);

    cv::Mat dist;
    cv::distanceTransform(binaryCanvas, dist, cv::DIST_L2, 5);
    cv::Mat distNorm;
    cv::normalize(dist, distNorm, 0, 255, cv::NORM_MINMAX, CV_8UC1);

    cv::Mat chB;
    cv::bitwise_not(distNorm, chB);

    cv::Mat finalImage;
    std::vector<cv::Mat> channels = {chB, chG, chR};
    cv::merge(channels, finalImage);

    cv::Mat replicated;
    cv::cvtColor(chR, replicated, cv::COLOR_GRAY2BGR);

    cv::imwrite(outputDir + "/" + prefix + "-ch-r-gray.png", chR);
    cv::imwrite(outputDir + "/" + prefix + "-ch-g-binary-inverted.png", chG);
    cv::imwrite(outputDir + "/" + prefix + "-ch-b-distance-inverted.png", chB);
    cv::imwrite(outputDir + "/" + prefix + "-final-3ch.png", finalImage);
    cv::imwrite(outputDir + "/" + prefix + "-simple-gray-replicated.png", replicated);

    std::vector<std::pair<std::string, cv::Scalar>> masks = {
        {"mask-r-zero", cv::Scalar(0, 255, 255)},
        {"mask-g-zero", cv::Scalar(255, 0, 255)},
        {"mask-b-zero", cv::Scalar(255, 255, 0)},
    };
    for (const auto& mask : masks) {
        cv::Mat masked = finalImage.clone();
        std::vector<cv::Mat> splitChannels;
        cv::split(masked, splitChannels);
        if (mask.first == "mask-r-zero") {
            splitChannels[2] = cv::Mat::zeros(splitChannels[2].size(), splitChannels[2].type());
        } else if (mask.first == "mask-g-zero") {
            splitChannels[1] = cv::Mat::zeros(splitChannels[1].size(), splitChannels[1].type());
        } else {
            splitChannels[0] = cv::Mat::zeros(splitChannels[0].size(), splitChannels[0].type());
        }
        cv::merge(splitChannels, masked);
        cv::imwrite(outputDir + "/" + prefix + "-" + mask.first + ".png", masked);
    }
}

static cv::Point maxDistancePoint(const cv::Mat& binaryCanvas, double* maxValue) {
    cv::Mat dist;
    cv::distanceTransform(binaryCanvas, dist, cv::DIST_L2, 5);

    cv::Point maxLoc;
    double minValue = 0.0;
    cv::Point minLoc;
    cv::minMaxLoc(dist, &minValue, maxValue, &minLoc, &maxLoc);
    return maxLoc;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: direct_pipeline_remaining_test <image_path> [output_dir]\n";
        return 1;
    }

    const std::string imagePath = argv[1];
    const std::string outputDir = argc >= 3
        ? argv[2]
        : "shared_volume/preprocess-remaining-test";
    fs::create_directories(outputDir);

    const cv::Mat original = cv::imread(imagePath);
    if (original.empty()) {
        std::cerr << "Failed to load: " << imagePath << "\n";
        return 1;
    }

    const cv::Mat resized = resizeKeepingAspectRatio(original, 768);
    const cv::Mat denoised = denoise(resized);
    const cv::Mat gray = toGray(denoised);
    const cv::Mat binary = adaptiveBinary(gray, 7, 3);
    const cv::Mat closed3 = morph(binary, cv::MORPH_CLOSE, 3);

    cv::imwrite(outputDir + "/00-gray-input.png", gray);
    cv::imwrite(outputDir + "/00-binary-baseline-b7-c3.png", binary);

    std::ofstream binCsv(outputDir + "/03b-binarize-test.csv");
    binCsv << "case,foregroundPixels,components,smallComponents,diffFromBaselinePixels\n";

    const cv::Mat shaded = addLightingGradient(gray);
    cv::imwrite(outputDir + "/03b-uneven-lighting-input.png", shaded);
    const cv::Mat shadedAdaptive = adaptiveBinary(shaded, 7, 3);
    const cv::Mat shadedOtsu = otsuBinary(shaded);
    cv::imwrite(outputDir + "/03b-uneven-adaptive-b7-c3.png", shadedAdaptive);
    cv::imwrite(outputDir + "/03b-uneven-otsu.png", shadedOtsu);
    cv::Mat shadedDiff;
    cv::absdiff(shadedAdaptive, shadedOtsu, shadedDiff);
    cv::imwrite(outputDir + "/03b-uneven-adaptive-vs-otsu-diff.png", shadedDiff);

    const std::vector<std::pair<std::string, cv::Mat>> binCases = {
        {"baseline-adaptive-b7-c3", binary},
        {"uneven-adaptive-b7-c3", shadedAdaptive},
        {"uneven-otsu", shadedOtsu},
        {"block-11-c3", adaptiveBinary(gray, 11, 3)},
        {"block-15-c3", adaptiveBinary(gray, 15, 3)},
        {"block-21-c3", adaptiveBinary(gray, 21, 3)},
        {"block-7-c1", adaptiveBinary(gray, 7, 1)},
        {"block-7-c5", adaptiveBinary(gray, 7, 5)},
        {"block-7-c10", adaptiveBinary(gray, 7, 10)},
    };

    for (const auto& testCase : binCases) {
        const auto stats = getBinaryStats(testCase.second);
        cv::Mat diff;
        cv::absdiff(binary, testCase.second, diff);
        cv::imwrite(outputDir + "/03b-" + testCase.first + ".png", testCase.second);
        cv::imwrite(outputDir + "/03b-" + testCase.first + "-diff.png", diff);
        binCsv << testCase.first << ","
               << stats.foregroundPixels << ","
               << stats.components << ","
               << stats.smallComponents << ","
               << cv::countNonZero(diff) << "\n";
    }

    std::ofstream morphCsv(outputDir + "/03c-morphology-test.csv");
    morphCsv << "case,foregroundPixels,components,smallComponents,avgMs\n";
    const std::vector<std::tuple<std::string, int, int>> morphCases = {
        {"close-k3", cv::MORPH_CLOSE, 3},
        {"open-k3", cv::MORPH_OPEN, 3},
        {"close-k7", cv::MORPH_CLOSE, 7},
    };
    for (const auto& testCase : morphCases) {
        const auto name = std::get<0>(testCase);
        const auto operation = std::get<1>(testCase);
        const auto kernelSize = std::get<2>(testCase);
        const cv::Mat output = morph(binary, operation, kernelSize);
        const auto stats = getBinaryStats(output);
        cv::imwrite(outputDir + "/03c-" + name + ".png", output);
        morphCsv << name << ","
                 << stats.foregroundPixels << ","
                 << stats.components << ","
                 << stats.smallComponents << ","
                 << std::fixed << std::setprecision(4)
                 << benchmarkMorph(binary, operation, kernelSize, 50) << "\n";
    }

    std::ofstream roiCsv(outputDir + "/03d-roi-test.csv");
    roiCsv << "case,x,y,width,height,note\n";
    const std::vector<std::pair<std::string, cv::Mat>> roiCases = [] {
        std::vector<std::pair<std::string, cv::Mat>> cases;
        cv::Mat blank(256, 256, CV_8UC1, cv::Scalar(0));
        cases.push_back({"blank", blank});

        cv::Mat dot(256, 256, CV_8UC1, cv::Scalar(0));
        dot.at<uchar>(128, 128) = 255;
        cases.push_back({"single-dot", dot});

        cv::Mat edge(256, 256, CV_8UC1, cv::Scalar(0));
        cv::rectangle(edge, cv::Rect(0, 48, 80, 160), cv::Scalar(255), cv::FILLED);
        cases.push_back({"edge-touching", edge});

        cv::Mat stains(256, 256, CV_8UC1, cv::Scalar(0));
        cv::circle(stains, cv::Point(40, 40), 2, cv::Scalar(255), cv::FILLED);
        cv::circle(stains, cv::Point(180, 60), 2, cv::Scalar(255), cv::FILLED);
        cv::circle(stains, cv::Point(120, 180), 2, cv::Scalar(255), cv::FILLED);
        cases.push_back({"stains-only", stains});
        return cases;
    }();

    for (const auto& testCase : roiCases) {
        const cv::Rect roi = getContentRoi(testCase.second);
        cv::imwrite(outputDir + "/03d-" + testCase.first + ".png", testCase.second);
        cv::imwrite(outputDir + "/03d-" + testCase.first + "-roi.png", drawRoi(testCase.second, roi));
        roiCsv << testCase.first << ","
               << roi.x << ","
               << roi.y << ","
               << roi.width << ","
               << roi.height << ","
               << (roi == cv::Rect(0, 0, testCase.second.cols, testCase.second.rows) ? "fallback-full" : "detected")
               << "\n";
    }

    const cv::Rect actualRoi = getContentRoi(closed3) & cv::Rect(0, 0, gray.cols, gray.rows);
    const cv::Mat roiGray = gray(actualRoi);
    const cv::Mat roiBinary = closed3(actualRoi);

    cv::Rect whitePlaced;
    const cv::Mat whiteGray = letterbox(roiGray, 512, cv::INTER_AREA, 255, &whitePlaced);
    const cv::Mat whiteBinary = letterbox(roiBinary, 512, cv::INTER_NEAREST, 0, nullptr);
    cv::Rect blackPlaced;
    const cv::Mat blackGray = letterbox(roiGray, 512, cv::INTER_AREA, 0, &blackPlaced);
    const cv::Mat stretchedGray = [] (const cv::Mat& input) {
        cv::Mat output;
        cv::resize(input, output, cv::Size(512, 512), 0, 0, cv::INTER_AREA);
        return output;
    }(roiGray);

    cv::imwrite(outputDir + "/03e-letterbox-white-pad.png", whiteGray);
    cv::imwrite(outputDir + "/03e-letterbox-black-pad.png", blackGray);
    cv::imwrite(outputDir + "/03e-stretch-512.png", stretchedGray);

    cv::Mat wide(128, 512, CV_8UC1, cv::Scalar(230));
    cv::line(wide, cv::Point(24, 64), cv::Point(488, 64), cv::Scalar(80), 3);
    cv::Rect widePlaced;
    const cv::Mat wideLetterbox = letterbox(wide, 512, cv::INTER_AREA, 255, &widePlaced);
    cv::imwrite(outputDir + "/03e-wide-roi-letterbox.png", wideLetterbox);

    std::ofstream letterCsv(outputDir + "/03e-letterbox-test.csv");
    letterCsv << "case,roiWidth,roiHeight,placedX,placedY,placedWidth,placedHeight,leftPad,topPad,rightPad,bottomPad\n";
    letterCsv << "actual-white,"
              << roiGray.cols << "," << roiGray.rows << ","
              << whitePlaced.x << "," << whitePlaced.y << ","
              << whitePlaced.width << "," << whitePlaced.height << ","
              << whitePlaced.x << "," << whitePlaced.y << ","
              << 512 - whitePlaced.x - whitePlaced.width << ","
              << 512 - whitePlaced.y - whitePlaced.height << "\n";
    letterCsv << "wide-synthetic,"
              << wide.cols << "," << wide.rows << ","
              << widePlaced.x << "," << widePlaced.y << ","
              << widePlaced.width << "," << widePlaced.height << ","
              << widePlaced.x << "," << widePlaced.y << ","
              << 512 - widePlaced.x - widePlaced.width << ","
              << 512 - widePlaced.y - widePlaced.height << "\n";

    saveChannelSet(outputDir, "04", whiteGray, whiteBinary);
    double maxDistance = 0.0;
    const cv::Point maxPoint = maxDistancePoint(whiteBinary, &maxDistance);
    std::ofstream channelCsv(outputDir + "/04-channel-test.csv");
    channelCsv << "metric,value\n";
    channelCsv << "rNonWhitePixels," << (whiteGray.total() - cv::countNonZero(whiteGray == 255)) << "\n";
    channelCsv << "gBlackStrokePixels," << cv::countNonZero(whiteBinary) << "\n";
    channelCsv << "distanceMaxValue," << std::fixed << std::setprecision(4) << maxDistance << "\n";
    channelCsv << "distanceMaxX," << maxPoint.x << "\n";
    channelCsv << "distanceMaxY," << maxPoint.y << "\n";
    channelCsv << "aiInferenceComparison,not-run-local-preprocess-only\n";

    std::cout << "[output] " << outputDir << "\n";
    return 0;
}
