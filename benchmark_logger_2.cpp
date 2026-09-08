#include "src/Singularity/Core/Logger.hpp"
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace ECA;

int main() {
    auto& logger = Logger::instance();
    // We want to benchmark the background worker's processing of _streams map lookups.
    // So we will enqueue a lot of messages, then measure how long the worker takes to process them.
    // However, if we do it normally, the worker might process them concurrently.

    // Instead, let's just measure total time including I/O, but we can write to /dev/null to avoid disk bottleneck?
    // Actually, the default logger writes to logs/ system files. We will just measure overall time.

    logger.setLevel(LogLevel::Verbose);
    logger.setCategoryLevel(LogCategory::System, LogLevel::Verbose);

    auto start = std::chrono::high_resolution_clock::now();

    const int num_iterations = 500000;
    for (int i = 0; i < num_iterations; ++i) {
        logger.log(LogCategory::System, "BENCHMARK", "This is a test message for performance benchmarking.");
    }

    auto end_enqueue = std::chrono::high_resolution_clock::now();

    // Wait for the queue to empty
    logger.shutdown();

    auto end_process = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> diff_enqueue = end_enqueue - start;
    std::chrono::duration<double> diff_total = end_process - start;
    std::chrono::duration<double> diff_process = end_process - end_enqueue;

    std::cout << "Time to enqueue: " << diff_enqueue.count() << " seconds\n";
    std::cout << "Time to process (shutdown wait): " << diff_process.count() << " seconds\n";
    std::cout << "Total time: " << diff_total.count() << " seconds\n";
    std::cout << "Total logs per second: " << num_iterations / diff_total.count() << "\n";

    return 0;
}
