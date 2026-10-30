#include <iostream>
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
    std::error_code ec;
    std::filesystem::create_directories(logDir, ec);

    // new identity filenames use reversible byte-hex with an `id-` prefix, reject overlong/empty IDs rather than truncating
    if (stableIdentity.empty() || stableIdentity.length() > 128) {
        // Return a dummy path or throw, let's use a safe fallback or throw.
        // Actually, reject means it shouldn't log or throws an error. Let's return empty path.
        return {};
    }

    std::stringstream hex;
    for (unsigned char c : stableIdentity) {
        hex << std::hex << std::setw(2) << std::setfill('0') << (int)c;
    }
    std::string safeId = "id-" + hex.str();

    return logDir / (safeId + ".jsonl");
}

void DurableLogger::evictIfNeeded() {
    while (_streams.size() >= kMaxStreams && !_lru.empty()) {
        std::string lruId = _lru.back();
        _lru.pop_back();
        auto it = _streams.find(lruId);
        if (it != _streams.end()) {
            if (it->second.is_open()) {
                it->second.flush();
                it->second.close();
            }
            _streams.erase(it);
        }
    }
}

std::ofstream& DurableLogger::getStream(const std::string& stableIdentity) {
    auto it = _streams.find(stableIdentity);
    if (it == _streams.end()) {
        evictIfNeeded();
        auto path = resolvePath(stableIdentity);
        // append mode preserves history
        auto result = _streams.emplace(stableIdentity, std::ofstream(path, std::ios::app));
        _lru.push_front(stableIdentity);

        if (!result.first->second.is_open()) {
            std::cerr << "[DurableLogger] Failed to open durable log: " << path << "\n";
        }

        return result.first->second;
    }

    // update LRU
    _lru.remove(stableIdentity);
    _lru.push_front(stableIdentity);
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
        if (!(stream << entry.dump() << "\n")) {
            std::cerr << "[DurableLogger] Write failed for identity: " << stableIdentity << "\n";
        }
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
        _lru.remove(stableIdentity);
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
    _lru.clear();
}

} // namespace Logging
} // namespace ECA
