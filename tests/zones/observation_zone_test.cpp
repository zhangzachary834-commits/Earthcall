#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include <iostream>
#include <memory>
#include <cassert>

namespace {
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
} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running observation_zone_test...\n";
    std::cout << "============================================================\n";

    // 1. Create a regular zone and check if it's an observation zone
    Zone regularZone("RegularZone", "default");
    check(!isObservationZone(regularZone), "A regular zone should not be an observation zone");

    // 2. Create an observation zone and check if it's an observation zone
    // According to src/ZonesOfEarth/ZoneManager.cpp:3319, an observation zone has quality "kind" set to "test-observation"
    Zone observationZone("ObservationZone", "default");
    observationZone.setQuality("kind", "test-observation");
    check(isObservationZone(observationZone), "A zone with quality kind='test-observation' should be an observation zone");

    // 3. Create a zone with a different kind
    Zone differentKindZone("DifferentKindZone", "default");
    differentKindZone.setQuality("kind", "other-kind");
    check(!isObservationZone(differentKindZone), "A zone with a different kind should not be an observation zone");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cerr << "observation_zone_test: FAILED\n";
        return 1;
    }
    std::cout << "observation_zone_test: ALL OK\n";
    return 0;
}
