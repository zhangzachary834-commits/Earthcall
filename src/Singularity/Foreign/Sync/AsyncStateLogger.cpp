#include "AsyncStateLogger.hpp"
#include "Singularity/Core/Logger.hpp"

// Scaffold implementation

AsyncStateLogger::AsyncStateLogger() : _maxBufferSize(1000) {
}

AsyncStateLogger::~AsyncStateLogger() {
    flush();
}

void AsyncStateLogger::logEvent(const std::string& entityId, const std::string& key, const std::string& value, double timestamp) {
    _buffer.push_back({entityId, key, value, timestamp});
    
    if (_buffer.size() >= _maxBufferSize) {
        flush();
    }
}

void AsyncStateLogger::flush() {
    if (_buffer.empty()) {
        return;
    }
    
    nlohmann::json entries = nlohmann::json::array();
    for (const auto& entry : _buffer) {
        entries.push_back({
            {"entityId", entry.entityId},
            {"key", entry.key},
            {"value", entry.value},
            {"timestamp", entry.timestamp}
        });
    }

    ECA::Logger::instance().log(
        ECA::LogCategory::State,
        "STATE_SYNC",
        "Flushed " + std::to_string(_buffer.size()) + " state events",
        nlohmann::json{{"count", _buffer.size()}, {"entries", entries}}
    );

    _buffer.clear();
}
