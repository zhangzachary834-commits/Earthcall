// The Person's data home: where the save root lands, and that authored
// "saves/..." strings follow it there without their files being edited.
// Witness for SaveSystem::chooseDataRoot and VirtualFileSystem rule 8.
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/VirtualFileSystem.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

namespace fs = std::filesystem;
using Singularity::Storage::VirtualFileSystem;

namespace {

int g_failures = 0;

void check(bool condition, const std::string& desc) {
    std::printf("  %s: %s\n", condition ? "ok" : "FAILED", desc.c_str());
    if (!condition) ++g_failures;
    std::fflush(stdout);
}

std::string norm(const fs::path& p) { return p.lexically_normal().string(); }

} // namespace

int main() {
    std::printf("Running data_home_test...\n");

    std::error_code ec;
    const fs::path sandbox = fs::temp_directory_path(ec) /
        ("earthcall_data_home_test_" + std::to_string(std::rand()));
    fs::remove_all(sandbox, ec);
    const fs::path home = sandbox / "home";
    const fs::path checkout = sandbox / "checkout" / "saves";
    fs::create_directories(checkout, ec);
    ::setenv("EARTHCALL_DATA", home.c_str(), 1);

    // 1. The environment names the home.
    check(SaveSystem::platformDataHome() == home, "EARTHCALL_DATA names the data home");

    // 2. A home directory without the marker is never trusted over a checkout.
    fs::create_directories(home, ec);
    std::string note;
    fs::path chosen = SaveSystem::chooseDataRoot(checkout, note);
    check(chosen == checkout, "unmarked home loses to the checkout's saves/");
    check(note.find("not migrated") != std::string::npos, "the fallback says so loudly");

    // 3. The marker makes the home win.
    std::ofstream(home / SaveSystem::kDataHomeMarker) << "{}\n";
    chosen = SaveSystem::chooseDataRoot(checkout, note);
    check(chosen == home, "marked home wins over the checkout");

    // 4. An installed app (no checkout) creates and marks the home.
    const fs::path fresh = sandbox / "fresh";
    ::setenv("EARTHCALL_DATA", fresh.c_str(), 1);
    chosen = SaveSystem::chooseDataRoot({}, note);
    check(chosen == fresh, "no checkout: the data home is used");
    check(fs::is_regular_file(fresh / SaveSystem::kDataHomeMarker, ec),
          "first creation writes the marker");

    // 4b. An installed app's first run copies its Foundation in, never over
    //     anything, and versions the home from the start.
    const fs::path foundation = sandbox / "Foundation";
    fs::create_directories(foundation / "laws" / "welcome", ec);
    std::ofstream(foundation / "laws" / "welcome" / "law.json") << "{\"id\":\"welcome\"}\n";
    const fs::path seededHome = sandbox / "seeded";
    ::setenv("EARTHCALL_DATA", seededHome.c_str(), 1);
    chosen = SaveSystem::chooseDataRoot({}, note, foundation);
    check(chosen == seededHome, "first run with a Foundation uses the data home");
    check(fs::is_regular_file(seededHome / "laws" / "welcome" / "law.json", ec),
          "the Foundation is copied into the new home");
    check(fs::is_directory(seededHome / ".git", ec) ||
              note.find("git unavailable") != std::string::npos,
          "the new home is a git repository (or says why not)");
    std::ofstream(seededHome / "laws" / "welcome" / "law.json") << "{\"id\":\"mine\"}\n";
    chosen = SaveSystem::chooseDataRoot({}, note, foundation);
    std::ifstream kept(seededHome / "laws" / "welcome" / "law.json");
    std::string keptText((std::istreambuf_iterator<char>(kept)), {});
    check(keptText.find("mine") != std::string::npos,
          "a later launch never re-seeds over the Person's edits");

    // 5. Empty root is still "saves" under cwd (what every other test relies on).
    SaveSystem::setSaveRoot("");
    check(SaveSystem::rootPath() == fs::path("saves"), "empty root means ./saves");
    check(VirtualFileSystem::resolve("saves/worlds/a.json") == norm("saves/worlds/a.json"),
          "with no root set, saves/... resolves to itself");

    // 6. Authored checkout spelling follows the root; nothing else is captured.
    SaveSystem::setSaveRoot(home.string());
    check(VirtualFileSystem::resolve("saves/worlds/export.json") ==
              norm(home / "worlds" / "export.json"),
          "saves/worlds/export.json lands in the data home");
    check(VirtualFileSystem::resolve("saves/seed.ecgraph") == norm(home / "seed.ecgraph"),
          "saves/seed.ecgraph lands in the data home");
    check(VirtualFileSystem::resolve("saves") == norm(home), "bare saves is the root");
    check(VirtualFileSystem::resolve("save://laws/x/law.json") ==
              norm(home / "laws" / "x" / "law.json"),
          "save:// follows the root");
    check(VirtualFileSystem::resolve("savesX/a.json") == "savesX/a.json",
          "a sibling name that merely starts with 'saves' is untouched");
    check(VirtualFileSystem::resolve("docs/saves/a.json") == "docs/saves/a.json",
          "a nested saves component is untouched");
    check(VirtualFileSystem::resolve("/tmp/saves/a.json") == "/tmp/saves/a.json",
          "an absolute path is untouched");

    SaveSystem::setSaveRoot("");
    ::unsetenv("EARTHCALL_DATA");
    fs::remove_all(sandbox, ec);

    if (g_failures) {
        std::printf("data_home_test: %d FAILED\n", g_failures);
        return 1;
    }
    std::printf("data_home_test: all passed\n");
    return 0;
}
