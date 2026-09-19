#include "Menu.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <string>

#define STB_EASY_FONT_IMPLEMENTATION   // only in this translation unit
#include "stb_easy_font.h"             // header-only bitmap font
#include "Singularity/Screen/Renderer.hpp"

Menu::Menu() {
    // The first-mover menu is intentionally small, but the living menu already
    // exceeds eight entries. Reserving the expected shell keeps registration
    // from reallocating repeatedly during boot.
    try { options.reserve(24); } catch (...) {}
}

void Menu::addOption(const std::string& label, int key, std::function<void()> action) {
    fprintf(stdout, "[Menu] addOption begin: key=%d, current options=%zu\n", key, options.size());
    fflush(stdout);

    // One visible shortcut must mean one thing. Historically two menu rows both
    // advertised F3; processInput then silently let the later row win. Preserve
    // that deterministic last-registration ownership, but make the displaced row
    // honestly show no direct shortcut and leave a diagnostic trail.
    if (key != GLFW_KEY_UNKNOWN) {
        auto existing = keyToIndex.find(key);
        if (existing != keyToIndex.end() && existing->second < options.size()) {
            const size_t previousIndex = existing->second;
            fprintf(stderr,
                    "[Menu] shortcut collision: key=%d moved from '%s' to '%s'; "
                    "the earlier row remains navigable without a direct shortcut.\n",
                    key, options[previousIndex].label.c_str(), label.c_str());
            fflush(stderr);
            options[previousIndex].key = GLFW_KEY_UNKNOWN;
            _keyPressedLast.erase(key);
        }
    }

    try {
        Option opt{label, key, std::move(action)};
        options.push_back(std::move(opt));
    } catch (const std::length_error& e) {
        fprintf(stderr, "[Menu] length_error on push_back: %s. Using minimal no-op action.\n", e.what());
        fflush(stderr);
        try {
            Option opt{label, key, [](){}};
            options.push_back(std::move(opt));
        } catch (...) {
            fprintf(stderr, "[Menu] push_back failed again; skipping option.\n");
            fflush(stderr);
            return;
        }
    } catch (const std::exception& e) {
        fprintf(stderr, "[Menu] exception on push_back: %s\n", e.what());
        fflush(stderr);
        return;
    } catch (...) {
        fprintf(stderr, "[Menu] unknown exception on push_back.\n");
        fflush(stderr);
        return;
    }

    if (key != GLFW_KEY_UNKNOWN) {
        try {
            keyToIndex[key] = options.size() - 1;
        } catch (const std::exception& e) {
            fprintf(stderr, "[Menu] exception updating keyToIndex: %s\n", e.what());
            fflush(stderr);
        }
    }

    fprintf(stdout, "[Menu] addOption end: new options=%zu\n", options.size());
    fflush(stdout);
}

void Menu::open() {
    openState = true;
    _needsInputPriming = true;
    Core::EventBus::instance().publish(ECA::Event{"menu-opened", nullptr, nullptr, std::time(nullptr)});
}
void Menu::close() {
    openState = false;
    Core::EventBus::instance().publish(ECA::Event{"menu-closed", nullptr, nullptr, std::time(nullptr)});
}
void Menu::toggle() {
    openState = !openState;
    if (openState) _needsInputPriming = true;
    Core::EventBus::instance().publish(
        ECA::Event{openState ? "menu-opened" : "menu-closed", nullptr, nullptr, std::time(nullptr)});
}
bool Menu::isOpen() const { return openState; }

namespace {
    constexpr float kListTop = 84.0f;
    constexpr float kFooterHeight = 34.0f;
    constexpr float kRowTextHeight = 18.0f;

    thread_local char t_menuKeyLabelBuf[16] = {0};

    const char* menuKeyLabel(int key) {
        if (key == GLFW_KEY_UNKNOWN) return "-";
        if (key >= GLFW_KEY_A && key <= GLFW_KEY_Z) {
            t_menuKeyLabelBuf[0] = static_cast<char>('A' + (key - GLFW_KEY_A));
            t_menuKeyLabelBuf[1] = '\0';
            return t_menuKeyLabelBuf;
        }
        if (key >= GLFW_KEY_0 && key <= GLFW_KEY_9) {
            t_menuKeyLabelBuf[0] = static_cast<char>('0' + (key - GLFW_KEY_0));
            t_menuKeyLabelBuf[1] = '\0';
            return t_menuKeyLabelBuf;
        }
        if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F25) {
            std::snprintf(t_menuKeyLabelBuf, sizeof(t_menuKeyLabelBuf), "F%d",
                          key - GLFW_KEY_F1 + 1);
            return t_menuKeyLabelBuf;
        }
        switch (key) {
            case GLFW_KEY_SPACE: return "Space";
            case GLFW_KEY_ENTER: return "Enter";
            case GLFW_KEY_ESCAPE: return "Esc";
            case GLFW_KEY_TAB: return "Tab";
            case GLFW_KEY_GRAVE_ACCENT: return "`";
            default: return "?";
        }
    }

    struct MenuPanel {
        float panelX, panelY, panelW, panelH;
        float listX, listY, lineH;
    };

    MenuPanel computeMenuPanel(int w, int h, size_t optionCount) {
        MenuPanel p;
        p.lineH = 28.0f;
        p.panelW = std::min(520.0f, std::max(1.0f, static_cast<float>(w) - 40.0f));
        const float contentH = kListTop + static_cast<float>(optionCount) * p.lineH + kFooterHeight;
        p.panelH = std::min(contentH, std::max(120.0f, static_cast<float>(h) - 80.0f));
        p.panelX = (static_cast<float>(w) - p.panelW) * 0.5f;
        p.panelY = (static_cast<float>(h) - p.panelH) * 0.5f;
        p.listX = p.panelX + 24.0f;
        p.listY = p.panelY + kListTop;
        return p;
    }

    size_t visibleRowCount(const MenuPanel& p) {
        const float clipBottom = p.panelY + p.panelH - kFooterHeight;
        if (clipBottom < p.listY + kRowTextHeight) return 0;
        const float remaining = clipBottom - (p.listY + kRowTextHeight);
        return 1u + static_cast<size_t>(std::floor(remaining / p.lineH));
    }

    size_t maxFirstVisible(size_t optionCount, size_t visibleRows) {
        if (visibleRows == 0 || optionCount <= visibleRows) return 0;
        return optionCount - visibleRows;
    }

    void keepSelectionVisible(int selectedIndex, size_t optionCount,
                              size_t visibleRows, size_t& firstVisible) {
        if (optionCount == 0 || visibleRows == 0) {
            firstVisible = 0;
            return;
        }

        const size_t selected = static_cast<size_t>(std::max(0, selectedIndex));
        if (selected < firstVisible) {
            firstVisible = selected;
        } else if (selected >= firstVisible + visibleRows) {
            firstVisible = selected - visibleRows + 1;
        }
        firstVisible = std::min(firstVisible, maxFirstVisible(optionCount, visibleRows));
    }
}

void Menu::draw(int winW, int winH) const {
    if (!openState) return;

    Renderer& r = currentRenderer();
    r.begin2D(static_cast<uint32_t>(winW), static_cast<uint32_t>(winH));

    // Backdrop: reduce world competition so the shell reads as a deliberate
    // place the Person entered, not text floating over the world.
    r.drawTris2D(draw::rectTris({0.f, 0.f, static_cast<float>(winW), static_cast<float>(winH)}),
                 glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));

    const MenuPanel p = computeMenuPanel(winW, winH, options.size());
    const glm::vec4 panel(p.panelX, p.panelY, p.panelX + p.panelW, p.panelY + p.panelH);
    r.drawTris2D(draw::rectTris(panel), glm::vec4(0.08f, 0.08f, 0.10f, 0.92f));
    r.drawLines2D(draw::rectOutline(panel), glm::vec4(1.0f, 1.0f, 1.0f, 0.10f), 1.0f);

    const float titleX = p.panelX + 24.0f;
    const float titleY = p.panelY + 34.0f;
    {
        char buf[8000];
        int quads = stb_easy_font_print(titleX, titleY, const_cast<char*>("EARTHCALL"), nullptr, buf, sizeof(buf));
        r.drawTris2D(draw::easyFontToTris(buf, quads), glm::vec4(1.0f, 0.95f, 0.6f, 1.0f));
    }

    int clampedSelected = _selectedIndex;
    if (options.empty()) {
        clampedSelected = 0;
    } else {
        clampedSelected = std::max(0, std::min(clampedSelected, static_cast<int>(options.size()) - 1));
    }

    const size_t visibleRows = visibleRowCount(p);
    const size_t firstVisible = std::min(_firstVisibleIndex,
                                         maxFirstVisible(options.size(), visibleRows));
    const size_t lastVisibleExclusive = std::min(options.size(), firstVisible + visibleRows);

    char buf[6000];
    for (size_t i = firstVisible; i < lastVisibleExclusive; ++i) {
        const size_t row = i - firstVisible;
        const float y = p.listY + static_cast<float>(row) * p.lineH;
        std::string line = options[i].label + "   [" + menuKeyLabel(options[i].key) + "]";

        if (static_cast<int>(i) == clampedSelected) {
            r.drawTris2D(draw::rectTris({p.listX - 8.0f, y - 6.0f,
                                         p.panelX + p.panelW - 24.0f, y + 18.0f}),
                         glm::vec4(0.90f, 0.85f, 0.40f, 0.18f));
        }

        int quads = stb_easy_font_print(p.listX, y, const_cast<char*>(line.c_str()), nullptr, buf, sizeof(buf));
        r.drawTris2D(draw::easyFontToTris(buf, quads), glm::vec4(0.98f, 0.98f, 0.90f, 1.0f));
    }

    // The old menu simply stopped drawing after the clip boundary. Make the
    // viewport legible: a Person can see that more commands exist and how to
    // reach them, instead of keyboard focus disappearing into invisible rows.
    {
        const size_t above = firstVisible;
        const size_t below = options.size() > lastVisibleExclusive
            ? options.size() - lastVisibleExclusive
            : 0;
        char footer[192];
        if (above || below) {
            std::snprintf(footer, sizeof(footer),
                          "%zu up | %zu down   Wheel/Arrows  PgUp/PgDn  Home/End  Enter",
                          above, below);
        } else {
            std::snprintf(footer, sizeof(footer),
                          "Wheel/Arrows  PgUp/PgDn  Home/End  Enter");
        }
        int quads = stb_easy_font_print(p.panelX + 24.0f, p.panelY + p.panelH - 18.0f,
                                        footer, nullptr, buf, sizeof(buf));
        r.drawTris2D(draw::easyFontToTris(buf, quads), glm::vec4(0.68f, 0.70f, 0.74f, 1.0f));
    }

    r.end2D();
}

void Menu::processInput(GLFWwindow* win) {
    if (!openState || !win) return;

    int fbW = 0, fbH = 0, winW = 0, winH = 0;
    glfwGetFramebufferSize(win, &fbW, &fbH);
    glfwGetWindowSize(win, &winW, &winH);
    const MenuPanel p = computeMenuPanel(fbW, fbH, options.size());
    const size_t visibleRows = visibleRowCount(p);

    double mx = 0.0, my = 0.0;
    glfwGetCursorPos(win, &mx, &my);
    const float scaleX = (winW > 0) ? static_cast<float>(fbW) / static_cast<float>(winW) : 1.0f;
    const float scaleY = (winH > 0) ? static_cast<float>(fbH) / static_cast<float>(winH) : 1.0f;
    mx *= scaleX;
    my *= scaleY;
    const bool pointerInsidePanel = mx >= p.panelX && mx <= p.panelX + p.panelW &&
                                    my >= p.panelY && my <= p.panelY + p.panelH;

    const bool upNow = glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS;
    const bool downNow = glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS;
    const bool homeNow = glfwGetKey(win, GLFW_KEY_HOME) == GLFW_PRESS;
    const bool endNow = glfwGetKey(win, GLFW_KEY_END) == GLFW_PRESS;
    const bool pageUpNow = glfwGetKey(win, GLFW_KEY_PAGE_UP) == GLFW_PRESS;
    const bool pageDownNow = glfwGetKey(win, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS;
    const bool enterNow = glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS ||
                          glfwGetKey(win, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
    const bool mouseLeftNow = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    // Opening with M (or from another shell control) can happen while another
    // key/button is already held. Seed edge history once so entering the menu
    // cannot accidentally activate whatever happened to be under that input.
    if (_needsInputPriming) {
        for (const auto& option : options) {
            if (option.key != GLFW_KEY_UNKNOWN) {
                _keyPressedLast[option.key] = glfwGetKey(win, option.key) == GLFW_PRESS;
            }
        }
        _upPressedLast = upNow;
        _downPressedLast = downNow;
        _homePressedLast = homeNow;
        _endPressedLast = endNow;
        _pageUpPressedLast = pageUpNow;
        _pageDownPressedLast = pageDownNow;
        _enterPressedLast = enterNow;
        _mouseLeftPressedLast = mouseLeftNow;
        _needsInputPriming = false;
        keepSelectionVisible(_selectedIndex, options.size(), visibleRows, _firstVisibleIndex);
        return;
    }

    int fired = -1;
    for (size_t i = 0; i < options.size(); ++i) {
        const int key = options[i].key;
        if (key == GLFW_KEY_UNKNOWN) continue;
        const bool now = glfwGetKey(win, key) == GLFW_PRESS;
        const bool last = _keyPressedLast[key];
        if (now && !last) fired = static_cast<int>(i);
        _keyPressedLast[key] = now;
    }

    if (!options.empty()) {
        const int lastIndex = static_cast<int>(options.size()) - 1;
        const int pageStep = std::max(1, static_cast<int>(visibleRows > 1 ? visibleRows - 1 : 1));

        if (homeNow && !_homePressedLast) _selectedIndex = 0;
        if (endNow && !_endPressedLast) _selectedIndex = lastIndex;
        if (upNow && !_upPressedLast) {
            _selectedIndex = (_selectedIndex - 1 + static_cast<int>(options.size())) %
                             static_cast<int>(options.size());
        }
        if (downNow && !_downPressedLast) {
            _selectedIndex = (_selectedIndex + 1) % static_cast<int>(options.size());
        }
        if (pageUpNow && !_pageUpPressedLast) {
            _selectedIndex = std::max(0, _selectedIndex - pageStep);
        }
        if (pageDownNow && !_pageDownPressedLast) {
            _selectedIndex = std::min(lastIndex, _selectedIndex + pageStep);
        }

        if (pointerInsidePanel && ImGui::GetCurrentContext()) {
            const float wheel = ImGui::GetIO().MouseWheel;
            if (wheel > 0.0f) _selectedIndex = std::max(0, _selectedIndex - 3);
            if (wheel < 0.0f) _selectedIndex = std::min(lastIndex, _selectedIndex + 3);
        }
    }

    keepSelectionVisible(_selectedIndex, options.size(), visibleRows, _firstVisibleIndex);

    int hovered = -1;
    const size_t firstVisible = std::min(_firstVisibleIndex,
                                         maxFirstVisible(options.size(), visibleRows));
    const size_t lastVisibleExclusive = std::min(options.size(), firstVisible + visibleRows);
    for (size_t i = firstVisible; i < lastVisibleExclusive; ++i) {
        const size_t row = i - firstVisible;
        const float y = p.listY + static_cast<float>(row) * p.lineH;
        const float x0 = p.listX - 8.0f;
        const float y0 = y - 6.0f;
        const float x1 = p.panelX + p.panelW - 24.0f;
        const float y1 = y + 18.0f;
        if (mx >= x0 && mx <= x1 && my >= y0 && my <= y1) {
            hovered = static_cast<int>(i);
            break;
        }
    }
    if (hovered >= 0) _selectedIndex = hovered;

    const int enterIdx = (enterNow && !_enterPressedLast && !options.empty())
        ? std::max(0, std::min(_selectedIndex, static_cast<int>(options.size()) - 1))
        : -1;
    const int clickIdx = (mouseLeftNow && !_mouseLeftPressedLast && hovered >= 0) ? hovered : -1;

    _upPressedLast = upNow;
    _downPressedLast = downNow;
    _homePressedLast = homeNow;
    _endPressedLast = endNow;
    _pageUpPressedLast = pageUpNow;
    _pageDownPressedLast = pageDownNow;
    _enterPressedLast = enterNow;
    _mouseLeftPressedLast = mouseLeftNow;

    if (fired >= 0) {
        options[static_cast<size_t>(fired)].onSelect();
        return;
    }
    if (enterIdx >= 0) {
        options[static_cast<size_t>(enterIdx)].onSelect();
        return;
    }
    if (clickIdx >= 0) {
        options[static_cast<size_t>(clickIdx)].onSelect();
    }
}
