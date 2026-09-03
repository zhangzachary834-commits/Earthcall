#include "Singularity/Foreign/ForeignChannel.hpp"
#include "Singularity/Foreign/Sync/AsyncStateLogger.hpp"
#include "Singularity/Foreign/Sync/InferenceLawBridge.hpp"
#include "Singularity/Foreign/Sync/ForeignSyncManager.hpp"
#include "Singularity/Foreign/Adapters/MacOSAccessibilityAdapter.hpp"
#include "Singularity/Foreign/Web/WindowManager.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"   // LawManager is declared here
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdio>
#include <map>
#include <string>

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "foreign_integration_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "foreign_integration_test", nullptr, nullptr);
    if (window) {
        glfwMakeContextCurrent(window);
    }

    {
        // 1. Test ForeignChannel properties
        ForeignChannel channel("calendar");
        assert(channel.foreignAppName() == "calendar");
        assert(channel.isFirstMover() == true);

        // The addressing contract. resolveBeingToken matches `@token` against
        // getIdentifier(), so this is what makes `@foreign-channel.calendar.enabled`
        // resolvable at all — and it must not drift back to a generated law-<N>.
        assert(channel.getIdentifier() == "foreign-channel.calendar");

        // The lazy-build contract: buildProperties must not ALSO be called from
        // the constructor, or every property is registered twice.
        //
        // Asserted by NAME rather than by total count. The total was hard-coded
        // at 3 and went stale the moment Singular::registerTelosProperty landed
        // (a1aeab5a) and gave every being a "telos" — a deliberate change that
        // turned this test red for something it was never watching. What it
        // actually guards is "no name appears twice", and that survives the base
        // vocabulary growing.
        {
            std::map<std::string, int> seen;
            for (Property* property : channel.listProperties()) seen[property->name()]++;
            for (const auto& [name, count] : seen) {
                if (count != 1) {
                    std::fprintf(stderr, "property '%s' registered %d times\n", name.c_str(), count);
                }
                assert(count == 1);
            }
            assert(seen.count("enabled") && seen.count("connected") && seen.count("rate_limit"));
        }

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
        // By name, not by total — same reason as the channel above.
        {
            std::map<std::string, int> seen;
            for (Property* property : inference.listProperties()) seen[property->name()]++;
            for (const auto& [name, count] : seen) {
                if (count != 1) {
                    std::fprintf(stderr, "property '%s' registered %d times\n", name.c_str(), count);
                }
                assert(count == 1);
            }
            assert(seen.count("confidence_threshold"));
        }
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

        // 5. Test Integration::WindowManager and Integration::ExternalWindow
        Integration::WindowManager& wm = Integration::WindowManager::instance();
        Integration::ExternalWindow::Config winConfig;
        winConfig.name = "test_overlay";
        winConfig.process_name = "test_proc";
        winConfig.window_title = "Test Overlay Window";
        winConfig.allow_overlay = true;

        assert(wm.registerWindow(winConfig) == true);
        assert(wm.registerWindow(winConfig) == false); // Duplicate registration fails

        Integration::ExternalWindow* extWin = wm.getWindow("test_overlay");
        assert(extWin != nullptr);
        assert(extWin->isOverlayMode() == false);

        extWin->setOverlayMode(true);
        assert(extWin->isOverlayMode() == true);
        assert(wm.isAnyWindowOverlayed() == true);

        extWin->setTransparency(0.5f);
        assert(extWin->getTransparency() == 0.5f);

        extWin->setOverlayMode(false);
        assert(extWin->isOverlayMode() == false);
        assert(wm.isAnyWindowOverlayed() == false);

        wm.unregisterWindow("test_overlay");
        assert(wm.getWindow("test_overlay") == nullptr);
    }

    if (window) glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("foreign_integration_test: ALL OK");
    return 0;
}
