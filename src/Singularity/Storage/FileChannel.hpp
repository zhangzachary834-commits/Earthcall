#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <string>

namespace Singularity {
namespace Storage {

// FileChannel implements sense and act for native computer file system I/O.
// It is a first mover Law under Singularity/Storage whose properties are
// governed by ordinary K4 Laws (@file-channel.path, @file-channel.read, ...).
//
// It abides strictly by OS permissions (macOS system write restrictions,
// POSIX file mode bits, directory creation requirements) and TransferPolicy.
class FileChannel : public Law {
public:
    FileChannel();

    bool isFirstMover() const override { return true; }

    // A STABLE identifier so law text addresses this channel consistently
    // (`@file-channel.read := true`).
    std::string getIdentifier() const override { return "file-channel"; }

    static void syncRegister(LawManager& laws);
    static FileChannel* find(LawManager& laws);

    // Perform file operations directly
    bool executeRead();
    bool executeWrite();

    // OS permission and location validation
    bool checkOSPermissions(const std::string& targetPath, bool isWrite) const;
    bool isPathAllowed(const std::string& targetPath, bool isWrite) const;

private:
    void buildProperties() override;

    // Property getters/setters
    bool propEnabled() const { return _enabled; }
    void propSetEnabled(const bool& v) { _enabled = v; }

    bool propSandboxMode() const { return _sandboxMode; }
    void propSetSandboxMode(const bool& v) { _sandboxMode = v; }

    std::string propPath() const { return _path; }
    void propSetPath(const std::string& v) { _path = v; }

    std::string propContent() const { return _content; }
    void propSetContent(const std::string& v) { _content = v; }

    bool propReadTrigger() const { return _readTrigger; }
    void propSetReadTrigger(const bool& v);

    bool propWriteTrigger() const { return _writeTrigger; }
    void propSetWriteTrigger(const bool& v);

    double propBytesRead() const { return _bytesRead; }
    double propBytesWritten() const { return _bytesWritten; }

    std::string propStatus() const { return _status; }
    std::string propLastError() const { return _lastError; }
    bool propLastOperationSuccess() const { return _lastOperationSuccess; }

    // Computed properties
    bool propExists() const;
    bool propIsDirectory() const;
    bool propIsWritable() const;

    bool _enabled = true;
    bool _sandboxMode = true;
    std::string _path;
    std::string _content;
    bool _readTrigger = false;
    bool _writeTrigger = false;
    double _bytesRead = 0.0;
    double _bytesWritten = 0.0;
    std::string _status = "idle";
    std::string _lastError;
    bool _lastOperationSuccess = false;
};

} // namespace Storage
} // namespace Singularity
