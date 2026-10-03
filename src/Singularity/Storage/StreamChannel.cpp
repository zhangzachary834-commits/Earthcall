#include "Singularity/Storage/StreamChannel.hpp"
#include "Singularity/Storage/FileChannel.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"

#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <vector>

namespace Singularity {
namespace Storage {

StreamChannel::StreamChannel() : Law("stream-channel") {
    setName("Stream Channel");
    _enabled = true;
    _pipeType = "process";
    _mode = "w";
}

StreamChannel::~StreamChannel() {
    closePipe();
}

void StreamChannel::syncRegister(LawManager& laws) {
    if (laws.find("stream-channel")) return;

    auto stream = std::make_shared<StreamChannel>();
    laws.add(stream);
}

StreamChannel* StreamChannel::find(LawManager& laws) {
    return dynamic_cast<StreamChannel*>(laws.find("stream-channel"));
}

bool StreamChannel::createFifo(const std::string& path) {
    if (path.empty()) return false;
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        if (S_ISFIFO(st.st_mode)) return true; // Already exists as FIFO
        unlink(path.c_str());                  // Remove conflicting non-fifo file
    }
    return mkfifo(path.c_str(), 0666) == 0;
}

bool StreamChannel::removeFifo(const std::string& path) {
    if (path.empty()) return false;
    return unlink(path.c_str()) == 0;
}

bool StreamChannel::isOpen() const {
    return _pipeHandle != nullptr;
}

bool StreamChannel::openPipe(const std::string& target, const std::string& mode, const std::string& pipeType) {
    closePipe();

    if (!_enabled) {
        _status = "error: channel disabled";
        _lastError = "Stream channel is disabled";
        return false;
    }

    if (target.empty()) {
        _status = "error: empty target";
        _lastError = "No pipe target or process command specified";
        return false;
    }

    _target = target;
    _mode = mode.empty() ? "w" : mode;
    _pipeType = pipeType.empty() ? "process" : pipeType;

    if (_pipeType == "process") {
        _pipeHandle = popen(_target.c_str(), _mode.c_str());
        _isProcessPipe = true;
        if (!_pipeHandle) {
            _status = "error: popen failed";
            _lastError = "Failed to open process pipe: " + _target;
            return false;
        }
    } else {
        // FIFO mode
        createFifo(_target);
        const char* fmode = (_mode == "w" || _mode == "w+") ? "w" : "r";
        _pipeHandle = fopen(_target.c_str(), fmode);
        _isProcessPipe = false;
        if (!_pipeHandle) {
            _status = "error: fopen fifo failed";
            _lastError = "Failed to open FIFO pipe: " + _target;
            return false;
        }
    }

    _status = "open";
    _lastError = "";
    return true;
}

bool StreamChannel::closePipe() {
    if (!_pipeHandle) return true;

    if (_isProcessPipe) {
        pclose(_pipeHandle);
    } else {
        fclose(_pipeHandle);
    }
    _pipeHandle = nullptr;
    _isProcessPipe = false;
    _status = "idle";
    return true;
}

bool StreamChannel::writeChunk(const uint8_t* data, size_t size) {
    if (!_pipeHandle || size == 0) return false;

    size_t written = fwrite(data, 1, size, _pipeHandle);
    fflush(_pipeHandle);
    if (written == size) {
        _bytesStreamed += static_cast<double>(written);
        _chunksTransferred += 1.0;
        _status = "streaming";
        _lastError = "";
        return true;
    } else {
        _status = "error: write failed";
        _lastError = "Pipe write failed";
        return false;
    }
}

bool StreamChannel::writeStringChunk(const std::string& data) {
    return writeChunk(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

bool StreamChannel::readChunk(size_t maxBytes, std::string& outData) {
    if (!_pipeHandle || maxBytes == 0) return false;

    std::vector<char> buffer(maxBytes);
    size_t bytesRead = fread(buffer.data(), 1, maxBytes, _pipeHandle);
    if (bytesRead > 0) {
        outData.assign(buffer.data(), bytesRead);
        _bytesStreamed += static_cast<double>(bytesRead);
        _chunksTransferred += 1.0;
        _status = "streaming";
        _lastError = "";
        return true;
    }
    return false;
}

void StreamChannel::flush() {
    if (_pipeHandle) {
        fflush(_pipeHandle);
    }
}

std::string StreamChannel::propChunkDataBase64() const {
    return FileChannel::base64Encode(_chunkData);
}

void StreamChannel::propSetChunkDataBase64(const std::string& v) {
    _chunkData = FileChannel::base64Decode(v);
}

void StreamChannel::propSetOpenTrigger(const bool& v) {
    _openTrigger = v;
    if (_openTrigger) {
        openPipe(_target, _mode, _pipeType);
        _openTrigger = false;
    }
}

void StreamChannel::propSetCloseTrigger(const bool& v) {
    _closeTrigger = v;
    if (_closeTrigger) {
        closePipe();
        _closeTrigger = false;
    }
}

void StreamChannel::propSetWriteTrigger(const bool& v) {
    _writeTrigger = v;
    if (_writeTrigger) {
        writeStringChunk(_chunkData);
        _writeTrigger = false;
    }
}

void StreamChannel::propSetReadTrigger(const bool& v) {
    _readTrigger = v;
    if (_readTrigger) {
        readChunk(4096, _chunkData);
        _readTrigger = false;
    }
}

void StreamChannel::buildProperties() {
    registerProperty(std::make_unique<ComputedProperty<StreamChannel, bool>>(
        "stream.enabled", this, &StreamChannel::propEnabled, &StreamChannel::propSetEnabled));
    registerProperty(std::make_unique<ComputedProperty<StreamChannel, std::string>>(
        "stream.target", this, &StreamChannel::propTarget, &StreamChannel::propSetTarget));
    registerProperty(std::make_unique<ComputedProperty<StreamChannel, std::string>>(
        "stream.pipeType", this, &StreamChannel::propPipeType, &StreamChannel::propSetPipeType));
    registerProperty(std::make_unique<ComputedProperty<StreamChannel, std::string>>(
        "stream.mode", this, &StreamChannel::propMode, &StreamChannel::propSetMode));

    registerProperty(std::make_unique<ComputedProperty<StreamChannel, std::string>>(
        "stream.chunkData", this, &StreamChannel::propChunkData, &StreamChannel::propSetChunkData));
    registerProperty(std::make_unique<ComputedProperty<StreamChannel, std::string>>(
        "stream.chunkDataBase64", this, &StreamChannel::propChunkDataBase64, &StreamChannel::propSetChunkDataBase64));

    registerProperty(std::make_unique<ComputedProperty<StreamChannel, bool>>(
        "stream.open", this, &StreamChannel::propOpenTrigger, &StreamChannel::propSetOpenTrigger));
    registerProperty(std::make_unique<ComputedProperty<StreamChannel, bool>>(
        "stream.close", this, &StreamChannel::propCloseTrigger, &StreamChannel::propSetCloseTrigger));
    registerProperty(std::make_unique<ComputedProperty<StreamChannel, bool>>(
        "stream.write", this, &StreamChannel::propWriteTrigger, &StreamChannel::propSetWriteTrigger));
    registerProperty(std::make_unique<ComputedProperty<StreamChannel, bool>>(
        "stream.read", this, &StreamChannel::propReadTrigger, &StreamChannel::propSetReadTrigger));

    registerProperty(std::make_unique<ComputedProperty<StreamChannel, bool>>(
        "stream.isOpen", this, &StreamChannel::propIsOpen, nullptr));
    registerProperty(std::make_unique<ComputedProperty<StreamChannel, double>>(
        "stream.bytesStreamed", this, &StreamChannel::propBytesStreamed, nullptr));
    registerProperty(std::make_unique<ComputedProperty<StreamChannel, double>>(
        "stream.chunksTransferred", this, &StreamChannel::propChunksTransferred, nullptr));
    registerProperty(std::make_unique<ComputedProperty<StreamChannel, std::string>>(
        "stream.status", this, &StreamChannel::propStatus, nullptr));
    registerProperty(std::make_unique<ComputedProperty<StreamChannel, std::string>>(
        "stream.lastError", this, &StreamChannel::propLastError, nullptr));
}

} // namespace Storage
} // namespace Singularity
