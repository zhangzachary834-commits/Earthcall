#include "Menu.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <string>

#define STB_EASY_FONT_IMPLEMENTATION   // only in this translation unit
#include "stb_easy_font.h"             // header‑only bitmap font
#include "Singularity/Screen/Renderer.hpp"

Menu::Menu() {
    // Pre-reserve a small number of options to avoid early reallocations
    try { options.reserve(8); } catch (...) {}
}

void Menu::addOption(const std::string& label, int key, std::function<void()> action) {
    // Diagnostics to pinpoint crash source quickly without heavy tooling
    fprintf(stdout, "[Menu] addOption begin: key=%d, current options=%zu\n", key, options.size());
    fflush(stdout);

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

    try {
        keyToIndex[key] = options.size() ? options.size() - 1 : 0;
    } catch (const std::exception& e) {
        fprintf(stderr, "[Menu] exception updating keyToIndex: %s\n", e.what());
        fflush(stderr);
    }

    fprintf(stdout, "[Menu] addOption end: new options=%zu\n", options.size());
    fflush(stdout);
}

void Menu::open() {
    openState = true;
    Core::EventBus::instance().publish(ECA::Event{"menu-opened", nullptr, nullptr, std::time(nullptr)});
}
void Menu::close() {
    openState = false;
    Core::EventBus::instance().publish(ECA::Event{"menu-closed", nullptr, nullptr, std::time(nullptr)});
}
void Menu::toggle() {
    openState = !openState;
    Core::EventBus::instance().publish(
        ECA::Event{openState ? "menu-opened" : "menu-closed", nullptr, nullptr, std::time(nullptr)});
}
bool Menu::isOpen() const { return openState; }

namespace {
    const char* menuKeyLabel(int key) {
        if (key >= GLFW_KEY_SPACE && key <= GLFW_KEY_Z) {
            static thread_local char buf[2] = {0, 0};
            buf[0] = static_cast<char>(key);
            return buf;
        }
        switch (key) {
            case GLFW_KEY_ENTER: return "Enter";
            case GLFW_KEY_ESCAPE: return "Esc";
            case GLFW_KEY_F2: return "F2";
            case GLFW_KEY_F3: return "F3";
            case GLFW_KEY_F4: return "F4";
            case GLFW_KEY_F5: return "F5";
            case GLFW_KEY_F8: return "F8";
            case GLFW_KEY_F9: return "F9";
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
        p.panelW = std::min(520.0f, (float)w - 40.0f);
        const float listTop = 84.0f;
        const float listBottomPad = 20.0f;
        const float contentH = listTop + (float)optionCount * p.lineH + listBottomPad;
        p.panelH = std::min(contentH, std::max(120.0f, (float)h - 80.0f));
        p.panelX = ((float)w - p.panelW) * 0.5f;
        p.panelY = ((float)h - p.panelH) * 0.5f;
        p.listX = p.panelX + 24.0f;
        p.listY = p.panelY + listTop;
        return p;
    }
}

void Menu::draw(int winW, int winH) const {
    if (!openState) return;

    // --- Switch to 2-D screen space (depth off, alpha blending on) --------

    Renderer& r = currentRenderer();
    r.begin2D(static_cast<uint32_t>(winW), static_cast<uint32_t>(winH));

    // ---------------------------------------------------------------------
    // Backdrop: semi-transparent dark overlay to focus attention
    r.drawTris2D(draw::rectTris({0.f, 0.f, (float)winW, (float)winH}),
                 glm::vec4(0.0f, 0.0f, 0.0f, 0.45f));

    const MenuPanel p = computeMenuPanel(winW, winH, options.size());
    const float panelW = p.panelW;
    const float panelH = p.panelH;
    const float panelX = p.panelX;
    const float panelY = p.panelY;

    // Panel background with subtle border
    const glm::vec4 panel(panelX, panelY, panelX + panelW, panelY + panelH);
    r.drawTris2D(draw::rectTris(panel), glm::vec4(0.08f, 0.08f, 0.10f, 0.92f));
    r.drawLines2D(draw::rectOutline(panel), glm::vec4(1.0f, 1.0f, 1.0f, 0.10f), 1.0f);

    // Title
    const float titleX = panelX + 24.0f;
    const float titleY = panelY + 34.0f;
    {
        char buf[8000];
        int quads = stb_easy_font_print(titleX, titleY, const_cast<char*>("EARTHCALL"), nullptr, buf, sizeof(buf));
        r.drawTris2D(draw::easyFontToTris(buf, quads), glm::vec4(1.0f, 0.95f, 0.6f, 1.0f));
    }

    // Options list
    const float listX = p.listX;
    const float listY = p.listY;
    const float lineH = p.lineH;
    const float clipBottom = panelY + panelH - 8.0f;

    // Guard current selection against dynamic size
    int clampedSelected = _selectedIndex;
    if (options.empty()) clampedSelected = 0; else if (clampedSelected >= (int)options.size()) clampedSelected = (int)options.size() - 1;

    char buf[6000];
    for (size_t i = 0; i < options.size(); ++i) {
        std::string line = options[i].label + "   [" + menuKeyLabel(options[i].key) + "]";

        float y = listY + (float)i * lineH;
        if (y + 18.0f > clipBottom) break;

        // Highlight selected row
        if ((int)i == clampedSelected) {
            r.drawTris2D(draw::rectTris({listX - 8.0f, y - 6.0f,
                                         panelX + panelW - 24.0f, y + 18.0f}),
                         glm::vec4(0.90f, 0.85f, 0.40f, 0.18f));
        }

        // Render text
        int quads = stb_easy_font_print(listX, y, const_cast<char*>(line.c_str()), nullptr, buf, sizeof(buf));
        r.drawTris2D(draw::easyFontToTris(buf, quads), glm::vec4(0.98f, 0.98f, 0.90f, 1.0f));
    }

    r.end2D();
}

void Menu::processInput(GLFWwindow* win) {
    if (!openState) return;

    bool upNow = glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS;
    bool downNow = glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS;
    bool enterNow = glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
    bool mouseLeftNow = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    int fired = -1;
    for (size_t i = 0; i < options.size(); ++i) {
        const int key = options[i].key;
        const bool now = glfwGetKey(win, key) == GLFW_PRESS;
        const bool last = _keyPressedLast[key];
        if (now && !last) fired = static_cast<int>(i);
        _keyPressedLast[key] = now;
    }

    if (upNow && !_upPressedLast && !options.empty()) {
        _selectedIndex = (_selectedIndex - 1 + (int)options.size()) % (int)options.size();
    }
    if (downNow && !_downPressedLast && !options.empty()) {
        _selectedIndex = (_selectedIndex + 1) % (int)options.size();
    }

    int fbW = 0, fbH = 0, winW = 0, winH = 0;
    glfwGetFramebufferSize(win, &fbW, &fbH);
    glfwGetWindowSize(win, &winW, &winH);
    const MenuPanel p = computeMenuPanel(fbW, fbH, options.size());
    const float clipBottom = p.panelY + p.panelH - 8.0f;

    double mx = 0.0, my = 0.0;
    glfwGetCursorPos(win, &mx, &my);
    const float scaleX = (winW > 0) ? static_cast<float>(fbW) / static_cast<float>(winW) : 1.0f;
    const float scaleY = (winH > 0) ? static_cast<float>(fbH) / static_cast<float>(winH) : 1.0f;
    mx *= scaleX;
    my *= scaleY;

    int hovered = -1;
    for (size_t i = 0; i < options.size(); ++i) {
        float y = p.listY + (float)i * p.lineH;
        if (y + 18.0f > clipBottom) break;
        float x0 = p.listX - 8.0f;
        float y0 = y - 6.0f;
        float x1 = p.panelX + p.panelW - 24.0f;
        float y1 = y + 18.0f;
        if (mx >= x0 && mx <= x1 && my >= y0 && my <= y1) {
            hovered = (int)i;
            break;
        }
    }
    if (hovered >= 0) _selectedIndex = hovered;

    const int enterIdx = (enterNow && !_enterPressedLast && !options.empty())
        ? std::max(0, std::min(_selectedIndex, (int)options.size() - 1))
        : -1;
    const int clickIdx = (mouseLeftNow && !_mouseLeftPressedLast && hovered >= 0) ? hovered : -1;

    _upPressedLast = upNow;
    _downPressedLast = downNow;
    _enterPressedLast = enterNow;
    _mouseLeftPressedLast = mouseLeftNow;

    if (fired >= 0) {
        options[(size_t)fired].onSelect();
        return;
    }
    if (enterIdx >= 0) {
        options[(size_t)enterIdx].onSelect();
        return;
    }
    if (clickIdx >= 0) {
        options[(size_t)clickIdx].onSelect();
    }
}