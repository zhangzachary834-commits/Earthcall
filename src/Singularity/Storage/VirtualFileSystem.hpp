#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <string>
#include <map>
#include <vector>
#include <mutex>

namespace Singularity {
namespace Storage {

// VirtualFileSystem (VFS) provides uniform, portable URI addressing for Earthcall
// assets, zones, homes, recordings, and in-memory ephemeral files.
//
// Supported URI Schemes:
//   - save://worlds/my_world.json       -> <saveRoot>/worlds/my_world.json
//   - zone://<zone-id>/zone.json        -> <saveRoot>/zones/<sanitized-id>/zone.json
//   - home://<person-id>/home.json      -> <saveRoot>/homes/<sanitized-id>/home.json
//   - recording://session.ppm           -> <saveRoot>/recordings/session.ppm
//   - memory://<id>                     -> in-RAM virtual file (zero disk I/O)
//   - file://<path>                     -> canonical local path
//   - custom mounts (e.g. assets://)    -> mounted physical directory
//
// Refusal #1 & Refusal #6 compliant:
// Registered as a first-mover Law (@vfs.*).
class VirtualFileSystem : public Law {
public:
    VirtualFileSystem();
    ~VirtualFileSystem() override = default;

    bool isFirstMover() const override { return true; }
    std::string getIdentifier() const override { return "vfs"; }

    static void syncRegister(LawManager& laws);
    static VirtualFileSystem* find(LawManager& laws);
    static VirtualFileSystem& instance();

    // URI Resolution
    static bool isVirtual(const std::string& uri);
    static bool isMemory(const std::string& uri);
    static std::string resolve(const std::string& uri);

    // Mount Table
    void mount(const std::string& prefix, const std::string& targetPath);
    void unmount(const std::string& prefix);
    std::string getMountTableString() const;

    // In-Memory Virtual Files
    bool writeMemoryFile(const std::string& uri, const std::string& content);
    bool readMemoryFile(const std::string& uri, std::string& outContent) const;
    bool deleteMemoryFile(const std::string& uri);
    bool memoryFileExists(const std::string& uri) const;
    size_t memoryFileSize(const std::string& uri) const;
    std::vector<std::string> listMemoryFiles() const;
    void clearMemoryFiles();

private:
    void buildProperties() override;

    // Property getters/setters
    bool propEnabled() const { return _enabled; }
    void propSetEnabled(const bool& v) { _enabled = v; }

    std::string propQueryUri() const { return _queryUri; }
    void propSetQueryUri(const std::string& v) { _queryUri = v; }

    std::string propResolvedPath() const;
    bool propIsMemory() const;
    bool propExists() const;

    std::string propMountPrefix() const { return _mountPrefix; }
    void propSetMountPrefix(const std::string& v) { _mountPrefix = v; }

    std::string propMountTarget() const { return _mountTarget; }
    void propSetMountTarget(const std::string& v) { _mountTarget = v; }

    bool propMountTrigger() const { return _mountTrigger; }
    void propSetMountTrigger(const bool& v);

    bool propUnmountTrigger() const { return _unmountTrigger; }
    void propSetUnmountTrigger(const bool& v);

    std::string propMountTable() const { return getMountTableString(); }
    double propMemoryFilesCount() const;
    double propMemoryBytesAllocated() const;

    bool _enabled = true;
    std::string _queryUri;
    std::string _mountPrefix;
    std::string _mountTarget;
    bool _mountTrigger = false;
    bool _unmountTrigger = false;

    static std::mutex s_mutex;
    static std::map<std::string, std::string> s_mounts;
    static std::map<std::string, std::string> s_memoryFiles;
};

} // namespace Storage
} // namespace Singularity
