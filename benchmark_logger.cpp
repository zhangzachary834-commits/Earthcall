#include "src/Singularity/Core/Logger.hpp"
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace ECA;

int main() {
    auto& logger = Logger::instance();
    logger.setLevel(LogLevel::Off); // We don't want to actually write files for the benchmark, wait if Off it returns early
    logger.setCategoryLevel(LogCategory::System, LogLevel::Verbose); // Ensure it logs

    auto start = std::chrono::high_resolution_clock::now();

    const int num_iterations = 100000;
    for (int i = 0; i < num_iterations; ++i) {
        logger.log(LogCategory::System, "BENCHMARK", "This is a test message for performance benchmarking.");
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;

    std::cout << "Time taken for " << num_iterations << " logs: " << diff.count() << " seconds\n";
    std::cout << "Logs per second: " << num_iterations / diff.count() << "\n";

    // Wait for the queue to empty
    logger.shutdown();

    return 0;
}
