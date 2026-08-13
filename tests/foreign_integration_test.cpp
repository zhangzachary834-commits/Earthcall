#include "Singularity/Foreign/ForeignChannel.hpp"
#include "Singularity/Foreign/AsyncStateLogger.hpp"
#include "Singularity/Foreign/InferenceLawBridge.hpp"
#include "Singularity/Foreign/ForeignSyncManager.hpp"
#include "Singularity/Foreign/Adapters/MacOSAccessibilityAdapter.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"   // LawManager is declared here
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdio>
#include <string>

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "foreign_integration_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "foreign_integration_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "foreign_integration_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        // 1. Test ForeignChannel properties
        ForeignChannel channel("calendar");
        assert(channel.foreignAppName() == "calendar");
        assert(channel.isFirstMover() == true);

        // The addressing contract. resolveBeingToken matches `@token` against
        // getIdentifier(), so this is what makes `@foreign-channel.calendar.enabled`
        // resolvable at all — and it must not drift back to a generated law-<N>.
        assert(channel.getIdentifier() == "foreign-channel.calendar");

        // Exactly three, once. Guards the lazy-build contract: buildProperties
        // must not also be called from the constructor, or every property here
        // is registered twice.
        assert(channel.listProperties().size() == 3);

        PropertyValue v;
        assert(PropertyPath::parse("enabled").getValue(channel, v) == PropertyPath::PathResult::Ok);
        assert(std::get<bool>(v) == true);

        // 2. Test AsyncStateLogger basic buffering
        AsyncStateLogger logger;
        logger.logEvent("btn-1", "label", "Next", 1.0);
        logger.flush();
        
        // 3. Test InferenceLawBridge properties (hybrid ML approach)
        InferenceLawBridge inference("calendar_classifier");
        assert(inference.isFirstMover() == true);
        assert(inference.getIdentifier() == "inference.calendar_classifier");
        assert(inference.listProperties().size() == 3);
        assert(PropertyPath::parse("confidence_threshold").getValue(inference, v) == PropertyPath::PathResult::Ok);
        assert(std::get<float>(v) == 0.85f);

        // 4. Test Adapter and SyncManager initialization.
        // NOTE: everything below is a smoke test over empty scaffolds — it
        // proves construction and linkage, not behavior. The adapter polls
        // nothing and the sync manager subscribes to nothing yet.
        Universe& universe = Universe::instance();   // singleton; ctor is private
        LawManager laws;
        MacOSAccessibilityAdapter adapter(universe, laws);
        ForeignSyncManager syncManager(universe, laws);
        syncManager.initialize();
        
        // Verify we can call act methods without crashing (scaffolds)
        adapter.executeClick("os-btn-123");
        adapter.executeMove("os-window-123", 10.0f, 20.0f);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("foreign_integration_test: ALL OK");
    return 0;
}
