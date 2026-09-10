#include "Singularity/Storage/StreamChannel.hpp"
#include "Singularity/Storage/FileChannel.hpp"
#include "Singularity/Screen/ScreenRecorder.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"

#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cassert>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace fs = std::filesystem;
using namespace Singularity::Storage;
using namespace Singularity::Screen;

namespace {

int g_failures = 0;

void check(bool condition, const std::string& desc) {
    if (!condition) {
        std::printf("  FAILED: %s\n", desc.c_str());
        ++g_failures;
    } else {
        std::printf("  ok: %s\n", desc.c_str());
    }
    std::fflush(stdout);
}

} // namespace

int main() {
    std::printf("Running stream_channel_test...\n");
    std::fflush(stdout);

    LawManager laws;
    StreamChannel::syncRegister(laws);
    ScreenRecorder::syncRegister(laws);

    StreamChannel* stream = StreamChannel::find(laws);
    ScreenRecorder* recorder = ScreenRecorder::find(laws);

    check(stream != nullptr, "StreamChannel first mover registered successfully");
    check(recorder != nullptr, "ScreenRecorder registered successfully");

    if (!stream || !recorder) {
        return 1;
    }

    // -----------------------------------------------------------------------
    // Case 1: Process Pipe Output Stream (Direct Pipeline Execution)
    // -----------------------------------------------------------------------
    std::string testOutFile = "/tmp/earthcall_piped_output.txt";
    unlink(testOutFile.c_str());

    std::string pipeCmd = "cat > " + testOutFile;
    bool openOk = stream->openPipe(pipeCmd, "w", "process");
    check(openOk == true, "openPipe for process command succeeded");
    check(stream->isOpen() == true, "stream->isOpen() reports true");

    std::string chunk1 = "Earthcall Streaming Pipe Frame 1\n";
    std::string chunk2 = "Earthcall Streaming Pipe Frame 2\n";

    bool w1 = stream->writeStringChunk(chunk1);
    bool w2 = stream->writeStringChunk(chunk2);
    check(w1 && w2, "writeStringChunk pushed sequential chunks to process pipe");

    PropertyValue val;
    lawGetValue(*stream, PropertyPath::parse("stream.bytesStreamed"), val);
    double streamedBytes = std::get<double>(val);
    check(streamedBytes == static_cast<double>(chunk1.size() + chunk2.size()),
          "stream.bytesStreamed tracks byte count accurately");

    lawGetValue(*stream, PropertyPath::parse("stream.chunksTransferred"), val);
    check(std::get<double>(val) == 2.0, "stream.chunksTransferred counts 2 chunks");

    stream->closePipe();
    check(stream->isOpen() == false, "stream->isOpen() reports false after close");

    // Verify file written by the piped subprocess
    std::ifstream in(testOutFile);
    std::string line1, line2;
    std::getline(in, line1);
    std::getline(in, line2);
    check(line1 == "Earthcall Streaming Pipe Frame 1" && line2 == "Earthcall Streaming Pipe Frame 2",
          "Underlying process received and wrote all streamed chunks without corruption");
    unlink(testOutFile.c_str());

    // -----------------------------------------------------------------------
    // Case 2: Process Pipe Input Stream (Reading external stream)
    // -----------------------------------------------------------------------
    bool openInOk = stream->openPipe("echo 'Telemetry Live Stream Payload'", "r", "process");
    check(openInOk == true, "openPipe for read process stream succeeded");

    std::string readData;
    bool readOk = stream->readChunk(1024, readData);
    check(readOk == true && readData.find("Telemetry Live Stream Payload") != std::string::npos,
          "readChunk received data from process stream");

    stream->closePipe();

    // -----------------------------------------------------------------------
    // Case 3: Named Pipe (FIFO) Lifecycle
    // -----------------------------------------------------------------------
    std::string fifoPath = "/tmp/earthcall_test_fifo.pipe";
    StreamChannel::removeFifo(fifoPath);

    bool created = StreamChannel::createFifo(fifoPath);
    check(created == true, "createFifo succeeded");

    struct stat st;
    bool isFifo = (stat(fifoPath.c_str(), &st) == 0) && S_ISFIFO(st.st_mode);
    check(isFifo == true, "Created node on filesystem is confirmed a POSIX FIFO");

    bool removed = StreamChannel::removeFifo(fifoPath);
    check(removed == true, "removeFifo successfully removed FIFO node");
    check(!fs::exists(fs::path(fifoPath)), "FIFO confirmed erased from filesystem");

    // -----------------------------------------------------------------------
    // Case 4: Base64 Chunk Data Property
    // -----------------------------------------------------------------------
    std::string binaryChunk = "\x00\xFF\x80\x40";
    std::string b64Expected = FileChannel::base64Encode(binaryChunk);

    lawSetValue(*stream, PropertyPath::parse("stream.chunkDataBase64"), PropertyValue(b64Expected));
    lawGetValue(*stream, PropertyPath::parse("stream.chunkData"), val);
    check(std::get<std::string>(val) == binaryChunk, "Setting chunkDataBase64 decodes into chunkData");

    if (g_failures > 0) {
        std::printf("stream_channel_test: FAILED (%d failures)\n", g_failures);
        return 1;
    }

    std::printf("stream_channel_test: ALL OK (all 4 cases passed)\n");
    return 0;
}
