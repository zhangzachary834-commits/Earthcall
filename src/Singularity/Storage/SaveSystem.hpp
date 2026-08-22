#pragma once
#include <string>
#include <vector>
#include "json.hpp"

namespace SaveSystem {

// Save types for organization
enum class SaveType {
    WORLD,       // Session snapshot (camera, laws, working-set refs). Not the Zone identity.
    AVATAR,     // Avatar saves
    PERSON,     // Registered Persons / user profiles
    DESIGN,     // Design system saves
    BACKUP,     // Automatic backups
    CUSTOM,     // Custom saves
    INTEGRATION, // Integration system saves (web apps, external windows, etc.)
    ZONE         // Per-Zone identity directory under saves/zones/<id>/
};

// Pin every relative save path to this directory (the repository `saves/`
// folder). Empty means "saves" under the process cwd, which is what tests
// use. The engine sets this at boot so a launch from `build/` or Finder
// does not write a second, invisible tree.
void setSaveRoot(const std::string& absoluteSavesDir);
std::string saveRoot();

// Ensure organized save folder structure exists
std::string ensureSaveFolder();
std::string ensureSaveTypeFolder(SaveType type);

// Build filename with timestamp or custom label stored in organized folders
std::string makeFilename(const std::string& customLabel = "", SaveType type = SaveType::WORLD, const std::string& ext = ".json");

// Get a formatted timestamp string
std::string timestamp();

// Strip path separators and parent-directory hops out of a caller-supplied
// label. Every path built from user- or save-file-derived text must pass
// through here; makeFilename applies it, and direct path builders must too.
std::string sanitizeLabel(const std::string& label);



// Return list of files that still exist for a specific save type; also prunes stale entries from log
std::vector<std::string> listFiles(SaveType type = SaveType::WORLD);

// One row per world. json + ecsave + _delta are the same save written three
// ways; listing them as independent loads is how switching worlds felt like
// a pile of twins. Prefer the readable json; never offer delta or empty files.
struct WorldEntry {
    std::string label;
    std::string path;
};
std::vector<WorldEntry> listWorlds(SaveType type = SaveType::WORLD);
// Remove json, ecsave, and delta of one stem so a delete does not leave a twin.
void removeWorld(const std::string& stem, SaveType type = SaveType::WORLD);

// Write JSON to disk via generated filename and log it; returns full path
// For dry-run/parallel testing, this will also write a binary .ecsave alongside it.
std::string writeSaveData(const nlohmann::json& j, const std::string& customLabel = "", SaveType type = SaveType::WORLD);
void writeSaveDataAsync(const nlohmann::json& j, const std::string& customLabel = "", SaveType type = SaveType::WORLD);

// Overloads for raw binary data (e.g. FlatBuffers)
std::string writeSaveData(const std::vector<uint8_t>& data, const std::string& customLabel, const std::string& ext, SaveType type);
void writeSaveDataAsync(const std::vector<uint8_t>& data, const std::string& customLabel, const std::string& ext, SaveType type);

bool isSaving();

// Unified read function: detects whether the file is binary MessagePack or plain JSON and loads it.
nlohmann::json readSaveData(const std::string& filepath);

// Get save type folder name
std::string getSaveTypeFolderName(SaveType type);

// Unpack a monolithic JSON save into individual files in a directory
void unpackSaveToDirectory(const nlohmann::json& j, const std::string& directoryPath);

// Compile an unpacked directory back into a monolithic JSON save
nlohmann::json compileSaveFromDirectory(const std::string& directoryPath);

// Check if an unpacked directory exists and has been modified since the monolithic file was saved
bool isUnpackedDirectoryNewer(const std::string& directoryPath, const std::string& monolithicFilePath);

// Create backup of existing save
std::string createBackup(const std::string& originalFile, SaveType type = SaveType::WORLD);

// Clean up old saves (keep only the most recent N saves)
void cleanupOldSaves(SaveType type = SaveType::WORLD, int keepCount = 10);

// Get save metadata (creation time, size, etc.)
struct SaveMetadata {
    std::string filename;
    std::string fullPath;
    std::time_t creationTime;
    size_t fileSize;
    SaveType type;
    std::string customLabel;
    
    // Cloud foundations
    bool isCloudOnly = false;
    bool keepLocal = true;
};

std::vector<SaveMetadata> getSaveMetadata(SaveType type = SaveType::WORLD);

// Merge two save files (j2 takes precedence over j1 in conflicts)
nlohmann::json mergeSaveFiles(const std::string& file1, const std::string& file2);

// Merge two save files and save the result, returns the new filename
std::string mergeAndSaveFiles(const std::string& file1, const std::string& file2, const std::string& outputLabel = "", SaveType type = SaveType::WORLD);

// ------------------------------------------------------------------
// Zone identity store. A Zone is not a copy inside a session/"world"
// file: it lives at saves/zones/<sanitized-identifier>/zone.json and
// is named, forked, and evolved across sessions. Session files
// (SaveType::WORLD) reference these identities.
// ------------------------------------------------------------------
std::string zoneDirectory(const std::string& identifier);
std::string zoneIdentityPath(const std::string& identifier);
bool zoneIdentityExists(const std::string& identifier);
bool writeZoneIdentity(const std::string& identifier, const nlohmann::json& j);
nlohmann::json readZoneIdentity(const std::string& identifier);
// Identifiers as stored (from zone.json `identifier`/`name`, else the folder).
std::vector<std::string> listZoneIdentities();

} // namespace SaveSystem