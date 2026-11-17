#include "Singularity/Storage/FileWatcher.hpp"
#include "Singularity/Storage/VirtualFileSystem.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

#include <chrono>
#include <system_error>

namespace Singularity {
namespace Storage {

namespace fs = std::filesystem;

FileWatcher::FileWatcher() : Law("file-watcher") {
    setName("File Watcher");
    _enabled = true;
    _watchPath = "saves";
    _recursive = true;
    _pollIntervalMs = 250.0;
    _status = "idle";
    _lastPollTime = std::chrono::steady_clock::now();
    _scanWorker = std::thread(&FileWatcher::workerLoop, this);
}

FileWatcher::~FileWatcher() {
    { std::lock_guard<std::mutex> lock(_scanMutex); _shutdown = true; }
    _scanCv.notify_one();
    if (_scanWorker.joinable()) _scanWorker.join();
}

FileWatcher::ScanConfig FileWatcher::scanConfig() const {
    return {VirtualFileSystem::resolve(_watchPath), _recursive, _filterExtension, _configGeneration};
}

FileWatcher::ScanResult FileWatcher::scanSnapshot(const ScanConfig& config) {
    ScanResult result{config.generation, {}, {}};
    std::error_code ec;
    if (config.target.empty() || !fs::exists(config.target, ec)) {
        result.error = "Watch path does not exist: " + config.target.string();
        return result;
    }
    try {
        scanDirectory(config.target, config.recursive, config.filter, result.files);
    } catch (const std::exception& e) {
        result.files.clear();
        result.error = std::string("Watch scan failed: ") + e.what();
    }
    return result;
}

void FileWatcher::workerLoop() {
    for (;;) {
        std::unique_lock<std::mutex> lock(_scanMutex);
        _scanCv.wait(lock, [this] { return _shutdown || _pendingScan.has_value(); });
        if (_shutdown) return;
        auto config = std::move(*_pendingScan);
        _pendingScan.reset();
        lock.unlock();
        auto result = scanSnapshot(config);
        lock.lock();
        if (_shutdown) return;
        _completedScan = std::move(result);
        _scanBusy = false;
    }
}

void FileWatcher::syncRegister(LawManager& laws) {
    if (laws.find("file-watcher")) return;

    auto watcher = std::make_shared<FileWatcher>();
    laws.add(watcher);
}

FileWatcher* FileWatcher::find(LawManager& laws) {
    return dynamic_cast<FileWatcher*>(laws.find("file-watcher"));
}

void FileWatcher::propSetWatchPath(const std::string& v) {
    _watchPath = v;
    rescanBaseline();
}

void FileWatcher::rescanBaseline() {
    ++_configGeneration;
    auto result = scanSnapshot(scanConfig());
    applySnapshot(std::move(result), true);
}

void FileWatcher::scanDirectory(const fs::path& target, bool recursive,
                                const std::string& filterExtension,
                                std::unordered_map<std::string, FileEntry>& outFiles) {
    std::error_code ec;
    if (fs::is_regular_file(target, ec)) {
        if (filterExtension.empty() || target.extension() == filterExtension) {
            auto lwt = fs::last_write_time(target, ec);
            auto sz = fs::file_size(target, ec);
            if (!ec) {
                outFiles[target.lexically_normal().string()] = {lwt, sz};
            }
        }
        return;
    }

    if (!fs::is_directory(target, ec)) return;

    if (recursive) {
        for (const auto& entry : fs::recursive_directory_iterator(target, ec)) {
            if (entry.is_regular_file(ec)) {
                if (filterExtension.empty() || entry.path().extension() == filterExtension) {
                    auto lwt = entry.last_write_time(ec);
                    auto sz = entry.file_size(ec);
                    if (!ec) {
                        outFiles[entry.path().lexically_normal().string()] = {lwt, sz};
                    }
                }
            }
        }
    } else {
        for (const auto& entry : fs::directory_iterator(target, ec)) {
            if (entry.is_regular_file(ec)) {
                if (filterExtension.empty() || entry.path().extension() == filterExtension) {
                    auto lwt = entry.last_write_time(ec);
                    auto sz = entry.file_size(ec);
                    if (!ec) {
                        outFiles[entry.path().lexically_normal().string()] = {lwt, sz};
                    }
                }
            }
        }
    }
}

void FileWatcher::tick() {
    if (!_enabled) return;

    std::optional<ScanResult> completed;
    {
        std::lock_guard<std::mutex> lock(_scanMutex);
        completed = std::move(_completedScan);
        _completedScan.reset();
    }
    if (completed && completed->generation == _configGeneration)
        applySnapshot(std::move(*completed), !_baselineInitialized);

    auto now = std::chrono::steady_clock::now();
    double elapsedMs = std::chrono::duration<double, std::milli>(now - _lastPollTime).count();
    if (elapsedMs < _pollIntervalMs) return;

    _lastPollTime = now;
    {
        std::lock_guard<std::mutex> lock(_scanMutex);
        if (!_scanBusy) {
            _scanBusy = true;
            _pendingScan = scanConfig();
            _scanCv.notify_one();
        }
    }
}

void FileWatcher::checkNow() {
    if (!_enabled) return;
    ++_configGeneration;
    applySnapshot(scanSnapshot(scanConfig()), false);
}

void FileWatcher::applySnapshot(ScanResult result, bool baseline) {
    if (!result.error.empty()) {
        _status = "error: target does not exist";
        _lastError = std::move(result.error);
        if (baseline) {
            std::lock_guard<std::mutex> lock(_mutex);
            _trackedFiles.clear();
            _baselineInitialized = false;
        }
        return;
    }
    auto& currentFiles = result.files;
    if (baseline) {
        std::lock_guard<std::mutex> lock(_mutex);
        _trackedFiles = std::move(currentFiles);
        _baselineInitialized = true;
        _status = "watching";
        _lastError.clear();
        return;
    }

    std::vector<std::pair<std::string, std::string>> detectedEvents; // {path, eventType}

    {
        std::lock_guard<std::mutex> lock(_mutex);

        // 1. Detect creations and modifications
        for (const auto& [path, entry] : currentFiles) {
            auto it = _trackedFiles.find(path);
            if (it == _trackedFiles.end()) {
                // Brand new file
                detectedEvents.emplace_back(path, "file-created");
            } else if (it->second.lastWriteTime != entry.lastWriteTime ||
                       it->second.size != entry.size) {
                // File modified
                detectedEvents.emplace_back(path, "file-modified");
            }
        }

        // 2. Detect deletions
        for (const auto& [path, entry] : _trackedFiles) {
            (void)entry;
            if (currentFiles.find(path) == currentFiles.end()) {
                detectedEvents.emplace_back(path, "file-deleted");
            }
        }

        _trackedFiles = std::move(currentFiles);
        _baselineInitialized = true;
        _status = "watching";
        _lastError = "";
    }

    // 3. Publish edge transitions
    for (const auto& [path, eventType] : detectedEvents) {
        _lastModifiedFile = path;
        _lastEventType = eventType;
        _lastEventTimestamp = std::chrono::duration<double>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        _totalEventsPublished += 1.0;

        // Publish to Core::EventBus as an ECA::Event
        Core::EventBus::instance().publish(ECA::Event{
            eventType, this, nullptr, Moment{}
        });

        // Fire any C++ hot-reloading callbacks
        std::vector<ChangeCallback> callbacksCopy;
        {
            std::lock_guard<std::mutex> lock(_mutex);
            callbacksCopy = _callbacks;
        }
        for (const auto& cb : callbacksCopy) {
            if (cb) cb(path, eventType);
        }
    }
}

void FileWatcher::addCallback(ChangeCallback cb) {
    if (!cb) return;
    std::lock_guard<std::mutex> lock(_mutex);
    _callbacks.push_back(std::move(cb));
}

void FileWatcher::clearCallbacks() {
    std::lock_guard<std::mutex> lock(_mutex);
    _callbacks.clear();
}

double FileWatcher::propFilesTracked() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return static_cast<double>(_trackedFiles.size());
}

void FileWatcher::propSetCheckNowTrigger(const bool& v) {
    _checkNowTrigger = v;
    if (_checkNowTrigger) {
        checkNow();
        _checkNowTrigger = false;
    }
}
void FileWatcher::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<FileWatcher, bool>>(
        "watcher.enabled", this, &FileWatcher::propEnabled, &FileWatcher::propSetEnabled));
    registerProperty(std::make_unique<ComputedProperty<FileWatcher, std::string>>(
        "watcher.watchPath", this, &FileWatcher::propWatchPath, &FileWatcher::propSetWatchPath));
    registerProperty(std::make_unique<ComputedProperty<FileWatcher, bool>>(
        "watcher.recursive", this, &FileWatcher::propRecursive, &FileWatcher::propSetRecursive));
    registerProperty(std::make_unique<ComputedProperty<FileWatcher, std::string>>(
        "watcher.filterExtension", this, &FileWatcher::propFilterExtension, &FileWatcher::propSetFilterExtension));
    registerProperty(std::make_unique<ComputedProperty<FileWatcher, double>>(
        "watcher.pollIntervalMs", this, &FileWatcher::propPollIntervalMs, &FileWatcher::propSetPollIntervalMs));

    registerProperty(std::make_unique<ComputedProperty<FileWatcher, std::string>>(
        "watcher.lastModifiedFile", this, &FileWatcher::propLastModifiedFile, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileWatcher, std::string>>(
        "watcher.lastEventType", this, &FileWatcher::propLastEventType, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileWatcher, double>>(
        "watcher.lastEventTimestamp", this, &FileWatcher::propLastEventTimestamp, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileWatcher, double>>(
        "watcher.totalEventsPublished", this, &FileWatcher::propTotalEventsPublished, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileWatcher, double>>(
        "watcher.filesTracked", this, &FileWatcher::propFilesTracked, nullptr));

    registerProperty(std::make_unique<ComputedProperty<FileWatcher, bool>>(
        "watcher.checkNow", this, &FileWatcher::propCheckNowTrigger, &FileWatcher::propSetCheckNowTrigger));

    registerProperty(std::make_unique<ComputedProperty<FileWatcher, std::string>>(
        "watcher.status", this, &FileWatcher::propStatus, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileWatcher, std::string>>(
        "watcher.lastError", this, &FileWatcher::propLastError, nullptr));
}

} // namespace Storage
} // namespace Singularity
