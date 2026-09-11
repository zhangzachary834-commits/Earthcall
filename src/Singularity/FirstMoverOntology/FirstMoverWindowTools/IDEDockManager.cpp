#include "IDEDockManager.hpp"
#include "Singularity/Core/Engine.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

namespace Rendering {

IDEDockManager& IDEDockManager::instance() {
    static IDEDockManager inst;
    return inst;
}

IDEDockManager::IDEDockManager() {
    _ideMode = false;
    _showTopBar = true;
    _leftWidth = 380.0f;
    _rightWidth = 380.0f;
    _bottomHeight = 240.0f;
}

void IDEDockManager::registerWindow(const std::string& id, const std::string& title, const std::string& shortcut,
                                    DockSlot defaultSlot, bool* openPtr, std::function<void()> renderContent) {
    for (auto& w : _windows) {
        if (w.id == id) {
            w.title = title;
            w.shortcut = shortcut;
            w.defaultSlot = defaultSlot;
            w.open = openPtr;
            w.renderContent = std::move(renderContent);
            return;
        }
    }

    DockableWindow win;
    win.id = id;
    win.title = title;
    win.shortcut = shortcut;
    win.defaultSlot = defaultSlot;
    win.currentSlot = defaultSlot;
    win.open = openPtr;
    win.renderContent = std::move(renderContent);
    _windows.push_back(std::move(win));
}

DockableWindow* IDEDockManager::findWindow(const std::string& id) {
    for (auto& w : _windows) {
        if (w.id == id) return &w;
    }
    return nullptr;
}

std::vector<DockableWindow*> IDEDockManager::getWindowsInSlot(DockSlot slot) {
    std::vector<DockableWindow*> res;
    for (auto& w : _windows) {
        if (w.currentSlot == slot) {
            res.push_back(&w);
        }
    }
    return res;
}

void IDEDockManager::setWindowSlot(const std::string& id, DockSlot slot) {
    if (auto* w = findWindow(id)) {
        w->currentSlot = slot;
        if (slot == DockSlot::Left) _activeLeftTab = id;
        else if (slot == DockSlot::Right) _activeRightTab = id;
        else if (slot == DockSlot::Bottom) _activeBottomTab = id;
    }
}

std::string IDEDockManager::activeTabInSlot(DockSlot slot) const {
    if (slot == DockSlot::Left) return _activeLeftTab;
    if (slot == DockSlot::Right) return _activeRightTab;
    if (slot == DockSlot::Bottom) return _activeBottomTab;
    return "";
}

void IDEDockManager::setActiveTabInSlot(DockSlot slot, const std::string& windowId) {
    if (slot == DockSlot::Left) _activeLeftTab = windowId;
    else if (slot == DockSlot::Right) _activeRightTab = windowId;
    else if (slot == DockSlot::Bottom) _activeBottomTab = windowId;
}

void IDEDockManager::resetToDefaultLayout() {
    _leftWidth = 380.0f;
    _rightWidth = 380.0f;
    _bottomHeight = 240.0f;
    _leftCollapsed = false;
    _rightCollapsed = false;
    _bottomCollapsed = false;
    for (auto& w : _windows) {
        w.currentSlot = w.defaultSlot;
    }
}

void IDEDockManager::renderTopWorkspaceBar(Core::Engine* engine) {
    const ImGuiIO& io = ImGui::GetIO();
    const float screenW = io.DisplaySize.x;
    constexpr float topBarH = 28.0f;

    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(screenW, topBarH));
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoSavedSettings |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.09f, 0.10f, 0.12f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.22f, 0.26f, 0.80f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 3));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 2));

    if (ImGui::Begin("##IDETopWorkspaceBar", nullptr, flags)) {
        // Workspace brand/mode indicator
        if (_ideMode) {
            ImGui::TextColored(ImVec4(0.25f, 0.85f, 0.95f, 1.0f), "Earthcall IDE");
        } else {
            ImGui::TextColored(ImVec4(0.60f, 0.65f, 0.70f, 1.0f), "Earthcall");
        }
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // Global IDE Mode Toggle Button
        if (_ideMode) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.45f, 0.65f, 0.90f));
            if (ImGui::Button("Docked IDE Mode [F10]")) {
                toggleIDEMode();
            }
            ImGui::PopStyleColor();
        } else {
            if (ImGui::Button("Floating Mode [F10]")) {
                toggleIDEMode();
            }
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Toggle between IDE docked sidebars and free-floating windows (F10)");
        }

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // Dock section toggles when in IDE mode
        if (_ideMode) {
            // Left sidebar toggle
            bool leftHasOpen = false;
            for (auto* w : getWindowsInSlot(DockSlot::Left)) {
                if (w->open && *w->open) { leftHasOpen = true; break; }
            }
            if (_leftCollapsed) {
                if (ImGui::Button("◧ Left (Collapsed)")) toggleLeftCollapsed();
            } else if (leftHasOpen) {
                if (ImGui::Button("◧ Left [F8]")) toggleLeftCollapsed();
            } else {
                if (ImGui::Button("◧ Open Creator [F8]")) {
                    if (auto* w = findWindow("creator_console")) {
                        if (w->open) *w->open = true;
                        _activeLeftTab = "creator_console";
                        _leftCollapsed = false;
                    }
                }
            }
            ImGui::SameLine();

            // Bottom bar toggle
            bool bottomHasOpen = false;
            for (auto* w : getWindowsInSlot(DockSlot::Bottom)) {
                if (w->open && *w->open) { bottomHasOpen = true; break; }
            }
            if (_bottomCollapsed) {
                if (ImGui::Button("⬓ Bottom (Collapsed)")) toggleBottomCollapsed();
            } else if (bottomHasOpen) {
                if (ImGui::Button("⬓ Bottom [H]")) toggleBottomCollapsed();
            } else {
                if (ImGui::Button("⬓ Open Chat [H]")) {
                    if (auto* w = findWindow("chat")) {
                        if (w->open) *w->open = true;
                        _activeBottomTab = "chat";
                        _bottomCollapsed = false;
                    }
                }
            }
            ImGui::SameLine();

            // Right sidebar toggle
            bool rightHasOpen = false;
            for (auto* w : getWindowsInSlot(DockSlot::Right)) {
                if (w->open && *w->open) { rightHasOpen = true; break; }
            }
            if (_rightCollapsed) {
                if (ImGui::Button("◨ Right (Collapsed)")) toggleRightCollapsed();
            } else if (rightHasOpen) {
                if (ImGui::Button("◨ Right [F3]")) toggleRightCollapsed();
            } else {
                if (ImGui::Button("◨ Open Metrics [F3]")) {
                    if (auto* w = findWindow("perf_metrics")) {
                        if (w->open) *w->open = true;
                        _activeRightTab = "perf_metrics";
                        _rightCollapsed = false;
                    }
                }
            }
            ImGui::SameLine();
            ImGui::TextDisabled("|");
            ImGui::SameLine();
        }

        // Quick window buttons
        for (auto& win : _windows) {
            if (!win.open) continue;
            bool isOpen = *win.open;
            if (isOpen) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.40f, 0.30f, 0.90f));
            }
            std::string btnLabel = win.shortcut.empty() ? win.id : (win.id + " [" + win.shortcut + "]");
            if (ImGui::Button(btnLabel.c_str())) {
                *win.open = !isOpen;
                if (!isOpen) {
                    if (win.currentSlot == DockSlot::Left) { _activeLeftTab = win.id; _leftCollapsed = false; }
                    else if (win.currentSlot == DockSlot::Right) { _activeRightTab = win.id; _rightCollapsed = false; }
                    else if (win.currentSlot == DockSlot::Bottom) { _activeBottomTab = win.id; _bottomCollapsed = false; }
                    if (engine) engine->ensureCursorUnlocked();
                }
            }
            if (isOpen) {
                ImGui::PopStyleColor();
            }
            ImGui::SameLine();
        }

        // Layout reset button on far right
        const float rightAlignX = screenW - 85.0f;
        if (ImGui::GetCursorPosX() < rightAlignX) {
            ImGui::SetCursorPosX(rightAlignX);
        }
        if (ImGui::SmallButton("Reset Layout")) {
            resetToDefaultLayout();
        }
    }
    ImGui::End();

    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(2);
}

void IDEDockManager::renderPanelHeader(DockSlot slot, DockableWindow* activeWin, const std::vector<DockableWindow*>& dockedWins) {
    if (dockedWins.empty()) return;

    // Tabs for windows in this slot
    for (size_t i = 0; i < dockedWins.size(); ++i) {
        auto* win = dockedWins[i];
        bool isActive = (win == activeWin);

        if (isActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.45f, 0.65f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.16f, 0.19f, 0.8f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.70f, 0.75f, 0.80f, 1.0f));
        }

        std::string tabTitle = win->title;
        if (ImGui::Button(tabTitle.c_str())) {
            setActiveTabInSlot(slot, win->id);
        }
        ImGui::PopStyleColor(2);
        ImGui::SameLine();
    }

    // Dock control actions on the right side of header
    if (activeWin) {
        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        // Dock to Left
        if (slot != DockSlot::Left) {
            if (ImGui::SmallButton("◧ Left")) {
                setWindowSlot(activeWin->id, DockSlot::Left);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Attach window to Left Sidebar");
            ImGui::SameLine();
        }

        // Dock to Bottom
        if (slot != DockSlot::Bottom) {
            if (ImGui::SmallButton("⬓ Bottom")) {
                setWindowSlot(activeWin->id, DockSlot::Bottom);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Attach window to Bottom Bar");
            ImGui::SameLine();
        }

        // Dock to Right
        if (slot != DockSlot::Right) {
            if (ImGui::SmallButton("◨ Right")) {
                setWindowSlot(activeWin->id, DockSlot::Right);
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Attach window to Right Sidebar");
            ImGui::SameLine();
        }

        // Pop out to floating
        if (ImGui::SmallButton("❐ Float")) {
            setWindowSlot(activeWin->id, DockSlot::Floating);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Detach window into free-floating window");
        ImGui::SameLine();

        // Collapse slot
        if (slot == DockSlot::Left) {
            if (ImGui::SmallButton("◀")) toggleLeftCollapsed();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Collapse Left Sidebar");
        } else if (slot == DockSlot::Right) {
            if (ImGui::SmallButton("▶")) toggleRightCollapsed();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Collapse Right Sidebar");
        } else if (slot == DockSlot::Bottom) {
            if (ImGui::SmallButton("▼")) toggleBottomCollapsed();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Collapse Bottom Bar");
        }
        ImGui::SameLine();

        // Close window button
        if (ImGui::SmallButton("✕")) {
            if (activeWin->open) *activeWin->open = false;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Close window");
    }

    ImGui::NewLine();
    ImGui::Separator();
}

void IDEDockManager::renderLeftSidebar(float topY, float availH, float availW) {
    std::vector<DockableWindow*> dockedWins;
    for (auto* w : getWindowsInSlot(DockSlot::Left)) {
        if (w->open && *w->open) {
            dockedWins.push_back(w);
        }
    }

    if (dockedWins.empty()) return;

    if (_leftCollapsed) {
        // Collapsed slim edge bar
        constexpr float slimW = 28.0f;
        ImGui::SetNextWindowPos(ImVec2(0, topY));
        ImGui::SetNextWindowSize(ImVec2(slimW, availH));
        ImGuiWindowFlags slimFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;
        if (ImGui::Begin("##LeftSidebarCollapsed", nullptr, slimFlags)) {
            if (ImGui::Button("▶", ImVec2(18, 24))) {
                toggleLeftCollapsed();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Expand Left Sidebar");
        }
        ImGui::End();
        return;
    }

    float width = std::clamp(_leftWidth, 200.0f, availW * 0.5f);
    ImGui::SetNextWindowPos(ImVec2(0, topY));
    ImGui::SetNextWindowSize(ImVec2(width, availH));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_MenuBar |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.13f, 0.15f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.26f, 0.30f, 0.90f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

    if (ImGui::Begin("##IDEDockLeft", nullptr, flags)) {
        // Determine active window in this slot
        DockableWindow* activeWin = nullptr;
        for (auto* w : dockedWins) {
            if (w->id == _activeLeftTab) {
                activeWin = w;
                break;
            }
        }
        if (!activeWin && !dockedWins.empty()) {
            activeWin = dockedWins.front();
            _activeLeftTab = activeWin->id;
        }

        renderPanelHeader(DockSlot::Left, activeWin, dockedWins);

        if (activeWin && activeWin->renderContent) {
            ImGui::BeginChild("##LeftContentRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            activeWin->renderContent();
            ImGui::EndChild();
        }
    }
    ImGui::End();

    // Draggable splitter on right border of Left Sidebar
    {
        ImGui::SetNextWindowPos(ImVec2(width - 4.0f, topY));
        ImGui::SetNextWindowSize(ImVec2(8.0f, availH));
        ImGuiWindowFlags splitFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                      ImGuiWindowFlags_NoBackground;
        if (ImGui::Begin("##LeftSplitterWindow", nullptr, splitFlags)) {
            ImGui::InvisibleButton("##LeftSplitterBtn", ImVec2(8.0f, availH));
            if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }
            if (ImGui::IsItemActive()) {
                _leftWidth = std::clamp(_leftWidth + ImGui::GetIO().MouseDelta.x, 200.0f, availW * 0.5f);
            }
        }
        ImGui::End();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void IDEDockManager::renderRightSidebar(float topY, float availH, float availW) {
    std::vector<DockableWindow*> dockedWins;
    for (auto* w : getWindowsInSlot(DockSlot::Right)) {
        if (w->open && *w->open) {
            dockedWins.push_back(w);
        }
    }

    if (dockedWins.empty()) return;

    if (_rightCollapsed) {
        // Collapsed slim edge bar
        constexpr float slimW = 28.0f;
        ImGui::SetNextWindowPos(ImVec2(availW - slimW, topY));
        ImGui::SetNextWindowSize(ImVec2(slimW, availH));
        ImGuiWindowFlags slimFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;
        if (ImGui::Begin("##RightSidebarCollapsed", nullptr, slimFlags)) {
            if (ImGui::Button("◀", ImVec2(18, 24))) {
                toggleRightCollapsed();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Expand Right Sidebar");
        }
        ImGui::End();
        return;
    }

    float width = std::clamp(_rightWidth, 200.0f, availW * 0.5f);
    float posX = availW - width;
    ImGui::SetNextWindowPos(ImVec2(posX, topY));
    ImGui::SetNextWindowSize(ImVec2(width, availH));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_MenuBar |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.12f, 0.13f, 0.15f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.26f, 0.30f, 0.90f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

    if (ImGui::Begin("##IDEDockRight", nullptr, flags)) {
        DockableWindow* activeWin = nullptr;
        for (auto* w : dockedWins) {
            if (w->id == _activeRightTab) {
                activeWin = w;
                break;
            }
        }
        if (!activeWin && !dockedWins.empty()) {
            activeWin = dockedWins.front();
            _activeRightTab = activeWin->id;
        }

        renderPanelHeader(DockSlot::Right, activeWin, dockedWins);

        if (activeWin && activeWin->renderContent) {
            ImGui::BeginChild("##RightContentRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            activeWin->renderContent();
            ImGui::EndChild();
        }
    }
    ImGui::End();

    // Draggable splitter on left border of Right Sidebar
    {
        ImGui::SetNextWindowPos(ImVec2(posX - 4.0f, topY));
        ImGui::SetNextWindowSize(ImVec2(8.0f, availH));
        ImGuiWindowFlags splitFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                      ImGuiWindowFlags_NoBackground;
        if (ImGui::Begin("##RightSplitterWindow", nullptr, splitFlags)) {
            ImGui::InvisibleButton("##RightSplitterBtn", ImVec2(8.0f, availH));
            if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            }
            if (ImGui::IsItemActive()) {
                _rightWidth = std::clamp(_rightWidth - ImGui::GetIO().MouseDelta.x, 200.0f, availW * 0.5f);
            }
        }
        ImGui::End();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void IDEDockManager::renderBottomBar(float leftX, float bottomY, float barW, float barH) {
    std::vector<DockableWindow*> dockedWins;
    for (auto* w : getWindowsInSlot(DockSlot::Bottom)) {
        if (w->open && *w->open) {
            dockedWins.push_back(w);
        }
    }

    if (dockedWins.empty()) return;

    if (_bottomCollapsed) {
        constexpr float slimH = 26.0f;
        const float posY = bottomY + barH - slimH;
        ImGui::SetNextWindowPos(ImVec2(leftX, posY));
        ImGui::SetNextWindowSize(ImVec2(barW, slimH));
        ImGuiWindowFlags slimFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar;
        if (ImGui::Begin("##BottomBarCollapsed", nullptr, slimFlags)) {
            if (ImGui::Button("▲ Bottom Drawer")) {
                toggleBottomCollapsed();
            }
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("Expand Bottom Bar");
        }
        ImGui::End();
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(leftX, bottomY));
    ImGui::SetNextWindowSize(ImVec2(barW, barH));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar |
                             ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoMove |
                             ImGuiWindowFlags_NoBringToFrontOnFocus;

    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.11f, 0.12f, 0.14f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.26f, 0.30f, 0.90f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));

    if (ImGui::Begin("##IDEDockBottom", nullptr, flags)) {
        DockableWindow* activeWin = nullptr;
        for (auto* w : dockedWins) {
            if (w->id == _activeBottomTab) {
                activeWin = w;
                break;
            }
        }
        if (!activeWin && !dockedWins.empty()) {
            activeWin = dockedWins.front();
            _activeBottomTab = activeWin->id;
        }

        renderPanelHeader(DockSlot::Bottom, activeWin, dockedWins);

        if (activeWin && activeWin->renderContent) {
            ImGui::BeginChild("##BottomContentRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);
            activeWin->renderContent();
            ImGui::EndChild();
        }
    }
    ImGui::End();

    // Draggable splitter on top border of Bottom Bar
    {
        ImGui::SetNextWindowPos(ImVec2(leftX, bottomY - 4.0f));
        ImGui::SetNextWindowSize(ImVec2(barW, 8.0f));
        ImGuiWindowFlags splitFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
                                      ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
                                      ImGuiWindowFlags_NoBackground;
        if (ImGui::Begin("##BottomSplitterWindow", nullptr, splitFlags)) {
            ImGui::InvisibleButton("##BottomSplitterBtn", ImVec2(barW, 8.0f));
            if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNS);
            }
            if (ImGui::IsItemActive()) {
                _bottomHeight = std::clamp(_bottomHeight - ImGui::GetIO().MouseDelta.y, 100.0f, ImGui::GetIO().DisplaySize.y * 0.6f);
            }
        }
        ImGui::End();
    }

    ImGui::PopStyleVar();
    ImGui::PopStyleColor(2);
}

void IDEDockManager::renderFloatingWindows() {
    for (auto& win : _windows) {
        if (!win.open || !*win.open) continue;

        // In IDE mode, only render windows explicitly assigned to DockSlot::Floating
        // In Floating mode, render all open windows
        if (_ideMode && win.currentSlot != DockSlot::Floating) {
            continue;
        }

        ImGui::SetNextWindowSize(ImVec2(420, 480), ImGuiCond_FirstUseEver);

        std::string winTitle = win.title;
        if (ImGui::Begin(winTitle.c_str(), win.open)) {
            // If in IDE mode, provide dock buttons at the top of the floating window
            if (_ideMode) {
                ImGui::TextDisabled("Dock into:");
                ImGui::SameLine();
                if (ImGui::SmallButton("◧ Left")) {
                    win.currentSlot = DockSlot::Left;
                    _activeLeftTab = win.id;
                    _leftCollapsed = false;
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("⬓ Bottom")) {
                    win.currentSlot = DockSlot::Bottom;
                    _activeBottomTab = win.id;
                    _bottomCollapsed = false;
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("◨ Right")) {
                    win.currentSlot = DockSlot::Right;
                    _activeRightTab = win.id;
                    _rightCollapsed = false;
                }
                ImGui::Separator();
            }

            if (win.renderContent) {
                win.renderContent();
            }
        }
        ImGui::End();
    }
}

void IDEDockManager::render(Core::Engine* engine, ZoneManager& /*zoneMgr*/, GLFWwindow* /*window*/) {
    if (_showTopBar) {
        renderTopWorkspaceBar(engine);
    }

    if (!_ideMode) {
        // Traditional floating windows
        renderFloatingWindows();
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    const float screenW = io.DisplaySize.x;
    const float screenH = io.DisplaySize.y;
    const float topY = _showTopBar ? 28.0f : 0.0f;

    // Check if bottom bar has open windows
    bool hasBottomWins = false;
    for (auto* w : getWindowsInSlot(DockSlot::Bottom)) {
        if (w->open && *w->open) { hasBottomWins = true; break; }
    }

    float effectiveBottomH = 0.0f;
    if (hasBottomWins) {
        effectiveBottomH = _bottomCollapsed ? 26.0f : std::clamp(_bottomHeight, 100.0f, screenH * 0.6f);
    }

    float sidebarH = screenH - topY - effectiveBottomH;

    // Render Left Sidebar
    renderLeftSidebar(topY, sidebarH, screenW);

    // Render Right Sidebar
    renderRightSidebar(topY, sidebarH, screenW);

    // Calculate bottom bar horizontal bounds
    bool leftHasOpen = false;
    for (auto* w : getWindowsInSlot(DockSlot::Left)) {
        if (w->open && *w->open) { leftHasOpen = true; break; }
    }
    bool rightHasOpen = false;
    for (auto* w : getWindowsInSlot(DockSlot::Right)) {
        if (w->open && *w->open) { rightHasOpen = true; break; }
    }

    float leftOffset = (leftHasOpen && !_leftCollapsed) ? std::clamp(_leftWidth, 200.0f, screenW * 0.5f) : (leftHasOpen ? 28.0f : 0.0f);
    float rightOffset = (rightHasOpen && !_rightCollapsed) ? std::clamp(_rightWidth, 200.0f, screenW * 0.5f) : (rightHasOpen ? 28.0f : 0.0f);
    float bottomW = std::max(100.0f, screenW - leftOffset - rightOffset);
    float bottomY = screenH - effectiveBottomH;

    // Render Bottom Bar
    if (hasBottomWins) {
        renderBottomBar(leftOffset, bottomY, bottomW, effectiveBottomH);
    }

    // Render any windows popped out to Float while in IDE mode
    renderFloatingWindows();
}

} // namespace Rendering
