#include "TrainingUtils.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <set>
#include <stdexcept>
#include <vector>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

template <typename Function>
void requireInvalidArgument(Function function, const char* message) {
    try {
        function();
    } catch (const std::invalid_argument&) {
        return;
    }
    throw std::runtime_error(message);
}

void testRandomSplit() {
    const auto first = nnmtm::makeRandomSplit(20, 0.2, 123);
    const auto second = nnmtm::makeRandomSplit(20, 0.2, 123);
    require(first.train == second.train && first.validation == second.validation,
            "Random split must be deterministic for a fixed seed.");
    require(first.train.size() == 16 && first.validation.size() == 4,
            "Random split has unexpected sizes.");

    std::set<std::int64_t> all(first.train.begin(), first.train.end());
    all.insert(first.validation.begin(), first.validation.end());
    require(all.size() == 20, "Random split must contain each sample exactly once.");
}

void testStratifiedSplit() {
    const std::vector<std::int64_t> labels{0, 0, 0, 0, 1, 1, 1, 1, 2, 2};
    const auto split = nnmtm::makeStratifiedSplit(labels, 0.25, 7);

    for (std::int64_t label = 0; label <= 2; ++label) {
        const auto trainCount = std::count_if(split.train.begin(), split.train.end(),
            [&](std::int64_t index) { return labels.at(index) == label; });
        const auto validationCount = std::count_if(
            split.validation.begin(), split.validation.end(),
            [&](std::int64_t index) { return labels.at(index) == label; });
        require(trainCount > 0 && validationCount > 0,
                "Every class must occur in both split partitions.");
    }
}

void testInvalidInputs() {
    requireInvalidArgument([] { nnmtm::makeRandomSplit(1); },
                           "A one-sample random split must fail.");
    requireInvalidArgument([] { nnmtm::makeStratifiedSplit({0, 0}); },
                           "A one-class stratified split must fail.");
    requireInvalidArgument([] { nnmtm::makeStratifiedSplit({0, 0, 1}); },
                           "A class with one sample must fail.");
}

} // namespace

int main() {
    try {
        testRandomSplit();
        testStratifiedSplit();
        testInvalidInputs();
        std::cout << "All core logic tests passed.\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Test failure: " << error.what() << '\n';
        return 1;
    }
}
