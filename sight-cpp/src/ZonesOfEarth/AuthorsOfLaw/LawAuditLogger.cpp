#include "LawAuditLogger.hpp"
#include <filesystem>
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace ECA {

LawAuditLogger::LawAuditLogger() {
    std::filesystem::create_directories("logs");
    
    // Open in append mode
    _logFile.open("logs/law_audit.log", std::ios::app);
    _jsonlFile.open("logs/law_audit.jsonl", std::ios::app);
    
    if (!_logFile.is_open() || !_jsonlFile.is_open()) {
        std::cerr << "[LawAuditLogger] Warning: Could not open audit log files in logs/\n";
    }
    
    _worker = std::thread(&LawAuditLogger::backgroundWorker, this);
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
        _cv.notify_one();
        if (_worker.joinable()) {
            _worker.join();
        }
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
    _queue.push(entry);
    _cv.notify_one();
}

std::string LawAuditLogger::currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&in_time_t), "%Y-%m-%d %X");
    return ss.str();
}

void LawAuditLogger::log(const std::string& type, const std::string& message, const nlohmann::json& details) {
    LogEntry entry;
    entry.timestamp = currentTimestamp();
    entry.type = type;
    entry.message = message;
    entry.details = details;
    
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (_running) {
            _queue.push(std::move(entry));
        }
    }
    _cv.notify_one();
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
            break;
        }
        
        for (const auto& entry : batch) {
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
        
        if (_logFile.is_open()) _logFile.flush();
        if (_jsonlFile.is_open()) _jsonlFile.flush();
    }
}

} // namespace ECA
