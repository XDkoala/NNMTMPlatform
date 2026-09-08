#include "ANN.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

struct AnnFixtureCleanup {
    fs::path trainFile;
    fs::path inferenceFile;
    fs::path parameterRoot;
    fs::path labelRoot;

    ~AnnFixtureCleanup() {
        std::error_code error;
        fs::remove(trainFile, error);
        fs::remove(inferenceFile, error);
        fs::remove_all(parameterRoot, error);
        fs::remove_all(labelRoot, error);
    }
};

int main() {
    const std::string modelId = "ann_pipeline_check";
    AnnFixtureCleanup cleanup{
        fs::current_path() / "ann_pipeline_train.txt",
        fs::current_path() / "ann_pipeline_infer.txt",
        fs::current_path() / "data" / "parameters" / ("model_" + modelId),
        fs::current_path() / "data" / "label" / ("model_" + modelId)
    };

    try {
        {
            std::ofstream train(cleanup.trainFile);
            train << "0.0 0.0 low\n"
                  << "0.1 0.2 low\n"
                  << "0.2 0.1 low\n"
                  << "0.3 0.2 low\n"
                  << "0.8 0.9 high\n"
                  << "0.9 0.8 high\n"
                  << "1.0 0.9 high\n"
                  << "0.9 1.0 high\n";
        }

        ANN network(State::Train, Task::BinaryClassification, ANNType::SimpleMLP,
                    2, cleanup.trainFile.string(), modelId);
        network.set_training_options(2, 2, 0.001);
        network.conduct();

        if (!fs::exists(cleanup.parameterRoot / "model.pt") ||
            !fs::exists(cleanup.parameterRoot / "normalization.pt")) {
            throw std::runtime_error("ANN pipeline did not save its model and normalization data.");
        }
        if (!fs::exists(cleanup.labelRoot / "labels.txt")) {
            throw std::runtime_error("ANN pipeline did not save labels.txt.");
        }

        {
            std::ofstream inference(cleanup.inferenceFile);
            inference << "0.05 0.10\n"
                      << "0.95 0.90\n";
        }

        int predictionCount = 0;
        ANN inference(State::Infer, Task::BinaryClassification, ANNType::SimpleMLP,
                      2, cleanup.inferenceFile.string(), modelId);
        inference.set_training_options(2, 1, 0.001);
        inference.set_callbacks(
            nullptr,
            [&](const std::string& message, bool) {
                if (message.rfind("Sample ", 0) == 0) ++predictionCount;
            },
            nullptr);
        inference.conduct();

        if (predictionCount != 2) {
            throw std::runtime_error("ANN batched inference did not return two predictions.");
        }

        std::cout << "ANN train/save/load/infer pipeline passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "ANN pipeline check failed: " << error.what() << '\n';
        return 1;
    }
}
