#include "Singularity/Storage/SaveSystem.hpp"
#include <filesystem>
#include <fstream>
#include <ctime>
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <map>
#include "Singularity/Storage/CloudStorage.hpp"
#include "Identity/FirstMoverRegister.hpp"

#include <zlib.h>
#include <thread>
#include <atomic>

namespace SaveSystem {

namespace {
std::string g_saveRoot;
}

void setSaveRoot(const std::string& absoluteSavesDir) {
    g_saveRoot = absoluteSavesDir;
}

std::string saveRoot() {
    return g_saveRoot;
}

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
        case SaveType::WORLD: return "worlds";
        case SaveType::AVATAR: return "avatars";
        case SaveType::PERSON: return "persons";
        case SaveType::DESIGN: return "designs";
        case SaveType::BACKUP: return "backups";
        case SaveType::CUSTOM: return "custom";
        case SaveType::INTEGRATION: return "integrations";
        case SaveType::ZONE: return "zones";
        case SaveType::HOME: return "homes";
        default: return "games";
    }
}



std::string ensureSaveFolder() {
    std::filesystem::path p = g_saveRoot.empty() ? std::filesystem::path("saves")
                                                 : std::filesystem::path(g_saveRoot);
    std::error_code ec;
    if (!std::filesystem::exists(p, ec)) {
        if (!std::filesystem::create_directories(p, ec)) {
            std::cerr << "[SaveSystem] Failed to create saves folder: " << ec.message() << "\n";
            return "";
        }
    }
    
#ifdef __EMSCRIPTEN__
    ensureIdbMounted();
#endif
    
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
    // Control characters matter as much as separators here: an embedded NUL
    // truncates the name at the syscall boundary, so the path that gets
    // opened stops being the path that was checked.
    std::string safe;
    safe.reserve(label.size());
    for (unsigned char c : label) {
        if (c == '/' || c == '\\' || c == ':' || c < 0x20 || c == 0x7F) {
            safe.push_back('_');
        } else {
            safe.push_back(static_cast<char>(c));
        }
    }

    size_t pos = 0;
    while ((pos = safe.find("..", pos)) != std::string::npos) {
        safe.replace(pos, 2, "__");
        pos += 2;
    }

    // A name that is only dots still resolves to a directory entry rather than
    // a save, and an over-long one is rejected by the filesystem.
    if (safe.find_first_not_of('.') == std::string::npos) return "";
    if (safe.size() > 128) safe.resize(128);

    return safe;
}

std::string makeFilename(const std::string& customLabel, SaveType type, const std::string& ext) {
    std::string folder = ensureSaveTypeFolder(type);
    if (folder.empty()) return "";

    // A label that sanitizes away entirely must not collapse to a bare
    // extension ("saves/games/.ecsave"); fall back to the timestamp instead.
    std::string stem = customLabel.empty() ? std::string{} : sanitizeLabel(customLabel);
    if (stem.empty()) stem = timestamp();

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
                if (ext == ".ecsave" || ext == ".json" || ext == ".ecform") {
                    valid.push_back(entry.path().string());
                }
            }
        }
    }
    
    return valid;
}

std::vector<WorldEntry> listWorlds(SaveType type) {
    std::map<std::string, WorldEntry> byStem;
    for (const auto& path : listFiles(type)) {
        std::filesystem::path p(path);
        const std::string stem = p.stem().string();
        const std::string ext = p.extension().string();
        if (stem.size() >= 6 && stem.compare(stem.size() - 6, 6, "_delta") == 0)
            continue;
        std::error_code ec;
        if (!std::filesystem::exists(p, ec)) continue;
        if (std::filesystem::file_size(p, ec) == 0) continue;
        WorldEntry row{stem, path};
        auto it = byStem.find(stem);
        if (it == byStem.end()) {
            byStem.emplace(stem, std::move(row));
        } else if (ext == ".ecform") {
            it->second.path = path;
        } else if (ext == ".json" && it->second.path.find(".ecform") == std::string::npos) {
            it->second.path = path;
        }
    }
    std::vector<WorldEntry> out;
    out.reserve(byStem.size());
    for (auto& kv : byStem) out.push_back(std::move(kv.second));
    return out;
}

void removeWorld(const std::string& stem, SaveType type) {
    if (stem.empty()) return;
    std::string folder = ensureSaveTypeFolder(type);
    if (folder.empty()) return;
    const std::string safe = sanitizeLabel(stem);
    if (safe.empty()) return;
    std::error_code ec;
    std::filesystem::remove(folder + "/" + safe + ".ecform", ec);
    std::filesystem::remove(folder + "/" + safe + ".ecmatter", ec);
    std::filesystem::remove(folder + "/" + safe + ".json", ec);
    std::filesystem::remove(folder + "/" + safe + ".ecsave", ec);
    std::filesystem::remove(folder + "/" + safe + "_delta.ecsave", ec);
}

// The single gate every write passes through. Returns true when the write may
// proceed. With no First Mover session active this is always true -- the
// engine and in-world Persons are not governed here, and blocking them would
// be a regression, not a safeguard. Inside a session the mover must hold a
// scope covering this exact path.
static bool permitted(const std::string& filename) {
    auto& reg = Identity::FirstMoverRegister::instance();
    if (reg.permitsWrite(filename)) return true;

    // Refusals are loud. FIRST_MOVER_AUTHORING.md 8c is explicit that an
    // unauthorized write must be visible rather than silently dropped.
    std::cerr << "[SaveSystem] REFUSED write to " << filename << "\n"
              << "  mover: " << reg.activeMover().abbreviated() << "\n"
              << "  " << reg.explainWrite(filename) << "\n";
    return false;
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

// IDBFS state tracking
static bool s_idbMounted = false;

static void ensureIdbMounted() {
    if (s_idbMounted) return;
    // Mount the saves directory to IDBFS for persistence
    // IDBFS persists to IndexedDB and survives page reloads
    EM_JS(void, mount_idb, (), {
        // Check if IDBFS is already available
        if (typeof FS !== 'undefined' && FS.filesystems && FS.filesystems.IDBFS) {
            try {
                // Mount /saves to IDBFS
                FS.mkdir('/saves');
                FS.mount(IDBFS, { root: '/saves' }, '/saves');
                // Also mount the current directory if saves are there
                FS.mkdir('.');
                FS.mount(IDBFS, {}, '.');
            } catch (e) {
                console.error('IDBFS mount failed:', e);
            }
        }
    });
    s_idbMounted = true;
}

static void syncIdb() {
    // Sync IDBFS to IndexedDB
    EM_JS(void, sync_idb, (), {
        if (typeof FS !== 'undefined' && FS.syncfs) {
            try {
                FS.syncfs(true, function(err) {
                    if (err) {
                        console.error('IDBFS sync failed:', err);
                    } else {
                        console.log('IDBFS synced successfully');
                    }
                });
            } catch (e) {
                console.error('IDBFS sync error:', e);
            }
        }
    });
}

// There is no IDBFS mount, no FS.syncfs, and no --preload-file anywhere in
// this tree (see AUDIT_2026-08-10.md §2.7). The default wasm filesystem
// (MEMFS) is purely in-memory: the write below genuinely succeeds -- the
// bytes are readable back for the rest of this tab's session -- but nothing
// makes them survive a reload or a closed tab. Say so on every save, plainly,
// rather than let a successful in-memory write read as "saved." This is
// intentionally NOT a one-time warning like the cloud-sync notice below: it
// is the outcome of the specific save the Person just asked for, not a fixed
// fact about the build they can be told once and forget.
static void warnNotDurable(const std::string& filename) {
    std::cerr << "[SaveSystem] " << filename
              << " exists only in this browser tab's in-memory filesystem; it "
                 "will be LOST on reload or tab close. No persistent storage "
                 "(IDBFS or equivalent) is wired up for this build yet -- this "
                 "is not a real save.\n";
}
#endif

// Cloud sync has no server-independent implementation on any platform, and on
// wasm CloudStorage's callback fails synchronously on every single call (no
// HTTP client is wired for wasm; see CloudStorage.cpp). Reporting "Failed to
// sync ... to cloud" on every save reads as an ongoing problem when it is a
// fixed, known fact about this build -- say it once per session rather than
// spam it.
static std::atomic<bool> g_cloudSyncUnavailableWarned{false};
static void reportCloudSyncResult(bool success, const std::string& filename) {
    if (success) {
        std::cout << "[SaveSystem] Successfully synced " << filename << " to cloud.\n";
        return;
    }
    if (!g_cloudSyncUnavailableWarned.exchange(true)) {
        std::cerr << "[SaveSystem] Cloud sync unavailable in this build; saves "
                     "stay local to this session only (further failures this "
                     "session will not be logged again).\n";
    }
}

std::string writeSaveData(const nlohmann::json& j, const std::string& customLabel, SaveType type) {
    std::string filename = makeFilename(customLabel, type, ".ecform");
    if (filename.empty()) return "";
    if (!permitted(filename)) return "";

    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cerr << "[SaveSystem] Failed to open file for writing: " << filename << "\n";
        return "";
    }
    
    out << j.dump(2);
    out.flush();
    const bool wroteOk = static_cast<bool>(out);
    out.close();
    if (!wroteOk) {
        std::cerr << "[SaveSystem] Failed to write " << filename << "\n";
        return "";
    }
    std::error_code ec;
    if (!std::filesystem::exists(filename, ec) || std::filesystem::file_size(filename, ec) == 0) {
        std::cerr << "[SaveSystem] Write reported success but " << filename << " is missing or empty\n";
        return "";
    }

#ifdef __EMSCRIPTEN__
    ensureIdbMounted();
    syncIdb();
#endif

    return std::filesystem::absolute(filename).string();
}

std::string writeSaveData(const std::vector<uint8_t>& data, const std::string& customLabel, const std::string& ext, SaveType type) {
    std::string filename = makeFilename(customLabel, type, ext);
    if (filename.empty()) return "";
    if (!permitted(filename)) return "";

    std::ofstream out(filename, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to open " << filename << " for saving binary data.\n";
        return filename;
    }
    
    std::vector<uint8_t> compressed;
    try {
        compressed = compressData(data);
    } catch (const std::exception& e) {
        std::cerr << "[SaveSystem] Failed to compress " << filename << ": " << e.what() << "\n";
        return "";
    }
    out.write(reinterpret_cast<const char*>(compressed.data()), compressed.size());
    out.close();

#ifdef __EMSCRIPTEN__
    ensureIdbMounted();
    syncIdb();
#endif

    // Upload Binary to cloud
    Util::CloudStorage::uploadSaveAsync(filename, data, type, [filename](bool success) {
        reportCloudSyncResult(success, filename);
        // Check keepLocal logic here eventually
    });

    return filename;
}

std::atomic<bool> g_isSaving{false};

bool isSaving() {
    return g_isSaving.load();
}

void writeSaveDataAsync(const nlohmann::json& j, const std::string& customLabel, SaveType type) {
    g_isSaving.store(true);
    
#ifdef __EMSCRIPTEN__
    try {
        writeSaveData(j, customLabel, type);
    } catch (const std::exception& e) {
        std::cerr << "[SaveSystem] save failed: " << e.what() << "\n";
    } catch (...) {
        std::cerr << "[SaveSystem] save failed: unknown error\n";
    }
    g_isSaving.store(false);
#else
    // We deep copy the JSON object to pass it safely to the detached thread.
    // Nothing may escape this lambda: an exception crossing the top of a
    // thread is std::terminate, and the flag must clear on every path or the
    // UI reports a save that is still in flight forever.
    std::thread([j_copy = j, customLabel, type]() {
        try {
            writeSaveData(j_copy, customLabel, type);
        } catch (const std::exception& e) {
            std::cerr << "[SaveSystem] Async save failed: " << e.what() << "\n";
        } catch (...) {
            std::cerr << "[SaveSystem] Async save failed: unknown error\n";
        }
        g_isSaving.store(false);
    }).detach();
#endif
}

std::string writeMatterData(const std::vector<uint8_t>& data, const std::string& customLabel, SaveType type) {
    std::string filename = makeFilename(customLabel, type, ".ecmatter");
    if (filename.empty()) return "";
    if (!permitted(filename)) return "";

    std::ofstream out(filename, std::ios::binary);
    if (!out.is_open()) {
        std::cerr << "[SaveSystem] Failed to open " << filename << " for saving matter data.\n";
        return "";
    }
    out.write(reinterpret_cast<const char*>(data.data()), data.size());
    out.flush();
    const bool wroteOk = static_cast<bool>(out);
    out.close();
    if (!wroteOk) {
        std::cerr << "[SaveSystem] Failed to write matter data to " << filename << "\n";
        return "";
    }

#ifdef __EMSCRIPTEN__
    ensureIdbMounted();
    syncIdb();
#endif

    return filename;
}

void writeMatterDataAsync(const std::vector<uint8_t>& data, const std::string& customLabel, SaveType type) {
    g_isSaving.store(true);
#ifdef __EMSCRIPTEN__
    try {
        writeMatterData(data, customLabel, type);
    } catch (const std::exception& e) {
        std::cerr << "[SaveSystem] writeMatterData failed: " << e.what() << "\n";
    } catch (...) {
        std::cerr << "[SaveSystem] writeMatterData failed: unknown error\n";
    }
    g_isSaving.store(false);
#else
    std::thread([data_copy = data, customLabel, type]() {
        try {
            writeMatterData(data_copy, customLabel, type);
        } catch (const std::exception& e) {
            std::cerr << "[SaveSystem] Async writeMatterData failed: " << e.what() << "\n";
        } catch (...) {
            std::cerr << "[SaveSystem] Async writeMatterData failed: unknown error\n";
        }
        g_isSaving.store(false);
    }).detach();
#endif
}

std::vector<uint8_t> readMatterData(const std::string& filepath) {
    std::filesystem::path p(filepath);
    if (p.extension() != ".ecmatter") {
        p.replace_extension(".ecmatter");
    }
    std::error_code ec;
    if (!std::filesystem::exists(p, ec)) {
        return {};
    }
    std::ifstream in(p.string(), std::ios::binary);
    if (!in.is_open()) {
        return {};
    }
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}


nlohmann::json readSaveData(const std::string& filepath) {
    std::filesystem::path p(filepath);
    std::string actualPath = filepath;
    std::error_code ec;
    if (!std::filesystem::exists(p, ec)) {
        std::filesystem::path ecformPath = p;
        ecformPath.replace_extension(".ecform");
        if (std::filesystem::exists(ecformPath, ec)) {
            actualPath = ecformPath.string();
        } else {
            std::filesystem::path ecsavePath = p;
            ecsavePath.replace_extension(".ecsave");
            if (std::filesystem::exists(ecsavePath, ec)) {
                actualPath = ecsavePath.string();
            } else {
                std::filesystem::path jsonPath = p;
                jsonPath.replace_extension(".json");
                if (std::filesystem::exists(jsonPath, ec)) {
                    actualPath = jsonPath.string();
                }
            }
        }
    }

    std::ifstream in(actualPath, std::ios::binary);
    if (!in.is_open()) {
        std::cerr << "[SaveSystem] Failed to open file for reading: " << filepath << "\n";
        return nlohmann::json();
    }
    
    // Check magic bytes or extension to determine if it's msgpack
    if (actualPath.length() > 7 && actualPath.substr(actualPath.length() - 7) == ".ecsave") {
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
        try {
            in >> j;
        } catch (const std::exception& e) {
            std::cerr << "[SaveSystem] Failed to parse JSON " << filepath
                      << ": " << e.what() << "\n";
            return nlohmann::json();
        }
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

void unpackSaveToDirectory(const nlohmann::json& j, const std::string& directoryPath) {
    std::error_code ec;
    std::filesystem::create_directories(directoryPath, ec);
    std::filesystem::create_directories(directoryPath + "/objects", ec);
    
    nlohmann::json meta = j;
    if (meta.contains("objects")) {
        const auto& objects = meta["objects"];
        for (const auto& obj : objects) {
            std::string objId = "unknown";
            if (obj.contains("identifier")) {
                objId = obj["identifier"].get<std::string>();
            } else if (obj.contains("id")) {
                objId = obj["id"].get<std::string>();
            }
            std::string objPath = directoryPath + "/objects/object_" + sanitizeLabel(objId) + ".json";
            std::ofstream objFile(objPath);
            if (objFile.is_open()) {
                objFile << std::setw(2) << obj << std::endl;
            }
        }
        meta.erase("objects");
    }
    
    if (meta.contains("authoredLaws") && meta["authoredLaws"].contains("laws")) {
        std::filesystem::create_directories(directoryPath + "/authored_laws", ec);
        const auto& laws = meta["authoredLaws"]["laws"];
        for (const auto& law : laws) {
            std::string lawId = "unknown";
            if (law.contains("identifier")) {
                lawId = law["identifier"].get<std::string>();
            } else if (law.contains("id")) {
                lawId = law["id"].get<std::string>();
            }
            std::string lawPath = directoryPath + "/authored_laws/law_" + sanitizeLabel(lawId) + ".json";
            std::ofstream lawFile(lawPath);
            if (lawFile.is_open()) {
                lawFile << std::setw(2) << law << std::endl;
            }
        }
        meta["authoredLaws"].erase("laws");
    }
    
    if (meta.contains("zones")) {
        std::filesystem::create_directories(directoryPath + "/zones", ec);
        const auto& zones = meta["zones"];
        for (const auto& zone : zones) {
            std::string zoneId = "unknown";
            if (zone.contains("name")) {
                zoneId = zone["name"].get<std::string>();
            }
            std::string zonePath = directoryPath + "/zones/zone_" + sanitizeLabel(zoneId) + ".json";
            std::ofstream zoneFile(zonePath);
            if (zoneFile.is_open()) {
                zoneFile << std::setw(2) << zone << std::endl;
            }
        }
        meta.erase("zones");
    }
    
    std::string metaPath = directoryPath + "/world_meta.json";
    std::ofstream metaFile(metaPath);
    if (metaFile.is_open()) {
        metaFile << std::setw(2) << meta << std::endl;
    }
}

nlohmann::json compileSaveFromDirectory(const std::string& directoryPath) {
    std::string metaPath = directoryPath + "/world_meta.json";
    std::ifstream metaFile(metaPath);
    nlohmann::json j;
    if (metaFile.is_open()) {
        try {
            metaFile >> j;
        } catch (...) {
            std::cerr << "[SaveSystem] Failed to parse world_meta.json\n";
        }
    }
    
    nlohmann::json objects = nlohmann::json::array();
    std::string objectsDir = directoryPath + "/objects";
    std::error_code ec;
    if (std::filesystem::exists(objectsDir, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(objectsDir, ec)) {
            if (entry.path().extension() == ".json") {
                std::ifstream objFile(entry.path());
                if (objFile.is_open()) {
                    try {
                        nlohmann::json objJson;
                        objFile >> objJson;
                        objects.push_back(objJson);
                    } catch (...) {
                        std::cerr << "[SaveSystem] Failed to parse object JSON: " << entry.path().string() << "\n";
                    }
                }
            }
        }
    }
    
    j["objects"] = objects;
    
    if (j.contains("authoredLaws")) {
        nlohmann::json laws = nlohmann::json::array();
        std::string lawsDir = directoryPath + "/authored_laws";
        if (std::filesystem::exists(lawsDir, ec)) {
            for (const auto& entry : std::filesystem::directory_iterator(lawsDir, ec)) {
                if (entry.path().extension() == ".json") {
                    std::ifstream lawFile(entry.path());
                    if (lawFile.is_open()) {
                        try {
                            nlohmann::json lawJson;
                            lawFile >> lawJson;
                            laws.push_back(lawJson);
                        } catch (...) {
                            std::cerr << "[SaveSystem] Failed to parse authored law JSON: " << entry.path().string() << "\n";
                        }
                    }
                }
            }
        }
        j["authoredLaws"]["laws"] = laws;
    }
    
    nlohmann::json zones = nlohmann::json::array();
    std::string zonesDir = directoryPath + "/zones";
    if (std::filesystem::exists(zonesDir, ec)) {
        for (const auto& entry : std::filesystem::directory_iterator(zonesDir, ec)) {
            if (entry.path().extension() == ".json") {
                std::ifstream zoneFile(entry.path());
                if (zoneFile.is_open()) {
                    try {
                        nlohmann::json zoneJson;
                        zoneFile >> zoneJson;
                        zones.push_back(zoneJson);
                    } catch (...) {
                        std::cerr << "[SaveSystem] Failed to parse zone JSON: " << entry.path().string() << "\n";
                    }
                }
            }
        }
    }
    if (!zones.empty()) {
        j["zones"] = zones;
    }
    
    return j;
}

bool isUnpackedDirectoryNewer(const std::string& directoryPath, const std::string& monolithicFilePath) {
    std::error_code ec;
    if (!std::filesystem::exists(directoryPath, ec) || !std::filesystem::exists(monolithicFilePath, ec)) {
        return false;
    }
    
    auto monolithicTime = std::filesystem::last_write_time(monolithicFilePath, ec);
    if (ec) return false;
    
    auto getNewestTime = [](const std::string& path) -> std::filesystem::file_time_type {
        std::filesystem::file_time_type newest = std::filesystem::file_time_type::min();
        std::error_code ec;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path, ec)) {
            auto time = std::filesystem::last_write_time(entry.path(), ec);
            if (!ec && time > newest) newest = time;
        }
        return newest;
    };
    
    auto dirNewestTime = getNewestTime(directoryPath);
    return dirNewestTime > monolithicTime;
}

std::string zoneDirectory(const std::string& identifier) {
    std::string folder = ensureSaveTypeFolder(SaveType::ZONE);
    if (folder.empty()) return "";
    const std::string safe = sanitizeLabel(identifier);
    if (safe.empty()) return "";
    const std::filesystem::path dir = std::filesystem::path(folder) / safe;
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        std::cerr << "[SaveSystem] Failed to create zone directory "
                  << dir.string() << ": " << ec.message() << "\n";
        return "";
    }
    return dir.string();
}

std::string zoneIdentityPath(const std::string& identifier) {
    const std::string dir = zoneDirectory(identifier);
    if (dir.empty()) return "";
    return dir + "/zone.json";
}

bool zoneIdentityExists(const std::string& identifier) {
    if (identifier.empty()) return false;
    std::string folder = ensureSaveTypeFolder(SaveType::ZONE);
    if (folder.empty()) return false;
    const std::string safe = sanitizeLabel(identifier);
    if (safe.empty()) return false;
    std::error_code ec;
    const auto path = std::filesystem::path(folder) / safe / "zone.json";
    return std::filesystem::exists(path, ec) && std::filesystem::file_size(path, ec) > 0;
}

bool writeZoneIdentity(const std::string& identifier, const nlohmann::json& j) {
    const std::string path = zoneIdentityPath(identifier);
    if (path.empty()) return false;
    if (!permitted(path)) return false;
    std::ofstream out(path);
    if (!out) {
        std::cerr << "[SaveSystem] Failed to open zone identity for writing: "
                  << path << "\n";
        return false;
    }
    out << j.dump(2);
    out.flush();
    if (!out) {
        std::cerr << "[SaveSystem] Failed to write zone identity: " << path << "\n";
        return false;
    }
    return true;
}

nlohmann::json readZoneIdentity(const std::string& identifier) {
    if (!zoneIdentityExists(identifier)) return nlohmann::json();
    std::string folder = ensureSaveTypeFolder(SaveType::ZONE);
    if (folder.empty()) return nlohmann::json();
    const std::string safe = sanitizeLabel(identifier);
    const auto path = (std::filesystem::path(folder) / safe / "zone.json").string();
    return readSaveData(path);
}

std::vector<std::string> listZoneIdentities() {
    std::vector<std::string> out;
    std::string folder = ensureSaveTypeFolder(SaveType::ZONE);
    if (folder.empty()) return out;
    std::error_code ec;
    if (!std::filesystem::exists(folder, ec)) return out;
    for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
        if (!entry.is_directory()) continue;
        const auto zoneFile = entry.path() / "zone.json";
        if (!std::filesystem::exists(zoneFile, ec)) continue;
        if (std::filesystem::file_size(zoneFile, ec) == 0) continue;
        nlohmann::json j = readSaveData(zoneFile.string());
        std::string id;
        if (j.is_object()) {
            id = j.value("identifier", j.value("name", std::string{}));
        }
        if (id.empty()) id = entry.path().filename().string();
        out.push_back(std::move(id));
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::string homeDirectory(const std::string& identifier) {
    std::string folder = ensureSaveTypeFolder(SaveType::HOME);
    if (folder.empty()) return "";
    const std::string safe = sanitizeLabel(identifier);
    if (safe.empty()) return "";
    const std::filesystem::path dir = std::filesystem::path(folder) / safe;
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    if (ec) {
        std::cerr << "[SaveSystem] Failed to create home directory "
                  << dir.string() << ": " << ec.message() << "\n";
        return "";
    }
    return dir.string();
}

std::string homeIdentityPath(const std::string& identifier) {
    const std::string dir = homeDirectory(identifier);
    if (dir.empty()) return "";
    return dir + "/home.json";
}

bool homeIdentityExists(const std::string& identifier) {
    if (identifier.empty()) return false;
    std::string folder = ensureSaveTypeFolder(SaveType::HOME);
    if (folder.empty()) return false;
    const std::string safe = sanitizeLabel(identifier);
    if (safe.empty()) return false;
    std::error_code ec;
    const auto path = std::filesystem::path(folder) / safe / "home.json";
    return std::filesystem::exists(path, ec) && std::filesystem::file_size(path, ec) > 0;
}

bool writeHomeIdentity(const std::string& identifier, const nlohmann::json& j) {
    const std::string path = homeIdentityPath(identifier);
    if (path.empty()) return false;
    if (!permitted(path)) return false;
    std::ofstream out(path);
    if (!out) {
        std::cerr << "[SaveSystem] Failed to open home identity for writing: "
                  << path << "\n";
        return false;
    }
    out << j.dump(2);
    out.flush();
    if (!out) {
        std::cerr << "[SaveSystem] Failed to write home identity: " << path << "\n";
        return false;
    }
    return true;
}

nlohmann::json readHomeIdentity(const std::string& identifier) {
    if (!homeIdentityExists(identifier)) return nlohmann::json();
    std::string folder = ensureSaveTypeFolder(SaveType::HOME);
    if (folder.empty()) return nlohmann::json();
    const std::string safe = sanitizeLabel(identifier);
    const auto path = (std::filesystem::path(folder) / safe / "home.json").string();
    return readSaveData(path);
}

std::vector<std::string> listHomeIdentities() {
    std::vector<std::string> out;
    std::string folder = ensureSaveTypeFolder(SaveType::HOME);
    if (folder.empty()) return out;
    std::error_code ec;
    if (!std::filesystem::exists(folder, ec)) return out;
    for (const auto& entry : std::filesystem::directory_iterator(folder, ec)) {
        if (!entry.is_directory()) continue;
        const auto homeFile = entry.path() / "home.json";
        if (!std::filesystem::exists(homeFile, ec)) continue;
        if (std::filesystem::file_size(homeFile, ec) == 0) continue;
        nlohmann::json j = readSaveData(homeFile.string());
        std::string id;
        if (j.is_object()) {
            id = j.value("identifier", j.value("name", std::string{}));
        }
        if (id.empty()) id = entry.path().filename().string();
        out.push_back(std::move(id));
    }
    std::sort(out.begin(), out.end());
    return out;
}

} // namespace SaveSystem 