#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <queue>
#include <atomic>
#include <fstream>
#include <unordered_map>
#include "json.hpp"

namespace ECA {

enum class LogCategory {
    Laws,
    System,
    Person,
    State,
    Language,
    Audio
};

enum class LogLevel {
    Off = 0,
    Summary = 1,
    Verbose = 2
};

std::string categoryToString(LogCategory cat);
LogCategory stringToCategory(const std::string& name);

class Logger {
public:
    struct LogEntry {
        LogCategory category;
        std::string type;         // EVENT, LAW, ACTION, CONDITION, INFO, ERROR, SYSTEM, etc.
        std::string message;      // Human readable message
        nlohmann::json details;   // JSON payload
        std::string timestamp;
    };

    static Logger& instance();

    ~Logger();

    void setLevel(LogLevel level) { _level = level; }
    void setCategoryLevel(LogCategory cat, LogLevel level);
    LogLevel level() const { return _level; }
    LogLevel categoryLevel(LogCategory cat) const;
    bool wouldLog(LogCategory cat, const std::string& type) const;

    // Sets active world save name for context
    void setActiveWorld(const std::string& worldName);
    std::string activeWorld() const;

    // Log a entry to a specific category
    void log(LogCategory cat, const std::string& type, const std::string& message, const nlohmann::json& details = nlohmann::json::object());

    // Conveniences for string-based category resolution
    void log(const std::string& categoryName, const std::string& type, const std::string& message, const nlohmann::json& details = nlohmann::json::object());

    // Gracefully stop the background worker thread
    void shutdown();

    static constexpr std::size_t kMaxLinesPerRun = 200000;
    static constexpr std::uintmax_t kMaxFileBytes = 32ull * 1024 * 1024;

private:
    Logger();

    void backgroundWorker();
    std::string currentTimestamp();
    std::ios_base::openmode openModeFor(const std::string& path) const;
    void ensureCategoryStreams(LogCategory cat);

    struct CategoryStreams {
        std::ofstream logFile;
        std::ofstream jsonlFile;
        std::size_t linesWritten = 0;
        std::size_t linesSinceFlush = 0;
        bool budgetNoticeWritten = false;
    };

    std::string _activeWorld = "Unknown";
    std::atomic<LogLevel> _level{LogLevel::Summary};

    mutable std::mutex _categoryLevelMutex;
    std::unordered_map<LogCategory, LogLevel> _categoryLevels;

    std::unordered_map<LogCategory, CategoryStreams> _streams;
    std::ofstream _legacyLawLogFile;    // Mirror for logs/law_audit.log compatibility
    std::ofstream _legacyLawJsonlFile;  // Mirror for logs/law_audit.jsonl compatibility

    std::queue<LogEntry> _queue;
    mutable std::mutex _mutex;
    std::condition_variable _cv;
    std::atomic<bool> _running{true};
    std::thread _worker;
};

} // namespace ECA
