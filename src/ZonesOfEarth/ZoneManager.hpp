#pragma once
#include <vector>
#include <string>
#include <optional>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include "json.hpp"
#include "Zone/Zone.hpp"
#include "SaveContext.hpp"

class LawManager;
class Person;

bool isObservationZone(const Zone& zone);

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
    // Laws retired from a Zone by an authored act, by Zone identifier.
    std::unordered_map<std::string, std::unordered_set<std::string>> _retiredLawIdsByZone;
    // Residence index for locate(): being -> the Zone whose store holds it.
    // Derived state (DERIVED_STATE_LEDGER): rebuilt when the Universe clock
    // has advanced since the last build, and on any miss. Stale for at most
    // the remainder of a tick for a being moved between stores mid-tick.
    mutable std::unordered_map<const Singular*, Zone*> _residenceIndex;
    mutable double _residenceStamp = -1.0;
    mutable bool _residenceBuilt = false;

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

    // A Law born while a Zone is active belongs to that Zone only when the
    // authored act says so. Universal Singular creation uses this after the
    // Law is registered: it enters the SAME closure switchTo loaded from
    // `lawRefs`, so leaving the Zone releases it and Save Zone can persist it.
    // This is authored membership, not inference from the global LawManager.
    bool adoptLawIntoActiveZone(const std::string& lawId);

    // The inverse, and equally an authored act (a confirmed deletion, via a
    // Destroy whose victim is a Law): the Law leaves the active Zone's
    // closure. Save Zone otherwise only APPENDS to `lawRefs`, so the
    // retirement is remembered per Zone and dropped from `lawRefs` on save —
    // and honoured if the Zone is re-entered before a save. The Law's own
    // file under saves/laws/ stays on disk as history.
    bool retireLawFromActiveZone(const std::string& lawId);
    bool isLawRetiredFrom(const std::string& zoneId, const std::string& lawId) const;

    // ------------------------------------------------------------------
    // Zones as mathematical bounds — the kernel locator
    // (docs/plans/ZONES_AS_MATHEMATICAL_BOUNDS_PLAN_2026-09-23.md, Rung 1).
    // A SENSE, not a decision: it answers where a being is from its authored
    // coordinates and the Zones' authored extents. It never moves, owns, or
    // governs anything. Implemented in ZoneBounds.cpp.
    // ------------------------------------------------------------------
    Zone* findZone(const std::string& identifier) const;
    // The Zone this one is `within`, or null (a cycle or dangling id is null).
    Zone* withinOf(const Zone& zone) const;
    // Follow `within` to the outermost Zone: the continuum this Zone lives in.
    Zone* dimensionalRootOf(const Zone& zone) const;
    // Which Zone's store holds the being — the frame its coordinates are
    // written in. A Person resides in the Zone they are present in (active,
    // until Rung 2). A Zone's residence is the Zone it is within.
    Zone* residenceOf(const Singular& being) const;
    // Every Zone that contains the being, outermost first. Without axes on
    // the continuum, location falls back to the residence chain (today's
    // meaning), so an unauthored world answers exactly as before.
    std::vector<Zone*> locate(const Singular& being) const;
    // The being's coordinates in `frame`'s own frame (axis -> number).
    // False when the continuum has no axes or an axis does not read.
    bool coordinatesIn(const Singular& being, const Zone& frame,
                       std::map<std::string, PropertyValue>& out) const;
    // @world.zoneId / @world.zonePath / @world.dimensionalZoneId, answered
    // through ZoneManager::live(). Idempotent.
    static void installZoneReadings();

    // Primary Home is a kernel fact: find-or-mint the Person's dwelling,
    // not "any Zone they own". The Person-aware overload is the ordinary live
    // path: ownership is witnessed by an `owned-by` Relation whose endpoint is
    // the Person being, so a later change from legacy display spelling to a
    // cryptographic SingularId does not create a new house. The string overload
    // remains for legacy/tests and refuses ambiguous duplicate primaries.
    bool ensureHomeZone(Person& person);
    // Kernel admission invariant: every live Person must have >= 1 primary
    // Home. This is existential, not uniqueness: two primaries are an
    // unresolved policy conflict, but still satisfy "at least one".
    std::size_t primaryHomeCount(const Person& person) const;
    bool enforcePrimaryHomeInvariant(Person& person);
    Zone* findPrimaryHome(Person& person);
    const Zone* findPrimaryHome(const Person& person) const;
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

    // Zone identity store (saves/zones/<id>/zone.json). The ordinary authoring
    // path is Zone-native: persistZone/persistActiveZone write exactly one
    // Zone/Home identity plus the shared Law roots it names. They MUST NOT
    // create or rewrite a conglomerate saves/worlds session file, nor touch
    // unrelated Zone identities. persistZones remains the compatibility/bulk
    // writer used by legacy session migration and cross-root operations.
    bool persistZone(size_t index) const;
    bool persistActiveZone() const;
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