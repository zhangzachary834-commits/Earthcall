#include "src/Singularity/Core/Logger.hpp"
#include <iostream>
#include <vector>
#include <chrono>

using namespace ECA;

int main() {
    auto& logger = Logger::instance();
    logger.setLevel(LogLevel::Off);
    for (int i = 0; i < 6; ++i) {
        logger.setCategoryLevel(static_cast<LogCategory>(i), LogLevel::Off);
    }

    auto start = std::chrono::high_resolution_clock::now();
    const int num_iterations = 10000000;
    long long sum = 0;
    for (int i = 0; i < num_iterations; ++i) {
        sum += static_cast<int>(logger.categoryLevel(static_cast<LogCategory>(i % 6)));
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "Sum: " << sum << "\n";
    std::cout << "Time for " << num_iterations << " wouldLog checks: " << diff.count() << " seconds\n";
    std::cout << "Checks per second: " << num_iterations / diff.count() << "\n";

    return 0;
}
