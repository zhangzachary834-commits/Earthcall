#include "Singularity/Core/Logging/DurableLogger.hpp"
#include <chrono>
#include <ctime>
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
    // Avoid constructing a locale-aware iostream for every Law event.
    // Preserve the exact UTC ISO-8601 seconds format in the JSONL schema.
    char timestamp[32]{};
    if (std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%SZ", &utc) == 0)
        throw std::runtime_error("Unable to format durable logging timestamp");
    return std::string(timestamp);
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
        if (_linesThisRun >= kMaxLinesPerRun ||
            _bytesThisRun >= kMaxBytesPerRun) return false;
        auto it = _streams.find(identity);
        if (it == _streams.end()) {
            const auto path = resolvePath(identity);
            std::error_code ec;
            std::filesystem::create_directories(path.parent_path(), ec);
            if (ec) return false;
            // Read the existing size before opening: never truncate prior history.
            auto existingBytes = std::filesystem::file_size(path, ec);
            if (ec) {
                if (ec != std::errc::no_such_file_or_directory) return false;
                existingBytes = 0;
            }
            if (existingBytes >= kMaxFileBytes) return false;
            // Bound open file descriptors; evicted streams reopen in append mode.
            if (_streams.size() >= kMaxOpenStreams) _streams.erase(_streams.begin());
            auto inserted = _streams.try_emplace(identity);
            it = inserted.first;
            it->second.bytes = existingBytes;
            it->second.file.open(path, std::ios::out | std::ios::app);
        }
        if (!it->second.file.is_open() || !it->second.file.good()) {
            // A failed open must not poison this identity permanently.
            // The next call should retry after permissions or disk recover.
            _streams.erase(it);
            return false;
        }
        // A stream that has reached its exact file cap cannot accept another
        // record. Reject before timestamp creation and JSON serialization;
        // preserve all existing bytes and report the unconfirmed append.
        if (it->second.bytes >= kMaxFileBytes) return false;
        nlohmann::json entry = {
            {"timestamp", currentTimestamp()},
            {"identity", identity},
            {"type", type},
            {"message", message}
        };
        if (!details.empty()) entry["details"] = details;
        const std::string record = entry.dump() + '\n';
        const auto bytes = static_cast<std::uintmax_t>(record.size());
        // Finite per-identity and per-process budgets prevent a hot Law from
        // exhausting disk. Exhaustion never deletes existing authored history.
        if (bytes > kMaxFileBytes || it->second.bytes > kMaxFileBytes - bytes ||
            bytes > kMaxBytesPerRun || _bytesThisRun > kMaxBytesPerRun - bytes)
            return false;
        it->second.file << record;
        it->second.file.flush();
        if (!it->second.file.good()) {
            // Reopen on the next attempt and re-read the actual file size;
            // a failed write may have left a partial record on disk.
            _streams.erase(it);
            return false;
        }
        it->second.bytes += bytes;
        _bytesThisRun += bytes;
        ++_linesThisRun;
        return true;
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
