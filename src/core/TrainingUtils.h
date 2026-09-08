#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <numeric>
#include <random>
#include <stdexcept>
#include <vector>

namespace nnmtm {

struct IndexSplit {
    std::vector<std::int64_t> train;
    std::vector<std::int64_t> validation;
};

inline std::size_t validationCount(std::size_t sampleCount, double validationRatio) {
    if (sampleCount < 2) {
        throw std::invalid_argument("At least two samples are required for training.");
    }
    if (!(validationRatio > 0.0 && validationRatio < 1.0)) {
        throw std::invalid_argument("Validation ratio must be between 0 and 1.");
    }

    const auto requested = static_cast<std::size_t>(
        std::floor(static_cast<double>(sampleCount) * validationRatio));
    return std::min(sampleCount - 1, std::max<std::size_t>(1, requested));
}

inline IndexSplit makeRandomSplit(std::size_t sampleCount,
                                  double validationRatio = 0.1,
                                  std::uint32_t seed = 42) {
    std::vector<std::int64_t> indices(sampleCount);
    std::iota(indices.begin(), indices.end(), 0);

    std::mt19937 generator(seed);
    std::shuffle(indices.begin(), indices.end(), generator);

    const auto validationSize = validationCount(sampleCount, validationRatio);
    IndexSplit split;
    split.validation.assign(indices.begin(), indices.begin() + validationSize);
    split.train.assign(indices.begin() + validationSize, indices.end());
    return split;
}

inline IndexSplit makeStratifiedSplit(const std::vector<std::int64_t>& labels,
                                      double validationRatio = 0.1,
                                      std::uint32_t seed = 42) {
    if (labels.size() < 2) {
        throw std::invalid_argument("At least two samples are required for training.");
    }

    std::map<std::int64_t, std::vector<std::int64_t>> groups;
    for (std::size_t index = 0; index < labels.size(); ++index) {
        groups[labels[index]].push_back(static_cast<std::int64_t>(index));
    }
    if (groups.size() < 2) {
        throw std::invalid_argument("Classification training requires at least two classes.");
    }

    IndexSplit split;
    for (auto& [label, indices] : groups) {
        (void)label;
        if (indices.size() < 2) {
            throw std::invalid_argument(
                "Each class needs at least two samples for a stratified split.");
        }

        std::mt19937 generator(seed ^ static_cast<std::uint32_t>(label));
        std::shuffle(indices.begin(), indices.end(), generator);
        const auto validationSize = validationCount(indices.size(), validationRatio);
        split.validation.insert(split.validation.end(), indices.begin(),
                                indices.begin() + validationSize);
        split.train.insert(split.train.end(), indices.begin() + validationSize,
                           indices.end());
    }

    std::mt19937 trainGenerator(seed);
    std::mt19937 validationGenerator(seed + 1);
    std::shuffle(split.train.begin(), split.train.end(), trainGenerator);
    std::shuffle(split.validation.begin(), split.validation.end(), validationGenerator);
    return split;
}

} // namespace nnmtm
