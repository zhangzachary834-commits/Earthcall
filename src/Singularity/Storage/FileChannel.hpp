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
//
// Refusal 6 compliance:
// All properties are registered under buildProperties().
// Kernel-level exemptions: internal atomic temp counter is a process-local sequence.
class FileChannel : public Law {
public:
    FileChannel();

    bool isFirstMover() const override { return true; }

    // A STABLE identifier so law text addresses this channel consistently
    // (`@file-channel.read := true`).
    std::string getIdentifier() const override { return "file-channel"; }

    static void syncRegister(LawManager& laws);
    static FileChannel* find(LawManager& laws);

    // Core I/O operations
    bool executeRead();
    bool executeWrite();
    bool executeAppend();
    bool executeDelete();
    bool executeCreateDir();
    bool executeListDir();
    bool executeCopy();
    bool executeMove();

    // OS permission and location validation
    bool checkOSPermissions(const std::string& targetPath, bool isWrite) const;
    bool isPathAllowed(const std::string& targetPath, bool isWrite) const;

    // Static utilities for encoding, hashes, and MIME detection
    static std::string base64Encode(const std::string& input);
    static std::string base64Decode(const std::string& input);
    static std::string hexEncode(const std::string& input);
    static std::string hexDecode(const std::string& input);
    static std::string computeSha256(const std::string& data);
    static std::string detectMimeType(const std::string& path, const std::string& content);
    static std::string categorizeFileType(const std::string& mimeType, const std::string& path);

private:
    void buildProperties() override;

    // Property getters/setters - Core state
    bool propEnabled() const { return _enabled; }
    void propSetEnabled(const bool& v) { _enabled = v; }

    bool propSandboxMode() const { return _sandboxMode; }
    void propSetSandboxMode(const bool& v) { _sandboxMode = v; }

    std::string propPath() const { return _path; }
    void propSetPath(const std::string& v) { _path = v; }

    std::string propContent() const { return _content; }
    void propSetContent(const std::string& v) { _content = v; }

    // Triggers
    bool propReadTrigger() const { return _readTrigger; }
    void propSetReadTrigger(const bool& v);

    bool propWriteTrigger() const { return _writeTrigger; }
    void propSetWriteTrigger(const bool& v);

    bool propAppendTrigger() const { return _appendTrigger; }
    void propSetAppendTrigger(const bool& v);

    bool propDeleteTrigger() const { return _deleteTrigger; }
    void propSetDeleteTrigger(const bool& v);

    bool propCreateDirTrigger() const { return _createDirTrigger; }
    void propSetCreateDirTrigger(const bool& v);

    bool propListDirTrigger() const { return _listDirTrigger; }
    void propSetListDirTrigger(const bool& v);

    bool propCopyTrigger() const { return _copyTrigger; }
    void propSetCopyTrigger(const bool& v);

    bool propMoveTrigger() const { return _moveTrigger; }
    void propSetMoveTrigger(const bool& v);

    // Mode and configuration
    std::string propWriteMode() const { return _writeMode; }
    void propSetWriteMode(const std::string& v) { _writeMode = v; }

    bool propAtomicWrite() const { return _atomicWrite; }
    void propSetAtomicWrite(const bool& v) { _atomicWrite = v; }

    std::string propEncoding() const { return _encoding; }
    void propSetEncoding(const std::string& v) { _encoding = v; }

    bool propStripBom() const { return _stripBom; }
    void propSetStripBom(const bool& v) { _stripBom = v; }

    bool propNormalizeNewlines() const { return _normalizeNewlines; }
    void propSetNormalizeNewlines(const bool& v) { _normalizeNewlines = v; }

    double propMaxFileSize() const { return _maxFileSize; }
    void propSetMaxFileSize(const double& v) { _maxFileSize = v; }

    std::string propCopyTo() const { return _copyTo; }
    void propSetCopyTo(const std::string& v) { _copyTo = v; }

    std::string propMoveTo() const { return _moveTo; }
    void propSetMoveTo(const std::string& v) { _moveTo = v; }

    // Results and telemetry
    double propBytesRead() const { return _bytesRead; }
    double propBytesWritten() const { return _bytesWritten; }

    std::string propStatus() const { return _status; }
    std::string propLastError() const { return _lastError; }
    std::string propErrorCode() const { return _errorCode; }
    bool propLastOperationSuccess() const { return _lastOperationSuccess; }
    std::string propDirectoryEntries() const { return _directoryEntries; }

    // Computed / format properties
    std::string propContentBase64() const;
    void propSetContentBase64(const std::string& v);

    std::string propContentHex() const;
    void propSetContentHex(const std::string& v);

    bool propExists() const;
    bool propIsDirectory() const;
    bool propIsRegularFile() const;
    bool propIsSymlink() const;
    bool propIsWritable() const;
    bool propIsBinary() const;

    double propSize() const;
    double propLineCount() const;
    std::string propLastModified() const;
    std::string propSha256() const;

    // File type and metadata
    std::string propMimeType() const;
    std::string propFileType() const;
    std::string propExtension() const;
    std::string propStem() const;
    std::string propFilename() const;
    std::string propDirectory() const;

    // JSON structured operations
    bool propJsonValid() const;
    std::string propJsonCompact() const;
    std::string propJsonPretty() const;

    // State members
    bool _enabled = true;
    bool _sandboxMode = true;
    std::string _path;
    std::string _content;
    bool _readTrigger = false;
    bool _writeTrigger = false;
    bool _appendTrigger = false;
    bool _deleteTrigger = false;
    bool _createDirTrigger = false;
    bool _listDirTrigger = false;
    bool _copyTrigger = false;
    bool _moveTrigger = false;

    std::string _writeMode = "overwrite";  // "overwrite" or "append"
    bool _atomicWrite = true;
    std::string _encoding = "auto";       // "auto", "text", "base64", "hex"
    bool _stripBom = true;
    bool _normalizeNewlines = false;
    double _maxFileSize = 67108864.0;      // 64 MB default ceiling

    std::string _copyTo;
    std::string _moveTo;
    std::string _directoryEntries;

    double _bytesRead = 0.0;
    double _bytesWritten = 0.0;
    std::string _status = "idle";
    std::string _lastError;
    std::string _errorCode = "none";
    bool _lastOperationSuccess = false;
};

} // namespace Storage
} // namespace Singularity
