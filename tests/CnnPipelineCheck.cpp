#include "CNN.h"

#include <opencv2/imgcodecs.hpp>
#include <opencv2/core.hpp>

#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

void setDevicePreference(const char* value) {
#ifdef _WIN32
    _putenv_s("NNMTM_DEVICE", value);
#else
    if (value[0] == '\0') unsetenv("NNMTM_DEVICE");
    else setenv("NNMTM_DEVICE", value, 1);
#endif
}

struct FixtureCleanup {
    fs::path imageRoot;
    fs::path parameterRoot;
    fs::path labelRoot;

    ~FixtureCleanup() {
        std::error_code error;
        fs::remove_all(imageRoot, error);
        fs::remove_all(parameterRoot, error);
        fs::remove_all(labelRoot, error);
    }
};

int main() {
    const bool hasCuda = torch::cuda::is_available();

    const std::string modelId = "cnn_pipeline_check";
    const fs::path imageRoot = fs::current_path() / "cnn_pipeline_fixture";
    FixtureCleanup cleanup{
        imageRoot,
        fs::current_path() / "data" / "parameters" / ("model_" + modelId),
        fs::current_path() / "data" / "label" / ("model_" + modelId)
    };

    try {
        std::error_code error;
        fs::remove_all(imageRoot, error);
        fs::create_directories(imageRoot / "dark");
        fs::create_directories(imageRoot / "bright");
        for (int index = 0; index < 2; ++index) {
            cv::Mat dark(48, 48, CV_8UC3, cv::Scalar(10 + index, 10, 10));
            cv::Mat bright(48, 48, CV_8UC3, cv::Scalar(240 - index, 240, 240));
            if (!cv::imwrite((imageRoot / "dark" / (std::to_string(index) + ".png")).string(), dark) ||
                !cv::imwrite((imageRoot / "bright" / (std::to_string(index) + ".png")).string(), bright)) {
                throw std::runtime_error("Failed to create CNN test images.");
            }
        }

        setDevicePreference(hasCuda ? "cuda" : "cpu");
        bool selectedDevice = false;
        CNN network(State::Train, Task::BinaryClassification, CNNType::LeNet,
                    imageRoot.string(), modelId);
        network.set_training_options(2, 2, 0.001);
        network.set_callbacks(
            nullptr,
            [&](const std::string& message, bool) {
                if (message.find(hasCuda ? "CUDA:0" : "CPU") != std::string::npos) selectedDevice = true;
            },
            nullptr);
        network.conduct();
        setDevicePreference("");

        if (!selectedDevice) throw std::runtime_error("CNN pipeline did not select its expected device.");
        if (!fs::exists(cleanup.parameterRoot / "model.pt")) {
            throw std::runtime_error("CNN pipeline did not save model.pt.");
        }
        if (!fs::exists(cleanup.labelRoot / "labels.txt")) {
            throw std::runtime_error("CNN pipeline did not save labels.txt.");
        }

        const fs::path inferenceRoot = imageRoot / "inference";
        fs::create_directories(inferenceRoot);
        cv::Mat inferenceDark(48, 48, CV_8UC3, cv::Scalar(12, 12, 12));
        cv::Mat inferenceBright(48, 48, CV_8UC3, cv::Scalar(238, 238, 238));
        cv::imwrite((inferenceRoot / "dark.png").string(), inferenceDark);
        cv::imwrite((inferenceRoot / "bright.png").string(), inferenceBright);

        int predictionCount = 0;
        CNN inference(State::Infer, Task::BinaryClassification, CNNType::LeNet,
                      inferenceRoot.string(), modelId);
        inference.set_training_options(2, 2, 0.001);
        inference.set_callbacks(
            nullptr,
            [&](const std::string& message, bool) {
                if (message.rfind("Sample ", 0) == 0) ++predictionCount;
            },
            nullptr);
        inference.conduct();
        if (predictionCount != 2) {
            throw std::runtime_error("CNN streaming inference did not return two predictions.");
        }

        std::cout << "CNN streaming train/save/load/infer pipeline passed on "
                  << (hasCuda ? "CUDA:0" : "CPU") << ".\n";
        return 0;
    } catch (const std::exception& error) {
        setDevicePreference("");
        std::cerr << "CNN pipeline check failed: " << error.what() << '\n';
        return 1;
    }
}
