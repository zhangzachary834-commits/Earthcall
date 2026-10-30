#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <iostream>
#include <filesystem>

int main() {
    SaveSystem::setSaveRoot("saves");
    ZoneManager::instance().init();
    
    int ambientIndex = -1;
    const auto& zones = ZoneManager::instance().getZones();
    for (size_t i = 0; i < zones.size(); ++i) {
        if (zones[i] && zones[i]->getIdentifier() == "Ambient Zone") {
            ambientIndex = i;
            break;
        }
    }
    
    if (ambientIndex == -1) {
        std::cerr << "Ambient Zone not found!\n";
        return 1;
    }
    
    std::cout << "Found Ambient Zone at index " << ambientIndex << "\n";
    
    // Simulate switchTo closure validation
    std::vector<PreparedZoneLaw> prepared;
    std::unordered_set<std::string> requestedLawIds;
    nlohmann::json identity;
    bool success = ZoneManager::instance().prepareZoneLawClosure(ambientIndex, prepared, requestedLawIds, &identity);
    
    if (success) {
        std::cout << "SUCCESS: prepareZoneLawClosure passed!\n";
    } else {
        std::cout << "FAILURE: prepareZoneLawClosure failed!\n";
        // Let's run the logic to see why
        const nlohmann::json lawRefs = identity.is_object() ? identity.value("lawRefs", nlohmann::json::array()) : nlohmann::json::array();
        for (const auto& r : lawRefs) {
            std::string ref = r.is_string() ? r.get<std::string>() : std::string{};
            try {
                const nlohmann::json root = SaveSystem::readLawIdentity(ref);
                if (!root.is_object()) throw std::runtime_error("missing Law root");
                if (root.value("identifier", std::string{}) != ref) throw std::runtime_error("Law root identity mismatch");
                if (!root.contains("law") || !root["law"].is_object()) throw std::runtime_error("no law object");
                auto law = Law::fromJson(root["law"]);
                if (!law || law->getIdentifier() != ref) throw std::runtime_error("serialized Law id mismatch");
                
                const auto& lawJson = root["law"];
                if (!lawJson.contains("authors") || lawJson["authors"].empty()) throw std::runtime_error("no recorded author");
                for (const auto& authorJson : lawJson["authors"]) {
                    std::string id = authorJson.get<std::string>();
                    
                    // Simple resolution mock
                    bool found = false;
                    for (const auto& object : zones[ambientIndex]->getOwnedObjects()) {
                        if (object->getIdentifier() == id) { found = true; break; }
                    }
                    if (!found) throw std::runtime_error("cannot resolve author " + id);
                }
            } catch (const std::exception& e) {
                std::cout << "Law " << ref << " failed: " << e.what() << "\n";
            }
        }
    }
    return 0;
}
