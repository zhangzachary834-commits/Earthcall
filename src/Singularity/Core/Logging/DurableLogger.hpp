#pragma once
#include "Singularity/Core/AppPaths.hpp"
#include <string>
#include <filesystem>
#include <fstream>
#include <unordered_map>
#include <list>
#include <mutex>
#include "json.hpp"

namespace ECA {
namespace Logging {

class DurableLogger {
public:
    DurableLogger(const std::string& domain) : _domain(domain) {}
    ~DurableLogger() { closeAll(); }

    void log(const std::string& stableIdentity, const std::string& type, const std::string& message, const nlohmann::json& details = nlohmann::json::object());
    void closeAll();
    void close(const std::string& stableIdentity);

private:
    std::filesystem::path resolvePath(const std::string& stableIdentity) const;
    std::ofstream& getStream(const std::string& stableIdentity);

    std::string _domain;
    std::unordered_map<std::string, std::ofstream> _streams;
    std::list<std::string> _lru;
    static constexpr size_t kMaxStreams = 32;
    void evictIfNeeded();
    std::mutex _mutex;
};

} // namespace Logging
} // namespace ECA
