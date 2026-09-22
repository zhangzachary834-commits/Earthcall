#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>

namespace Singularity {
namespace Storage {

// StreamChannel provides real-time streaming I/O pipes, FIFOs (named pipes),
// and child process pipelines (e.g. ffmpeg streaming) for Earthcall.
//
// Capabilities:
//   - Process pipes (popen/pclose): stream directly into external CLI utilities (e.g. ffmpeg, mpv).
//   - Named Pipes (FIFOs via mkfifo): zero-copy inter-process streaming pipelines.
//   - Chunked read/write with Base64 and raw binary views.
//
// Refusal #1 & Refusal #6 compliant:
// Registered as a first-mover Law (@stream-channel.*).
class StreamChannel : public Law {
public:
    StreamChannel();
    ~StreamChannel() override;

    bool isFirstMover() const override { return true; }
    std::string getIdentifier() const override { return "stream-channel"; }

    static void syncRegister(LawManager& laws);
    static StreamChannel* find(LawManager& laws);

    // Pipe management
    bool openPipe(const std::string& target, const std::string& mode, const std::string& pipeType);
    bool closePipe();
    bool isOpen() const;

    // Streaming I/O
    bool writeChunk(const uint8_t* data, size_t size);
    bool writeStringChunk(const std::string& data);
    bool readChunk(size_t maxBytes, std::string& outData);
    void flush();

    // Static FIFO utilities
    static bool createFifo(const std::string& path);
    static bool removeFifo(const std::string& path);

private:
    void buildProperties() override;

    // Property getters/setters
    bool propEnabled() const { return _enabled; }
    void propSetEnabled(const bool& v) { _enabled = v; }

    std::string propTarget() const { return _target; }
    void propSetTarget(const std::string& v) { _target = v; }

    std::string propPipeType() const { return _pipeType; }
    void propSetPipeType(const std::string& v) { _pipeType = v; }

    std::string propMode() const { return _mode; }
    void propSetMode(const std::string& v) { _mode = v; }

    std::string propChunkData() const { return _chunkData; }
    void propSetChunkData(const std::string& v) { _chunkData = v; }

    std::string propChunkDataBase64() const;
    void propSetChunkDataBase64(const std::string& v);

    bool propOpenTrigger() const { return _openTrigger; }
    void propSetOpenTrigger(const bool& v);

    bool propCloseTrigger() const { return _closeTrigger; }
    void propSetCloseTrigger(const bool& v);

    bool propWriteTrigger() const { return _writeTrigger; }
    void propSetWriteTrigger(const bool& v);

    bool propReadTrigger() const { return _readTrigger; }
    void propSetReadTrigger(const bool& v);

    bool propIsOpen() const { return isOpen(); }
    double propBytesStreamed() const { return _bytesStreamed; }
    double propChunksTransferred() const { return _chunksTransferred; }
    std::string propStatus() const { return _status; }
    std::string propLastError() const { return _lastError; }

    bool _enabled = true;
    std::string _target;
    std::string _pipeType = "process"; // "process" or "fifo"
    std::string _mode = "w";          // "w" or "r"
    std::string _chunkData;

    bool _openTrigger = false;
    bool _closeTrigger = false;
    bool _writeTrigger = false;
    bool _readTrigger = false;

    double _bytesStreamed = 0.0;
    double _chunksTransferred = 0.0;
    std::string _status = "idle";
    std::string _lastError;

    FILE* _pipeHandle = nullptr;
    bool _isProcessPipe = false;
};

} // namespace Storage
} // namespace Singularity
