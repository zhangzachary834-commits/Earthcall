#pragma once
#include <vector>
#include <string>
#include "json.hpp"
#include "Zone/Zone.hpp"
#include "SaveContext.hpp"

// Persistence and UI state for save/load operations
struct SaveLoadState {
    std::vector<std::string> files;
    bool showLoadWindow = false;
    bool showSaveWindow = false;
    bool showManager = false;
    char customName[256] = "";
    std::string lastLoadReport;
    std::string lastSaveReport;
    std::string loadedSaveName;
    std::string saveDirectory = "saves/worlds/";
    bool unpackForAuthoring = false;
};

class ZoneManager {
    std::vector<std::shared_ptr<Zone>> _zones;
    size_t _currentIndex = 0;
    std::vector<std::shared_ptr<Object>> globalObjects; // Repository of all objects
    SaveLoadState _saveLoad;

public:
    void addZone(std::shared_ptr<Zone> zone);
    void switchTo(size_t index);
    void describeCurrent() const;

    void loadZone();
    void organizeLoad();

    Zone& active();

    // Create a Zone "cross-interaction" system later, so Zones can interact with each other. 
    // Zones can integrate and unite, rather than being siloed.
    // ZoneManager needs to be able to load individual Singulars from their zones at the same time
    // New zones can be birthed from the synthesis of existing ones, use zone creation methods.

    // Accessors to iterate over all zones (needed for serialization)
    std::vector<std::shared_ptr<Zone>>& zones();
    const std::vector<std::shared_ptr<Zone>>& zones() const;

    // Current active zone index
    size_t currentIndex() const { return _currentIndex; }

    // Save/Load state access
    SaveLoadState& getSaveLoadState() { return _saveLoad; }
    const SaveLoadState& getSaveLoadState() const { return _saveLoad; }

    // Save/Load methods (moved from Game)
    void ensureHomeZone(const std::string& playerId);
    void updateSaveFiles();
    void setSaveDirectory(const std::string& dir);
    std::string getSaveDirectory() const;
    
    // Save/load state carried in from outside the Zone layer (see SaveContext.hpp)
    nlohmann::json buildSaveJson(const SaveContext& ctx) const;
    void saveState(const std::string& filename, SaveContext& ctx);
    void loadState(const std::string& filename, SaveContext& ctx);
    void saveStateWithLog(const std::string& customName, SaveContext& ctx);
    // Dedicated slot loadState writes the live world into before replacing
    // it. Stem is "before-load"; folder is SaveType::BACKUP. Loading that
    // path itself does not re-stash (or recovery would overwrite the stash
    // with the world being left). Callers of the path: loadState,
    // AssetsConsole Restore unsaved, unsaved_preserve_test.
    static std::string beforeLoadSnapshotPath();
    // Load a test dump into an isolated observation Zone without replacing
    // Home or the Person's other zones. DeveloperToolsWindow is the caller;
    // loadState is the replace-all office and must not be used for this.
    void loadTestObservation(const std::string& filename, SaveContext& ctx);
    std::vector<uint8_t> buildSaveChunkFlatBuffer();
    void loadSaveChunkFlatBuffer(const std::vector<uint8_t>& buffer);
};