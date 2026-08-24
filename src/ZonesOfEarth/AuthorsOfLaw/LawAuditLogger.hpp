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
#include "json.hpp"

namespace ECA {

class LawAuditLogger {
public:
    struct LogEntry {
        std::string type;         // EVENT, LAW, ACTION, CONDITION
        std::string message;      // Human readable message
        nlohmann::json details;   // JSON payload
        std::string timestamp;
    };

    static LawAuditLogger& instance() {
        static LawAuditLogger logger;
        return logger;
    }

    // ------------------------------------------------------------------
    // What gets written.
    //
    // Every condition evaluation used to reach the disk. One continuous law
    // over two hundred beings is twelve thousand flushed lines a second,
    // appended across every run the machine has ever done — a log that
    // scrolls that fast is exactly as opaque as silence, and it is a disk
    // leak besides. So the firehose categories are off unless asked for.
    //
    //   Summary  EVENT and LAW — what happened and what came of it.
    //   Verbose  adds CONDITION and ACTION: per-evaluation, per-node. The
    //            debugging setting, not the running one.
    //   Off      nothing is written.
    // ------------------------------------------------------------------
    enum class Level { Off = 0, Summary = 1, Verbose = 2 };

    ~LawAuditLogger();

    void setLevel(Level level) { _level = level; }
    Level level() const { return _level; }
    bool wouldLog(const std::string& type) const;

    // Sets the active world save name for context
    void setActiveWorld(const std::string& worldName);

    // Enqueue a log entry
    void log(const std::string& type, const std::string& message, const nlohmann::json& details = nlohmann::json::object());

    // Stop the logger background thread gracefully
    void shutdown();

    // A run's budget. Past it the logger stops writing and says so once —
    // an audit trail that fills the disk stops being an audit trail.
    static constexpr std::size_t kMaxLinesPerRun = 200000;
    // A log file bigger than this is truncated when the logger opens, so a
    // session never inherits an unbounded history from every session before.
    static constexpr std::uintmax_t kMaxFileBytes = 32ull * 1024 * 1024;

private:
    LawAuditLogger();

    void backgroundWorker();
    std::string currentTimestamp();
    std::ios_base::openmode openModeFor(const std::string& path) const;

    std::string _activeWorld = "Unknown";

    std::ofstream _logFile;
    std::ofstream _jsonlFile;

    std::atomic<Level> _level{Level::Summary};
    std::size_t _linesWritten = 0;
    std::size_t _linesSinceFlush = 0;
    bool _budgetNoticeWritten = false;

    std::queue<LogEntry> _queue;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::atomic<bool> _running{true};
    std::thread _worker;
};

} // namespace ECA
