
#pragma once
#include <string>
#include <vector>
#include <ctime>
#include "imgui.h"

struct ChatMessage {
    std::string sender;
    std::string text;
    std::time_t timestamp;
};

class Chat {
public:
    // Add a new chat message from a sender
    void addMessage(const std::string& sender, const std::string& text);

    // Render the chat UI. Optionally pass a pointer to an open flag for closing the window.
    void renderUI(bool* p_open = nullptr);

private:
    std::vector<ChatMessage> messages;
    char inputBuffer[256] = "";
}; 