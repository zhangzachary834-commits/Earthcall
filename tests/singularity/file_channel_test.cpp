#include "Singularity/Storage/FileChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"

#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cassert>
#include <string>

namespace fs = std::filesystem;
using namespace Singularity::Storage;

namespace {

int g_failures = 0;

void check(bool condition, const std::string& desc) {
    if (!condition) {
        std::printf("  FAILED: %s\n", desc.c_str());
        ++g_failures;
    } else {
        std::printf("  ok: %s\n", desc.c_str());
    }
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "file_channel_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(64, 64, "file_channel_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "file_channel_test: no window\n");
        glfwTerminate();
        return 1;
    }

    std::printf("Running file_channel_test...\n");

    LawManager laws;
    FileChannel::syncRegister(laws);
    FileChannel* channel = FileChannel::find(laws);
    check(channel != nullptr, "FileChannel first mover registered successfully");

    if (!channel) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // Prepare clean test directories under saves/
    fs::path testDir = fs::path("saves") / "test_file_channel_sandbox";
    std::error_code ec;
    fs::remove_all(testDir, ec);
    fs::create_directories(testDir, ec);

    // -----------------------------------------------------------------------
    // Case 1: Basic Write and Read
    // -----------------------------------------------------------------------
    fs::path testFile = testDir / "test_hello.txt";
    std::string testPath = testFile.string();
    std::string sampleText = "Earthcall Substrate File Channel Test\nNative File I/O Verified";

    // Set path and content
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(testPath));
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(sampleText));

    // Trigger write
    lawSetValue(*channel, PropertyPath::parse("file.write"), PropertyValue(true));

    PropertyValue val;
    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == true, "Write operation reported success");

    lawGetValue(*channel, PropertyPath::parse("file.exists"), val);
    check(std::get<bool>(val) == true, "file.exists returns true for written file");

    lawGetValue(*channel, PropertyPath::parse("file.bytesWritten"), val);
    check(std::get<double>(val) == static_cast<double>(sampleText.size()), "bytesWritten matches sample text size");

    // Clear content buffer in memory
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(std::string("")));

    // Trigger read
    lawSetValue(*channel, PropertyPath::parse("file.read"), PropertyValue(true));

    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == true, "Read operation reported success");

    lawGetValue(*channel, PropertyPath::parse("file.content"), val);
    check(std::get<std::string>(val) == sampleText, "Read content matches written sample text");

    lawGetValue(*channel, PropertyPath::parse("file.bytesRead"), val);
    check(std::get<double>(val) == static_cast<double>(sampleText.size()), "bytesRead matches sample text size");

    // -----------------------------------------------------------------------
    // Case 2: Parent Directory Creation Requirements
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
    // Case 3: OS Permission & System Path Write Restrictions
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
    // Case 4: Sandbox Mode & Path Traversal Prevention
    // -----------------------------------------------------------------------
    lawSetValue(*channel, PropertyPath::parse("file.sandboxMode"), PropertyValue(true));
    std::string traversalPath = "../../outside_sandbox.txt";
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(traversalPath));
    lawSetValue(*channel, PropertyPath::parse("file.write"), PropertyValue(true));

    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == false, "Path traversal escaping sandbox blocked in sandboxMode = true");

    // -----------------------------------------------------------------------
    // Case 5: Computed Properties (exists, isDirectory, isWritable)
    // -----------------------------------------------------------------------
    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(testDir.string()));
    lawGetValue(*channel, PropertyPath::parse("file.isDirectory"), val);
    check(std::get<bool>(val) == true, "file.isDirectory returns true for directory path");

    lawGetValue(*channel, PropertyPath::parse("file.isWritable"), val);
    check(std::get<bool>(val) == true, "file.isWritable returns true for writable directory path");

    // Clean up test files
    fs::remove_all(testDir, ec);

    glfwDestroyWindow(window);
    glfwTerminate();

    if (g_failures > 0) {
        std::printf("file_channel_test: FAILED (%d failures)\n", g_failures);
        return 1;
    }

    std::printf("file_channel_test: ALL OK\n");
    return 0;
}
