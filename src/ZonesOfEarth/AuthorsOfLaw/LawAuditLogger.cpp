#include "LawAuditLogger.hpp"
#include <filesystem>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ECA {

// Append, unless the file has already outgrown a session's worth of history —
// then start clean. Appending forever meant every run inherited every run
// before it, and the file only ever grew.
std::ios_base::openmode LawAuditLogger::openModeFor(const std::string& path) const {
    std::error_code ec;
    const std::uintmax_t size = std::filesystem::file_size(path, ec);
    if (!ec && size > kMaxFileBytes) return std::ios::trunc;
    return std::ios::app;
}

LawAuditLogger::LawAuditLogger() {
    std::filesystem::create_directories("logs");

    _logFile.open("logs/law_audit.log", openModeFor("logs/law_audit.log"));
    _jsonlFile.open("logs/law_audit.jsonl", openModeFor("logs/law_audit.jsonl"));

    if (!_logFile.is_open() || !_jsonlFile.is_open()) {
        std::cerr << "[LawAuditLogger] Warning: Could not open audit log files in logs/\n";
    }

#ifndef __EMSCRIPTEN__
    _worker = std::thread(&LawAuditLogger::backgroundWorker, this);
#endif
}

bool LawAuditLogger::wouldLog(const std::string& type) const {
    const Level level = _level.load();
    if (level == Level::Off) return false;
    if (level == Level::Verbose) return true;
    return type != "CONDITION" && type != "ACTION";
}

LawAuditLogger::~LawAuditLogger() {
    shutdown();
}

void LawAuditLogger::shutdown() {
    if (_running) {
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _running = false;
        }
#ifndef __EMSCRIPTEN__
        _cv.notify_one();
        if (_worker.joinable()) {
            _worker.join();
        }
#endif
    }
}

void LawAuditLogger::setActiveWorld(const std::string& worldName) {
    std::lock_guard<std::mutex> lock(_mutex);
    _activeWorld = worldName;
    
    // Log the switch immediately
    LogEntry entry;
    entry.timestamp = currentTimestamp();
    entry.type = "SYSTEM";
    entry.message = "Active world changed to: " + worldName;
    entry.details = nlohmann::json{{"world", worldName}};
#ifdef __EMSCRIPTEN__
    if (_logFile.is_open()) {
        _logFile << "[" << entry.timestamp << "] [" << entry.type << "] " << entry.message << "\n";
    }
#else
    _queue.push(entry);
    _cv.notify_one();
#endif
}

std::string LawAuditLogger::currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}

void LawAuditLogger::log(const std::string& type, const std::string& message, const nlohmann::json& details) {
    // Checked before the entry is even built: at Summary the CONDITION and
    // ACTION categories are the hot path, and formatting a string plus a
    // JSON payload per condition evaluation costs more than the write.
    if (!wouldLog(type)) return;

    LogEntry entry;
    entry.timestamp = currentTimestamp();
    entry.type = type;
    entry.message = message;
    entry.details = details;
    
#ifdef __EMSCRIPTEN__
    if (_logFile.is_open()) {
        _logFile << "[" << entry.timestamp << "] [" << entry.type << "] " << entry.message << "\n";
    }
#else
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_running) {
            _queue.push(std::move(entry));
        }
    }
    _cv.notify_one();
#endif
}

void LawAuditLogger::backgroundWorker() {
    while (true) {
        std::vector<LogEntry> batch;
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _cv.wait(lock, [this]() { return !_queue.empty() || !_running; });
            
            while (!_queue.empty()) {
                batch.push_back(std::move(_queue.front()));
                _queue.pop();
            }
        }
        
        if (batch.empty() && !_running) {
            if (_logFile.is_open()) _logFile.flush();
            if (_jsonlFile.is_open()) _jsonlFile.flush();
            break;
        }
        
        for (const auto& entry : batch) {
            if (_linesWritten >= kMaxLinesPerRun) {
                if (!_budgetNoticeWritten) {
                    _budgetNoticeWritten = true;
                    if (_logFile.is_open()) {
                        _logFile << "[" << entry.timestamp
                                 << "] [SYSTEM] audit budget reached ("
                                 << kMaxLinesPerRun
                                 << " lines); further entries dropped this run\n";
                        _logFile.flush();
                    }
                }
                break;
            }
            ++_linesWritten;
            ++_linesSinceFlush;
            if (_logFile.is_open()) {
                _logFile << "[" << entry.timestamp << "] [" << entry.type << "] " << entry.message << "\n";
            }
            if (_jsonlFile.is_open()) {
                nlohmann::json j;
                j["timestamp"] = entry.timestamp;
                j["type"] = entry.type;
                j["message"] = entry.message;
                j["world"] = _activeWorld;
                j["details"] = entry.details;
                _jsonlFile << j.dump() << "\n";
            }
        }

        // Flushing every batch made a WhileTrue world wait on disk once per
        // tick. Cap is still kMaxLinesPerRun; last batch flushes on shutdown.
        if (_linesSinceFlush >= 256) {
            if (_logFile.is_open()) _logFile.flush();
            if (_jsonlFile.is_open()) _jsonlFile.flush();
            _linesSinceFlush = 0;
        }
    }
}

} // namespace ECA
