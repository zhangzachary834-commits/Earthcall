#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/IDEDockManager.hpp"
#include <cassert>
#include <iostream>

using namespace Rendering;

int main() {
    std::cout << "=== Running IDEDockManager Tests ===" << std::endl;

    auto& dockMgr = IDEDockManager::instance();

    // 1. Initial mode
    dockMgr.setIDEMode(false);
    assert(!dockMgr.isIDEMode());
    dockMgr.toggleIDEMode();
    assert(dockMgr.isIDEMode());
    dockMgr.toggleIDEMode();
    assert(!dockMgr.isIDEMode());
    std::cout << "  ✓ IDE mode toggling passes" << std::endl;

    // 2. Window registration
    bool creatorOpen = false;
    bool devToolsOpen = false;
    bool perfOpen = false;
    bool chatOpen = false;

    dockMgr.registerWindow("creator_console", "Creator Console", "F8",
        DockSlot::Left, &creatorOpen, [](){});
    dockMgr.registerWindow("dev_tools", "Developer Tools", "`",
        DockSlot::Left, &devToolsOpen, [](){});
    dockMgr.registerWindow("perf_metrics", "Performance & Coords", "F3",
        DockSlot::Right, &perfOpen, [](){});
    dockMgr.registerWindow("chat", "Chat", "H",
        DockSlot::Bottom, &chatOpen, [](){});

    assert(dockMgr.findWindow("creator_console") != nullptr);
    assert(dockMgr.findWindow("dev_tools") != nullptr);
    assert(dockMgr.findWindow("perf_metrics") != nullptr);
    assert(dockMgr.findWindow("chat") != nullptr);
    assert(dockMgr.findWindow("non_existent") == nullptr);
    std::cout << "  ✓ Window registration and lookup passes" << std::endl;

    // 3. Slot filtering
    auto leftWins = dockMgr.getWindowsInSlot(DockSlot::Left);
    assert(leftWins.size() == 2);
    auto rightWins = dockMgr.getWindowsInSlot(DockSlot::Right);
    assert(rightWins.size() == 1);
    auto bottomWins = dockMgr.getWindowsInSlot(DockSlot::Bottom);
    assert(bottomWins.size() == 1);
    std::cout << "  ✓ Slot membership passes" << std::endl;

    // 4. Moving window between slots
    dockMgr.setWindowSlot("chat", DockSlot::Right);
    assert(dockMgr.getWindowsInSlot(DockSlot::Bottom).empty());
    assert(dockMgr.getWindowsInSlot(DockSlot::Right).size() == 2);
    std::cout << "  ✓ Window relocation between dock slots passes" << std::endl;

    // 5. Pop out to floating
    dockMgr.setWindowSlot("creator_console", DockSlot::Floating);
    assert(dockMgr.getWindowsInSlot(DockSlot::Left).size() == 1);
    assert(dockMgr.getWindowsInSlot(DockSlot::Floating).size() == 1);
    std::cout << "  ✓ Detaching window to float passes" << std::endl;

    // 6. Active tabs
    dockMgr.setActiveTabInSlot(DockSlot::Left, "dev_tools");
    assert(dockMgr.activeTabInSlot(DockSlot::Left) == "dev_tools");
    std::cout << "  ✓ Active tab selection passes" << std::endl;

    // 7. Dimension clamping
    dockMgr.setLeftWidth(50.0f);
    assert(dockMgr.leftWidth() >= 180.0f);
    dockMgr.setRightWidth(20.0f);
    assert(dockMgr.rightWidth() >= 180.0f);
    dockMgr.setBottomHeight(10.0f);
    assert(dockMgr.bottomHeight() >= 100.0f);
    std::cout << "  ✓ Dimension clamping passes" << std::endl;

    // 8. Collapse toggling
    assert(!dockMgr.isLeftCollapsed());
    dockMgr.toggleLeftCollapsed();
    assert(dockMgr.isLeftCollapsed());
    dockMgr.toggleLeftCollapsed();
    assert(!dockMgr.isLeftCollapsed());

    assert(!dockMgr.isRightCollapsed());
    dockMgr.toggleRightCollapsed();
    assert(dockMgr.isRightCollapsed());
    dockMgr.toggleRightCollapsed();
    assert(!dockMgr.isRightCollapsed());

    assert(!dockMgr.isBottomCollapsed());
    dockMgr.toggleBottomCollapsed();
    assert(dockMgr.isBottomCollapsed());
    dockMgr.toggleBottomCollapsed();
    assert(!dockMgr.isBottomCollapsed());
    std::cout << "  ✓ Collapse states toggling passes" << std::endl;

    // 9. Reset layout
    dockMgr.resetToDefaultLayout();
    assert(dockMgr.findWindow("creator_console")->currentSlot == DockSlot::Left);
    assert(dockMgr.findWindow("chat")->currentSlot == DockSlot::Bottom);
    assert(dockMgr.leftWidth() == 380.0f);
    assert(dockMgr.rightWidth() == 380.0f);
    assert(dockMgr.bottomHeight() == 240.0f);
    std::cout << "  ✓ Reset to default layout passes" << std::endl;

    std::cout << "All IDEDockManager tests PASSED!" << std::endl;
    return 0;
}
