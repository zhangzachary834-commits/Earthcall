#pragma once
#include <string>
#include <vector>
#include "json.hpp"

namespace SaveSystem {

// Save types for organization
enum class SaveType {
    GAME,       // Game state saves
    AVATAR,     // Avatar saves
    DESIGN,     // Design system saves
    BACKUP,     // Automatic backups
    CUSTOM,     // Custom saves
    INTEGRATION // Integration system saves (web apps, external windows, etc.)
};

// Ensure organized save folder structure exists
std::string ensureSaveFolder();
std::string ensureSaveTypeFolder(SaveType type);

// Build filename with timestamp or custom label stored in organized folders
std::string makeFilename(const std::string& customLabel = "", SaveType type = SaveType::GAME);

// Append new entry to appropriate save log
void addToLog(const std::string& filepath, SaveType type = SaveType::GAME);

// Return list of files that still exist for a specific save type; also prunes stale entries from log
std::vector<std::string> listFiles(SaveType type = SaveType::GAME);

// Write JSON to disk via generated filename and log it; returns full path
std::string writeJson(const nlohmann::json& j, const std::string& customLabel = "", SaveType type = SaveType::GAME);

// Get save type folder name
std::string getSaveTypeFolderName(SaveType type);

// Get save type log file name
std::string getSaveTypeLogName(SaveType type);

// Create backup of existing save
std::string createBackup(const std::string& originalFile, SaveType type = SaveType::GAME);

// Clean up old saves (keep only the most recent N saves)
void cleanupOldSaves(SaveType type = SaveType::GAME, int keepCount = 10);

// Get save metadata (creation time, size, etc.)
struct SaveMetadata {
    std::string filename;
    std::string fullPath;
    std::time_t creationTime;
    size_t fileSize;
    SaveType type;
    std::string customLabel;
};

std::vector<SaveMetadata> getSaveMetadata(SaveType type = SaveType::GAME);

} // namespace SaveSystem 