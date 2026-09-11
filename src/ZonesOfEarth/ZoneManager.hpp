#pragma once
#include <vector>
#include <string>
#include <optional>
#include <unordered_set>
#include "json.hpp"
#include "Zone/Zone.hpp"
#include "SaveContext.hpp"

class LawManager;

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
    LawManager* _lawManager = nullptr;
    // Derived activation cache beneath the persistence boundary: the ids in
    // the currently active Zone's lawRefs. The authored references themselves
    // remain visible in zone.json; this set only tells switchTo which runtime
    // registrations it must release on departure.
    std::unordered_set<std::string> _activeZoneLawIds;

public:
    std::vector<std::shared_ptr<Object>>& getGlobalObjects() { return globalObjects; }
    const std::vector<std::shared_ptr<Object>>& getGlobalObjects() const { return globalObjects; }

    void addZone(std::shared_ptr<Zone> zone);
    bool switchTo(size_t index);
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

    // The running Engine binds the global manager so Law text (AuthorZone)
    // can mint into the live working set. Tests that fire that action bind
    // their own. Not a second registry — one live pointer.
    void bindLive();
    static ZoneManager* live();

    // Bind the one running Law register. Zone activation resolves lawRefs
    // through it atomically; ZoneManager does not own or duplicate Laws.
    void bindLawManager(LawManager* manager) { _lawManager = manager; }

    // Primary Home is a kernel fact: find-or-mint the Person's dwelling,
    // not "any Zone they own". Additional Homes go through authorZone.
    void ensureHomeZone(const std::string& personId);
    Zone* findPrimaryHome(const std::string& personId);
    const Zone* findPrimaryHome(const std::string& personId) const;

    // Person/Relationship/Community-authored mint. Refuses gathering (that
    // is Ourverse::ensureGatheringZone), refuses a second primary Home
    // (ensureHomeZone), refuses identifier collision. kind is authored:
    // home / community-home / community-zone / empty ordinary Zone.
    std::shared_ptr<Zone> authorZone(const std::string& identifier,
                                     const std::string& ownerId,
                                     const std::string& kind,
                                     const std::string& ownerKind = "");
    void updateSaveFiles();
    void setSaveDirectory(const std::string& dir);
    std::string getSaveDirectory() const;
    
    // Save/load state carried in from outside the Zone layer (see SaveContext.hpp)
    nlohmann::json buildSaveJson(const SaveContext& ctx) const;
    void saveState(const std::string& filename, SaveContext& ctx);
    void loadState(const std::string& filename, SaveContext& ctx);
    void saveStateWithLog(const std::string& customName, SaveContext& ctx);

    // Zone identity store (saves/zones/<id>/zone.json). A session/"world"
    // file names a working set; the Zone itself is not a copy inside that
    // file. persistZones writes every live identity-stable Zone; hydrate
    // fills empty boot Zones and admits stored Zones the manager does not
    // yet hold. forkZone copies an identity under a new name (branch);
    // diffZones compares object identifiers of two identities.
    void persistZones() const;
    void hydrateFromZoneStore();
    bool forkZone(const std::string& sourceId, const std::string& newId);
    nlohmann::json diffZones(const std::string& aId, const std::string& bId) const;
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
    
    // Split substrate (.ecmatter) FlatBuffer methods
    // scopeZoneIds absent (default) = every live Zone, matching what the
    // .ecform half of an ordinary Save/Quick Save also embeds (buildSaveJson
    // iterates all _zones too, so both artifacts already agree there).
    // Present = only those Zones' objects are serialized, for a caller that
    // knows its semantic root names a narrower set — see the "Legacy JSON
    // splitter" call site in loadState (Sol's Invariant 1, agent intercom
    // "Basic Pixel Changer Zone Identity Bug 9-7-26", 2026-09-08): dumping
    // every hydrated Zone into a matter buffer for a World that itself named
    // only one is how the real basic_pixel_changer.ecmatter reached 1,441
    // entities.
    std::vector<uint8_t> buildMatterFlatBuffer(
        const std::optional<std::unordered_set<std::string>>& scopeZoneIds = std::nullopt) const;
    void applyMatterFlatBuffer(const std::vector<uint8_t>& buffer);

    std::vector<uint8_t> buildSaveChunkFlatBuffer();
    void loadSaveChunkFlatBuffer(const std::vector<uint8_t>& buffer);
};
