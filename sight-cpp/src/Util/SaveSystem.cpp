#include "Util/SaveSystem.hpp"
#include <filesystem>
#include <fstream>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include "Util/CloudStorage.hpp"

#include <zlib.h>
#include <thread>
#include <atomic>

namespace SaveSystem {

// Helper to compress a byte vector using zlib
std::vector<uint8_t> compressData(const std::vector<uint8_t>& data) {
    uLongf compressedLen = compressBound(data.size());
    std::vector<uint8_t> compressed(compressedLen);
    
    if (compress(compressed.data(), &compressedLen, data.data(), data.size()) != Z_OK) {
        std::cerr << "[SaveSystem] zlib compression failed!\n";
        throw std::runtime_error("Compression failed");
    }
    
    compressed.resize(compressedLen);
    
    // We prepend the original uncompressed size (8 bytes) so we know how much to allocate for decompression
    std::vector<uint8_t> result(sizeof(size_t) + compressed.size());
    size_t originalSize = data.size();
    std::memcpy(result.data(), &originalSize, sizeof(size_t));
    std::memcpy(result.data() + sizeof(size_t), compressed.data(), compressed.size());
    
    return result;
}

// Helper to decompress a byte vector using zlib
std::vector<uint8_t> decompressData(const std::vector<uint8_t>& data) {
    if (data.size() <= sizeof(size_t)) return data; // Invalid or uncompressed
    
    size_t originalSize;
    std::memcpy(&originalSize, data.data(), sizeof(size_t));
    
    // Cross-check against compressed size and hard limits to avoid OOM
    if (originalSize > (data.size() - sizeof(size_t)) * 1032 + 1024 || originalSize > 1024 * 1024 * 1024) {
        return data; // Invalid size, fallback
    }
    
    // Sanity check for uncompressed saves (if they don't have the size prefix, decompression will just fail and we fallback)
    // A MessagePack payload usually starts with 0x8. If originalSize happens to match that, it might try to decompress.
    // To be perfectly safe, we'll try to decompress, and if it fails, we assume it's an uncompressed legacy .ecsave.
    std::vector<uint8_t> uncompressed(originalSize);
    uLongf destLen = originalSize;
    
    int res = uncompress(uncompressed.data(), &destLen, data.data() + sizeof(size_t), data.size() - sizeof(size_t));
    if (res != Z_OK) {
        // Fallback: This might be an uncompressed .ecsave from Phase 3.
        return data;
    }
    return uncompressed;
}

std::string getSaveTypeFolderName(SaveType type) {
    switch (type) {
        case SaveType::GAME: return "games";
        case SaveType::AVATAR: return "avatars";
        case SaveType::PERSON: return "persons";
        case SaveType::DESIGN: return "designs";
        case SaveType::BACKUP: return "backups";
        case SaveType::CUSTOM: return "custom";
        case SaveType::INTEGRATION: return "integrations";
        default: return "games";
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
    struct tm tm_buf;
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    std::strftime(buf, sizeof(buf), "%Y%m%d_%H%M%S", &tm_buf);
    return buf;
}

std::string sanitizeLabel(const std::string& label) {
    std::string safe = label;
    for (char& c : safe) {
        if (c == '/' || c == '\\' || c == ':') c = '_';
    }
    size_t pos;
    while ((pos = safe.find("..")) != std::string::npos) {
        safe.replace(pos, 2, "__");
    }
    return safe;
}

std::string makeFilename(const std::string& customLabel, SaveType type, const std::string& ext) {
    std::string folder = ensureSaveTypeFolder(type);
    if (folder.empty()) return "";
    
    std::string stem = customLabel.empty() ? timestamp() : sanitizeLabel(customLabel);
    return folder + "/" + stem + ext;
}



std::vector<std::string> listFiles(SaveType type) {
    std::vector<std::string> valid;
    std::string folder = ensureSaveTypeFolder(type);
    if (folder.empty()) return valid;
    
    // Enumerate the save folder directly rather than trusting the log file.
    // The log is an ordinary writable file, so listing it required an explicit
    // path-traversal guard against poisoned entries; iterating the folder
    // cannot escape it by construction, which retires the guard rather than
    // dropping it.
    std::error_code ec;
    if (std::filesystem::exists(folder, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
            if (entry.is_regular_file()) {
                std::string ext = entry.path().extension().string();
                if (ext == ".ecsave" || ext == ".json") {
                    valid.push_back(entry.path().string());
                }
            }
        }
    }
    
    return valid;
}

std::string writeSaveData(const nlohmann::json& j, const std::string& customLabel, SaveType type) {
    std::string filename = makeFilename(customLabel, type, ".ecsave");
    if (filename.empty()) return "";
    
    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "[SaveSystem] Failed to open binary file for writing: " << filename << "\n";
        return "";
    }
    
    std::vector<uint8_t> v = nlohmann::json::to_msgpack(j);
    std::vector<uint8_t> compressed = compressData(v);
    out.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
    out.close();
    
    // Upload Binary to cloud
    Util::CloudStorage::uploadSaveAsync(filename, v, type, [filename](bool success) {
        if (success) {
            std::cout << "[SaveSystem] Successfully synced binary " << filename << " to cloud.\n";
            // Here we would check keepLocal and potentially delete the local file
        } else {
            std::cerr << "[SaveSystem] Failed to sync binary " << filename << " to cloud.\n";
        }
    });
    
    return filename;
}

std::string writeSaveData(const std::vector<uint8_t>& data, const std::string& customLabel, const std::string& ext, SaveType type) {
    std::string filename = makeFilename(customLabel, type, ext);
    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open " << filename << " for saving binary data.\n";
        return filename;
    }
    
    std::vector<uint8_t> compressed = compressData(data);
    out.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
    out.close();
    
    // Upload Binary to cloud
    Util::CloudStorage::uploadSaveAsync(filename, data, type, [filename](bool success) {
        if (success) {
            std::cout << "[SaveSystem] Successfully synced binary " << filename << " to cloud.\n";
            // Check keepLocal logic here eventually
        } else {
            std::cerr << "[SaveSystem] Failed to sync binary " << filename << " to cloud.\n";
        }
    });
    
    return filename;
}

std::atomic<bool> g_isSaving{false};

bool isSaving() {
    return g_isSaving.load();
}

void writeSaveDataAsync(const nlohmann::json& j, const std::string& customLabel, SaveType type) {
    g_isSaving.store(true);
    
    // We deep copy the JSON object to pass it safely to the detached thread
    std::thread([j_copy = j, customLabel, type]() {
        writeSaveData(j_copy, customLabel, type);
        g_isSaving.store(false);
    }).detach();
}

void writeSaveDataAsync(const std::vector<uint8_t>& data, const std::string& customLabel, const std::string& ext, SaveType type) {
    g_isSaving.store(true);
    
    // Deep copy the vector
    std::thread([data_copy = data, customLabel, ext, type]() {
        writeSaveData(data_copy, customLabel, ext, type);
        g_isSaving.store(false);
    }).detach();
}


nlohmann::json readSaveData(const std::string& filepath) {
    std::ifstream in(filepath, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "[SaveSystem] Failed to open file for reading: " << filepath << "\n";
        return nlohmann::json();
    }
    
    // Check magic bytes or extension to determine if it's msgpack
    if (filepath.length() > 7 && filepath.substr(filepath.length() - 7) == ".ecsave") {
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        std::vector<uint8_t> decompressed = decompressData(bytes);
        try {
            return nlohmann::json::from_msgpack(decompressed);
        } catch (...) {
            std::cerr << "[SaveSystem] Malformed msgpack in: " << filepath << "\n";
            return nlohmann::json();
        }
    } else {
        // Fallback to plain JSON
        nlohmann::json j;
        in >> j;
        return j;
    }
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

nlohmann::json mergeSaveFiles(const std::string& file1, const std::string& file2) {
    nlohmann::json j1 = readSaveData(file1);
    nlohmann::json j2 = readSaveData(file2);
    
    if (j1.is_null() && !j2.is_null()) return j2;
    if (j2.is_null() && !j1.is_null()) return j1;
    if (j1.is_null() && j2.is_null()) return nlohmann::json::object();
    
    // If both are objects, we can merge them
    if (j1.is_object() && j2.is_object()) {
        j1.update(j2);
    } else {
        // Fallback: just return j2 if they aren't objects that can be merged
        return j2;
    }
    
    return j1;
}

std::string mergeAndSaveFiles(const std::string& file1, const std::string& file2, const std::string& outputLabel, SaveType type) {
    nlohmann::json merged = mergeSaveFiles(file1, file2);
    if (merged.is_null() || merged.empty()) {
        std::cerr << "[SaveSystem] Merge resulted in empty/null data, aborting save.\n";
        return "";
    }
    
    std::string label = outputLabel.empty() ? "merged_" + timestamp() : outputLabel;
    return writeSaveData(merged, label, type);
}

} // namespace SaveSystem 