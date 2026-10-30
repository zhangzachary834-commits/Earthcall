#include "Singularity/Core/Logging/DurableLogger.hpp"
#include "Singularity/Core/Logger.hpp"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ECA {
namespace Logging {

static std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &in_time_t);
#else
    localtime_r(&in_time_t, &tm_buf);
#endif
    std::stringstream ss;
    ss << std::put_time(&tm_buf, "%Y-%m-%d %X");
    return ss.str();
}

std::filesystem::path DurableLogger::resolvePath(const std::string& stableIdentity) const {
    auto logDir = AppPaths::logDirectory() / _domain;
    std::filesystem::create_directories(logDir);

    // basic filesystem-safe encoding (could be improved)
    std::string safeId = stableIdentity;
    for (char& c : safeId) {
        if (!std::isalnum(c) && c != '-' && c != '_') {
            c = '_';
        }
    }

    return logDir / (safeId + ".jsonl");
}

std::ofstream& DurableLogger::getStream(const std::string& stableIdentity) {
    auto it = _streams.find(stableIdentity);
    if (it == _streams.end()) {
        auto path = resolvePath(stableIdentity);
        // append mode preserves history
        auto result = _streams.emplace(stableIdentity, std::ofstream(path, std::ios::app));
        return result.first->second;
    }
    return it->second;
}

void DurableLogger::log(const std::string& stableIdentity, const std::string& type, const std::string& message, const nlohmann::json& details) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto& stream = getStream(stableIdentity);

    nlohmann::json entry;
    entry["timestamp"] = currentTimestamp();
    entry["type"] = type;
    entry["message"] = message;
    if (!details.empty()) {
        entry["details"] = details;
    }

    if (stream.is_open()) {
        stream << entry.dump() << "\n";
        stream.flush(); // ensure durability
    }
}

void DurableLogger::close(const std::string& stableIdentity) {
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _streams.find(stableIdentity);
    if (it != _streams.end()) {
        if (it->second.is_open()) {
            it->second.flush();
            it->second.close();
        }
        _streams.erase(it);
    }
}

void DurableLogger::closeAll() {
    std::lock_guard<std::mutex> lock(_mutex);
    for (auto& pair : _streams) {
        if (pair.second.is_open()) {
            pair.second.flush();
            pair.second.close();
        }
    }
    _streams.clear();
}

} // namespace Logging
} // namespace ECA
