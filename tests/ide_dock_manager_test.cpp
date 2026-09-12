#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/IDEDockManager.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/CreatorConsoleState.hpp"
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

    // 10. CreatorConsole state section transitions
    auto& consoleState = Rendering::getCreatorConsoleState();
    consoleState.currentSection = CreatorSection::Paint;
    assert(consoleState.currentSection == CreatorSection::Paint);
    consoleState.currentSection = CreatorSection::Assets;
    assert(consoleState.currentSection == CreatorSection::Assets);
    consoleState.currentSection = CreatorSection::Create3D;
    assert(consoleState.currentSection == CreatorSection::Create3D);
    consoleState.currentSection = CreatorSection::Zones;
    assert(consoleState.currentSection == CreatorSection::Zones);
    std::cout << "  ✓ Creator Console section state transitions pass" << std::endl;

    // 11. Slot layout mode (Tabbed vs Stacked)
    dockMgr.resetToDefaultLayout();
    assert(dockMgr.slotLayout(DockSlot::Left) == SlotLayout::Tabbed);
    assert(dockMgr.slotLayout(DockSlot::Right) == SlotLayout::Tabbed);
    assert(dockMgr.slotLayout(DockSlot::Bottom) == SlotLayout::Tabbed);

    dockMgr.setSlotLayout(DockSlot::Left, SlotLayout::Stacked);
    assert(dockMgr.slotLayout(DockSlot::Left) == SlotLayout::Stacked);
    dockMgr.toggleSlotLayout(DockSlot::Left);
    assert(dockMgr.slotLayout(DockSlot::Left) == SlotLayout::Tabbed);
    dockMgr.toggleSlotLayout(DockSlot::Left);
    assert(dockMgr.slotLayout(DockSlot::Left) == SlotLayout::Stacked);

    dockMgr.setSlotLayout(DockSlot::Bottom, SlotLayout::Stacked);
    assert(dockMgr.slotLayout(DockSlot::Bottom) == SlotLayout::Stacked);
    dockMgr.toggleSlotLayout(DockSlot::Bottom);
    assert(dockMgr.slotLayout(DockSlot::Bottom) == SlotLayout::Tabbed);

    dockMgr.resetToDefaultLayout();
    assert(dockMgr.slotLayout(DockSlot::Left) == SlotLayout::Tabbed);
    assert(dockMgr.slotLayout(DockSlot::Bottom) == SlotLayout::Tabbed);
    std::cout << "  ✓ Slot layout mode (Tabbed vs Stacked) passes" << std::endl;

    // 12. Stacked window state (accordion collapse and dimensions)
    auto* creatorWin = dockMgr.findWindow("creator_console");
    auto* chatWin = dockMgr.findWindow("chat");
    assert(creatorWin != nullptr && chatWin != nullptr);
    assert(!creatorWin->collapsedInStack);
    creatorWin->collapsedInStack = true;
    creatorWin->stackHeight = 250.0f;
    chatWin->stackWidth = 400.0f;
    assert(creatorWin->collapsedInStack);
    assert(creatorWin->stackHeight == 250.0f);
    assert(chatWin->stackWidth == 400.0f);

    dockMgr.resetToDefaultLayout();
    assert(!creatorWin->collapsedInStack);
    assert(creatorWin->stackHeight == 0.0f);
    assert(chatWin->stackWidth == 0.0f);
    std::cout << "  ✓ Stacked window state & reset passes" << std::endl;

    // 13. Sequence priority & corner collision resolution
    // Initially Left seq > Bottom seq
    dockMgr.resetToDefaultLayout();
    uint64_t leftSeq = dockMgr.slotSeq(DockSlot::Left);
    uint64_t bottomSeq = dockMgr.slotSeq(DockSlot::Bottom);
    assert(leftSeq > bottomSeq);

    // Opening or touching Bottom slot makes Bottom seq higher
    dockMgr.touchSlot(DockSlot::Bottom);
    uint64_t bottomSeq2 = dockMgr.slotSeq(DockSlot::Bottom);
    assert(bottomSeq2 > leftSeq);

    // Helper checking the corner priority logic:
    // When left and bottom both open:
    // if leftSeq > bottomSeq -> left fills corner (leftWins = true)
    // if bottomSeq >= leftSeq -> bottom fills corner (leftWins = false)
    auto doesLeftWinCorner = [](uint64_t lSeq, uint64_t bSeq) {
        return lSeq > bSeq;
    };
    assert(!doesLeftWinCorner(dockMgr.slotSeq(DockSlot::Left), dockMgr.slotSeq(DockSlot::Bottom)));

    // Now touching Left slot (e.g. user clicked tab or tool in left sidebar)
    dockMgr.touchSlot(DockSlot::Left);
    assert(doesLeftWinCorner(dockMgr.slotSeq(DockSlot::Left), dockMgr.slotSeq(DockSlot::Bottom)));

    // Symmetrically for Right vs Bottom
    dockMgr.touchSlot(DockSlot::Bottom);
    auto doesRightWinCorner = [](uint64_t rSeq, uint64_t bSeq) {
        return rSeq > bSeq;
    };
    assert(!doesRightWinCorner(dockMgr.slotSeq(DockSlot::Right), dockMgr.slotSeq(DockSlot::Bottom)));
    dockMgr.touchSlot(DockSlot::Right);
    assert(doesRightWinCorner(dockMgr.slotSeq(DockSlot::Right), dockMgr.slotSeq(DockSlot::Bottom)));
    std::cout << "  ✓ Sequence priority & corner collision resolution logic passes" << std::endl;

    std::cout << "All IDEDockManager tests PASSED!" << std::endl;
    return 0;
}
