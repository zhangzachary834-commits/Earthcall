#include "Util/SaveSystem.hpp"
#include <filesystem>
#include <fstream>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace SaveSystem {

std::string getSaveTypeFolderName(SaveType type) {
    switch (type) {
        case SaveType::GAME: return "games";
        case SaveType::AVATAR: return "avatars";
        case SaveType::DESIGN: return "designs";
        case SaveType::BACKUP: return "backups";
        case SaveType::CUSTOM: return "custom";
        case SaveType::INTEGRATION: return "integrations";
        default: return "games";
    }
}

std::string getSaveTypeLogName(SaveType type) {
    switch (type) {
        case SaveType::GAME: return "game_save_log.txt";
        case SaveType::AVATAR: return "avatar_save_log.txt";
        case SaveType::DESIGN: return "design_save_log.txt";
        case SaveType::BACKUP: return "backup_save_log.txt";
        case SaveType::CUSTOM: return "custom_save_log.txt";
        case SaveType::INTEGRATION: return "integration_save_log.txt";
        default: return "game_save_log.txt";
    }
}

std::string ensureSaveFolder() {
    std::filesystem::path p = "saves";
    std::error_code ec;
    if (!std::filesystem::exists(p, ec)) {
        if (!std::filesystem::create_directory(p, ec)) {
            std::cerr << "[SaveSystem] Failed to create saves folder: " << ec.message() << "\n";
            return "";
        }
    }
    return p.string();
}

std::string ensureSaveTypeFolder(SaveType type) {
    std::string mainFolder = ensureSaveFolder();
    if (mainFolder.empty()) return "";
    
    std::filesystem::path mainPath(mainFolder);
    std::filesystem::path typeFolder = mainPath / getSaveTypeFolderName(type);
    std::error_code ec;
    if (!std::filesystem::exists(typeFolder, ec)) {
        if (!std::filesystem::create_directory(typeFolder, ec)) {
            std::cerr << "[SaveSystem] Failed to create " << typeFolder.string() << " folder: " << ec.message() << "\n";
            return "";
        }
    }
    return typeFolder.string();
}

std::string timestamp() {
    std::time_t t = std::time(nullptr);
    char buf[32]; 
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", std::localtime(&t));
    return buf;
}

std::string makeFilename(const std::string& customLabel, SaveType type) {
    std::string folder = ensureSaveTypeFolder(type);
    if (folder.empty()) return "";
    
    std::string stem = customLabel.empty() ? timestamp() : customLabel;
    return folder + "/" + stem + ".json";
}

void addToLog(const std::string& filepath, SaveType type) {
    std::string logFile = "saves/logs/" + getSaveTypeLogName(type);
    
    // Ensure logs directory exists
    std::filesystem::path logsDir = "saves/logs";
    std::error_code ec;
    if (!std::filesystem::exists(logsDir, ec)) {
        if (!std::filesystem::create_directories(logsDir, ec)) {
            std::cerr << "[SaveSystem] Failed to create logs folder: " << ec.message() << "\n";
            return;
        }
    }
    
    std::ofstream log(logFile, std::ios::app);
    if (log.is_open()) {
        log << filepath << "\n";
        log.close();
    } else {
        std::cerr << "[SaveSystem] Failed to open log file: " << logFile << "\n";
    }
}

std::vector<std::string> listFiles(SaveType type) {
    std::vector<std::string> valid;
    std::string logFile = "saves/logs/" + getSaveTypeLogName(type);
    
    std::ifstream in(logFile);
    if (!in.is_open()) {
        // Log file doesn't exist yet, that's okay
        return valid;
    }
    
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        if (std::filesystem::exists(line)) {
            valid.push_back(line);
        }
    }
    in.close();
    
    // Rewrite log with pruned list if necessary
    std::ofstream out(logFile, std::ios::trunc);
    for (const auto& f : valid) {
        out << f << "\n";
    }
    out.close();
    
    return valid;
}

std::string writeJson(const nlohmann::json& j, const std::string& customLabel, SaveType type) {
    std::string filename = makeFilename(customLabel, type);
    if (filename.empty()) return "";
    
    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "[SaveSystem] Failed to open file for writing: " << filename << "\n";
        return "";
    }
    
    out << j.dump(2);
    out.close();
    
    addToLog(filename, type);
    return filename;
}

std::string createBackup(const std::string& originalFile, SaveType type) {
    if (!std::filesystem::exists(originalFile)) {
        std::cerr << "[SaveSystem] Cannot backup non-existent file: " << originalFile << "\n";
        return "";
    }
    
    std::string backupFolder = ensureSaveTypeFolder(SaveType::BACKUP);
    if (backupFolder.empty()) return "";
    
    std::filesystem::path originalPath(originalFile);
    std::string backupName = "backup_" + timestamp() + "_" + originalPath.filename().string();
    std::string backupPath = backupFolder + "/" + backupName;
    
    std::error_code ec;
    if (std::filesystem::copy_file(originalFile, backupPath, ec)) {
        addToLog(backupPath, SaveType::BACKUP);
        return backupPath;
    } else {
        std::cerr << "[SaveSystem] Failed to create backup: " << ec.message() << "\n";
        return "";
    }
}

void cleanupOldSaves(SaveType type, int keepCount) {
    auto files = listFiles(type);
    if (files.size() <= static_cast<size_t>(keepCount)) {
        return; // No cleanup needed
    }
    
    // Sort files by modification time (oldest first)
    std::vector<std::pair<std::string, std::time_t>> fileTimes;
    for (const auto& file : files) {
        std::error_code ec;
        auto modTime = std::filesystem::last_write_time(file, ec);
        if (!ec) {
            auto timeT = std::chrono::duration_cast<std::chrono::seconds>(
                modTime.time_since_epoch()).count();
            fileTimes.push_back({file, timeT});
        }
    }
    
    std::sort(fileTimes.begin(), fileTimes.end(), 
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    // Remove oldest files
    int toRemove = fileTimes.size() - keepCount;
    for (int i = 0; i < toRemove; ++i) {
        std::error_code ec;
        if (std::filesystem::remove(fileTimes[i].first, ec)) {
            std::cout << "[SaveSystem] Cleaned up old save: " << fileTimes[i].first << "\n";
        } else {
            std::cerr << "[SaveSystem] Failed to remove old save: " << fileTimes[i].first << "\n";
        }
    }
    
    // Refresh the log file
    listFiles(type);
}

std::vector<SaveMetadata> getSaveMetadata(SaveType type) {
    std::vector<SaveMetadata> metadata;
    auto files = listFiles(type);
    
    for (const auto& file : files) {
        SaveMetadata meta;
        meta.fullPath = file;
        meta.type = type;
        
        std::filesystem::path path(file);
        meta.filename = path.filename().string();
        
        // Extract custom label from filename (remove timestamp and .json)
        std::string name = meta.filename;
        if (name.length() > 5 && name.substr(name.length() - 5) == ".json") {
            name = name.substr(0, name.length() - 5);
        }
        
        // Check if it starts with timestamp pattern (YYYYMMDD_HHMMSS)
        if (name.length() >= 15 && name[8] == '_') {
            std::string timestamp = name.substr(0, 15);
            if (name.length() > 16) {
                meta.customLabel = name.substr(16); // Everything after timestamp_
            }
        } else {
            meta.customLabel = name;
        }
        
        // Get file stats
        std::error_code ec;
        auto modTime = std::filesystem::last_write_time(file, ec);
        if (!ec) {
            meta.creationTime = std::chrono::duration_cast<std::chrono::seconds>(
                modTime.time_since_epoch()).count();
        }
        
        auto fileSize = std::filesystem::file_size(file, ec);
        if (!ec) {
            meta.fileSize = fileSize;
        }
        
        metadata.push_back(meta);
    }
    
    // Sort by creation time (newest first)
    std::sort(metadata.begin(), metadata.end(), 
              [](const auto& a, const auto& b) { return a.creationTime > b.creationTime; });
    
    return metadata;
}

} // namespace SaveSystem 