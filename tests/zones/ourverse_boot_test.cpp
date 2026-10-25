// Headless Ourverse boot-contract witness. Engine::initLogic itself is not
// executed here: BootedEngineHarness mirrors its post-hydration registration.
// This test deliberately avoids a GLFW context and real authored save roots.
#include "support/test_harness.hpp"
#include "support/test_save_helper.hpp"

#include <cassert>
#include <cstdio>

int main() {
    TempSaveRoot sandbox;
    {
        TestSupport::BootedEngineHarness h("Player", "humanoid", false);
        auto gatheringLaw = h.lawManager.find("ourverse-gathering-unowned");
        auto filamentLaw = h.lawManager.find("ourverse-filaments-mutual");
        assert(gatheringLaw && gatheringLaw->isFirstMover());
        assert(filamentLaw && filamentLaw->isFirstMover());

        Zone* gathering = h.ourverse.gatheringZone();
        assert(gathering && gathering->isOurverseGathering());
        assert(gathering->owner().empty());
        int gatheringCount = 0;
        for (const auto& zone : h.zones.zones()) {
            if (zone && zone->isOurverseGathering()) {
                ++gatheringCount;
                assert(zone.get() == gathering);
            }
        }
        assert(gatheringCount == 1);
        // Boot must not duplicate the gathering Zone on a second request.
        assert(&h.ourverse.ensureGatheringZone(h.zones) == gathering);
        int after = 0;
        for (const auto& zone : h.zones.zones())
            if (zone && zone->isOurverseGathering()) ++after;
        assert(after == 1);
    }
    std::puts("ourverse_boot_test: ALL OK");
    return 0;
}
