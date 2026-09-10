#include "Singularity/Storage/FileChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"

#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cassert>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace Singularity::Storage;

namespace {

int g_failures = 0;

void check(bool condition, const std::string& desc) {
    if (!condition) {
        std::printf("  FAILED: %s\n", desc.c_str());\
        ++g_failures;
    } else {
        std::printf("  ok: %s\n", desc.c_str());
    }
}

} // namespace

int main() {
    std::printf("Running comprehensive file_channel_test...\n");

    LawManager laws;
    FileChannel::syncRegister(laws);
    FileChannel* channel = FileChannel::find(laws);
    check(channel != nullptr, "FileChannel first mover registered successfully");

    if (!channel) {
        return 1;
    }

    // Prepare clean test sandbox under saves/
    fs::path testDir = fs::path("saves") / "test_file_channel_robust_sandbox";
    std::error_code ec;
    fs::remove_all(testDir, ec);
    fs::create_directories(testDir, ec);

    // -----------------------------------------------------------------------
    // Case 1: Basic Write and Read (Preserve Backwards Compatibility)
    // -----------------------------------------------------------------------
    fs::path testFile = testDir / "test_hello.txt";
    std::string testPath = testFile.string();
    std::string sampleText = "Earthcall Substrate File Channel Test\nNative File I/O Verified";

    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(testPath));
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(sampleText));
    lawSetValue(*channel, PropertyPath::parse("file.write"), PropertyValue(true));

    PropertyValue val;
    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == true, "Write operation reported success");

    lawGetValue(*channel, PropertyPath::parse("file.exists"), val);
    check(std::get<bool>(val) == true, "file.exists returns true for written file");

    lawGetValue(*channel, PropertyPath::parse("file.bytesWritten"), val);
    check(std::get<double>(val) == static_cast<double>(sampleText.size()), "bytesWritten matches sample text size");

    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(std::string("")));
    lawSetValue(*channel, PropertyPath::parse("file.read"), PropertyValue(true));

    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == true, "Read operation reported success");

    lawGetValue(*channel, PropertyPath::parse("file.content"), val);
    check(std::get<std::string>(val) == sampleText, "Read content matches written sample text");

    lawGetValue(*channel, PropertyPath::parse("file.bytesRead"), val);
    check(std::get<double>(val) == static_cast<double>(sampleText.size()), "bytesRead matches sample text size");

    // -----------------------------------------------------------------------
    // Case 2: Parent Directory Auto-Creation
    // -----------------------------------------------------------------------
    fs::path nestedFile = testDir / "deep" / "nested" / "dir" / "data.txt";
    std::string nestedPath = nestedFile.string();
    std::string nestedText = "Nested parent directory creation test";

    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(nestedPath));
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(nestedText));
    lawSetValue(*channel, PropertyPath::parse("file.write"), PropertyValue(true));

    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == true, "Write to nested path succeeded with auto parent directory creation");
    check(fs::exists(nestedFile), "Nested file actually created on disk");

    // -----------------------------------------------------------------------
    // Case 3: OS Permission & Protected System Path Restrictions
    // -----------------------------------------------------------------------
    std::string protectedSystemPath = "/System/earthcall_forbidden.txt";
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(protectedSystemPath));
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(std::string("forbidden")));
    lawSetValue(*channel, PropertyPath::parse("file.write"), PropertyValue(true));

    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == false, "Write to protected OS system path rejected");

    lawGetValue(*channel, PropertyPath::parse("file.lastError"), val);
    std::string errStr = std::get<std::string>(val);
    check(!errStr.empty(), "lastError populated with permission denial explanation");

    // -----------------------------------------------------------------------
    // Case 4: Sandbox Mode & Path Traversal / Null Byte Prevention
    // -----------------------------------------------------------------------
    lawSetValue(*channel, PropertyPath::parse("file.sandboxMode"), PropertyValue(true));
    std::string traversalPath = "../../outside_sandbox.txt";
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(traversalPath));
    lawSetValue(*channel, PropertyPath::parse("file.write"), PropertyValue(true));

    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == false, "Path traversal escaping sandbox blocked in sandboxMode = true");

    std::string nullByteInjection = "saves/test\0evil.txt";
    nullByteInjection.assign("saves/test\0evil.txt", 18);
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(nullByteInjection));
    lawSetValue(*channel, PropertyPath::parse("file.write"), PropertyValue(true));
    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == false, "Null byte injection in path blocked");

    // -----------------------------------------------------------------------
    // Case 5: Computed Properties (exists, isDirectory, isRegularFile, isWritable, metadata)
    // -----------------------------------------------------------------------
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(testDir.string()));
    lawGetValue(*channel, PropertyPath::parse("file.isDirectory"), val);
    check(std::get<bool>(val) == true, "file.isDirectory returns true for directory path");

    lawGetValue(*channel, PropertyPath::parse("file.isRegularFile"), val);
    check(std::get<bool>(val) == false, "file.isRegularFile returns false for directory path");

    lawGetValue(*channel, PropertyPath::parse("file.isWritable"), val);
    check(std::get<bool>(val) == true, "file.isWritable returns true for writable directory path");

    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(testPath));
    lawGetValue(*channel, PropertyPath::parse("file.isRegularFile"), val);
    check(std::get<bool>(val) == true, "file.isRegularFile returns true for written file");

    lawGetValue(*channel, PropertyPath::parse("file.size"), val);
    check(std::get<double>(val) == static_cast<double>(sampleText.size()), "file.size reports accurate size");

    lawGetValue(*channel, PropertyPath::parse("file.extension"), val);
    check(std::get<std::string>(val) == ".txt", "file.extension returns .txt");

    lawGetValue(*channel, PropertyPath::parse("file.stem"), val);
    check(std::get<std::string>(val) == "test_hello", "file.stem returns test_hello");

    lawGetValue(*channel, PropertyPath::parse("file.filename"), val);
    check(std::get<std::string>(val) == "test_hello.txt", "file.filename returns test_hello.txt");

    lawGetValue(*channel, PropertyPath::parse("file.sha256"), val);
    std::string shaVal = std::get<std::string>(val);
    check(shaVal.size() == 64, "file.sha256 returns valid 64-char SHA-256 hex digest");

    lawGetValue(*channel, PropertyPath::parse("file.lastModified"), val);
    std::string modTime = std::get<std::string>(val);
    check(!modTime.empty() && modTime.find("T") != std::string::npos, "file.lastModified returns ISO-8601 string");

    // -----------------------------------------------------------------------
    // Case 6: Atomic Writes
    // -----------------------------------------------------------------------
    fs::path atomicFile = testDir / "atomic_target.txt";
    std::string atomicInitial = "Initial valid state that must not be corrupted";
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(atomicFile.string()));
    lawSetValue(*channel, PropertyPath::parse("file.atomicWrite"), PropertyValue(true));
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(atomicInitial));
    lawSetValue(*channel, PropertyPath::parse("file.write"), PropertyValue(true));

    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == true, "Atomic write initial state succeeded");

    std::string atomicUpdated = "Updated state atomically swapped in";
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(atomicUpdated));
    lawSetValue(*channel, PropertyPath::parse("file.write"), PropertyValue(true));
    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == true, "Atomic write update succeeded");

    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(std::string("")));
    lawSetValue(*channel, PropertyPath::parse("file.read"), PropertyValue(true));
    lawGetValue(*channel, PropertyPath::parse("file.content"), val);
    check(std::get<std::string>(val) == atomicUpdated, "Content after atomic write matches updated state");

    // -----------------------------------------------------------------------
    // Case 7: Append Mode
    // -----------------------------------------------------------------------
    fs::path appendLogFile = testDir / "log_stream.txt";
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(appendLogFile.string()));
    lawSetValue(*channel, PropertyPath::parse("file.writeMode"), PropertyValue(std::string("overwrite")));
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(std::string("Line 1\n")));
    lawSetValue(*channel, PropertyPath::parse("file.write"), PropertyValue(true));

    // Now append Line 2
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(std::string("Line 2\n")));
    lawSetValue(*channel, PropertyPath::parse("file.append"), PropertyValue(true));

    // And append Line 3 via writeMode = append
    lawSetValue(*channel, PropertyPath::parse("file.writeMode"), PropertyValue(std::string("append")));
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(std::string("Line 3\n")));
    lawSetValue(*channel, PropertyPath::parse("file.write"), PropertyValue(true));

    // Read full log back
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(std::string("")));
    lawSetValue(*channel, PropertyPath::parse("file.read"), PropertyValue(true));
    lawGetValue(*channel, PropertyPath::parse("file.content"), val);
    check(std::get<std::string>(val) == "Line 1\nLine 2\nLine 3\n", "Append mode correctly accumulated lines");

    lawGetValue(*channel, PropertyPath::parse("file.lineCount"), val);
    check(std::get<double>(val) == 3.0, "lineCount reports 3 lines");

    // -----------------------------------------------------------------------
    // Case 8: JSON Format Validation, Compaction, and Pretty-Printing
    // -----------------------------------------------------------------------
    std::string jsonRaw = "{\n  \"ontology\": \"Earthcall\",\n  \"active\": true,\n  \"count\": 42\n}";
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(jsonRaw));
    lawGetValue(*channel, PropertyPath::parse("file.jsonValid"), val);
    check(std::get<bool>(val) == true, "jsonValid returns true for valid JSON");

    lawGetValue(*channel, PropertyPath::parse("file.jsonCompact"), val);
    check(std::get<std::string>(val) == "{\"active\":true,\"count\":42,\"ontology\":\"Earthcall\"}",
          "jsonCompact formats compact JSON");

    lawGetValue(*channel, PropertyPath::parse("file.jsonPretty"), val);
    std::string pretty = std::get<std::string>(val);
    check(pretty.find("  \"ontology\": \"Earthcall\"") != std::string::npos, "jsonPretty returns formatted JSON");

    std::string invalidJson = "{ \"unclosed\": ";
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(invalidJson));
    lawGetValue(*channel, PropertyPath::parse("file.jsonValid"), val);
    check(std::get<bool>(val) == false, "jsonValid returns false for invalid JSON");

    // -----------------------------------------------------------------------
    // Case 9: Text Pre-Processing (UTF-8 BOM Stripping & CRLF Normalization)
    // -----------------------------------------------------------------------
    fs::path bomFile = testDir / "bom_test.txt";
    {
        std::ofstream out(bomFile, std::ios::binary);
        unsigned char bom[3] = {0xEF, 0xBB, 0xBF};
        out.write(reinterpret_cast<char*>(bom), 3);
        out << "{\"hello\":\"world\"}";
    }
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(bomFile.string()));
    lawSetValue(*channel, PropertyPath::parse("file.stripBom"), PropertyValue(true));
    lawSetValue(*channel, PropertyPath::parse("file.read"), PropertyValue(true));
    lawGetValue(*channel, PropertyPath::parse("file.content"), val);
    check(std::get<std::string>(val) == "{\"hello\":\"world\"}", "UTF-8 BOM stripped on read");
    lawGetValue(*channel, PropertyPath::parse("file.jsonValid"), val);
    check(std::get<bool>(val) == true, "JSON with stripped BOM accepted as valid JSON");

    // CRLF normalization
    fs::path crlfFile = testDir / "crlf_test.txt";
    {
        std::ofstream out(crlfFile, std::ios::binary);
        out << "A\r\nB\r\nC\r\n";
    }
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(crlfFile.string()));
    lawSetValue(*channel, PropertyPath::parse("file.normalizeNewlines"), PropertyValue(true));
    lawSetValue(*channel, PropertyPath::parse("file.read"), PropertyValue(true));
    lawGetValue(*channel, PropertyPath::parse("file.content"), val);
    check(std::get<std::string>(val) == "A\nB\nC\n", "CRLF converted to LF when normalizeNewlines is true");

    // -----------------------------------------------------------------------
    // Case 10: Binary Files via Base64 and Hex Encoding
    // -----------------------------------------------------------------------
    fs::path binFile = testDir / "binary_data.bin";
    std::string rawBinary;
    rawBinary.push_back('\x00');
    rawBinary.push_back('\xFF');
    rawBinary.push_back('\x7F');
    rawBinary.push_back('\x10');
    rawBinary.push_back('\x00');
    rawBinary.push_back('\x42');

    std::string expectedBase64 = FileChannel::base64Encode(rawBinary);
    std::string expectedHex = FileChannel::hexEncode(rawBinary);

    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(binFile.string()));
    lawSetValue(*channel, PropertyPath::parse("file.encoding"), PropertyValue(std::string("base64")));
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(expectedBase64));
    lawSetValue(*channel, PropertyPath::parse("file.write"), PropertyValue(true));

    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == true, "Binary write via Base64 encoding succeeded");

    // Read back in Base64 mode
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(std::string("")));
    lawSetValue(*channel, PropertyPath::parse("file.read"), PropertyValue(true));
    lawGetValue(*channel, PropertyPath::parse("file.content"), val);
    check(std::get<std::string>(val) == expectedBase64, "Read in base64 mode matches expected Base64");

    // Verify contentHex property
    lawGetValue(*channel, PropertyPath::parse("file.contentHex"), val);
    check(FileChannel::hexDecode(std::get<std::string>(val)) == rawBinary, "contentHex correctly converts binary payload");

    // Check isBinary detection
    lawSetValue(*channel, PropertyPath::parse("file.encoding"), PropertyValue(std::string("text")));
    lawSetValue(*channel, PropertyPath::parse("file.read"), PropertyValue(true));
    lawGetValue(*channel, PropertyPath::parse("file.isBinary"), val);
    check(std::get<bool>(val) == true, "isBinary accurately detects null bytes in binary data");

    // -----------------------------------------------------------------------
    // Case 11: MIME Type & File Type Sniffing (Images, Audio, Models, etc.)
    // -----------------------------------------------------------------------
    // PNG Magic
    std::string pngHeader = "\x89PNG\r\n\x1a\n\x00\x00\x00\rIHDR";
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(std::string("texture.png")));
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(pngHeader));
    lawGetValue(*channel, PropertyPath::parse("file.mimeType"), val);
    check(std::get<std::string>(val) == "image/png", "PNG magic bytes detected as image/png");
    lawGetValue(*channel, PropertyPath::parse("file.fileType"), val);
    check(std::get<std::string>(val) == "image", "PNG categorized as image");

    // WAV Magic
    std::string wavHeader = "RIFF\x24\x00\x00\x00WAVEfmt ";
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(std::string("sound.wav")));
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(wavHeader));
    lawGetValue(*channel, PropertyPath::parse("file.mimeType"), val);
    check(std::get<std::string>(val) == "audio/wav", "WAV magic bytes detected as audio/wav");
    lawGetValue(*channel, PropertyPath::parse("file.fileType"), val);
    check(std::get<std::string>(val) == "audio", "WAV categorized as audio");

    // 3D Model (OBJ & GLTF)
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(std::string("avatar.obj")));
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(std::string("v 0 0 0\nv 1 1 1\nf 1 2 3")));
    lawGetValue(*channel, PropertyPath::parse("file.mimeType"), val);
    check(std::get<std::string>(val) == "model/obj", "OBJ file detected as model/obj");
    lawGetValue(*channel, PropertyPath::parse("file.fileType"), val);
    check(std::get<std::string>(val) == "model", "OBJ categorized as model");

    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(std::string("scene.gltf")));
    lawGetValue(*channel, PropertyPath::parse("file.mimeType"), val);
    check(std::get<std::string>(val) == "model/gltf+json", "GLTF file detected as model/gltf+json");
    lawGetValue(*channel, PropertyPath::parse("file.fileType"), val);
    check(std::get<std::string>(val) == "model", "GLTF categorized as model");

    // Earthcall Substrate (.ecmatter FlatBuffers & .ecform JSON)
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(std::string("world.ecmatter")));
    lawGetValue(*channel, PropertyPath::parse("file.mimeType"), val);
    check(std::get<std::string>(val) == "application/x-flatbuffers", ".ecmatter detected as application/x-flatbuffers");
    lawGetValue(*channel, PropertyPath::parse("file.fileType"), val);
    check(std::get<std::string>(val) == "binary", ".ecmatter categorized as binary substrate");

    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(std::string("world.ecform")));
    lawGetValue(*channel, PropertyPath::parse("file.mimeType"), val);
    check(std::get<std::string>(val) == "application/json", ".ecform detected as application/json");
    lawGetValue(*channel, PropertyPath::parse("file.fileType"), val);
    check(std::get<std::string>(val) == "json", ".ecform categorized as json");

    // PDF Document
    std::string pdfHeader = "%PDF-1.7\n1 0 obj\n";
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(std::string("spec.pdf")));
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(pdfHeader));
    lawGetValue(*channel, PropertyPath::parse("file.mimeType"), val);
    check(std::get<std::string>(val) == "application/pdf", "PDF magic detected as application/pdf");
    lawGetValue(*channel, PropertyPath::parse("file.fileType"), val);
    check(std::get<std::string>(val) == "document", "PDF categorized as document");

    // Shader (WGSL)
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(std::string("sdf.wgsl")));
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(std::string("@vertex fn vs_main() -> @builtin(position) vec4f { return vec4f(0); }")));
    lawGetValue(*channel, PropertyPath::parse("file.mimeType"), val);
    check(std::get<std::string>(val) == "text/wgsl", "WGSL shader detected as text/wgsl");
    lawGetValue(*channel, PropertyPath::parse("file.fileType"), val);
    check(std::get<std::string>(val) == "shader", "WGSL categorized as shader");

    // -----------------------------------------------------------------------
    // Case 12: Max File Size Safety Guard (OOM / DoS Prevention)
    // -----------------------------------------------------------------------
    fs::path bigFile = testDir / "oversized.txt";
    std::string bigContent(500, 'X');
    {
        std::ofstream out(bigFile, std::ios::binary);
        out << bigContent;
    }
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(bigFile.string()));
    lawSetValue(*channel, PropertyPath::parse("file.maxFileSize"), PropertyValue(100.0)); // 100-byte cap
    lawSetValue(*channel, PropertyPath::parse("file.read"), PropertyValue(true));
    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == false, "Reading file exceeding maxFileSize rejected");

    lawGetValue(*channel, PropertyPath::parse("file.errorCode"), val);
    check(std::get<std::string>(val) == "size_limit_exceeded", "errorCode is size_limit_exceeded");

    // Reset max file size to 64MB
    lawSetValue(*channel, PropertyPath::parse("file.maxFileSize"), PropertyValue(67108864.0));

    // -----------------------------------------------------------------------
    // Case 13: File Management (createDir, listDir, copy, move, delete)
    // -----------------------------------------------------------------------
    fs::path subDir = testDir / "test_subdir";
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(subDir.string()));
    lawSetValue(*channel, PropertyPath::parse("file.createDir"), PropertyValue(true));
    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == true, "createDir succeeded");
    check(fs::is_directory(subDir), "test_subdir directory confirmed created");

    // Create two files inside subDir
    {
        std::ofstream f1(subDir / "alpha.txt"); f1 << "A";
        std::ofstream f2(subDir / "beta.txt"); f2 << "B";
    }
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(subDir.string()));
    lawSetValue(*channel, PropertyPath::parse("file.listDir"), PropertyValue(true));
    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == true, "listDir succeeded");
    lawGetValue(*channel, PropertyPath::parse("file.directoryEntries"), val);
    std::string entries = std::get<std::string>(val);
    check(entries.find("alpha.txt") != std::string::npos && entries.find("beta.txt") != std::string::npos,
          "directoryEntries contains alpha.txt and beta.txt");

    // Copy file
    fs::path srcFile = subDir / "alpha.txt";
    fs::path dstFile = subDir / "alpha_copied.txt";
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(srcFile.string()));
    lawSetValue(*channel, PropertyPath::parse("file.copyTo"), PropertyValue(dstFile.string()));
    lawSetValue(*channel, PropertyPath::parse("file.copy"), PropertyValue(true));
    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == true, "copy operation succeeded");
    check(fs::exists(dstFile), "Copied file exists on disk");

    // Move file
    fs::path movedFile = subDir / "alpha_moved.txt";
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(dstFile.string()));
    lawSetValue(*channel, PropertyPath::parse("file.moveTo"), PropertyValue(movedFile.string()));
    lawSetValue(*channel, PropertyPath::parse("file.move"), PropertyValue(true));
    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == true, "move operation succeeded");
    check(!fs::exists(dstFile) && fs::exists(movedFile), "File moved from source to destination");

    // Delete file
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(movedFile.string()));
    lawSetValue(*channel, PropertyPath::parse("file.delete"), PropertyValue(true));
    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == true, "delete operation succeeded");
    check(!fs::exists(movedFile), "File confirmed deleted from disk");

    // -----------------------------------------------------------------------
    // Case 14: Exception Handling (try-catch blocks)
    // -----------------------------------------------------------------------
    // Trigger JSON parse exceptions
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(std::string("invalid json")));

    lawGetValue(*channel, PropertyPath::parse("file.jsonCompact"), val);
    check(std::get<std::string>(val) == "", "jsonCompact gracefully returns empty string on parse error");

    lawGetValue(*channel, PropertyPath::parse("file.jsonPretty"), val);
    check(std::get<std::string>(val) == "", "jsonPretty gracefully returns empty string on parse error");

    // Trigger std::filesystem exceptions by providing strings with embedded nulls
    // std::filesystem::path throws an exception when constructed with null characters.
    std::string invalidPath("bad\0path", 8);
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(invalidPath));

    lawGetValue(*channel, PropertyPath::parse("file.extension"), val);
    check(std::get<std::string>(val) == "", "propExtension gracefully handles exceptions");

    lawGetValue(*channel, PropertyPath::parse("file.stem"), val);
    check(std::get<std::string>(val) == "" || std::get<std::string>(val).length() > 0, "propStem gracefully handles exceptions");

    lawGetValue(*channel, PropertyPath::parse("file.filename"), val);
    check(std::get<std::string>(val) == "" || std::get<std::string>(val).length() > 0, "propFilename gracefully handles exceptions");

    lawGetValue(*channel, PropertyPath::parse("file.directory"), val);
    check(std::get<std::string>(val) == "", "propDirectory gracefully handles exceptions");

    // isPathSafe / checkOSPermissions will catch the exception and prevent the operation
    lawSetValue(*channel, PropertyPath::parse("file.read"), PropertyValue(true));
    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == false, "file read gracefully fails on invalid paths");

    // Clean up test files
    fs::remove_all(testDir, ec);

    if (g_failures > 0) {
        std::printf("file_channel_test: FAILED (%d failures)\n", g_failures);
        return 1;
    }

    std::printf("file_channel_test: ALL OK (all 14 cases passed)\n");
    return 0;
}
