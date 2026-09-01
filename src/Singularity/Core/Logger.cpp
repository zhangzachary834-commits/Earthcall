#include "Logger.hpp"
#include <filesystem>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace ECA {

std::string categoryToString(LogCategory cat) {
    switch (cat) {
        case LogCategory::Laws:     return "laws";
        case LogCategory::System:   return "system";
        case LogCategory::Person:   return "person";
        case LogCategory::State:    return "state";
        case LogCategory::Language: return "language";
        case LogCategory::Audio:    return "audio";
        default:                    return "system";
    }
}

LogCategory stringToCategory(const std::string& name) {
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "laws" || lower == "law") return LogCategory::Laws;
    if (lower == "person" || lower == "user") return LogCategory::Person;
    if (lower == "state" || lower == "sync") return LogCategory::State;
    if (lower == "language" || lower == "lang") return LogCategory::Language;
    if (lower == "audio" || lower == "sound") return LogCategory::Audio;
    return LogCategory::System;
}

Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

std::ios_base::openmode Logger::openModeFor(const std::string& path) const {
    std::error_code ec;
    const std::uintmax_t size = std::filesystem::file_size(path, ec);
    if (!ec && size > kMaxFileBytes) return std::ios::trunc;
    return std::ios::app;
}

void Logger::ensureCategoryStreams(LogCategory cat) {
    if (_streams.find(cat) != _streams.end()) return;

    std::string catName = categoryToString(cat);
    std::string dirPath = "logs/" + catName;
    std::filesystem::create_directories(dirPath);

    std::string logPath = dirPath + "/" + catName + ".log";
    std::string jsonlPath = dirPath + "/" + catName + ".jsonl";

    CategoryStreams cs;
    cs.logFile.open(logPath, openModeFor(logPath));
    cs.jsonlFile.open(jsonlPath, openModeFor(jsonlPath));

    if (!cs.logFile.is_open() || !cs.jsonlFile.is_open()) {
        std::cerr << "[Logger] Warning: Could not open log files in " << dirPath << "\n";
    }

    _streams[cat] = std::move(cs);
}

Logger::Logger() {
    std::filesystem::create_directories("logs");

    // Mirror for backwards compatibility with legacy law audit logs
    std::string legacyLog = "logs/law_audit.log";
    std::string legacyJsonl = "logs/law_audit.jsonl";
    _legacyLawLogFile.open(legacyLog, openModeFor(legacyLog));
    _legacyLawJsonlFile.open(legacyJsonl, openModeFor(legacyJsonl));

    // Initialize all standard category subdirectories and streams
    ensureCategoryStreams(LogCategory::Laws);
    ensureCategoryStreams(LogCategory::System);
    ensureCategoryStreams(LogCategory::Person);
    ensureCategoryStreams(LogCategory::State);
    ensureCategoryStreams(LogCategory::Language);
    ensureCategoryStreams(LogCategory::Audio);

#ifndef __EMSCRIPTEN__
    _worker = std::thread(&Logger::backgroundWorker, this);
#endif
}

Logger::~Logger() {
    shutdown();
}

void Logger::shutdown() {
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

void Logger::setCategoryLevel(LogCategory cat, LogLevel level) {
    std::lock_guard<std::mutex> lock(_categoryLevelMutex);
    _categoryLevels[cat] = level;
}

LogLevel Logger::categoryLevel(LogCategory cat) const {
    std::lock_guard<std::mutex> lock(_categoryLevelMutex);
    auto it = _categoryLevels.find(cat);
    if (it != _categoryLevels.end()) return it->second;
    return _level.load();
}

bool Logger::wouldLog(LogCategory cat, const std::string& type) const {
    const LogLevel lvl = categoryLevel(cat);
    if (lvl == LogLevel::Off) return false;
    if (lvl == LogLevel::Verbose) return true;
    return type != "CONDITION" && type != "ACTION";
}

void Logger::setActiveWorld(const std::string& worldName) {
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _activeWorld = worldName;
    }
    log(LogCategory::System, "SYSTEM", "Active world changed to: " + worldName, nlohmann::json{{"world", worldName}});
}

std::string Logger::activeWorld() const {
    std::lock_guard<std::mutex> lock(_mutex);
    return _activeWorld;
}

std::string Logger::currentTimestamp() {
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

void Logger::log(LogCategory cat, const std::string& type, const std::string& message, const nlohmann::json& details) {
    if (!wouldLog(cat, type)) return;

    LogEntry entry;
    entry.category = cat;
    entry.timestamp = currentTimestamp();
    entry.type = type;
    entry.message = message;
    entry.details = details;

#ifdef __EMSCRIPTEN__
    // Direct output in single-threaded WebAssembly build
    auto it = _streams.find(cat);
    if (it != _streams.end()) {
        if (it->second.logFile.is_open()) {
            it->second.logFile << "[" << entry.timestamp << "] [" << entry.type << "] " << entry.message << "\n";
        }
    }
    if (cat == LogCategory::Laws && _legacyLawLogFile.is_open()) {
        _legacyLawLogFile << "[" << entry.timestamp << "] [" << entry.type << "] " << entry.message << "\n";
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

void Logger::log(const std::string& categoryName, const std::string& type, const std::string& message, const nlohmann::json& details) {
    log(stringToCategory(categoryName), type, message, details);
}

void Logger::backgroundWorker() {
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
            for (auto& pair : _streams) {
                if (pair.second.logFile.is_open()) pair.second.logFile.flush();
                if (pair.second.jsonlFile.is_open()) pair.second.jsonlFile.flush();
            }
            if (_legacyLawLogFile.is_open()) _legacyLawLogFile.flush();
            if (_legacyLawJsonlFile.is_open()) _legacyLawJsonlFile.flush();
            break;
        }

        for (const auto& entry : batch) {
            auto it = _streams.find(entry.category);
            if (it == _streams.end()) continue;

            CategoryStreams& cs = it->second;
            if (cs.linesWritten >= kMaxLinesPerRun) {
                if (!cs.budgetNoticeWritten) {
                    cs.budgetNoticeWritten = true;
                    if (cs.logFile.is_open()) {
                        cs.logFile << "[" << entry.timestamp
                                   << "] [SYSTEM] audit budget reached ("
                                   << kMaxLinesPerRun
                                   << " lines); further entries dropped this run\n";
                        cs.logFile.flush();
                    }
                }
                continue;
            }

            ++cs.linesWritten;
            ++cs.linesSinceFlush;

            if (cs.logFile.is_open()) {
                cs.logFile << "[" << entry.timestamp << "] [" << entry.type << "] " << entry.message << "\n";
            }
            if (cs.jsonlFile.is_open()) {
                nlohmann::json j;
                j["timestamp"] = entry.timestamp;
                j["type"] = entry.type;
                j["category"] = categoryToString(entry.category);
                j["message"] = entry.message;
                j["world"] = _activeWorld;
                j["details"] = entry.details;
                cs.jsonlFile << j.dump() << "\n";
            }

            // Also mirror Laws category to legacy law_audit logs
            if (entry.category == LogCategory::Laws) {
                if (_legacyLawLogFile.is_open()) {
                    _legacyLawLogFile << "[" << entry.timestamp << "] [" << entry.type << "] " << entry.message << "\n";
                }
                if (_legacyLawJsonlFile.is_open()) {
                    nlohmann::json j;
                    j["timestamp"] = entry.timestamp;
                    j["type"] = entry.type;
                    j["message"] = entry.message;
                    j["world"] = _activeWorld;
                    j["details"] = entry.details;
                    _legacyLawJsonlFile << j.dump() << "\n";
                }
            }
        }

        for (auto& pair : _streams) {
            if (pair.second.linesSinceFlush >= 256) {
                if (pair.second.logFile.is_open()) pair.second.logFile.flush();
                if (pair.second.jsonlFile.is_open()) pair.second.jsonlFile.flush();
                pair.second.linesSinceFlush = 0;
            }
        }
        if (_legacyLawLogFile.is_open()) _legacyLawLogFile.flush();
        if (_legacyLawJsonlFile.is_open()) _legacyLawJsonlFile.flush();
    }
}

} // namespace ECA
