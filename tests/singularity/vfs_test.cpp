#include "Singularity/Storage/VirtualFileSystem.hpp"
#include "Singularity/Storage/FileChannel.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"

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
    std::fflush(stdout);
}

} // namespace

int main() {
    std::printf("Running vfs_test...\n");
    std::fflush(stdout);

    LawManager laws;
    VirtualFileSystem::syncRegister(laws);
    FileChannel::syncRegister(laws);

    VirtualFileSystem* vfs = VirtualFileSystem::find(laws);
    FileChannel* channel = FileChannel::find(laws);

    check(vfs != nullptr, "VirtualFileSystem first mover registered successfully");
    check(channel != nullptr, "FileChannel registered successfully");

    if (!vfs || !channel) {
        return 1;
    }

    // -----------------------------------------------------------------------
    // Case 1: Standard URI Resolution (save://, zone://, home://, recording://)
    // -----------------------------------------------------------------------
    std::string saveRes = VirtualFileSystem::resolve("save://worlds/my_world.json");
    check(saveRes.find("worlds/my_world.json") != std::string::npos, "save:// resolved to worlds path");

    std::string zoneRes = VirtualFileSystem::resolve("zone://Sanctum/zone.json");
    check(zoneRes.find("zones/Sanctum/zone.json") != std::string::npos, "zone:// resolved to zones path");

    std::string homeRes = VirtualFileSystem::resolve("home://zachary/home.json");
    check(homeRes.find("homes/zachary/home.json") != std::string::npos, "home:// resolved to homes path");

    std::string recRes = VirtualFileSystem::resolve("recording://clip.ppm");
    check(recRes.find("recordings/clip.ppm") != std::string::npos, "recording:// resolved to recordings path");

    std::string fileRes = VirtualFileSystem::resolve("file:///tmp/earthcall.txt");
    check(fileRes == "/tmp/earthcall.txt", "file:// scheme stripped cleanly");

    // -----------------------------------------------------------------------
    // Case 2: Custom Mount Table
    // -----------------------------------------------------------------------
    vfs->mount("assets://", "saves/assets/");
    std::string assetRes = VirtualFileSystem::resolve("assets://models/tree.obj");
    check(assetRes.find("saves/assets/models/tree.obj") != std::string::npos, "Custom mount assets:// resolved correctly");

    std::string mountTable = vfs->getMountTableString();
    check(mountTable.find("assets:// -> saves/assets/") != std::string::npos, "getMountTableString includes assets://");

    vfs->unmount("assets://");
    std::string unmountedRes = VirtualFileSystem::resolve("assets://models/tree.obj");
    check(unmountedRes == "assets://models/tree.obj", "Unmounted prefix returns unchanged uri");

    // -----------------------------------------------------------------------
    // Case 3: In-Memory Virtual Files (memory://)
    // -----------------------------------------------------------------------
    std::string memUri = "memory://test_shader.wgsl";
    std::string memContent = "@vertex fn vs() -> vec4f { return vec4f(0); }";

    bool writeMemOk = vfs->writeMemoryFile(memUri, memContent);
    check(writeMemOk == true, "writeMemoryFile succeeded");
    check(vfs->memoryFileExists(memUri) == true, "memoryFileExists returns true");
    check(vfs->memoryFileSize(memUri) == memContent.size(), "memoryFileSize reports accurate byte count");

    std::string readBack;
    bool readMemOk = vfs->readMemoryFile(memUri, readBack);
    check(readMemOk == true && readBack == memContent, "readMemoryFile returns exact payload");

    // Memory metrics
    PropertyValue val;
    lawGetValue(*vfs, PropertyPath::parse("vfs.memoryFilesCount"), val);
    check(std::get<double>(val) >= 1.0, "vfs.memoryFilesCount reports at least 1 file");

    lawGetValue(*vfs, PropertyPath::parse("vfs.memoryBytesAllocated"), val);
    check(std::get<double>(val) >= static_cast<double>(memContent.size()), "vfs.memoryBytesAllocated reports byte size");

    // -----------------------------------------------------------------------
    // Case 4: FileChannel Integration with Virtual Paths
    // -----------------------------------------------------------------------
    // Writing and reading memory:// via FileChannel
    std::string channelMemUri = "memory://channel_draft.json";
    std::string channelJson = "{\"earthcall\":true,\"vfs\":true}";

    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(channelMemUri));
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(channelJson));
    lawSetValue(*channel, PropertyPath::parse("file.write"), PropertyValue(true));

    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == true, "FileChannel write to memory:// succeeded");

    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(std::string("")));
    lawSetValue(*channel, PropertyPath::parse("file.read"), PropertyValue(true));

    lawGetValue(*channel, PropertyPath::parse("file.content"), val);
    check(std::get<std::string>(val) == channelJson, "FileChannel read from memory:// retrieved written data");

    lawGetValue(*channel, PropertyPath::parse("file.jsonValid"), val);
    check(std::get<bool>(val) == true, "FileChannel jsonValid operates transparently over memory:// file");

    // Writing to save:// via FileChannel
    fs::path testSandbox = fs::path("saves") / "test_vfs_sandbox";
    std::error_code ec;
    fs::remove_all(testSandbox, ec);
    fs::create_directories(testSandbox, ec);

    std::string vfsDiskFile = "save://test_vfs_sandbox/note.txt";
    std::string noteText = "Universal VFS addressing verified";

    lawSetValue(*channel, PropertyPath::parse("file.path"), PropertyValue(vfsDiskFile));
    lawSetValue(*channel, PropertyPath::parse("file.content"), PropertyValue(noteText));
    lawSetValue(*channel, PropertyPath::parse("file.write"), PropertyValue(true));

    lawGetValue(*channel, PropertyPath::parse("file.lastOperationSuccess"), val);
    check(std::get<bool>(val) == true, "FileChannel write via save:// URI succeeded");

    fs::path expectedPhysical = testSandbox / "note.txt";
    check(fs::exists(expectedPhysical), "Physical file created on disk at expected save:// resolution");

    // Clean up
    fs::remove_all(testSandbox, ec);
    vfs->clearMemoryFiles();

    if (g_failures > 0) {
        std::printf("vfs_test: FAILED (%d failures)\n", g_failures);
        return 1;
    }

    std::printf("vfs_test: ALL OK (all 4 cases passed)\n");
    return 0;
}
