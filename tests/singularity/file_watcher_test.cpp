#include "Singularity/Storage/FileWatcher.hpp"
#include "Singularity/Storage/VirtualFileSystem.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
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
    bool callbackStateMatches = true;
    bool callbackOnMainThread = true;
    const auto mainThread = std::this_thread::get_id();
    watcher->addCallback([&](const std::string& path, const std::string& type) {
        callbackOnMainThread &= std::this_thread::get_id() == mainThread;
        receivedPath = path;
        receivedType = type;
        PropertyValue currentPath, currentType;
        callbackStateMatches &= lawGetValue(*watcher, PropertyPath::parse("watcher.lastModifiedFile"), currentPath)
            && lawGetValue(*watcher, PropertyPath::parse("watcher.lastEventType"), currentType)
            && std::get<std::string>(currentPath) == path && std::get<std::string>(currentType) == type;
    });

    // Listen to EventBus for ECA::Event
    int ecaModifiedCount = 0;
    bool eventStateMatches = false;
    Core::EventBus::instance().subscribe<ECA::Event>([&](const ECA::Event& ev) {
        if (ev.type == "file-modified") {
            ++ecaModifiedCount;
            PropertyValue currentType;
            eventStateMatches = ev.subject == watcher && watcher->propLastModifiedFile() == fileA.lexically_normal().string()
                && lawGetValue(*watcher, PropertyPath::parse("watcher.lastEventType"), currentType)
                && std::get<std::string>(currentType) == ev.type;
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
    check(callbackStateMatches && eventStateMatches, "synchronous consumers observe the current edge properties");
    lawGetValue(*watcher, PropertyPath::parse("watcher.lastEventType"), val);
    check(std::get<std::string>(val) == "file-modified", "watcher.lastEventType is 'file-modified'");

    // Recursive watches cover ordinary nested files. An explicit check is
    // synchronous: the callback must have fired before the setter returns.
    fs::path nested = watchDir / "deep" / "branch" / "nested.txt";
    fs::create_directories(nested.parent_path(), ec);
    { std::ofstream out(nested); out << "nested"; }
    receivedType.clear();
    lawSetValue(*watcher, PropertyPath::parse("watcher.checkNow"), PropertyValue(true));
    check(receivedType == "file-created" && receivedPath == nested.lexically_normal().string(),
          "explicit check synchronously observes deep recursive creation");
    fs::remove(nested, ec);
    watcher->checkNow();

    // Re-selecting a watch path establishes a new silent baseline.
    receivedType.clear();
    lawSetValue(*watcher, PropertyPath::parse("watcher.watchPath"), PropertyValue(watchDir.string()));
    check(receivedType.empty(), "watch path rebaseline emits no creation callbacks");

    fs::path alternate = watchDir / "alternate";
    fs::create_directories(alternate, ec);
    { std::ofstream out(alternate / "existing.txt"); out << "already present"; }
    receivedType.clear();
    lawSetValue(*watcher, PropertyPath::parse("watcher.watchPath"), PropertyValue(alternate.string()));
    lawGetValue(*watcher, PropertyPath::parse("watcher.filesTracked"), val);
    check(receivedType.empty() && std::get<double>(val) == 1.0, "switching paths silently baselines the new path");
    fs::remove(alternate / "existing.txt", ec);
    fs::remove(alternate, ec);
    lawSetValue(*watcher, PropertyPath::parse("watcher.watchPath"), PropertyValue(watchDir.string()));

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
    lawGetValue(*watcher, PropertyPath::parse("watcher.filesTracked"), val);
    check(std::get<double>(val) == 2.0, "changing filter alone retains existing baseline until a scan");
    receivedType.clear();
    watcher->checkNow();
    check(receivedType == "file-deleted" && receivedPath == fileB.lexically_normal().string(),
          "next scan reports filtered-out baseline entry as deleted");
    watcher->rescanBaseline();

    lawGetValue(*watcher, PropertyPath::parse("watcher.filesTracked"), val);
    check(std::get<double>(val) == 1.0, "With .wgsl filter, only 1 file is tracked");

    // Case 5: exercise the runtime tick path and the authored checkNow trigger.
    receivedType.clear();
    receivedPath.clear();
    lawSetValue(*watcher, PropertyPath::parse("watcher.filterExtension"), PropertyValue(std::string("")));
    watcher->rescanBaseline();
    lawSetValue(*watcher, PropertyPath::parse("watcher.pollIntervalMs"), PropertyValue(100.0));

    // Establish a fresh poll timestamp before creating the file so this gate
    // assertion does not depend on time spent in earlier test cases.
    watcher->tick();
    receivedType.clear();
    receivedPath.clear();

    fs::path fileD = watchDir / "tick_test.txt";
    {
        std::ofstream d(fileD);
        d << "runtime tick test\\n";
    }

    watcher->tick();
    check(receivedType.empty(), "tick() is gated before pollIntervalMs elapses");
    std::this_thread::sleep_for(std::chrono::milliseconds(110));
    watcher->tick();
    for (int i = 0; i < 200 && receivedType.empty(); ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        watcher->tick();
    }
    check(receivedType == "file-created", "tick() detected file creation after poll interval");
    check(receivedPath == fileD.lexically_normal().string(), "tick() reported accurate file path");
    check(callbackOnMainThread, "periodic and synchronous callbacks run on the caller thread");

    receivedType.clear();
    receivedPath.clear();
    fs::remove(fileD, ec);
    lawSetValue(*watcher, PropertyPath::parse("watcher.checkNow"), PropertyValue(true));
    check(receivedType == "file-deleted", "watcher.checkNow property triggers immediate scan");

    // Case 6: a whole-Data-Home watch prunes integration workspace copies,
    // while save metadata and explicit sandbox watches remain observable.
    const auto previousRoot = SaveSystem::saveRoot();
    const fs::path isolatedRoot = fs::absolute(watchDir / "data-home");
    const fs::path authoredFile = isolatedRoot / "zones" / "witness.ecform";
    const fs::path sandboxFile =
        isolatedRoot / "gemini-mcp" / "sandboxes" / "trial" / "file.txt";
    const fs::path approvalFile =
        isolatedRoot / "gemini-mcp" / "approvals" / "approval.json";
    for (const auto& path : {authoredFile, sandboxFile, approvalFile}) {
        fs::create_directories(path.parent_path());
        std::ofstream(path) << "baseline";
    }
    SaveSystem::setSaveRoot(isolatedRoot.string());
    {
        FileWatcher dataWatcher;
        dataWatcher.rescanBaseline();
        lawGetValue(dataWatcher, PropertyPath::parse("watcher.filesTracked"), val);
        check(std::get<double>(val) == 2.0,
              "whole Data Home tracks authored files and approval metadata");

        int callbacks = 0;
        dataWatcher.addCallback([&](const std::string& path, const std::string& type) {
            ++callbacks;
            receivedPath = path;
            receivedType = type;
        });
        receivedPath.clear();
        receivedType.clear();
        std::ofstream(authoredFile, std::ios::app) << " authored edit";
        std::ofstream(sandboxFile, std::ios::app) << " sandbox edit";
        dataWatcher.checkNow();
        check(callbacks == 1 && receivedPath == authoredFile.string() &&
                  receivedType == "file-modified",
              "ordinary save edits publish while sandbox work stays outside the default scan");

        lawSetValue(dataWatcher, PropertyPath::parse("watcher.watchPath"),
                    PropertyValue(std::string("save://gemini-mcp/sandboxes")));
        callbacks = 0;
        receivedPath.clear();
        std::ofstream(sandboxFile, std::ios::app) << " explicit watch edit";
        dataWatcher.checkNow();
        check(callbacks == 1 && receivedPath == sandboxFile.string() &&
                  receivedType == "file-modified",
              "explicit sandbox watches retain change detection");
    }
    SaveSystem::setSaveRoot(previousRoot);

    // Clean up
    fs::remove_all(watchDir, ec);

    if (g_failures > 0) {
        std::printf("file_watcher_test: FAILED (%d failures)\n", g_failures);
        return 1;
    }

    std::printf("file_watcher_test: ALL OK (all 6 cases passed)\n");
    return 0;
}
