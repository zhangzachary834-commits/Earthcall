#include "Singularity/Storage/FileWatcher.hpp"
#include "Singularity/Storage/VirtualFileSystem.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"

#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cassert>
#include <string>
#include <thread>
#include <chrono>

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
    std::printf("Running file_watcher_test...\n");
    std::fflush(stdout);

    LawManager laws;
    VirtualFileSystem::syncRegister(laws);
    FileWatcher::syncRegister(laws);

    FileWatcher* watcher = FileWatcher::find(laws);
    check(watcher != nullptr, "FileWatcher first mover registered successfully");

    if (!watcher) return 1;

    // Prepare clean sandbox directory for watching
    fs::path watchDir = fs::path("saves") / "test_watcher_sandbox";
    std::error_code ec;
    fs::remove_all(watchDir, ec);
    fs::create_directories(watchDir, ec);

    // Seed with two baseline files
    fs::path fileA = watchDir / "shader.wgsl";
    fs::path fileB = watchDir / "rules.json";
    {
        std::ofstream a(fileA);
        a << "@vertex fn vs() -> vec4f { return vec4f(0); }\n";
        std::ofstream b(fileB);
        b << "{\"version\": 1}\n";
    }

    // Configure FileWatcher to watch the sandbox
    lawSetValue(*watcher, PropertyPath::parse("watcher.watchPath"), PropertyValue(watchDir.string()));
    lawSetValue(*watcher, PropertyPath::parse("watcher.pollIntervalMs"), PropertyValue(50.0));

    PropertyValue val;
    lawGetValue(*watcher, PropertyPath::parse("watcher.filesTracked"), val);
    check(std::get<double>(val) == 2.0, "FileWatcher initialized baseline tracking 2 files");

    // -----------------------------------------------------------------------
    // Case 1: Detect File Modification ("file-modified" edge event)
    // -----------------------------------------------------------------------
    std::string receivedPath;
    std::string receivedType;
    watcher->addCallback([&](const std::string& path, const std::string& type) {
        receivedPath = path;
        receivedType = type;
    });

    // Listen to EventBus for ECA::Event
    int ecaModifiedCount = 0;
    Core::EventBus::instance().subscribe<ECA::Event>([&](const ECA::Event& ev) {
        if (ev.type == "file-modified") {
            ++ecaModifiedCount;
        }
    });

    // Sleep 15ms so filesystem timestamp shifts reliably
    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    // Modify fileA
    {
        std::ofstream a(fileA, std::ios::app);
        a << "// live hot-reloaded comment\n";
    }

    watcher->checkNow();

    check(receivedType == "file-modified", "Hot-reload callback caught 'file-modified' event");
    check(receivedPath == fileA.lexically_normal().string(), "Callback reported accurate modified filepath");
    check(ecaModifiedCount == 1, "EventBus delivered ECA::Event('file-modified') on transition edge");

    lawGetValue(*watcher, PropertyPath::parse("watcher.lastEventType"), val);
    check(std::get<std::string>(val) == "file-modified", "watcher.lastEventType is 'file-modified'");

    // -----------------------------------------------------------------------
    // Case 2: Detect New File Creation ("file-created" event)
    // -----------------------------------------------------------------------
    receivedType.clear();
    receivedPath.clear();
    fs::path fileC = watchDir / "new_model.obj";
    {
        std::ofstream c(fileC);
        c << "v 0 0 0\nv 1 1 1\n";
    }

    watcher->checkNow();

    check(receivedType == "file-created", "FileWatcher detected 'file-created' edge event");
    check(receivedPath == fileC.lexically_normal().string(), "Reported new created file path accurately");

    lawGetValue(*watcher, PropertyPath::parse("watcher.filesTracked"), val);
    check(std::get<double>(val) == 3.0, "watcher.filesTracked increased to 3 files");

    // -----------------------------------------------------------------------
    // Case 3: Detect File Deletion ("file-deleted" event)
    // -----------------------------------------------------------------------
    receivedType.clear();
    receivedPath.clear();
    fs::remove(fileC, ec);

    watcher->checkNow();

    check(receivedType == "file-deleted", "FileWatcher detected 'file-deleted' event");
    check(receivedPath == fileC.lexically_normal().string(), "Reported deleted file path accurately");

    lawGetValue(*watcher, PropertyPath::parse("watcher.filesTracked"), val);
    check(std::get<double>(val) == 2.0, "watcher.filesTracked decremented back to 2 files");

    // -----------------------------------------------------------------------
    // Case 4: Extension Filter (.wgsl only)
    // -----------------------------------------------------------------------
    lawSetValue(*watcher, PropertyPath::parse("watcher.filterExtension"), PropertyValue(std::string(".wgsl")));
    watcher->rescanBaseline();

    lawGetValue(*watcher, PropertyPath::parse("watcher.filesTracked"), val);
    check(std::get<double>(val) == 1.0, "With .wgsl filter, only 1 file is tracked");

    // Clean up
    fs::remove_all(watchDir, ec);

    if (g_failures > 0) {
        std::printf("file_watcher_test: FAILED (%d failures)\n", g_failures);
        return 1;
    }

    std::printf("file_watcher_test: ALL OK (all 4 cases passed)\n");
    return 0;
}
