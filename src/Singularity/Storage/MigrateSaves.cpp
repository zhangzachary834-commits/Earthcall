#include "Singularity/Storage/SaveSystem.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " <old_save_with_3d_laws> <new_save_to_update>" << std::endl;
        return 1;
    }

    std::string oldPath = argv[1];
    std::string newPath = argv[2];

    std::cout << "Loading OLD save: " << oldPath << "\n";
    nlohmann::json oldJson = SaveSystem::readSaveData(oldPath);
    if (oldJson.is_null()) {
        std::cerr << "Failed to load OLD save.\n";
        return 1;
    }

    std::cout << "Loading NEW save: " << newPath << "\n";
    nlohmann::json newJson = SaveSystem::readSaveData(newPath);
    if (newJson.is_null()) {
        std::cerr << "Failed to load NEW save.\n";
        return 1;
    }

    // We want to port specific laws from old to new (e.g., law-shape-3d and law-placement-preview)
    std::vector<std::string> lawsToPort = {"law-shape-3d", "law-placement-preview"};
    
    if (oldJson.contains("authoredLaws") && oldJson["authoredLaws"].contains("laws")) {
        auto& oldLaws = oldJson["authoredLaws"]["laws"];
        
        if (!newJson.contains("authoredLaws")) {
            newJson["authoredLaws"] = nlohmann::json::object();
        }
        if (!newJson["authoredLaws"].contains("laws")) {
            newJson["authoredLaws"]["laws"] = nlohmann::json::array();
        }
        auto& newLaws = newJson["authoredLaws"]["laws"];

        for (const auto& oldLaw : oldLaws) {
            std::string id = oldLaw.value("id", "");
            bool shouldPort = false;
            for (const auto& portId : lawsToPort) {
                if (id == portId) shouldPort = true;
            }

            if (shouldPort) {
                bool found = false;
                for (auto& newLaw : newLaws) {
                    if (newLaw.value("id", "") == id) {
                        newLaw = oldLaw; // Overwrite
                        found = true;
                        std::cout << "Updated existing law in NEW save: " << id << "\n";
                        break;
                    }
                }
                if (!found) {
                    newLaws.push_back(oldLaw);
                    std::cout << "Appended law to NEW save: " << id << "\n";
                }
            }
        }
    } else {
        std::cerr << "No authored laws found in OLD save.\n";
    }

    std::string outPath = SaveSystem::writeSaveData(newJson, "Migrated_Save", SaveSystem::SaveType::GAME);
    std::cout << "Successfully wrote migrated save to: " << outPath << "\n";

    return 0;
}
