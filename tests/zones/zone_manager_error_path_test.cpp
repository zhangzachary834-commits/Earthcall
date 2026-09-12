#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include <iostream>
#include <stdexcept>
#include <memory>

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cerr << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

class FailingZone : public Zone {
public:
    FailingZone(const std::string& name) : Zone(name, "strict") {}

    // Override load to simulate a failure by throwing an exception
    void load() override {
        throw std::runtime_error("Simulated zone load failure");
    }
};

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running zone_manager_error_path_test...\n";
    std::cout << "============================================================\n";

    ZoneManager mgr;
    auto failZone = std::make_shared<FailingZone>("FailingZone");
    mgr.addZone(failZone);

    // If the try-catch block is missing in ZoneManager::switchTo, this call will terminate the program
    // If the try-catch block is present, it will catch the exception and proceed.
    bool threw = false;
    try {
        mgr.switchTo(0);
    } catch (...) {
        threw = true;
    }

    check(!threw, "switchTo caught the exception internally and didn't propagate it");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cerr << "zone_manager_error_path_test: FAILED\n";
        return 1;
    }
    std::cout << "zone_manager_error_path_test: ALL OK\n";
    return 0;
}
