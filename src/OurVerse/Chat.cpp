
#include "OurVerse/Chat.hpp"
#include <ctime>
#include <iomanip>
#include <sstream>

void Chat::addMessage(const std::string& sender, const std::string& text) {
    messages.push_back({sender, text, std::time(nullptr)});
}

void Chat::renderUI(bool* p_open) {
    ImGui::SetNextWindowBgAlpha(0.75f); // semi-transparent background
    // Begin window
    if (!ImGui::Begin("\xF0\x9F\x92\xAC Chat", p_open, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::End();
        return;
    }

    // Reserve space for messages plus input
    const float footerHeight = ImGui::GetFrameHeightWithSpacing();
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImGui::BeginChild("ScrollingRegion", ImVec2(avail.x, avail.y - footerHeight), false, ImGuiWindowFlags_HorizontalScrollbar);

    for (const auto& msg : messages) {
        // Format timestamp hh:mm
        std::tm* tm_info = std::localtime(&msg.timestamp);
        char timeStr[16];
        std::strftime(timeStr, sizeof(timeStr), "%H:%M", tm_info);
        ImGui::Text("[%s] %s: %s", timeStr, msg.sender.c_str(), msg.text.c_str());
    }

    // Auto-scroll to the bottom when at or near bottom
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }

    ImGui::EndChild();
    ImGui::Separator();

    // Input field for new message
    if (ImGui::InputText("##ChatInput", inputBuffer, IM_ARRAYSIZE(inputBuffer), ImGuiInputTextFlags_EnterReturnsTrue)) {
        if (inputBuffer[0] != '\0') {
            addMessage("Player", inputBuffer);
            inputBuffer[0] = '\0';
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Send")) {
        if (inputBuffer[0] != '\0') {
            addMessage("Player", inputBuffer);
            inputBuffer[0] = '\0';
        }
    }

    ImGui::End();
} 