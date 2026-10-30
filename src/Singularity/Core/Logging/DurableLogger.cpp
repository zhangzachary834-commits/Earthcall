#include "Singularity/Core/Logging/DurableLogger.hpp"
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <system_error>

namespace ECA::Logging {

namespace {
std::string currentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto epoch = std::chrono::system_clock::to_time_t(now);
    std::tm utc{};
#if defined(_WIN32)
    gmtime_s(&utc, &epoch);
#else
    gmtime_r(&epoch, &utc);
#endif
    std::ostringstream out;
    out << std::put_time(&utc, "%Y-%m-%dT%H:%M:%SZ");
    return out.str();
}

bool safeDomain(const std::string& domain) {
    if (domain.empty() || domain == "." || domain == "..") return false;
    for (unsigned char c : domain) {
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_')) return false;
    }
    return true;
}
} // namespace

std::filesystem::path DurableLogger::resolvePath(const std::string& identity) const {
    // Reject rather than truncate: truncation would alias two Law identities.
    if (!safeDomain(_domain) || identity.empty() || identity.size() > 110)
        throw std::invalid_argument("Invalid durable log domain or identity length");
    static constexpr char hex[] = "0123456789abcdef";
    std::string encoded = "id-";
    encoded.reserve(3 + identity.size() * 2);
    for (unsigned char c : identity) {
        encoded.push_back(hex[c >> 4]);
        encoded.push_back(hex[c & 15]);
    }
    // Prefix prevents collision with laws.jsonl, the aggregate log.
    return _root / _domain / (encoded + ".jsonl");
}

bool DurableLogger::log(const std::string& identity, const std::string& type,
                        const std::string& message, const nlohmann::json& details) {
    std::lock_guard<std::mutex> lock(_mutex);
    try {
        auto it = _streams.find(identity);
        if (it == _streams.end()) {
            const auto path = resolvePath(identity);
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
            if (ec) return false;
            // Bound open file descriptors; evicted streams reopen in append mode.
            if (_streams.size() >= kMaxOpenStreams) _streams.erase(_streams.begin());
            auto inserted = _streams.try_emplace(identity);
            it = inserted.first;
            it->second.open(path, std::ios::out | std::ios::app);
        }
        if (!it->second.is_open() || !it->second.good()) return false;
        nlohmann::json entry = {
            {"timestamp", currentTimestamp()},
            {"identity", identity},
            {"type", type},
            {"message", message}
        };
        if (!details.empty()) entry["details"] = details;
        it->second << entry.dump() << '\n';
        it->second.flush();
        return it->second.good();
    } catch (const std::exception&) {
        return false; // logging must not abort Law execution
    }
}

void DurableLogger::close(const std::string& identity) {
    std::lock_guard<std::mutex> lock(_mutex);
    _streams.erase(identity); // ofstream destructor flushes/closes
}

void DurableLogger::closeAll() {
    std::lock_guard<std::mutex> lock(_mutex);
    _streams.clear();
}

} // namespace ECA::Logging
