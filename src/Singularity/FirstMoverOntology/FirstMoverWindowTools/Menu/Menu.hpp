#pragma once
#include <vector>
#include <string>
#include <functional>
#include <map>
#include <GLFW/glfw3.h>

class Menu {
public:
    struct Option {
        std::string label;
        int key; // GLFW_KEY_*; GLFW_KEY_UNKNOWN means no direct shortcut
        std::function<void()> onSelect;
    };

    Menu();

    void addOption(const std::string& label, int key, std::function<void()> action);

    void open();
    void close();
    void toggle();
    bool isOpen() const;

    // First-mover shell chrome. In-world controls are authored beings + Laws;
    // this menu only exposes developer/runtime entry points around that world.
    void draw(int winW, int winH) const;
    void processInput(GLFWwindow* win);

private:
    bool openState = false;
    std::vector<Option> options;
    std::map<int, size_t> keyToIndex; // one visible meaning per direct shortcut

    // Navigation state. _firstVisibleIndex makes the list a real viewport:
    // keyboard selection can never walk into rows the Person cannot see.
    int _selectedIndex = 0;
    size_t _firstVisibleIndex = 0;
    bool _upPressedLast = false;
    bool _downPressedLast = false;
    bool _homePressedLast = false;
    bool _endPressedLast = false;
    bool _pageUpPressedLast = false;
    bool _pageDownPressedLast = false;
    bool _enterPressedLast = false;
    bool _mouseLeftPressedLast = false;
    std::map<int, bool> _keyPressedLast; // edge detection for option hotkeys
};