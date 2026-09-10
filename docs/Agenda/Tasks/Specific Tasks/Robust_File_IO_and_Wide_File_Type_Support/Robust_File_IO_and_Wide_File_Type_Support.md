# Robust Native File I/O & Wide File Type Classification (2026-09-08)

**Status:** ✅ done and verified  
**Section in the To-Do list:** Modalities · integration · substrate  
**Author:** Gemini Spark  
**Timestamp:** 2026-09-08T16:17:30-07:00  

---

## Origination & Scope
Zach asked to make Earthcall's file I/O even more robust and able to handle a much wider range of file types.
File I/O in Earthcall is represented ontologically by `FileChannel` (`Singularity/Storage/FileChannel.hpp` & `.cpp`), a first-mover Sense-Act Law (`@file-channel`) governed by K4 Laws.

## Key Improvements Landed

1. **Atomic Write Guarantee**:
   - `file.atomicWrite` (default `true`): writes to a unique temporary file (`path.tmp.<pid>.<seq>`) in the target directory, flushes, closes, and atomically renames (`fs::rename`) onto the target path. Destination files are never truncated or corrupted if an I/O error or crash occurs mid-write.

2. **Append Mode**:
   - Added `file.writeMode` (`"overwrite"` or `"append"`) and `file.append` trigger for stream logging, telemetry, and continuous record accumulation without re-writing entire files.

3. **Wide Range of File Types & MIME Detection**:
   - Deep magic byte sniffing and extension detection across:
     - **Images**: PNG, JPEG, GIF, WebP, BMP, TIFF, SVG, TGA.
     - **Audio**: WAV, MP3, OGG, FLAC, AIFF, MIDI.
     - **3D Geometry / Models**: Wavefront OBJ, STL, GLTF, GLB, PLY.
     - **Earthcall Substrates**: `.ecmatter` (FlatBuffers), `.ecform` (JSON), `.ecsave` (MessagePack).
     - **Structured Data**: JSON, CSV, TSV, XML, YAML, TOML.
     - **Documents & Archives**: PDF, ZIP, GZIP, TAR, 7Z.
     - **Shaders & Code**: WGSL, GLSL, C++, Python, JavaScript.
   - `file.fileType`: High-level ontological category (`"image"`, `"audio"`, `"model"`, `"json"`, `"csv"`, `"shader"`, `"code"`, `"document"`, `"archive"`, `"text"`, `"binary"`).
   - `file.mimeType`: Specific MIME string (`"image/png"`, `"application/json"`, etc.).

4. **Binary Data Pipelines (Base64 & Hex)**:
   - `file.contentBase64` & `file.contentHex`: Read and write binary files (e.g. textures, audio clips, meshes, `.ecmatter` blobs) as Base64 or Hex without string corruption or invalid UTF-8 issues.
   - `file.encoding`: `"auto"`, `"text"`, `"base64"`, `"hex"`.
   - `file.isBinary`: Computed check distinguishing binary vs printable text.

5. **JSON Operations & Text Normalization**:
   - `file.jsonValid`: Zero-copy JSON validation via `nlohmann::json::accept`.
   - `file.jsonCompact` & `file.jsonPretty`: Minification and 2-space indentation formatting.
   - `file.stripBom` (default `true`): Automatic UTF-8 Byte Order Mark (`0xEF, 0xBB, 0xBF`) stripping on read to avoid parser rejections.
   - `file.normalizeNewlines`: Converts Windows CRLF (`\r\n`) to POSIX LF (`\n`).
   - `file.lineCount`: Line counting for text and CSV files.

6. **Defense-in-Depth Robustness & Security**:
   - DoS / OOM protection: `file.maxFileSize` ceiling (default 64 MB) prevents unbounded memory allocation on massive files or pseudo-devices.
   - Rejection of special device files, FIFOs, and sockets (`not_regular_file`).
   - Rejection of embedded null bytes (`\0`) in path strings.
   - Hardened sandbox path resolution via `fs::weakly_canonical` to prevent symlink traversal escaping sandbox mode.

7. **Full File & Directory Sense/Act Management**:
   - `file.createDir`, `file.listDir`, `file.directoryEntries`, `file.copyTo` + `file.copy`, `file.moveTo` + `file.move`, `file.delete`.
   - File metadata inspection: `file.size`, `file.lastModified` (ISO 8601), `file.sha256` (OpenSSL SHA-256 digest), `file.extension`, `file.stem`, `file.filename`, `file.directory`.
   - Standardized `file.errorCode`: machine-readable failure diagnostics (`"none"`, `"not_found"`, `"permission_denied"`, `"size_limit_exceeded"`, `"not_regular_file"`, etc.).

8. **Engine Integration**:
   - Added `FileChannel::syncRegister(*_lawManager)` to `src/Singularity/Core/EngineInit.cpp` so `@file-channel` is registered and discoverable from the first frame alongside other first-mover channels.

## Verification
- Built and ran `tests/singularity/file_channel_test.cpp`: all 13 comprehensive test cases passed (100%).
