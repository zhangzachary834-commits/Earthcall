#include "Singularity/Foreign/ForeignChannel.hpp"
#include "Singularity/Foreign/AsyncStateLogger.hpp"
#include "Singularity/Foreign/InferenceLawBridge.hpp"
#include "Singularity/Foreign/ForeignSyncManager.hpp"
#include "Singularity/Foreign/Adapters/MacOSAccessibilityAdapter.hpp"
#include "ZonesOfEarth/Universe.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawManager.hpp"
#include "ConstructedBeing/Property/PropertyPath.hpp"

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
        assert(PropertyPath::parse("confidence_threshold").getValue(inference, v) == PropertyPath::PathResult::Ok);
        assert(std::get<float>(v) == 0.85f);
        
        // 4. Test Adapter and SyncManager initialization
        Universe universe;
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
