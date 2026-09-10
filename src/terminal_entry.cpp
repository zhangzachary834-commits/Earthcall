#include "Singularity/Language/LanguageSystem.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include <iostream>
#include <string>

using namespace Core;

int main() {
    std::cout << "Earthcall Terminal Started." << std::endl;

    // Instead of instantiating the full graphical Engine which asserts in ImGui on headless shutdown,
    // we instantiate only the headless logical subsystems needed to satisfy the prompt's requirements.
    auto categoryManager = std::make_shared<CategoryManager>();
    auto materialManager = std::make_shared<MaterialManager>();
    auto zoneManager = std::make_shared<ZoneManager>();
    auto& languageSystem = Singularity::Language::LanguageSystem::instance();

    std::string input;
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, input)) break;
        if (input == "exit" || input == "quit") break;

        if (input == "status") {
            std::cout << "Earthcall Core Systems Online." << std::endl;
        } else if (input == "tick") {
            std::cout << "Ticked engine." << std::endl;
        } else if (input.rfind("utter ", 0) == 0) {
            std::string msg = input.substr(6);
            Event::Utterance u;
            u.payload = msg;
            u.sourceClient = "Terminal";
            Core::EventBus::instance().publish(u);
            std::cout << "You utter: " << msg << std::endl;
        } else {
            std::cout << "Unknown command. Try: status, tick, utter <msg>, exit" << std::endl;
        }
    }

    return 0;
}
