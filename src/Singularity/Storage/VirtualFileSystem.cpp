#include "Singularity/Storage/VirtualFileSystem.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

#include <algorithm>
#include <filesystem>
#include <sstream>

namespace Singularity {
namespace Storage {

namespace fs = std::filesystem;

std::mutex VirtualFileSystem::s_mutex;
std::map<std::string, std::string> VirtualFileSystem::s_mounts;
std::map<std::string, std::string> VirtualFileSystem::s_memoryFiles;

VirtualFileSystem::VirtualFileSystem() : Law("vfs") {
    setName("Virtual File System");
    _enabled = true;
    _queryUri = "save://worlds/my_world.json";
}

VirtualFileSystem& VirtualFileSystem::instance() {
    static VirtualFileSystem s_vfs;
    return s_vfs;
}

void VirtualFileSystem::syncRegister(LawManager& laws) {
    if (laws.find("vfs")) return;

    auto vfs = std::make_shared<VirtualFileSystem>();
    laws.add(vfs);
}

VirtualFileSystem* VirtualFileSystem::find(LawManager& laws) {
    return dynamic_cast<VirtualFileSystem*>(laws.find("vfs"));
}

bool VirtualFileSystem::isVirtual(const std::string& uri) {
    if (uri.empty()) return false;
    if (uri.rfind("save://", 0) == 0) return true;
    if (uri.rfind("zone://", 0) == 0) return true;
    if (uri.rfind("home://", 0) == 0) return true;
    if (uri.rfind("recording://", 0) == 0) return true;
    if (uri.rfind("memory://", 0) == 0) return true;
    if (uri.rfind("file://", 0) == 0) return true;

    std::lock_guard<std::mutex> lock(s_mutex);
    for (const auto& kv : s_mounts) {
        if (uri.rfind(kv.first, 0) == 0) return true;
    }
    return false;
}

bool VirtualFileSystem::isMemory(const std::string& uri) {
    return uri.rfind("memory://", 0) == 0;
}

std::string VirtualFileSystem::resolve(const std::string& uri) {
    if (uri.empty()) return "";

    // 1. In-memory virtual RAM files
    if (uri.rfind("memory://", 0) == 0) {
        return uri;
    }

    // 2. Direct file:// URL
    if (uri.rfind("file://", 0) == 0) {
        return uri.substr(7);
    }

    // 3. save://
    if (uri.rfind("save://", 0) == 0) {
        std::string rel = uri.substr(7);
        std::string root = SaveSystem::saveRoot();
        if (root.empty()) root = "saves";
        return (fs::path(root) / rel).lexically_normal().string();
    }

    // 4. zone://<zone-id>/...
    if (uri.rfind("zone://", 0) == 0) {
        std::string rest = uri.substr(7);
        size_t slash = rest.find('/');
        std::string zoneId = (slash != std::string::npos) ? rest.substr(0, slash) : rest;
        std::string sub = (slash != std::string::npos) ? rest.substr(slash + 1) : "";
        std::string dir = SaveSystem::zoneDirectory(zoneId);
        return sub.empty() ? dir : (fs::path(dir) / sub).lexically_normal().string();
    }

    // 5. home://<person-id>/...
    if (uri.rfind("home://", 0) == 0) {
        std::string rest = uri.substr(7);
        size_t slash = rest.find('/');
        std::string homeId = (slash != std::string::npos) ? rest.substr(0, slash) : rest;
        std::string sub = (slash != std::string::npos) ? rest.substr(slash + 1) : "";
        std::string dir = SaveSystem::homeDirectory(homeId);
        return sub.empty() ? dir : (fs::path(dir) / sub).lexically_normal().string();
    }

    // 6. recording://...
    if (uri.rfind("recording://", 0) == 0) {
        std::string rel = uri.substr(12);
        std::string root = SaveSystem::saveRoot();
        if (root.empty()) root = "saves";
        return (fs::path(root) / "recordings" / rel).lexically_normal().string();
    }

    // 7. Custom mount points
    {
        std::lock_guard<std::mutex> lock(s_mutex);
        for (const auto& kv : s_mounts) {
            if (uri.rfind(kv.first, 0) == 0) {
                std::string sub = uri.substr(kv.first.size());
                return (fs::path(kv.second) / sub).lexically_normal().string();
            }
        }
    }

    // Default: return as-is
    return uri;
}

void VirtualFileSystem::mount(const std::string& prefix, const std::string& targetPath) {
    if (prefix.empty() || targetPath.empty()) return;
    std::lock_guard<std::mutex> lock(s_mutex);
    s_mounts[prefix] = targetPath;
}

void VirtualFileSystem::unmount(const std::string& prefix) {
    if (prefix.empty()) return;
    std::lock_guard<std::mutex> lock(s_mutex);
    s_mounts.erase(prefix);
}

std::string VirtualFileSystem::getMountTableString() const {
    std::lock_guard<std::mutex> lock(s_mutex);
    std::ostringstream ss;
    ss << "save:// -> " << (SaveSystem::saveRoot().empty() ? "saves" : SaveSystem::saveRoot()) << "\n";
    ss << "zone://<id>/ -> " << (SaveSystem::saveRoot().empty() ? "saves" : SaveSystem::saveRoot()) << "/zones/<id>/\n";
    ss << "home://<id>/ -> " << (SaveSystem::saveRoot().empty() ? "saves" : SaveSystem::saveRoot()) << "/homes/<id>/\n";
    ss << "recording:// -> " << (SaveSystem::saveRoot().empty() ? "saves" : SaveSystem::saveRoot()) << "/recordings/\n";
    ss << "memory://<id> -> In-RAM volatile storage (" << s_memoryFiles.size() << " files)\n";
    for (const auto& kv : s_mounts) {
        ss << kv.first << " -> " << kv.second << "\n";
    }
    return ss.str();
}

bool VirtualFileSystem::writeMemoryFile(const std::string& uri, const std::string& content) {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_memoryFiles[uri] = content;
    return true;
}

bool VirtualFileSystem::readMemoryFile(const std::string& uri, std::string& outContent) const {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_memoryFiles.find(uri);
    if (it == s_memoryFiles.end()) return false;
    outContent = it->second;
    return true;
}

bool VirtualFileSystem::deleteMemoryFile(const std::string& uri) {
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_memoryFiles.erase(uri) > 0;
}

bool VirtualFileSystem::memoryFileExists(const std::string& uri) const {
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_memoryFiles.find(uri) != s_memoryFiles.end();
}

size_t VirtualFileSystem::memoryFileSize(const std::string& uri) const {
    std::lock_guard<std::mutex> lock(s_mutex);
    auto it = s_memoryFiles.find(uri);
    if (it == s_memoryFiles.end()) return 0;
    return it->second.size();
}

std::vector<std::string> VirtualFileSystem::listMemoryFiles() const {
    std::lock_guard<std::mutex> lock(s_mutex);
    std::vector<std::string> list;
    list.reserve(s_memoryFiles.size());
    for (const auto& kv : s_memoryFiles) {
        list.push_back(kv.first);
    }
    return list;
}

void VirtualFileSystem::clearMemoryFiles() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_memoryFiles.clear();
}

std::string VirtualFileSystem::propResolvedPath() const {
    return resolve(_queryUri);
}

bool VirtualFileSystem::propIsMemory() const {
    return isMemory(_queryUri);
}

bool VirtualFileSystem::propExists() const {
    if (isMemory(_queryUri)) {
        return memoryFileExists(_queryUri);
    }
    std::string res = resolve(_queryUri);
    if (res.empty()) return false;
    std::error_code ec;
    return fs::exists(fs::path(res), ec);
}

void VirtualFileSystem::propSetMountTrigger(const bool& v) {
    _mountTrigger = v;
    if (_mountTrigger) {
        mount(_mountPrefix, _mountTarget);
        _mountTrigger = false;
    }
}

void VirtualFileSystem::propSetUnmountTrigger(const bool& v) {
    _unmountTrigger = v;
    if (_unmountTrigger) {
        unmount(_mountPrefix);
        _unmountTrigger = false;
    }
}

double VirtualFileSystem::propMemoryFilesCount() const {
    std::lock_guard<std::mutex> lock(s_mutex);
    return static_cast<double>(s_memoryFiles.size());
}

double VirtualFileSystem::propMemoryBytesAllocated() const {
    std::lock_guard<std::mutex> lock(s_mutex);
    size_t total = 0;
    for (const auto& kv : s_memoryFiles) {
        total += kv.second.size();
    }
    return static_cast<double>(total);
}

void VirtualFileSystem::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<VirtualFileSystem, bool>>(
        "vfs.enabled", this, &VirtualFileSystem::propEnabled, &VirtualFileSystem::propSetEnabled));
    registerProperty(std::make_unique<ComputedProperty<VirtualFileSystem, std::string>>(
        "vfs.queryUri", this, &VirtualFileSystem::propQueryUri, &VirtualFileSystem::propSetQueryUri));
    registerProperty(std::make_unique<ComputedProperty<VirtualFileSystem, std::string>>(
        "vfs.resolvedPath", this, &VirtualFileSystem::propResolvedPath, nullptr));
    registerProperty(std::make_unique<ComputedProperty<VirtualFileSystem, bool>>(
        "vfs.isMemory", this, &VirtualFileSystem::propIsMemory, nullptr));
    registerProperty(std::make_unique<ComputedProperty<VirtualFileSystem, bool>>(
        "vfs.exists", this, &VirtualFileSystem::propExists, nullptr));

    registerProperty(std::make_unique<ComputedProperty<VirtualFileSystem, std::string>>(
        "vfs.mountPrefix", this, &VirtualFileSystem::propMountPrefix, &VirtualFileSystem::propSetMountPrefix));
    registerProperty(std::make_unique<ComputedProperty<VirtualFileSystem, std::string>>(
        "vfs.mountTarget", this, &VirtualFileSystem::propMountTarget, &VirtualFileSystem::propSetMountTarget));
    registerProperty(std::make_unique<ComputedProperty<VirtualFileSystem, bool>>(
        "vfs.mount", this, &VirtualFileSystem::propMountTrigger, &VirtualFileSystem::propSetMountTrigger));
    registerProperty(std::make_unique<ComputedProperty<VirtualFileSystem, bool>>(
        "vfs.unmount", this, &VirtualFileSystem::propUnmountTrigger, &VirtualFileSystem::propSetUnmountTrigger));

    registerProperty(std::make_unique<ComputedProperty<VirtualFileSystem, std::string>>(
        "vfs.mountTable", this, &VirtualFileSystem::propMountTable, nullptr));
    registerProperty(std::make_unique<ComputedProperty<VirtualFileSystem, double>>(
        "vfs.memoryFilesCount", this, &VirtualFileSystem::propMemoryFilesCount, nullptr));
    registerProperty(std::make_unique<ComputedProperty<VirtualFileSystem, double>>(
        "vfs.memoryBytesAllocated", this, &VirtualFileSystem::propMemoryBytesAllocated, nullptr));
}

} // namespace Storage
} // namespace Singularity
