#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <chrono>
#include <filesystem>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

namespace Singularity {
namespace Storage {

// FileWatcher provides reactive file system sensing and live hot-reloading for Earthcall.
//
// In Earthcall's Sense-Act ontology:
//   - Senses: tracks filesystem modifications, creations, and deletions across directories.
//   - Emits: publishes past-tense noun-verbed ECA edge events:
//       * "file-modified"
//       * "file-created"
//       * "file-deleted"
//   - Enables: live hot-reloading of shaders, scripts, assets, and world rules when
//     edited externally by Persons in VS Code, TextEdit, Blender, etc.
//
// Refusal #1 & Refusal #6 compliant:
// Registered as a first-mover Law (@file-watcher.*).
class FileWatcher : public Law {
public:
    using ChangeCallback = std::function<void(const std::string& path, const std::string& eventType)>;

    FileWatcher();
    ~FileWatcher() override = default;

    bool isFirstMover() const override { return true; }
    std::string getIdentifier() const override { return "file-watcher"; }

    static void syncRegister(LawManager& laws);
    static FileWatcher* find(LawManager& laws);

    // Sensory tick: scans watched target at pollIntervalMs
    void tick();

    // Direct synchronous check
    void checkNow();

    // Hot-reloading listener registration
    void addCallback(ChangeCallback cb);
    void clearCallbacks();

    // Resets baseline snapshot of tracked files
    void rescanBaseline();

private:
    void buildProperties() override;

    struct FileEntry {
        std::filesystem::file_time_type lastWriteTime{};
        uintmax_t size = 0;
    };

    void scanDirectory(const std::filesystem::path& dir,
                       std::unordered_map<std::string, FileEntry>& outFiles);

    // Property getters/setters
    bool propEnabled() const { return _enabled; }
    void propSetEnabled(const bool& v) { _enabled = v; }

    std::string propWatchPath() const { return _watchPath; }
    void propSetWatchPath(const std::string& v);

    bool propRecursive() const { return _recursive; }
    void propSetRecursive(const bool& v) { _recursive = v; }

    std::string propFilterExtension() const { return _filterExtension; }
    void propSetFilterExtension(const std::string& v) { _filterExtension = v; }

    double propPollIntervalMs() const { return _pollIntervalMs; }
    void propSetPollIntervalMs(const double& v) { _pollIntervalMs = v > 10.0 ? v : 10.0; }

    std::string propLastModifiedFile() const { return _lastModifiedFile; }
    std::string propLastEventType() const { return _lastEventType; }
    double propLastEventTimestamp() const { return _lastEventTimestamp; }
    double propTotalEventsPublished() const { return _totalEventsPublished; }
    double propFilesTracked() const;

    bool propCheckNowTrigger() const { return _checkNowTrigger; }
    void propSetCheckNowTrigger(const bool& v);

    std::string propStatus() const { return _status; }
    std::string propLastError() const { return _lastError; }

    bool _enabled = true;
    std::string _watchPath = "saves";
    bool _recursive = true;
    std::string _filterExtension; // empty = match all
    double _pollIntervalMs = 250.0;

    std::string _lastModifiedFile;
    std::string _lastEventType = "none";
    double _lastEventTimestamp = 0.0;
    double _totalEventsPublished = 0.0;
    bool _checkNowTrigger = false;

    std::string _status = "idle";
    std::string _lastError;

    std::chrono::steady_clock::time_point _lastPollTime{};
    mutable std::mutex _mutex;
    std::unordered_map<std::string, FileEntry> _trackedFiles;
    std::vector<ChangeCallback> _callbacks;
};

} // namespace Storage
} // namespace Singularity
