#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <algorithm>
#include <imgui.h>

class ZoneManager;
class Person;
class Object;
struct GLFWwindow;
namespace Core { class Engine; }

namespace Rendering {

enum class DockSlot {
    Floating,   // Free-floating window
    Left,       // Attached to Left Sidebar
    Right,      // Attached to Right Sidebar
    Bottom      // Attached to Bottom Bar / Drawer
};

struct DockableWindow {
    std::string id;
    std::string title;
    std::string shortcut;
    DockSlot defaultSlot = DockSlot::Floating;
    DockSlot currentSlot = DockSlot::Floating;
    bool* open = nullptr;
    std::function<void()> renderContent = nullptr;
};

class IDEDockManager {
public:
    static IDEDockManager& instance();

    // Mode toggles
    bool isIDEMode() const { return _ideMode; }
    void setIDEMode(bool enabled) { _ideMode = enabled; }
    void toggleIDEMode() { _ideMode = !_ideMode; }

    // Top activity bar visibility
    bool isTopBarVisible() const { return _showTopBar; }
    void setTopBarVisible(bool v) { _showTopBar = v; }
    void toggleTopBar() { _showTopBar = !_showTopBar; }

    // Dimensions
    float leftWidth() const { return _leftWidth; }
    void setLeftWidth(float w) { _leftWidth = std::max(180.0f, w); }
    float rightWidth() const { return _rightWidth; }
    void setRightWidth(float w) { _rightWidth = std::max(180.0f, w); }
    float bottomHeight() const { return _bottomHeight; }
    void setBottomHeight(float h) { _bottomHeight = std::max(100.0f, h); }

    // Collapsed states
    bool isLeftCollapsed() const { return _leftCollapsed; }
    void setLeftCollapsed(bool c) { _leftCollapsed = c; }
    void toggleLeftCollapsed() { _leftCollapsed = !_leftCollapsed; }

    bool isRightCollapsed() const { return _rightCollapsed; }
    void setRightCollapsed(bool c) { _rightCollapsed = c; }
    void toggleRightCollapsed() { _rightCollapsed = !_rightCollapsed; }

    bool isBottomCollapsed() const { return _bottomCollapsed; }
    void setBottomCollapsed(bool c) { _bottomCollapsed = c; }
    void toggleBottomCollapsed() { _bottomCollapsed = !_bottomCollapsed; }

    // Window registration & retrieval
    void registerWindow(const std::string& id, const std::string& title, const std::string& shortcut,
                        DockSlot defaultSlot, bool* openPtr, std::function<void()> renderContent);
    DockableWindow* findWindow(const std::string& id);
    std::vector<DockableWindow*> getWindowsInSlot(DockSlot slot);
    const std::vector<DockableWindow>& allWindows() const { return _windows; }

    // Slot assignment
    void setWindowSlot(const std::string& id, DockSlot slot);

    // Active tab in each slot
    std::string activeTabInSlot(DockSlot slot) const;
    void setActiveTabInSlot(DockSlot slot, const std::string& windowId);

    // Reset layout
    void resetToDefaultLayout();

    // Master render call, called each frame from Engine::tick
    void render(Core::Engine* engine, ZoneManager& zoneMgr, GLFWwindow* window);

private:
    IDEDockManager();
    ~IDEDockManager() = default;
    IDEDockManager(const IDEDockManager&) = delete;
    IDEDockManager& operator=(const IDEDockManager&) = delete;

    void renderTopWorkspaceBar(Core::Engine* engine);
    void renderLeftSidebar(float topY, float availH, float availW);
    void renderRightSidebar(float topY, float availH, float availW);
    void renderBottomBar(float leftX, float bottomY, float barW, float barH);
    void renderFloatingWindows();

    void renderPanelHeader(DockSlot slot, DockableWindow* activeWin, const std::vector<DockableWindow*>& dockedWins);

    bool _ideMode = false;
    bool _showTopBar = true;

    float _leftWidth = 380.0f;
    float _rightWidth = 380.0f;
    float _bottomHeight = 240.0f;

    bool _leftCollapsed = false;
    bool _rightCollapsed = false;
    bool _bottomCollapsed = false;

    std::string _activeLeftTab;
    std::string _activeRightTab;
    std::string _activeBottomTab;

    std::vector<DockableWindow> _windows;
};

} // namespace Rendering
