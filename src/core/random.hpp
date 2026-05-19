#pragma once
#include <random>

namespace vb {

// Единый генератор на процесс — аналог модульного random в Python.
inline std::mt19937& rng() {
    static std::mt19937 gen{std::random_device{}()};
    return gen;
}

inline double uniform(double lo, double hi) {
    return std::uniform_real_distribution<double>(lo, hi)(rng());
}

inline double gauss(double mean, double sigma) {
    return std::normal_distribution<double>(mean, sigma)(rng());
}

}  // namespace vb
