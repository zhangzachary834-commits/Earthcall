#include "Singularity/Storage/FileChannel.hpp"

#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace Singularity {
namespace Storage {

namespace fs = std::filesystem;

FileChannel::FileChannel() : Law("file-channel") {
    setName("File Channel");
    _enabled = true;
    _sandboxMode = true;
}

void FileChannel::syncRegister(LawManager& laws) {
    if (laws.find("file-channel")) return;

    auto channel = std::make_shared<FileChannel>();
    laws.add(channel);
}

FileChannel* FileChannel::find(LawManager& laws) {
    return dynamic_cast<FileChannel*>(laws.find("file-channel"));
}

bool FileChannel::isPathAllowed(const std::string& targetPath, bool isWrite) const {
    if (targetPath.empty()) return false;

    try {
        fs::path p(targetPath);
        // Normalize relative path hops
        fs::path norm = p.lexically_normal();

        if (_sandboxMode) {
            // Prevent escaping current directory / saves via parent directory hops
            std::string str = norm.string();
            if (str.find("..") != std::string::npos) {
                // If it starts with ../ or contains /../, deny in sandbox mode
                if (str.rfind("../", 0) == 0 || str.find("/../") != std::string::npos) {
                    return false;
                }
            }
        }
    } catch (...) {
        return false;
    }

    return checkOSPermissions(targetPath, isWrite);
}

bool FileChannel::checkOSPermissions(const std::string& targetPath, bool isWrite) const {
    if (targetPath.empty()) return false;

    try {
        fs::path p(targetPath);
        fs::path absPath = fs::absolute(p).lexically_normal();
        std::string absStr = absPath.string();

        // macOS and Unix system path protection rules
#if defined(__APPLE__) || defined(__linux__)
        if (isWrite) {
            // System-level protected locations
            static const std::vector<std::string> kSystemPaths = {
                "/System", "/usr/bin", "/usr/sbin", "/sbin", "/etc", "/var/root", "/private/var/root"
            };
            for (const auto& sysPath : kSystemPaths) {
                if (absStr.rfind(sysPath, 0) == 0) {
                    // Restricted system location write check
#if defined(__unix__) || defined(__APPLE__)
                    if (::getuid() != 0) {
                        return false; // Denied by OS permissions for non-root user
                    }
#else
                    return false;
#endif
                }
            }
        }
#endif

        if (isWrite) {
            // Check if target file or directory already exists
            std::error_code ec;
            if (fs::exists(absPath, ec)) {
#if defined(__unix__) || defined(__APPLE__)
                if (::access(absStr.c_str(), W_OK) != 0) {
                    return false;
                }
#endif
            } else {
                // File does not exist yet: check parent directory write access
                fs::path parent = absPath.parent_path();
                if (!parent.empty()) {
                    if (fs::exists(parent, ec)) {
#if defined(__unix__) || defined(__APPLE__)
                        if (::access(parent.string().c_str(), W_OK) != 0) {
                            return false;
                        }
#endif
                    }
                }
            }
        } else {
            // Read access check
            std::error_code ec;
            if (!fs::exists(absPath, ec)) {
                return false;
            }
            if (fs::is_directory(absPath, ec)) {
                return false; // Cannot read raw directory as plain file content
            }
#if defined(__unix__) || defined(__APPLE__)
            if (::access(absStr.c_str(), R_OK) != 0) {
                return false;
            }
#endif
        }
    } catch (...) {
        return false;
    }

    return true;
}

bool FileChannel::executeRead() {
    if (!_enabled) {
        _status = "error: channel disabled";
        _lastError = "File channel is disabled";
        _lastOperationSuccess = false;
        return false;
    }

    if (_path.empty()) {
        _status = "error: empty path";
        _lastError = "No path specified for read operation";
        _lastOperationSuccess = false;
        return false;
    }

    if (!isPathAllowed(_path, false)) {
        _status = "error: permission denied";
        _lastError = "OS or Sandbox permission denied reading file: " + _path;
        _lastOperationSuccess = false;
        return false;
    }

    try {
        std::ifstream file(_path, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            _status = "error: file open failed";
            _lastError = "Failed to open file for reading: " + _path;
            _lastOperationSuccess = false;
            return false;
        }

        std::ostringstream ss;
        ss << file.rdbuf();
        _content = ss.str();
        _bytesRead = static_cast<double>(_content.size());
        _status = "read-success";
        _lastError = "";
        _lastOperationSuccess = true;
        return true;
    } catch (const std::exception& e) {
        _status = "error: exception during read";
        _lastError = e.what();
        _lastOperationSuccess = false;
        return false;
    }
}

bool FileChannel::executeWrite() {
    if (!_enabled) {
        _status = "error: channel disabled";
        _lastError = "File channel is disabled";
        _lastOperationSuccess = false;
        return false;
    }

    if (_path.empty()) {
        _status = "error: empty path";
        _lastError = "No path specified for write operation";
        _lastOperationSuccess = false;
        return false;
    }

    if (!isPathAllowed(_path, true)) {
        _status = "error: permission denied";
        _lastError = "OS or Sandbox permission denied writing file: " + _path;
        _lastOperationSuccess = false;
        return false;
    }

    std::error_code ec;
    fs::path absPath = fs::absolute(fs::path(_path)).lexically_normal();
    if (fs::exists(absPath, ec) && fs::is_directory(absPath, ec)) {
        _status = "error: path is directory";
        _lastError = "Cannot write file content onto a directory: " + _path;
        _lastOperationSuccess = false;
        return false;
    }

    try {
        fs::path p(_path);
        fs::path parent = p.parent_path();
        if (!parent.empty() && !fs::exists(parent)) {
            if (!fs::create_directories(parent, ec)) {
                if (ec) {
                    _status = "error: directory creation failed";
                    _lastError = "OS permission denied creating directories: " + ec.message();
                    _lastOperationSuccess = false;
                    return false;
                }
            }
        }

        std::ofstream file(_path, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            _status = "error: file open failed";
            _lastError = "Failed to open file for writing: " + _path;
            _lastOperationSuccess = false;
            return false;
        }

        file.write(_content.data(), _content.size());
        file.flush();
        if (file.fail()) {
            _status = "error: write failed";
            _lastError = "Disk write failed or disk full writing: " + _path;
            _lastOperationSuccess = false;
            return false;
        }

        _bytesWritten = static_cast<double>(_content.size());
        _status = "write-success";
        _lastError = "";
        _lastOperationSuccess = true;
        return true;
    } catch (const std::exception& e) {
        _status = "error: exception during write";
        _lastError = e.what();
        _lastOperationSuccess = false;
        return false;
    }
}

void FileChannel::propSetReadTrigger(const bool& v) {
    _readTrigger = v;
    if (_readTrigger) {
        executeRead();
        _readTrigger = false; // Reset trigger after execution
    }
}

void FileChannel::propSetWriteTrigger(const bool& v) {
    _writeTrigger = v;
    if (_writeTrigger) {
        executeWrite();
        _writeTrigger = false; // Reset trigger after execution
    }
}

bool FileChannel::propExists() const {
    if (_path.empty()) return false;
    std::error_code ec;
    return fs::exists(_path, ec);
}

bool FileChannel::propIsDirectory() const {
    if (_path.empty()) return false;
    std::error_code ec;
    return fs::is_directory(_path, ec);
}

bool FileChannel::propIsWritable() const {
    if (_path.empty()) return false;
    return checkOSPermissions(_path, true);
}

void FileChannel::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.enabled", this, &FileChannel::propEnabled, &FileChannel::propSetEnabled));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.sandboxMode", this, &FileChannel::propSandboxMode, &FileChannel::propSetSandboxMode));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.path", this, &FileChannel::propPath, &FileChannel::propSetPath));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.content", this, &FileChannel::propContent, &FileChannel::propSetContent));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.read", this, &FileChannel::propReadTrigger, &FileChannel::propSetReadTrigger));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.write", this, &FileChannel::propWriteTrigger, &FileChannel::propSetWriteTrigger));

    registerProperty(std::make_unique<ComputedProperty<FileChannel, double>>(
        "file.bytesRead", this, &FileChannel::propBytesRead, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, double>>(
        "file.bytesWritten", this, &FileChannel::propBytesWritten, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.status", this, &FileChannel::propStatus, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.lastError", this, &FileChannel::propLastError, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.lastOperationSuccess", this, &FileChannel::propLastOperationSuccess, nullptr));

    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.exists", this, &FileChannel::propExists, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.isDirectory", this, &FileChannel::propIsDirectory, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.isWritable", this, &FileChannel::propIsWritable, nullptr));
}

} // namespace Storage
} // namespace Singularity
