#include <iostream>
#include <filesystem>
#include <string>
#include "Util/SaveSystem.hpp"
#include "json.hpp"

namespace fs = std::filesystem;

void migrateFolder(const std::string& folderPath, SaveSystem::SaveType type) {
    if (!fs::exists(folderPath)) {
        return;
    }
    
    std::cout << "Scanning " << folderPath << " for legacy JSON saves..." << std::endl;
    for (const auto& entry : fs::directory_iterator(folderPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".json") {
            std::string filepath = entry.path().string();
            std::string filename = entry.path().filename().string();
            // Exclude logs
            if (filename.find("_saves.json") != std::string::npos) continue;
            
            std::cout << "Migrating: " << filename << std::endl;
            try {
                // Read legacy JSON
                nlohmann::json j = SaveSystem::readSaveData(filepath);
                
                // Get custom label (filename without extension and without timestamp)
                // We'll just pass the full filename minus extension for simplicity
                std::string label = entry.path().stem().string();
                // But the filename already has a timestamp. SaveSystem adds another timestamp.
                // We don't want double timestamps, but we can't easily parse out the timestamp since it's formatting is arbitrary.
                // It's okay, Migrated saves will have a new timestamp.
                
                // Write binary (.ecsave)
                std::string newPath = SaveSystem::writeSaveData(j, label, type);
                std::cout << "  -> Success: " << newPath << std::endl;
            } catch (const std::exception& e) {
                std::cerr << "  -> Error migrating " << filename << ": " << e.what() << std::endl;
            }
        }
    }
}

int main() {
    std::cout << "=== Earthcall Legacy Save Migrator ===" << std::endl;
    
    migrateFolder("saves/games", SaveSystem::SaveType::GAME);
    migrateFolder("saves/avatars", SaveSystem::SaveType::AVATAR);
    migrateFolder("saves/persons", SaveSystem::SaveType::PERSON);
    migrateFolder("saves/designs", SaveSystem::SaveType::DESIGN);
    migrateFolder("saves/custom", SaveSystem::SaveType::CUSTOM);
    migrateFolder("saves/integrations", SaveSystem::SaveType::INTEGRATION);
    
    std::cout << "Migration complete." << std::endl;
    std::cout << "Note: You can manually delete the old .json files if everything works correctly." << std::endl;
    return 0;
}
