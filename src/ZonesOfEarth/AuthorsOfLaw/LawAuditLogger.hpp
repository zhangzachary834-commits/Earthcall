#pragma once

#include "Singularity/Core/Logger.hpp"
#include "Singularity/Core/Logging/DurableLogger.hpp"
#include <atomic>
#include <cstdint>
#include <iostream>

namespace ECA {

class LawAuditLogger {
public:
    using Level = LogLevel;
    using LogEntry = Logger::LogEntry;

    static LawAuditLogger& instance() {
        static LawAuditLogger logger;
        return logger;
    }

    ~LawAuditLogger() = default;

    void setLevel(Level level) {
        Logger::instance().setCategoryLevel(LogCategory::Laws, level);
    }

    Level level() const {
        return Logger::instance().categoryLevel(LogCategory::Laws);
    }

    bool wouldLog(const std::string& type) const {
        return Logger::instance().wouldLog(LogCategory::Laws, type);
    }

    void setActiveWorld(const std::string& worldName) {
        Logger::instance().setActiveWorld(worldName);
    }

    void log(const std::string& type, const std::string& message,
             const nlohmann::json& details = nlohmann::json::object()) {
        Logger::instance().log(LogCategory::Laws, type, message, details);
    }

    // Independent of aggregate level: per-Law history must not disappear
    // when the console/aggregate chronicle is muted.
    bool logForLaw(const std::string& lawIdentity, const std::string& type,
                   const std::string& message,
                   const nlohmann::json& details = nlohmann::json::object()) {
        const bool ok = _durableLogger.log(lawIdentity, type, message, details);
        if (!ok) {
            // Count every unconfirmed attempt, including budget exhaustion.
            // Report the first and exponentially spaced later failures:
            // observability without flooding the simulation's hot path.
            const auto count = _unconfirmedDurableAppends.fetch_add(
                1, std::memory_order_relaxed) + 1;
            if (count == 1 || (count >= 1024 && (count & (count - 1)) == 0)) {
                std::cerr << "[LawAuditLogger] per-Law history append unconfirmed "
                             "or budget exhausted (cumulative attempts: "
                          << count << "; latest Law: " << lawIdentity << ")\n";
            }
        }
        return ok;
    }

    // Counts unsuccessful confirmations, not necessarily lost records.
    // A successful flush is not a stable-storage (fsync) guarantee.
    std::uint64_t unconfirmedDurableAppendCount() const noexcept {
        return _unconfirmedDurableAppends.load(std::memory_order_relaxed);
    }

    void shutdown() {
        _durableLogger.closeAll();
        Logger::instance().shutdown();
    }

    static constexpr std::size_t kMaxLinesPerRun = Logger::kMaxLinesPerRun;
    static constexpr std::uintmax_t kMaxFileBytes = Logger::kMaxFileBytes;

private:
    LawAuditLogger() : _durableLogger("laws") {}
    Logging::DurableLogger _durableLogger;
    std::atomic<std::uint64_t> _unconfirmedDurableAppends{0};
};

} // namespace ECA
