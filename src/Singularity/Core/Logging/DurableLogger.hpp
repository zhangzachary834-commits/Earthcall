#pragma once

#include "Singularity/Core/AppPaths.hpp"
#include "json.hpp"
#include <filesystem>
#include <fstream>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace ECA::Logging {

// Per-identity append-only JSONL, separate from aggregate category logs.
// The root may be injected by tests; production uses AppPaths::logDirectory().
class DurableLogger {
public:
    explicit DurableLogger(std::string domain,
                           std::filesystem::path root = AppPaths::logDirectory())
        : _domain(std::move(domain)), _root(std::filesystem::absolute(root)) {}
    ~DurableLogger() { closeAll(); }
    DurableLogger(const DurableLogger&) = delete;
    DurableLogger& operator=(const DurableLogger&) = delete;

    // False means append/flush was not confirmed (or a budget was reached).
    // A partial record remains possible on I/O failure; flush is not fsync.
    static constexpr std::uintmax_t kMaxFileBytes = 8ull * 1024 * 1024;
    static constexpr std::uintmax_t kMaxBytesPerRun = 32ull * 1024 * 1024;
    static constexpr std::size_t kMaxLinesPerRun = 200000;
    bool log(const std::string& stableIdentity, const std::string& type,
             const std::string& message,
             const nlohmann::json& details = nlohmann::json::object());
    void close(const std::string& stableIdentity);
    void closeAll();

private:
    std::filesystem::path resolvePath(const std::string& stableIdentity) const;
    static constexpr std::size_t kMaxOpenStreams = 32;
    std::string _domain;
    std::filesystem::path _root;
    struct OpenStream {
        std::ofstream file;
        std::uintmax_t bytes = 0;
    };
    std::unordered_map<std::string, OpenStream> _streams;
    std::uintmax_t _bytesThisRun = 0;
    std::size_t _linesThisRun = 0;
    std::mutex _mutex;
};

} // namespace ECA::Logging
