# Save System Upgrade - Organized Folder Structure

## Overview

The save system has been completely reorganized to provide structured storage, easier management, and improved persistence across Earthcall's ontology. Saves are categorized as **Worlds** rather than games, with dedicated folders and logs under `src/Singularity/Storage/SaveSystem.hpp`.

## Folder Structure

```
saves/
├── worlds/         # World and Zone state saves (.json / .ecsave)
├── avatars/        # Avatar saves
├── persons/        # Registered Person / user profiles
├── designs/        # Design system saves
├── backups/        # Automatic backups
├── logs/           # Save logs and metadata
├── formations/     # Formation saves (legacy)
├── integration/    # Integration and external adapter saves
└── objects/        # Object saves (legacy)
```

## Save Types

Defined in `SaveSystem::SaveType`:

- **`WORLD`**: World and Zone state saves (objects, laws, zones, physics)
- **`AVATAR`**: Avatar saves (character data, body parts, mesh customizations)
- **`PERSON`**: Registered Person profiles and identity ledgers
- **`DESIGN`**: Design system saves (layers, tool states)
- **`BACKUP`**: Automatic timestamped backups
- **`CUSTOM`**: Custom saves for specific purposes
- **`INTEGRATION`**: External application and bridge state saves

## Key Features

### 1. Organized Storage & Multi-Format Support
- Each save type has its own folder (`saves/worlds/`, `saves/avatars/`, etc.).
- Dual serialization: Unified `readSaveData()` and `writeSaveData()` support plain **JSON** (`.json`) as well as high-performance **MessagePack / FlatBuffers / Frontier** (`.ecsave`).

### 2. Enhanced Logging & Pruning
- Dedicated log files (`world_save_log.txt`, `avatar_save_log.txt`, `person_save_log.txt`).
- `listFiles(type)` automatically prunes stale entries from logs.

### 3. Metadata Tracking
- File creation timestamp and size.
- Custom labels and cloud sync metadata (`isCloudOnly`, `keepLocal`).

### 4. Backup & Retention
- Automated backup creation via `createBackup()`.
- `cleanupOldSaves(type, keepCount)` keeps only the most recent saves.

## Core API (`src/Singularity/Storage/SaveSystem.hpp`)

```cpp
namespace SaveSystem {

enum class SaveType {
    WORLD,       // World state saves
    AVATAR,      // Avatar saves
    PERSON,      // Registered Persons / user profiles
    DESIGN,      // Design system saves
    BACKUP,      // Automatic backups
    CUSTOM,      // Custom saves
    INTEGRATION  // Integration system saves
};

// Folder and filename utilities
std::string ensureSaveFolder();
std::string ensureSaveTypeFolder(SaveType type);
std::string makeFilename(const std::string& customLabel = "", SaveType type = SaveType::WORLD, const std::string& ext = ".json");
std::string sanitizeLabel(const std::string& label);

// Query files and metadata
std::vector<std::string> listFiles(SaveType type = SaveType::WORLD);
std::vector<SaveMetadata> getSaveMetadata(SaveType type = SaveType::WORLD);

// Serialization & Persistence (sync & async)
std::string writeSaveData(const nlohmann::json& j, const std::string& customLabel = "", SaveType type = SaveType::WORLD);
void writeSaveDataAsync(const nlohmann::json& j, const std::string& customLabel = "", SaveType type = SaveType::WORLD);

std::string writeSaveData(const std::vector<uint8_t>& data, const std::string& customLabel, const std::string& ext, SaveType type);
void writeSaveDataAsync(const std::vector<uint8_t>& data, const std::string& customLabel, const std::string& ext, SaveType type);

nlohmann::json readSaveData(const std::string& filepath);

// Backups and Retention
std::string createBackup(const std::string& originalFile, SaveType type = SaveType::WORLD);
void cleanupOldSaves(SaveType type = SaveType::WORLD, int keepCount = 10);

} // namespace SaveSystem
```

## Usage Examples

### Saving World State

```cpp
// Save world state with timestamp
SaveSystem::writeSaveData(worldData, "", SaveSystem::SaveType::WORLD);

// Save with custom name
SaveSystem::writeSaveData(worldData, "GenesisWorld", SaveSystem::SaveType::WORLD);
```

### Loading World State

```cpp
// Get list of world saves
auto worldSaves = SaveSystem::listFiles(SaveSystem::SaveType::WORLD);

// Load data (auto-detects JSON vs MessagePack)
nlohmann::json worldJson = SaveSystem::readSaveData(worldSaves[0]);
```
