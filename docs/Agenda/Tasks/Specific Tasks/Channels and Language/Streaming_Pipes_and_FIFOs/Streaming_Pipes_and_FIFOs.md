# Streaming Pipes and Process Pipelines (`StreamChannel`) (2026-09-08)

**Status:** ✅ done and verified  
**Section in the To-Do list:** Modalities · integration · substrate  
**Author:** Gemini Spark  
**Timestamp:** 2026-09-08T21:52:00-07:00  

---

## Origination & Scope
Zach requested adding Streaming Pipes (direct process and FIFO plumbing) to Earthcall.
In Earthcall's Sense-Act ontology, storage and streaming channels bridge physical host and foreign software without violating the ontology (no hardcoded domain nouns, no black boxes).

## Architecture & Features

1. **First-Mover Modality Law (`@stream-channel`)**:
   - Implemented `StreamChannel : public Law` under `src/Singularity/Storage/StreamChannel.{hpp,cpp}`.
   - Identifier: `"stream-channel"`.
   - Registered at engine boot in `src/Singularity/Core/EngineInit.cpp`.

2. **Process Pipelines (`popen`/`pclose`)**:
   - Enables in-world Laws to open bi-directional streams directly connected to external CLI utilities (e.g. `ffmpeg`, `mpv`, `grep`, `cat`).
   - Supports `"w"` (pushing stream to child process) and `"r"` (streaming output from child process into world properties).

3. **POSIX Named Pipes (FIFOs)**:
   - `StreamChannel::createFifo(path)`: creates a POSIX FIFO (`mkfifo`) on the filesystem.
   - `StreamChannel::removeFifo(path)`: unlinks and cleans up FIFO nodes.
   - Enables inter-process, zero-disk-waste streaming pipelines between Earthcall and independent external applications.

4. **Chunked I/O & Base64 Transport**:
   - `stream.chunkData`: raw UTF-8 / binary string payload.
   - `stream.chunkDataBase64`: base64 encoded view so binary streams do not corrupt string properties or network JSON.
   - `stream.bytesStreamed` and `stream.chunksTransferred`: live metrics.

5. **Exposed Law Properties (Refusal #6 Compliant)**:
   - Controls: `stream.enabled`, `stream.target`, `stream.pipeType` ("process" or "fifo"), `stream.mode` ("w" or "r"), `stream.chunkData`, `stream.chunkDataBase64`.
   - Triggers: `stream.open`, `stream.close`, `stream.write`, `stream.read`.
   - Telemetry: `stream.isOpen`, `stream.bytesStreamed`, `stream.chunksTransferred`, `stream.status`, `stream.lastError`.

## Verification
- Headless test suite `tests/singularity/stream_channel_test.cpp`:
  - Verified Case 1: Process pipe output stream pushing chunks to external process (`cat > ...`), verifying data integrity.
  - Verified Case 2: Process pipe input stream reading live output from external subprocess.
  - Verified Case 3: POSIX FIFO creation, `S_ISFIFO` filesystem verification, and clean removal.
  - Verified Case 4: Base64 chunk view bidirectional encoding and decoding.
  - Result: 4/4 tests passed (100%).
