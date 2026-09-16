// Live-path regression witness for Zach's 2026-09-15 report: after the
// Living Studio restoration, an isolated Zone-store boot passed but the real
// desktop boot still refused Creator Console -> Zones -> Move to
// SynthesisStudio.LivingInstrument. The difference is that the real app
// hydrates every checked-in Zone before the move.
//
// This witness intentionally uses the complete checked-in saves/zones store,
// protected by RealSaveTreeGuard, and performs no legacy World load. A failure
// must stay loud: ZoneManager::switchTo() prints the exact preflight refusal to
// stderr, which CI preserves with --output-on-failure.

#include "support/test_harness.hpp"

#include <cstddef>
#include <iostream>
#include <string>

int main() {
    TestSupport::RealSaveTreeGuard guard(TestSupport::GuardCurrentRoot);
    TestSupport::BootedEngineHarness harness("Zach");

    std::size_t livingIndex = harness.zones.zones().size();
    for (std::size_t i = 0; i < harness.zones.zones().size(); ++i) {
        const auto& zone = harness.zones.zones()[i];
        if (zone && zone->getIdentifier() == "SynthesisStudio.LivingInstrument") {
            livingIndex = i;
            break;
        }
    }

    if (livingIndex >= harness.zones.zones().size()) {
        std::cerr << "full-store boot did not discover SynthesisStudio.LivingInstrument\n";
        return 1;
    }

    std::cout << "full-store boot hydrated " << harness.zones.zones().size()
              << " Zones; attempting Living Studio move\n";

    if (!harness.zones.switchTo(livingIndex)) {
        std::cerr << "full-store fresh boot reproduced Zach's Move-to-Zone refusal\n";
        return 1;
    }

    std::cout << "full-store fresh boot moved to Living Studio successfully\n";
    return 0;
}
