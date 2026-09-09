#include "Singularity/Storage/FileChannel.hpp"
#include "Singularity/Storage/VirtualFileSystem.hpp"

#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "json.hpp"

#include <openssl/sha.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

#if defined(__unix__) || defined(__APPLE__)
#include <unistd.h>
#include <sys/stat.h>
#endif

namespace Singularity {
namespace Storage {

namespace fs = std::filesystem;

// Kernel-level atomic write counter for generating unique temporary files
static std::atomic<uint64_t> s_atomicTempCounter{1};

static const char kBase64Chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string FileChannel::base64Encode(const std::string& input) {
    std::string out;
    int val = 0;
    int valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(kBase64Chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) {
        out.push_back(kBase64Chars[((val << 8) >> (valb + 8)) & 0x3F]);
    }
    while (out.size() % 4 != 0) {
        out.push_back('=');
    }
    return out;
}

std::string FileChannel::base64Decode(const std::string& input) {
    std::string out;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) {
        T[static_cast<unsigned char>(kBase64Chars[i])] = i;
    }

    int val = 0;
    int valb = -8;
    for (unsigned char c : input) {
        if (std::isspace(c)) continue;
        if (c == '=') break;
        if (T[c] == -1) continue;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

std::string FileChannel::hexEncode(const std::string& input) {
    static const char hexDigits[] = "0123456789abcdef";
    std::string out;
    out.reserve(input.size() * 2);
    for (unsigned char c : input) {
        out.push_back(hexDigits[(c >> 4) & 0x0F]);
        out.push_back(hexDigits[c & 0x0F]);
    }
    return out;
}

std::string FileChannel::hexDecode(const std::string& input) {
    std::string out;
    auto hexVal = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    int hi = -1;
    for (char c : input) {
        if (std::isspace(static_cast<unsigned char>(c))) continue;
        int v = hexVal(c);
        if (v == -1) continue;
        if (hi == -1) {
            hi = v;
        } else {
            out.push_back(static_cast<char>((hi << 4) | v));
            hi = -1;
        }
    }
    return out;
}

std::string FileChannel::computeSha256(const std::string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), hash);
    static const char hexDigits[] = "0123456789abcdef";
    std::string out;
    out.reserve(SHA256_DIGEST_LENGTH * 2);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        out.push_back(hexDigits[(hash[i] >> 4) & 0x0F]);
        out.push_back(hexDigits[hash[i] & 0x0F]);
    }
    return out;
}

std::string FileChannel::detectMimeType(const std::string& path, const std::string& content) {
    // 1. Sniff magic bytes if content is available
    if (!content.empty()) {
        const size_t len = content.size();
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(content.data());

        // PNG: 89 50 4E 47 0D 0A 1A 0A
        if (len >= 8 && std::memcmp(bytes, "\x89PNG\r\n\x1a\n", 8) == 0) {
            return "image/png";
        }
        // JPEG: FF D8 FF
        if (len >= 3 && bytes[0] == 0xFF && bytes[1] == 0xD8 && bytes[2] == 0xFF) {
            return "image/jpeg";
        }
        // GIF: GIF87a or GIF89a
        if (len >= 6 && (content.rfind("GIF87a", 0) == 0 || content.rfind("GIF89a", 0) == 0)) {
            return "image/gif";
        }
        // WebP: RIFF....WEBP
        if (len >= 12 && content.rfind("RIFF", 0) == 0 && content.compare(8, 4, "WEBP") == 0) {
            return "image/webp";
        }
        // WAV: RIFF....WAVE
        if (len >= 12 && content.rfind("RIFF", 0) == 0 && content.compare(8, 4, "WAVE") == 0) {
            return "audio/wav";
        }
        // OGG: OggS
        if (len >= 4 && content.rfind("OggS", 0) == 0) {
            return "audio/ogg";
        }
        // FLAC: fLaC
        if (len >= 4 && content.rfind("fLaC", 0) == 0) {
            return "audio/flac";
        }
        // MP3: ID3 or frame sync FF FB / FF F3
        if ((len >= 3 && content.rfind("ID3", 0) == 0) ||
            (len >= 2 && bytes[0] == 0xFF && (bytes[1] & 0xFE) == 0xFA)) {
            return "audio/mpeg";
        }
        // BMP: BM
        if (len >= 2 && content.rfind("BM", 0) == 0) {
            return "image/bmp";
        }
        // PDF: %PDF-
        if (len >= 5 && content.rfind("%PDF-", 0) == 0) {
            return "application/pdf";
        }
        // ZIP: PK\x03\x04
        if (len >= 4 && bytes[0] == 0x50 && bytes[1] == 0x4B && bytes[2] == 0x03 && bytes[3] == 0x04) {
            return "application/zip";
        }
        // GZIP: \x1f\x8b
        if (len >= 2 && bytes[0] == 0x1F && bytes[1] == 0x8B) {
            return "application/gzip";
        }
        // WebAssembly: \0asm
        if (len >= 4 && bytes[0] == 0x00 && bytes[1] == 'a' && bytes[2] == 's' && bytes[3] == 'm') {
            return "application/wasm";
        }
        // JSON syntax check
        size_t firstNonWs = content.find_first_not_of(" \t\r\n");
        if (firstNonWs != std::string::npos && (content[firstNonWs] == '{' || content[firstNonWs] == '[')) {
            size_t lastNonWs = content.find_last_not_of(" \t\r\n");
            if (lastNonWs != std::string::npos && (content[lastNonWs] == '}' || content[lastNonWs] == ']')) {
                if (nlohmann::json::accept(content)) {
                    return "application/json";
                }
            }
        }
    }

    // 2. Extension matching
    std::string ext;
    if (!path.empty()) {
        try {
            ext = fs::path(path).extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
                return std::tolower(c);
            });
        } catch (...) {
            ext = "";
        }
    }

    if (ext == ".json" || ext == ".ecform") return "application/json";
    if (ext == ".ecmatter") return "application/x-flatbuffers";
    if (ext == ".ecsave") return "application/x-msgpack";
    if (ext == ".txt" || ext == ".log" || ext == ".ini" || ext == ".cfg" || ext == ".conf") return "text/plain";
    if (ext == ".md" || ext == ".markdown") return "text/markdown";
    if (ext == ".csv") return "text/csv";
    if (ext == ".tsv") return "text/tab-separated-values";
    if (ext == ".xml") return "application/xml";
    if (ext == ".html" || ext == ".htm") return "text/html";
    if (ext == ".yaml" || ext == ".yml") return "text/yaml";
    if (ext == ".toml") return "text/toml";
    if (ext == ".png") return "image/png";
    if (ext == ".jpg" || ext == ".jpeg") return "image/jpeg";
    if (ext == ".gif") return "image/gif";
    if (ext == ".webp") return "image/webp";
    if (ext == ".bmp") return "image/bmp";
    if (ext == ".svg") return "image/svg+xml";
    if (ext == ".tga") return "image/x-tga";
    if (ext == ".tif" || ext == ".tiff") return "image/tiff";
    if (ext == ".wav") return "audio/wav";
    if (ext == ".mp3") return "audio/mpeg";
    if (ext == ".ogg") return "audio/ogg";
    if (ext == ".flac") return "audio/flac";
    if (ext == ".aiff" || ext == ".aif") return "audio/aiff";
    if (ext == ".mid" || ext == ".midi") return "audio/midi";
    if (ext == ".obj") return "model/obj";
    if (ext == ".stl") return "model/stl";
    if (ext == ".gltf") return "model/gltf+json";
    if (ext == ".glb") return "model/gltf-binary";
    if (ext == ".ply") return "model/ply";
    if (ext == ".pdf") return "application/pdf";
    if (ext == ".zip") return "application/zip";
    if (ext == ".tar") return "application/x-tar";
    if (ext == ".gz" || ext == ".gzip") return "application/gzip";
    if (ext == ".7z") return "application/x-7z-compressed";
    if (ext == ".wgsl") return "text/wgsl";
    if (ext == ".glsl" || ext == ".vert" || ext == ".frag") return "text/x-glsl";
    if (ext == ".cpp" || ext == ".hpp" || ext == ".c" || ext == ".h" || ext == ".cc") return "text/x-c++src";
    if (ext == ".py") return "text/x-python";
    if (ext == ".js") return "application/javascript";
    if (ext == ".wasm") return "application/wasm";
    if (ext == ".bin" || ext == ".dat") return "application/octet-stream";

    // 3. Binary vs printable text fallback
    if (content.find('\0') != std::string::npos) {
        return "application/octet-stream";
    }
    return "text/plain";
}

std::string FileChannel::categorizeFileType(const std::string& mimeType, const std::string& path) {
    if (mimeType.rfind("image/", 0) == 0) return "image";
    if (mimeType.rfind("audio/", 0) == 0) return "audio";
    if (mimeType.rfind("model/", 0) == 0) return "model";
    if (mimeType == "application/json") return "json";
    if (mimeType == "text/csv" || mimeType == "text/tab-separated-values") return "csv";
    if (mimeType == "application/pdf") return "document";
    if (mimeType == "application/zip" || mimeType == "application/gzip" ||
        mimeType == "application/x-tar" || mimeType == "application/x-7z-compressed") {
        return "archive";
    }
    if (mimeType == "text/wgsl" || mimeType == "text/x-glsl") return "shader";
    if (mimeType == "text/x-c++src" || mimeType == "text/x-python" || mimeType == "application/javascript") {
        return "code";
    }
    if (mimeType == "application/x-flatbuffers" || mimeType == "application/x-msgpack" ||
        mimeType == "application/wasm" || mimeType == "application/octet-stream") {
        return "binary";
    }

    std::string ext;
    try {
        ext = fs::path(path).extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
            return std::tolower(c);
        });
    } catch (...) {
        ext = "";
    }
    if (ext == ".obj" || ext == ".stl" || ext == ".gltf" || ext == ".glb" || ext == ".ply") return "model";
    if (ext == ".csv" || ext == ".tsv") return "csv";
    if (ext == ".json" || ext == ".ecform") return "json";

    if (mimeType.rfind("text/", 0) == 0) return "text";
    return "binary";
}

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
    if (targetPath.find('\0') != std::string::npos) return false;

    if (VirtualFileSystem::isMemory(targetPath)) return true;

    std::string sanitized = VirtualFileSystem::resolve(targetPath);
    // Tilde expansion outside sandbox mode
    if (!_sandboxMode && (sanitized.rfind("~/", 0) == 0 || sanitized == "~")) {
        const char* home = std::getenv("HOME");
        if (home) {
            sanitized = std::string(home) + sanitized.substr(1);
        }
    }

    try {
        fs::path p(sanitized);
        fs::path norm = p.lexically_normal();
        std::string str = norm.string();

        if (_sandboxMode) {
            // Prevent escaping current directory / saves via parent directory hops
            if (str.rfind("../", 0) == 0 || str == ".." || str.find("/../") != std::string::npos) {
                return false;
            }

            std::error_code ec;
            fs::path cwd = fs::weakly_canonical(fs::current_path(ec), ec);
            fs::path targetCanon = fs::weakly_canonical(norm, ec);
            std::string targetStr = targetCanon.string();
            std::string cwdStr = cwd.string();

            // Allow if path resides within current working directory tree
            if (targetStr.rfind(cwdStr, 0) != 0) {
                return false;
            }
        }
    } catch (...) {
        return false;
    }

    return checkOSPermissions(sanitized, isWrite);
}

bool FileChannel::checkOSPermissions(const std::string& targetPath, bool isWrite) const {
    if (targetPath.empty()) return false;
    if (VirtualFileSystem::isMemory(targetPath)) return true;

    try {
        std::string actualPath = VirtualFileSystem::resolve(targetPath);
        fs::path p(actualPath);
        fs::path absPath = fs::absolute(p).lexically_normal();
        std::string absStr = absPath.string();

        // macOS and Unix system path protection rules
#if defined(__APPLE__) || defined(__linux__)
        if (isWrite) {
            static const std::vector<std::string> kSystemPaths = {
                "/System", "/usr/bin", "/usr/sbin", "/sbin", "/etc", "/var/root", "/private/var/root"
            };
            for (const auto& sysPath : kSystemPaths) {
                if (absStr.rfind(sysPath, 0) == 0) {
#if defined(__unix__) || defined(__APPLE__)
                    if (::getuid() != 0) {
                        return false;
                    }
#else
                    return false;
#endif
                }
            }
        }
#endif

        if (isWrite) {
            std::error_code ec;
            if (fs::exists(absPath, ec)) {
#if defined(__unix__) || defined(__APPLE__)
                if (::access(absStr.c_str(), W_OK) != 0) {
                    return false;
                }
#endif
            } else {
                fs::path parent = absPath.parent_path();
                if (!parent.empty() && fs::exists(parent, ec)) {
#if defined(__unix__) || defined(__APPLE__)
                    if (::access(parent.string().c_str(), W_OK) != 0) {
                        return false;
                    }
#endif
                }
            }
        } else {
            // Read access check
            std::error_code ec;
            if (!fs::exists(absPath, ec)) {
                return false;
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
        _errorCode = "disabled";
        _lastOperationSuccess = false;
        return false;
    }

    if (_path.empty()) {
        _status = "error: empty path";
        _lastError = "No path specified for read operation";
        _errorCode = "empty_path";
        _lastOperationSuccess = false;
        return false;
    }

    if (!isPathAllowed(_path, false)) {
        _status = "error: permission denied";
        _lastError = "OS or Sandbox permission denied reading file: " + _path;
        _errorCode = "permission_denied";
        _lastOperationSuccess = false;
        return false;
    }

    // In-memory virtual file
    if (VirtualFileSystem::isMemory(_path)) {
        std::string memData;
        if (!VirtualFileSystem::instance().readMemoryFile(_path, memData)) {
            _status = "error: file not found";
            _lastError = "Memory file not found: " + _path;
            _errorCode = "not_found";
            _lastOperationSuccess = false;
            return false;
        }
        if (_encoding == "base64") {
            _content = base64Encode(memData);
        } else if (_encoding == "hex") {
            _content = hexEncode(memData);
        } else {
            _content = std::move(memData);
        }
        _bytesRead = static_cast<double>(_content.size());
        _status = "read-success";
        _lastError = "";
        _errorCode = "none";
        _lastOperationSuccess = true;
        return true;
    }

    std::string resolvedPath = VirtualFileSystem::resolve(_path);
    std::error_code ec;
    fs::path absPath = fs::absolute(fs::path(resolvedPath)).lexically_normal();
    if (!fs::exists(absPath, ec)) {
        _status = "error: file not found";
        _lastError = "File not found: " + _path;
        _errorCode = "not_found";
        _lastOperationSuccess = false;
        return false;
    }

    if (fs::is_directory(absPath, ec)) {
        _status = "error: path is directory";
        _lastError = "Cannot read raw directory as plain file content: " + _path;
        _errorCode = "is_directory";
        _lastOperationSuccess = false;
        return false;
    }

    if (!fs::is_regular_file(absPath, ec) && !fs::is_symlink(absPath, ec)) {
        _status = "error: not regular file";
        _lastError = "Cannot read special device, socket, or FIFO stream: " + _path;
        _errorCode = "not_regular_file";
        _lastOperationSuccess = false;
        return false;
    }

    auto sz = fs::file_size(absPath, ec);
    if (!ec && sz > static_cast<uintmax_t>(_maxFileSize)) {
        _status = "error: file size exceeds limit";
        _lastError = "File size (" + std::to_string(sz) + " bytes) exceeds maxFileSize (" +
                     std::to_string(static_cast<uint64_t>(_maxFileSize)) + " bytes)";
        _errorCode = "size_limit_exceeded";
        _lastOperationSuccess = false;
        return false;
    }

    try {
        std::ifstream file(absPath, std::ios::in | std::ios::binary);
        if (!file.is_open()) {
            _status = "error: file open failed";
            _lastError = "Failed to open file for reading: " + _path;
            _errorCode = "open_failed";
            _lastOperationSuccess = false;
            return false;
        }

        std::ostringstream ss;
        ss << file.rdbuf();
        std::string raw = ss.str();

        // Optional UTF-8 BOM stripping
        if (_stripBom && raw.size() >= 3 &&
            static_cast<unsigned char>(raw[0]) == 0xEF &&
            static_cast<unsigned char>(raw[1]) == 0xBB &&
            static_cast<unsigned char>(raw[2]) == 0xBF) {
            raw.erase(0, 3);
        }

        // Optional CRLF newline normalization
        if (_normalizeNewlines && raw.find("\r\n") != std::string::npos) {
            std::string normalized;
            normalized.reserve(raw.size());
            for (size_t i = 0; i < raw.size(); ++i) {
                if (raw[i] == '\r' && i + 1 < raw.size() && raw[i + 1] == '\n') {
                    normalized.push_back('\n');
                    ++i;
                } else {
                    normalized.push_back(raw[i]);
                }
            }
            raw = std::move(normalized);
        }

        if (_encoding == "base64") {
            _content = base64Encode(raw);
        } else if (_encoding == "hex") {
            _content = hexEncode(raw);
        } else {
            _content = std::move(raw);
        }

        _bytesRead = static_cast<double>(_content.size());
        _status = "read-success";
        _lastError = "";
        _errorCode = "none";
        _lastOperationSuccess = true;
        return true;
    } catch (const std::exception& e) {
        _status = "error: exception during read";
        _lastError = e.what();
        _errorCode = "exception";
        _lastOperationSuccess = false;
        return false;
    }
}

bool FileChannel::executeWrite() {
    if (!_enabled) {
        _status = "error: channel disabled";
        _lastError = "File channel is disabled";
        _errorCode = "disabled";
        _lastOperationSuccess = false;
        return false;
    }

    if (_path.empty()) {
        _status = "error: empty path";
        _lastError = "No path specified for write operation";
        _errorCode = "empty_path";
        _lastOperationSuccess = false;
        return false;
    }

    if (!isPathAllowed(_path, true)) {
        _status = "error: permission denied";
        _lastError = "OS or Sandbox permission denied writing file: " + _path;
        _errorCode = "permission_denied";
        _lastOperationSuccess = false;
        return false;
    }

    // In-memory virtual file write
    if (VirtualFileSystem::isMemory(_path)) {
        std::string payload = _content;
        if (_encoding == "base64") {
            payload = base64Decode(_content);
        } else if (_encoding == "hex") {
            payload = hexDecode(_content);
        }
        if (_writeMode == "append") {
            std::string existing;
            VirtualFileSystem::instance().readMemoryFile(_path, existing);
            VirtualFileSystem::instance().writeMemoryFile(_path, existing + payload);
        } else {
            VirtualFileSystem::instance().writeMemoryFile(_path, payload);
        }
        _bytesWritten = static_cast<double>(payload.size());
        _status = "write-success";
        _lastError = "";
        _errorCode = "none";
        _lastOperationSuccess = true;
        return true;
    }

    std::string resolvedPath = VirtualFileSystem::resolve(_path);
    std::error_code ec;
    fs::path absPath = fs::absolute(fs::path(resolvedPath)).lexically_normal();
    if (fs::exists(absPath, ec) && fs::is_directory(absPath, ec)) {
        _status = "error: path is directory";
        _lastError = "Cannot write file content onto a directory: " + _path;
        _errorCode = "is_directory";
        _lastOperationSuccess = false;
        return false;
    }

    try {
        fs::path parent = absPath.parent_path();
        if (!parent.empty() && !fs::exists(parent, ec)) {
            if (!fs::create_directories(parent, ec) && ec) {
                _status = "error: directory creation failed";
                _lastError = "OS permission denied creating directories: " + ec.message();
                _errorCode = "dir_create_failed";
                _lastOperationSuccess = false;
                return false;
            }
        }

        std::string payload = _content;
        if (_encoding == "base64") {
            payload = base64Decode(_content);
        } else if (_encoding == "hex") {
            payload = hexDecode(_content);
        }

        if (_writeMode == "append") {
            std::ofstream file(absPath, std::ios::out | std::ios::binary | std::ios::app);
            if (!file.is_open()) {
                _status = "error: file open failed";
                _lastError = "Failed to open file for appending: " + _path;
                _errorCode = "open_failed";
                _lastOperationSuccess = false;
                return false;
            }
            file.write(payload.data(), payload.size());
            file.flush();
            if (file.fail()) {
                _status = "error: write failed";
                _lastError = "Disk write failed writing: " + _path;
                _errorCode = "write_failed";
                _lastOperationSuccess = false;
                return false;
            }
        } else if (_atomicWrite) {
            // Write to temporary file in the same directory then atomically rename
            fs::path tempPath = parent / (absPath.filename().string() + ".tmp." +
                                          std::to_string(::getpid()) + "." +
                                          std::to_string(s_atomicTempCounter++));
            {
                std::ofstream tempFile(tempPath, std::ios::out | std::ios::binary | std::ios::trunc);
                if (!tempFile.is_open()) {
                    _status = "error: temp file open failed";
                    _lastError = "Failed to open temporary file for writing: " + tempPath.string();
                    _errorCode = "temp_open_failed";
                    _lastOperationSuccess = false;
                    return false;
                }
                tempFile.write(payload.data(), payload.size());
                tempFile.flush();
                if (tempFile.fail()) {
                    tempFile.close();
                    fs::remove(tempPath, ec);
                    _status = "error: write failed";
                    _lastError = "Failed writing to temp file: " + tempPath.string();
                    _errorCode = "write_failed";
                    _lastOperationSuccess = false;
                    return false;
                }
                tempFile.close();
            }

            fs::rename(tempPath, absPath, ec);
            if (ec) {
                fs::remove(tempPath, ec);
                _status = "error: atomic rename failed";
                _lastError = "Atomic rename failed: " + ec.message();
                _errorCode = "rename_failed";
                _lastOperationSuccess = false;
                return false;
            }
        } else {
            std::ofstream file(absPath, std::ios::out | std::ios::binary | std::ios::trunc);
            if (!file.is_open()) {
                _status = "error: file open failed";
                _lastError = "Failed to open file for writing: " + _path;
                _errorCode = "open_failed";
                _lastOperationSuccess = false;
                return false;
            }

            file.write(payload.data(), payload.size());
            file.flush();
            if (file.fail()) {
                _status = "error: write failed";
                _lastError = "Disk write failed or disk full writing: " + _path;
                _errorCode = "write_failed";
                _lastOperationSuccess = false;
                return false;
            }
        }

        _bytesWritten = static_cast<double>(payload.size());
        _status = "write-success";
        _lastError = "";
        _errorCode = "none";
        _lastOperationSuccess = true;
        return true;
    } catch (const std::exception& e) {
        _status = "error: exception during write";
        _lastError = e.what();
        _errorCode = "exception";
        _lastOperationSuccess = false;
        return false;
    }
}

bool FileChannel::executeAppend() {
    std::string oldMode = _writeMode;
    _writeMode = "append";
    bool ok = executeWrite();
    _writeMode = oldMode;
    return ok;
}

bool FileChannel::executeDelete() {
    if (!_enabled) {
        _status = "error: channel disabled";
        _errorCode = "disabled";
        _lastOperationSuccess = false;
        return false;
    }
    if (_path.empty()) {
        _status = "error: empty path";
        _errorCode = "empty_path";
        _lastOperationSuccess = false;
        return false;
    }
    if (!isPathAllowed(_path, true)) {
        _status = "error: permission denied";
        _errorCode = "permission_denied";
        _lastOperationSuccess = false;
        return false;
    }

    if (VirtualFileSystem::isMemory(_path)) {
        bool del = VirtualFileSystem::instance().deleteMemoryFile(_path);
        if (del) {
            _status = "delete-success";
            _lastError = "";
            _errorCode = "none";
            _lastOperationSuccess = true;
            return true;
        } else {
            _status = "error: file not found";
            _lastError = "Memory file not found: " + _path;
            _errorCode = "not_found";
            _lastOperationSuccess = false;
            return false;
        }
    }

    std::string resolvedPath = VirtualFileSystem::resolve(_path);
    std::error_code ec;
    fs::path absPath = fs::absolute(fs::path(resolvedPath)).lexically_normal();
    if (!fs::exists(absPath, ec)) {
        _status = "error: file not found";
        _lastError = "File not found: " + _path;
        _errorCode = "not_found";
        _lastOperationSuccess = false;
        return false;
    }

    bool removed = fs::remove(absPath, ec);
    if (!removed || ec) {
        _status = "error: delete failed";
        _lastError = ec ? ec.message() : "Failed to remove target";
        _errorCode = "delete_failed";
        _lastOperationSuccess = false;
        return false;
    }

    _status = "delete-success";
    _lastError = "";
    _errorCode = "none";
    _lastOperationSuccess = true;
    return true;
}

bool FileChannel::executeCreateDir() {
    if (!_enabled) {
        _status = "error: channel disabled";
        _errorCode = "disabled";
        _lastOperationSuccess = false;
        return false;
    }
    if (_path.empty()) {
        _status = "error: empty path";
        _errorCode = "empty_path";
        _lastOperationSuccess = false;
        return false;
    }
    if (!isPathAllowed(_path, true)) {
        _status = "error: permission denied";
        _errorCode = "permission_denied";
        _lastOperationSuccess = false;
        return false;
    }

    std::error_code ec;
    fs::path absPath = fs::absolute(fs::path(_path)).lexically_normal();
    fs::create_directories(absPath, ec);
    if (ec) {
        _status = "error: directory creation failed";
        _lastError = ec.message();
        _errorCode = "dir_create_failed";
        _lastOperationSuccess = false;
        return false;
    }

    _status = "mkdir-success";
    _lastError = "";
    _errorCode = "none";
    _lastOperationSuccess = true;
    return true;
}

bool FileChannel::executeListDir() {
    if (!_enabled) {
        _status = "error: channel disabled";
        _errorCode = "disabled";
        _lastOperationSuccess = false;
        return false;
    }
    if (_path.empty()) {
        _status = "error: empty path";
        _errorCode = "empty_path";
        _lastOperationSuccess = false;
        return false;
    }
    if (!isPathAllowed(_path, false)) {
        _status = "error: permission denied";
        _errorCode = "permission_denied";
        _lastOperationSuccess = false;
        return false;
    }

    std::error_code ec;
    fs::path absPath = fs::absolute(fs::path(_path)).lexically_normal();
    if (!fs::exists(absPath, ec) || !fs::is_directory(absPath, ec)) {
        _status = "error: not a directory";
        _lastError = "Path is not an existing directory: " + _path;
        _errorCode = "not_a_directory";
        _lastOperationSuccess = false;
        return false;
    }

    std::vector<std::string> entries;
    for (const auto& entry : fs::directory_iterator(absPath, ec)) {
        entries.push_back(entry.path().filename().string());
    }
    std::sort(entries.begin(), entries.end());

    std::ostringstream ss;
    for (size_t i = 0; i < entries.size(); ++i) {
        if (i > 0) ss << "\n";
        ss << entries[i];
    }
    _directoryEntries = ss.str();
    _status = "listdir-success";
    _lastError = "";
    _errorCode = "none";
    _lastOperationSuccess = true;
    return true;
}

bool FileChannel::executeCopy() {
    if (!_enabled) {
        _status = "error: channel disabled";
        _errorCode = "disabled";
        _lastOperationSuccess = false;
        return false;
    }
    if (_path.empty() || _copyTo.empty()) {
        _status = "error: empty path or copyTo";
        _errorCode = "empty_path";
        _lastOperationSuccess = false;
        return false;
    }
    if (!isPathAllowed(_path, false) || !isPathAllowed(_copyTo, true)) {
        _status = "error: permission denied";
        _errorCode = "permission_denied";
        _lastOperationSuccess = false;
        return false;
    }

    std::error_code ec;
    fs::path src = fs::absolute(fs::path(_path)).lexically_normal();
    fs::path dst = fs::absolute(fs::path(_copyTo)).lexically_normal();
    if (!fs::exists(src, ec)) {
        _status = "error: source not found";
        _lastError = "Source file not found: " + _path;
        _errorCode = "not_found";
        _lastOperationSuccess = false;
        return false;
    }

    fs::path parent = dst.parent_path();
    if (!parent.empty() && !fs::exists(parent, ec)) {
        fs::create_directories(parent, ec);
    }

    fs::copy_file(src, dst, fs::copy_options::overwrite_existing, ec);
    if (ec) {
        _status = "error: copy failed";
        _lastError = ec.message();
        _errorCode = "copy_failed";
        _lastOperationSuccess = false;
        return false;
    }

    _status = "copy-success";
    _lastError = "";
    _errorCode = "none";
    _lastOperationSuccess = true;
    return true;
}

bool FileChannel::executeMove() {
    if (!_enabled) {
        _status = "error: channel disabled";
        _errorCode = "disabled";
        _lastOperationSuccess = false;
        return false;
    }
    if (_path.empty() || _moveTo.empty()) {
        _status = "error: empty path or moveTo";
        _errorCode = "empty_path";
        _lastOperationSuccess = false;
        return false;
    }
    if (!isPathAllowed(_path, true) || !isPathAllowed(_moveTo, true)) {
        _status = "error: permission denied";
        _errorCode = "permission_denied";
        _lastOperationSuccess = false;
        return false;
    }

    std::error_code ec;
    fs::path src = fs::absolute(fs::path(_path)).lexically_normal();
    fs::path dst = fs::absolute(fs::path(_moveTo)).lexically_normal();
    if (!fs::exists(src, ec)) {
        _status = "error: source not found";
        _lastError = "Source file not found: " + _path;
        _errorCode = "not_found";
        _lastOperationSuccess = false;
        return false;
    }

    fs::path parent = dst.parent_path();
    if (!parent.empty() && !fs::exists(parent, ec)) {
        fs::create_directories(parent, ec);
    }

    fs::rename(src, dst, ec);
    if (ec) {
        _status = "error: move failed";
        _lastError = ec.message();
        _errorCode = "move_failed";
        _lastOperationSuccess = false;
        return false;
    }

    _status = "move-success";
    _lastError = "";
    _errorCode = "none";
    _lastOperationSuccess = true;
    return true;
}

void FileChannel::propSetReadTrigger(const bool& v) {
    _readTrigger = v;
    if (_readTrigger) {
        executeRead();
        _readTrigger = false;
    }
}

void FileChannel::propSetWriteTrigger(const bool& v) {
    _writeTrigger = v;
    if (_writeTrigger) {
        executeWrite();
        _writeTrigger = false;
    }
}

void FileChannel::propSetAppendTrigger(const bool& v) {
    _appendTrigger = v;
    if (_appendTrigger) {
        executeAppend();
        _appendTrigger = false;
    }
}

void FileChannel::propSetDeleteTrigger(const bool& v) {
    _deleteTrigger = v;
    if (_deleteTrigger) {
        executeDelete();
        _deleteTrigger = false;
    }
}

void FileChannel::propSetCreateDirTrigger(const bool& v) {
    _createDirTrigger = v;
    if (_createDirTrigger) {
        executeCreateDir();
        _createDirTrigger = false;
    }
}

void FileChannel::propSetListDirTrigger(const bool& v) {
    _listDirTrigger = v;
    if (_listDirTrigger) {
        executeListDir();
        _listDirTrigger = false;
    }
}

void FileChannel::propSetCopyTrigger(const bool& v) {
    _copyTrigger = v;
    if (_copyTrigger) {
        executeCopy();
        _copyTrigger = false;
    }
}

void FileChannel::propSetMoveTrigger(const bool& v) {
    _moveTrigger = v;
    if (_moveTrigger) {
        executeMove();
        _moveTrigger = false;
    }
}

std::string FileChannel::propContentBase64() const {
    if (_encoding == "base64") {
        return _content;
    }
    if (_encoding == "hex") {
        return base64Encode(hexDecode(_content));
    }
    return base64Encode(_content);
}

void FileChannel::propSetContentBase64(const std::string& v) {
    if (_encoding == "base64") {
        _content = v;
    } else if (_encoding == "hex") {
        _content = hexEncode(base64Decode(v));
    } else {
        _content = base64Decode(v);
    }
}

std::string FileChannel::propContentHex() const {
    if (_encoding == "hex") {
        return _content;
    }
    if (_encoding == "base64") {
        return hexEncode(base64Decode(_content));
    }
    return hexEncode(_content);
}

void FileChannel::propSetContentHex(const std::string& v) {
    if (_encoding == "hex") {
        _content = v;
    } else if (_encoding == "base64") {
        _content = base64Encode(hexDecode(v));
    } else {
        _content = hexDecode(v);
    }
}

bool FileChannel::propExists() const {
    if (_path.empty()) return false;
    if (VirtualFileSystem::isMemory(_path)) {
        return VirtualFileSystem::instance().memoryFileExists(_path);
    }
    std::string physical = VirtualFileSystem::resolve(_path);
    std::error_code ec;
    return fs::exists(fs::path(physical), ec);
}

bool FileChannel::propIsDirectory() const {
    if (_path.empty()) return false;
    if (VirtualFileSystem::isMemory(_path)) return false;
    std::string physical = VirtualFileSystem::resolve(_path);
    std::error_code ec;
    return fs::is_directory(fs::path(physical), ec);
}

bool FileChannel::propIsRegularFile() const {
    if (_path.empty()) return false;
    if (VirtualFileSystem::isMemory(_path)) {
        return VirtualFileSystem::instance().memoryFileExists(_path);
    }
    std::string physical = VirtualFileSystem::resolve(_path);
    std::error_code ec;
    return fs::is_regular_file(fs::path(physical), ec);
}

bool FileChannel::propIsSymlink() const {
    if (_path.empty()) return false;
    if (VirtualFileSystem::isMemory(_path)) return false;
    std::string physical = VirtualFileSystem::resolve(_path);
    std::error_code ec;
    return fs::is_symlink(fs::path(physical), ec);
}

bool FileChannel::propIsWritable() const {
    if (_path.empty()) return false;
    if (VirtualFileSystem::isMemory(_path)) return true;
    return checkOSPermissions(_path, true);
}

bool FileChannel::propIsBinary() const {
    if (_content.find('\0') != std::string::npos) return true;
    std::string mime = propMimeType();
    std::string type = propFileType();
    return type == "image" || type == "audio" || type == "binary" || type == "archive" ||
           mime == "model/gltf-binary" || mime == "application/octet-stream";
}

double FileChannel::propSize() const {
    if (!_path.empty()) {
        if (VirtualFileSystem::isMemory(_path)) {
            return static_cast<double>(VirtualFileSystem::instance().memoryFileSize(_path));
        }
        std::string physical = VirtualFileSystem::resolve(_path);
        std::error_code ec;
        fs::path absPath = fs::absolute(fs::path(physical)).lexically_normal();
        if (fs::exists(absPath, ec) && !fs::is_directory(absPath, ec)) {
            auto sz = fs::file_size(absPath, ec);
            if (!ec) return static_cast<double>(sz);
        }
    }
    return static_cast<double>(_content.size());
}

double FileChannel::propLineCount() const {
    if (_content.empty()) return 0.0;
    size_t lines = 0;
    for (char c : _content) {
        if (c == '\n') ++lines;
    }
    if (_content.back() != '\n') ++lines;
    return static_cast<double>(lines);
}

std::string FileChannel::propLastModified() const {
    if (_path.empty()) return "";
    std::error_code ec;
    fs::path absPath = fs::absolute(fs::path(_path)).lexically_normal();
    if (!fs::exists(absPath, ec)) return "";
    auto ftime = fs::last_write_time(absPath, ec);
    if (ec) return "";

    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
    std::time_t cftime = std::chrono::system_clock::to_time_t(sctp);
    std::tm gm_tm;
#if defined(_WIN32)
    gmtime_s(&gm_tm, &cftime);
#else
    gmtime_r(&cftime, &gm_tm);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%SZ", &gm_tm);
    return std::string(buf);
}

std::string FileChannel::propSha256() const {
    if (!_content.empty()) {
        return computeSha256(_content);
    }
    if (_path.empty()) return "";
    std::error_code ec;
    fs::path absPath = fs::absolute(fs::path(_path)).lexically_normal();
    if (!fs::exists(absPath, ec) || fs::is_directory(absPath, ec)) return "";
    std::ifstream file(absPath, std::ios::in | std::ios::binary);
    if (!file.is_open()) return "";
    std::ostringstream ss;
    ss << file.rdbuf();
    return computeSha256(ss.str());
}

std::string FileChannel::propMimeType() const {
    return detectMimeType(_path, _content);
}

std::string FileChannel::propFileType() const {
    return categorizeFileType(propMimeType(), _path);
}

std::string FileChannel::propExtension() const {
    if (_path.empty()) return "";
    try {
        return fs::path(_path).extension().string();
    } catch (...) {
        return "";
    }
}

std::string FileChannel::propStem() const {
    if (_path.empty()) return "";
    try {
        return fs::path(_path).stem().string();
    } catch (...) {
        return "";
    }
}

std::string FileChannel::propFilename() const {
    if (_path.empty()) return "";
    try {
        return fs::path(_path).filename().string();
    } catch (...) {
        return "";
    }
}

std::string FileChannel::propDirectory() const {
    if (_path.empty()) return "";
    try {
        return fs::path(_path).parent_path().string();
    } catch (...) {
        return "";
    }
}

bool FileChannel::propJsonValid() const {
    if (_content.empty()) return false;
    return nlohmann::json::accept(_content);
}

std::string FileChannel::propJsonCompact() const {
    if (_content.empty()) return "";
    try {
        auto j = nlohmann::json::parse(_content);
        return j.dump();
    } catch (...) {
        return "";
    }
}

std::string FileChannel::propJsonPretty() const {
    if (_content.empty()) return "";
    try {
        auto j = nlohmann::json::parse(_content);
        return j.dump(2);
    } catch (...) {
        return "";
    }
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
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.append", this, &FileChannel::propAppendTrigger, &FileChannel::propSetAppendTrigger));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.delete", this, &FileChannel::propDeleteTrigger, &FileChannel::propSetDeleteTrigger));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.createDir", this, &FileChannel::propCreateDirTrigger, &FileChannel::propSetCreateDirTrigger));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.listDir", this, &FileChannel::propListDirTrigger, &FileChannel::propSetListDirTrigger));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.copy", this, &FileChannel::propCopyTrigger, &FileChannel::propSetCopyTrigger));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.move", this, &FileChannel::propMoveTrigger, &FileChannel::propSetMoveTrigger));

    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.writeMode", this, &FileChannel::propWriteMode, &FileChannel::propSetWriteMode));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.atomicWrite", this, &FileChannel::propAtomicWrite, &FileChannel::propSetAtomicWrite));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.encoding", this, &FileChannel::propEncoding, &FileChannel::propSetEncoding));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.stripBom", this, &FileChannel::propStripBom, &FileChannel::propSetStripBom));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.normalizeNewlines", this, &FileChannel::propNormalizeNewlines, &FileChannel::propSetNormalizeNewlines));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, double>>(
        "file.maxFileSize", this, &FileChannel::propMaxFileSize, &FileChannel::propSetMaxFileSize));

    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.copyTo", this, &FileChannel::propCopyTo, &FileChannel::propSetCopyTo));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.moveTo", this, &FileChannel::propMoveTo, &FileChannel::propSetMoveTo));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.directoryEntries", this, &FileChannel::propDirectoryEntries, nullptr));

    registerProperty(std::make_unique<ComputedProperty<FileChannel, double>>(
        "file.bytesRead", this, &FileChannel::propBytesRead, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, double>>(
        "file.bytesWritten", this, &FileChannel::propBytesWritten, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.status", this, &FileChannel::propStatus, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.lastError", this, &FileChannel::propLastError, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.errorCode", this, &FileChannel::propErrorCode, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.lastOperationSuccess", this, &FileChannel::propLastOperationSuccess, nullptr));

    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.contentBase64", this, &FileChannel::propContentBase64, &FileChannel::propSetContentBase64));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.contentHex", this, &FileChannel::propContentHex, &FileChannel::propSetContentHex));

    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.exists", this, &FileChannel::propExists, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.isDirectory", this, &FileChannel::propIsDirectory, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.isRegularFile", this, &FileChannel::propIsRegularFile, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.isSymlink", this, &FileChannel::propIsSymlink, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.isWritable", this, &FileChannel::propIsWritable, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.isBinary", this, &FileChannel::propIsBinary, nullptr));

    registerProperty(std::make_unique<ComputedProperty<FileChannel, double>>(
        "file.size", this, &FileChannel::propSize, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, double>>(
        "file.lineCount", this, &FileChannel::propLineCount, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.lastModified", this, &FileChannel::propLastModified, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.sha256", this, &FileChannel::propSha256, nullptr));

    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.mimeType", this, &FileChannel::propMimeType, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.fileType", this, &FileChannel::propFileType, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.extension", this, &FileChannel::propExtension, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.stem", this, &FileChannel::propStem, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.filename", this, &FileChannel::propFilename, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.directory", this, &FileChannel::propDirectory, nullptr));

    registerProperty(std::make_unique<ComputedProperty<FileChannel, bool>>(
        "file.jsonValid", this, &FileChannel::propJsonValid, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.jsonCompact", this, &FileChannel::propJsonCompact, nullptr));
    registerProperty(std::make_unique<ComputedProperty<FileChannel, std::string>>(
        "file.jsonPretty", this, &FileChannel::propJsonPretty, nullptr));
}

} // namespace Storage
} // namespace Singularity
