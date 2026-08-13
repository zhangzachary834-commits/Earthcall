#pragma once

#include <string>
#include <vector>

// 
// AsyncStateLogger 
// 
// Buffers raw OS accessibility events over `dt` (delta time) and flushes them
// asynchronously to the external Python ML First Mover.
// 
// Rule Enforcement: This class defines no domain logic, meaning it does not interpret
// what a "Button" or a "Window" means. It simply captures the raw graph of data 
// and hands it off.
//
class AsyncStateLogger {
public:
    AsyncStateLogger();
    ~AsyncStateLogger();

    // Log a raw property change or relation assertion at a given timestamp
    void logEvent(const std::string& entityId, const std::string& key, const std::string& value, double timestamp);

    // Flushes the buffer to the Python backend (e.g. via ZMQ or a socket)
    void flush();

private:
    struct LogEntry {
        std::string entityId;
        std::string key;
        std::string value;
        double timestamp;
    };

    std::vector<LogEntry> _buffer;
    size_t _maxBufferSize;
};
