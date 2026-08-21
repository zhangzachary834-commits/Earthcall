#include "Menu.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

#define STB_EASY_FONT_IMPLEMENTATION   // only in this translation unit
#include "stb_easy_font.h"             // header‑only bitmap font

Menu::Menu() {}

void Menu::addOption(const std::string& label, int key, std::function<void()> action) {
    options.push_back({label, key, action});
    keyToIndex[key] = options.size() - 1;
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
    // Render each option as yellow text (using stb_easy_font)
    const float startX = 24.0f;   // left margin
    const float startY = 40.0f;   // first line
    const float lineH  = 22.0f;   // line spacing

    char buf[6000];               // big enough for one line of verts

    for (size_t i = 0; i < options.size(); ++i) {
        // Compose label: “[key] label”
        std::string line = "[" + std::string(1, static_cast<char>(options[i].key)) + "]  " + options[i].label;

        // Convert to ASCII quads
        int quads = stb_easy_font_print(
            startX,
            startY + i * lineH,
            const_cast<char*>(line.c_str()),
            nullptr,
            buf,
            sizeof(buf)
        );

        glColor3f(1.0f, 1.0f, 0.0f);          // bright yellow
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

    glPopAttrib();                // depth / lighting / color
}

void Menu::processInput(GLFWwindow* win) {
    if (!openState) return;

    for (const auto& opt : options) {
        if (glfwGetKey(win, opt.key) == GLFW_PRESS) {
            opt.onSelect();
            // Optional: close after selection
            // close();
            break;
        }
    }
}  