#pragma once

#include "Singularity/Core/Logger.hpp"

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

    void log(const std::string& type, const std::string& message, const nlohmann::json& details = nlohmann::json::object()) {
        Logger::instance().log(LogCategory::Laws, type, message, details);
    }

    void shutdown() {
        Logger::instance().shutdown();
    }

    static constexpr std::size_t kMaxLinesPerRun = Logger::kMaxLinesPerRun;
    static constexpr std::uintmax_t kMaxFileBytes = Logger::kMaxFileBytes;

private:
    LawAuditLogger() = default;
};

} // namespace ECA
