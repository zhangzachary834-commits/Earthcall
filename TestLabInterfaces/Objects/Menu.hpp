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
        int key; // GLFW_KEY_*
        std::function<void()> onSelect;
    };

    Menu();

    void addOption(const std::string& label, int key, std::function<void()> action);

    void open();
    void close();
    void toggle();
    bool isOpen() const;

    // Manually called by main loop
    void draw() const;       // <-- You fill this in with your OpenGL code
    void processInput(GLFWwindow* win);     // <-- Handles hotkey logic

private:
    bool openState = false;
    std::vector<Option> options;
    std::map<int, size_t> keyToIndex; // quick lookup
};