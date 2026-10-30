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

    // False means the record was NOT appended; flush is not fsync.
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
    std::unordered_map<std::string, std::ofstream> _streams;
    std::mutex _mutex;
};

} // namespace ECA::Logging
