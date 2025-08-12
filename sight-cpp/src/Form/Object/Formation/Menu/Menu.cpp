#include "Menu.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

#define STB_EASY_FONT_IMPLEMENTATION   // only in this translation unit
#include "stb_easy_font.h"             // header‑only bitmap font

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

void Menu::open() { openState = true; }
void Menu::close() { openState = false; }
void Menu::toggle() { openState = !openState; }
bool Menu::isOpen() const { return openState; }

void Menu::draw() const {
    if (!openState) return;

    // --- Save current OpenGL state that we will change -------------------
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);     // always draw on top
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);           // enable transparency for overlay/panel accents
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // --- Switch to 2‑D orthographic projection ---------------------------
    int winW, winH;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &winW, &winH);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, winW, winH, 0, -1, 1);          // (0,0) == top‑left

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // ---------------------------------------------------------------------
    // Backdrop: semi-transparent dark overlay to focus attention
    glColor4f(0.0f, 0.0f, 0.0f, 0.45f);
    glBegin(GL_QUADS);
    glVertex2f(0.f, 0.f); glVertex2f((float)winW, 0.f); glVertex2f((float)winW, (float)winH); glVertex2f(0.f, (float)winH);
    glEnd();

    // Panel dimensions
    const float panelW = std::min(520.0f, (float)winW - 40.0f);
    const float panelH = std::min(360.0f, (float)winH - 80.0f);
    const float panelX = ((float)winW - panelW) * 0.5f;
    const float panelY = ((float)winH - panelH) * 0.5f;

    // Panel background with subtle border
    glColor4f(0.08f, 0.08f, 0.10f, 0.92f);
    glBegin(GL_QUADS);
    glVertex2f(panelX, panelY);
    glVertex2f(panelX + panelW, panelY);
    glVertex2f(panelX + panelW, panelY + panelH);
    glVertex2f(panelX, panelY + panelH);
    glEnd();
    glColor4f(1.0f, 1.0f, 1.0f, 0.10f);
    glBegin(GL_LINE_LOOP);
    glVertex2f(panelX, panelY);
    glVertex2f(panelX + panelW, panelY);
    glVertex2f(panelX + panelW, panelY + panelH);
    glVertex2f(panelX, panelY + panelH);
    glEnd();

    // Title
    const float titleX = panelX + 24.0f;
    const float titleY = panelY + 34.0f;
    {
        char buf[8000];
        int quads = stb_easy_font_print(titleX, titleY, const_cast<char*>("EARTHCALL"), nullptr, buf, sizeof(buf));
        glColor3f(1.0f, 0.95f, 0.6f);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 16, buf);
        glDrawArrays(GL_QUADS, 0, quads * 4);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    // Options list
    const float listX = panelX + 24.0f;
    const float listY = panelY + 84.0f;
    const float lineH = 28.0f;

    // Guard current selection against dynamic size
    int clampedSelected = _selectedIndex;
    if (options.empty()) clampedSelected = 0; else if (clampedSelected >= (int)options.size()) clampedSelected = (int)options.size() - 1;

    char buf[6000];
    for (size_t i = 0; i < options.size(); ++i) {
        // Compose label: “label  [Key]” for clarity
        std::string keyStr;
        if (options[i].key >= GLFW_KEY_SPACE && options[i].key <= GLFW_KEY_Z) {
            keyStr = std::string(1, (char)options[i].key);
        } else {
            // Map a few common non-printables
            if (options[i].key == GLFW_KEY_ENTER) keyStr = "Enter"; else
            if (options[i].key == GLFW_KEY_ESCAPE) keyStr = "Esc"; else
            if (options[i].key == GLFW_KEY_M) keyStr = "M"; else
            if (options[i].key == GLFW_KEY_R) keyStr = "R"; else
            if (options[i].key == GLFW_KEY_Q) keyStr = "Q"; else keyStr = "?";
        }
        std::string line = options[i].label + "   [" + keyStr + "]";

        float y = listY + (float)i * lineH;

        // Highlight selected row
        if ((int)i == clampedSelected) {
            glColor4f(0.90f, 0.85f, 0.40f, 0.18f);
            glBegin(GL_QUADS);
            glVertex2f(listX - 8.0f, y - 6.0f);
            glVertex2f(panelX + panelW - 24.0f, y - 6.0f);
            glVertex2f(panelX + panelW - 24.0f, y + 18.0f);
            glVertex2f(listX - 8.0f, y + 18.0f);
            glEnd();
        }

        // Render text
        int quads = stb_easy_font_print(listX, y, const_cast<char*>(line.c_str()), nullptr, buf, sizeof(buf));
        glColor3f(0.98f, 0.98f, 0.90f);
        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 16, buf);
        glDrawArrays(GL_QUADS, 0, quads * 4);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    // ---------------------------------------------------------------------
    // Restore previous matrices and state
    glPopMatrix();                // MODELVIEW
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glPopAttrib();                // depth / lighting / color
}

void Menu::processInput(GLFWwindow* win) {
    if (!openState) return;

    // Hotkeys for direct activation
    for (const auto& opt : options) {
        if (glfwGetKey(win, opt.key) == GLFW_PRESS) {
            opt.onSelect();
            return;
        }
    }

    // Keyboard navigation: Up/Down + Enter; Esc to close
    bool upNow = glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS;
    bool downNow = glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS;
    bool enterNow = glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS || glfwGetKey(win, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
    bool escNow = glfwGetKey(win, GLFW_KEY_ESCAPE) == GLFW_PRESS;

    if (upNow && !_upPressedLast) {
        if (!options.empty()) {
            _selectedIndex = (_selectedIndex - 1 + (int)options.size()) % (int)options.size();
        }
    }
    if (downNow && !_downPressedLast) {
        if (!options.empty()) {
            _selectedIndex = (_selectedIndex + 1) % (int)options.size();
        }
    }
    if (enterNow && !_enterPressedLast) {
        if (!options.empty()) {
            int idx = std::max(0, std::min(_selectedIndex, (int)options.size() - 1));
            options[(size_t)idx].onSelect();
            return;
        }
    }
    if (escNow && !_escapePressedLast) {
        // Close menu on Esc
        close();
        _escapePressedLast = escNow;
        return;
    }
    _upPressedLast = upNow;
    _downPressedLast = downNow;
    _enterPressedLast = enterNow;
    _escapePressedLast = escNow;

    // Mouse hover and click selection inside the panel
    int winW, winH; glfwGetFramebufferSize(win, &winW, &winH);
    const float panelW = std::min(520.0f, (float)winW - 40.0f);
    const float panelH = std::min(360.0f, (float)winH - 80.0f);
    const float panelX = ((float)winW - panelW) * 0.5f;
    const float panelY = ((float)winH - panelH) * 0.5f;

    const float listX = panelX + 24.0f;
    const float listY = panelY + 84.0f;
    const float lineH = 28.0f;

    // We need mouse position; GLFW provides cursor in window coordinates (top-left origin)
    double mx, my; glfwGetCursorPos(win, &mx, &my);

    // Detect hovered index
    int hovered = -1;
    for (size_t i = 0; i < options.size(); ++i) {
        float y = listY + (float)i * lineH;
        float x0 = listX - 8.0f;
        float y0 = y - 6.0f;
        float x1 = panelX + panelW - 24.0f;
        float y1 = y + 18.0f;
        if (mx >= x0 && mx <= x1 && my >= y0 && my <= y1) {
            hovered = (int)i;
            break;
        }
    }
    if (hovered >= 0) _selectedIndex = hovered;

    bool mouseLeftNow = glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (mouseLeftNow && !_mouseLeftPressedLast && hovered >= 0) {
        options[(size_t)hovered].onSelect();
        _mouseLeftPressedLast = mouseLeftNow;
        return;
    }
    _mouseLeftPressedLast = mouseLeftNow;
}