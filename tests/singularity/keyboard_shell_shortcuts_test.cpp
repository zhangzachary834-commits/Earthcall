// Regression witness for first-mover shell keyboard ergonomics.
//
// The same key that opens a shell surface must be able to close it while that
// surface owns ordinary keyboard navigation. Letter shortcuts must still defer
// while a text widget is actively asking for characters. This distinction is
// what lets H close Chat without making it impossible to type the letter 'h'
// into Chat.

#include "Singularity/Input/Keyboard/KeyboardHandler.hpp"
#include <imgui.h>
#include <cstdio>

namespace {
int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        ++failures;
    }
}
}

int main() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    KeyboardHandler keyboard;
    int menuToggles = 0;
    int chatToggles = 0;
    int keymapToggles = 0;
    int ordinaryActions = 0;

    keyboard.bindKey(GLFW_KEY_M, "toggle_menu", [&]() { ++menuToggles; });
    keyboard.bindKey(GLFW_KEY_H, "toggle_chat", [&]() { ++chatToggles; });
    keyboard.bindKey(GLFW_KEY_K, "toggle_keymap", [&]() { ++keymapToggles; });
    keyboard.bindKey(GLFW_KEY_J, "ordinary_action", [&]() { ++ordinaryActions; });

    // A menu toggle must be reversible from inside the menu. Historically the
    // menu-open guard swallowed this second M before its callback ran.
    keyboard.setMenuOpen(true);
    keyboard.handleKeyPress(GLFW_KEY_M);
    expect(menuToggles == 1, "M must pass through while the main menu is open");
    keyboard.handleKeyRelease(GLFW_KEY_M);
    keyboard.setMenuOpen(false);

    // A focused ImGui shell panel may capture keyboard navigation without
    // actually editing text. Shell toggles should remain reachable there.
    io.WantCaptureKeyboard = true;
    io.WantTextInput = false;

    keyboard.handleKeyPress(GLFW_KEY_H);
    expect(chatToggles == 1, "H must close/toggle Chat while a shell panel merely has keyboard focus");
    keyboard.handleKeyRelease(GLFW_KEY_H);

    keyboard.handleKeyPress(GLFW_KEY_K);
    expect(keymapToggles == 1, "K must close/toggle Keymap while a shell panel merely has keyboard focus");
    keyboard.handleKeyRelease(GLFW_KEY_K);

    keyboard.handleKeyPress(GLFW_KEY_J);
    expect(ordinaryActions == 0, "ordinary world/action keys must stay suppressed while ImGui captures keyboard input");
    keyboard.handleKeyRelease(GLFW_KEY_J);

    // Text entry is a stronger claim than panel focus. Letter shell shortcuts
    // yield so the Person can type normally.
    io.WantTextInput = true;
    keyboard.handleKeyPress(GLFW_KEY_H);
    expect(chatToggles == 1, "H must not toggle Chat while a text widget wants characters");
    keyboard.handleKeyRelease(GLFW_KEY_H);

    keyboard.handleKeyPress(GLFW_KEY_M);
    expect(menuToggles == 1, "M must not open/close the menu while a text widget wants characters");
    keyboard.handleKeyRelease(GLFW_KEY_M);

    ImGui::DestroyContext();

    if (failures) {
        std::fprintf(stderr, "keyboard_shell_shortcuts_test: %d failure(s)\n", failures);
        return 1;
    }

    std::printf("keyboard_shell_shortcuts_test: PASS\n");
    return 0;
}
