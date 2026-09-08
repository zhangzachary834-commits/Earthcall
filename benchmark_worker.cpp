#include "src/Singularity/Core/Logger.hpp"
#include <chrono>
#include <iostream>
#include <vector>
#include <thread>
#include <atomic>

// To access private members we can use a trick, but it's easier to just time the worker
// Let's modify the queue size limit? No limit.

using namespace ECA;

int main() {
    auto& logger = Logger::instance();
    logger.setLevel(LogLevel::Verbose);
    logger.setCategoryLevel(LogCategory::System, LogLevel::Verbose);

    const int num_iterations = 200000;

    // Pre-allocate strings to avoid measuring string allocation
    std::string type = "BENCHMARK";
    std::string msg = "This is a test message.";
    nlohmann::json details = nlohmann::json::object();

    auto start_enqueue = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < num_iterations; ++i) {
        logger.log(LogCategory::System, type, msg, details);
    }

    auto end_enqueue = std::chrono::high_resolution_clock::now();

    // The worker is running in the background. Shutdown waits for it to finish the queue.
    logger.shutdown();

    auto end_process = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> diff_enqueue = end_enqueue - start_enqueue;
    std::chrono::duration<double> diff_process = end_process - end_enqueue;
    std::chrono::duration<double> diff_total = end_process - start_enqueue;

    std::cout << "Time to enqueue: " << diff_enqueue.count() << " seconds\n";
    std::cout << "Time to process (shutdown wait): " << diff_process.count() << " seconds\n";
    std::cout << "Total time: " << diff_total.count() << " seconds\n";
    std::cout << "Total logs per second: " << num_iterations / diff_total.count() << "\n";

    return 0;
}
