#include "src/Singularity/Core/Logger.hpp"
#include <chrono>
#include <iostream>
#include <vector>

using namespace ECA;

// Mock the queue processing part
int main() {
    auto& logger = Logger::instance();
    logger.setLevel(LogLevel::Off); // Disable actual file writing if possible, but actually we want to see the unordered_map lookup overhead.

    // Let's create a dummy batch of LogEntries and measure how long it takes to just do the lookup and the condition checks.
    // Wait, the Logger's background worker writes to files, which will dominate the time.

    // Instead, let's just time how fast we can push to the logger, which will test the lock and everything.
    auto start = std::chrono::high_resolution_clock::now();

    const int num_iterations = 1000000;
    for (int i = 0; i < num_iterations; ++i) {
        // Use a mix of categories
        LogCategory cat = static_cast<LogCategory>(i % 6);
        logger.log(cat, "BENCHMARK", "This is a test message.");
    }

    auto end_enqueue = std::chrono::high_resolution_clock::now();
    logger.shutdown();
    auto end_process = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double> diff_total = end_process - start;

    std::cout << "Total time for " << num_iterations << " logs: " << diff_total.count() << " seconds\n";
    std::cout << "Total logs per second: " << num_iterations / diff_total.count() << "\n";

    return 0;
}
