#pragma once

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

    ~LawAuditLogger();

    // Sets the active world save name for context
    void setActiveWorld(const std::string& worldName);

    // Enqueue a log entry
    void log(const std::string& type, const std::string& message, const nlohmann::json& details = nlohmann::json::object());

    // Stop the logger background thread gracefully
    void shutdown();

private:
    LawAuditLogger();

    void backgroundWorker();
    std::string currentTimestamp();

    std::string _activeWorld = "Unknown";
    
    std::ofstream _logFile;
    std::ofstream _jsonlFile;

    std::queue<LogEntry> _queue;
    std::mutex _mutex;
    std::condition_variable _cv;
    std::atomic<bool> _running{true};
    std::thread _worker;
};

} // namespace ECA
