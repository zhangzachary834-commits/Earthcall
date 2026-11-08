// The Person's data home: where the save root lands, and that authored
// "saves/..." strings follow it there without their files being edited.
// Witness for SaveSystem::chooseDataRoot and VirtualFileSystem rule 8.
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/VirtualFileSystem.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

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

    // 4c. An entry the home already holds is the Person's, whatever it is
    //     named. Until 2026-11-04 a hard-coded list of showcase names was
    //     deleted and re-copied from the checkout whenever the root was chosen
    //     or Zones were listed, erasing in-app authoring. See
    //     docs/Agenda/Tasks/Specific Tasks/Serialization and Storage/
    //     Boot_Showcase_Overwrite/Boot_Showcase_Overwrite.md.
    {
        const fs::path liveHome = sandbox / "live";
        const fs::path liveCheckoutRoot = sandbox / "liveCheckout";
        const fs::path liveCheckout = liveCheckoutRoot / "saves";
        ::setenv("EARTHCALL_DATA", liveHome.c_str(), 1);
        const auto write = [&](const fs::path& p, const std::string& text) {
            fs::create_directories(p.parent_path(), ec);
            std::ofstream(p) << text;
        };
        const auto read = [](const fs::path& p) {
            std::ifstream in(p);
            return std::string((std::istreambuf_iterator<char>(in)), {});
        };
        const auto later = fs::file_time_type::clock::now() + std::chrono::hours(1);
        const auto writeNewer = [&](const fs::path& p, const std::string& text) {
            write(p, text);
            fs::last_write_time(p, later, ec);
        };

        write(liveHome / SaveSystem::kDataHomeMarker, "{}\n");
        const std::vector<fs::path> onTheOldList = {
            "zones/Northern Veil/zone.json",
            "zones/Sky-Island Sanctuary/zone.json",
            "zones/SynthesisStudio/zone.json",
            "materials/studio.A5/material.json",
            "materials/northern_veil.glacial_dais/material.json",
            "laws/law-sky-sanctuary-bell/law.json",
            "worlds/northern_veil.json",
        };
        for (const auto& rel : onTheOldList) {
            write(liveHome / rel, "{\"mine\":true}\n");
            writeNewer(liveCheckout / rel, "{\"snapshot\":true}\n");
        }
        write(liveHome / "zones/Northern Veil/only-in-home.txt", "keep\n");
        writeNewer(liveCheckout / "zones/Brand New/zone.json", "{\"new\":true}\n");

        // The Person edited this after the checkout: not an undelivered update.
        write(liveCheckout / "zones/Person Edited/zone.json", "{\"snapshot\":true}\n");
        write(liveHome / "zones/Person Edited/zone.json", "{\"mine\":true}\n");
        fs::last_write_time(liveCheckout / "zones/Person Edited/zone.json",
                            later - std::chrono::hours(24), ec);
        // Newer but byte-identical: nothing is waiting to be delivered.
        write(liveHome / "zones/Same Bytes/zone.json", "{\"same\":true}\n");
        writeNewer(liveCheckout / "zones/Same Bytes/zone.json", "{\"same\":true}\n");

        std::string liveNote;
        chosen = SaveSystem::chooseDataRoot(liveCheckout, liveNote);
        check(chosen == liveHome, "the marked home is chosen");
        bool allKept = true;
        for (const auto& rel : onTheOldList) {
            if (read(liveHome / rel).find("mine") == std::string::npos) {
                allKept = false;
                std::printf("    overwritten: %s\n", rel.string().c_str());
            }
        }
        check(allKept, "choosing the root keeps every existing entry the Person holds");
        check(fs::is_regular_file(liveHome / "zones/Northern Veil/only-in-home.txt", ec),
              "a file only the home has survives");
        check(fs::is_regular_file(liveHome / "zones/Brand New/zone.json", ec),
              "an identity the home lacks is still seeded");

        // The update that used to arrive by erasure must not vanish silently.
        check(liveNote.find("7 checkout identities") != std::string::npos,
              "the note counts the newer checkout entries that were not installed");
        check(liveNote.find("zones/Northern Veil") != std::string::npos,
              "the note names a newer checkout Zone that was not installed");
        check(liveNote.find("Person Edited") == std::string::npos,
              "an entry edited after the checkout is not reported");
        check(liveNote.find("Same Bytes") == std::string::npos,
              "a byte-identical entry is not reported");
        check(liveNote.find("Brand New") == std::string::npos,
              "a freshly seeded entry is not reported");

        // Desktop litter next to the identities is not an identity.
        writeNewer(liveCheckout / "zones/.DS_Store", "finder\n");
        write(liveHome / "zones/.DS_Store", "other\n");
        writeNewer(liveCheckout / "materials/.DS_Store", "finder\n");
        chosen = SaveSystem::chooseDataRoot(liveCheckout, liveNote);
        check(liveNote.find(".DS_Store") == std::string::npos, "dotfiles are never reported");
        check(!fs::exists(liveHome / "materials/.DS_Store", ec), "dotfiles are never seeded");

        // A long report stays readable.
        for (int i = 0; i < 12; ++i) {
            const fs::path rel = fs::path("materials") / ("flood." + std::to_string(i)) / "material.json";
            write(liveHome / rel, "{\"mine\":true}\n");
            writeNewer(liveCheckout / rel, "{\"snapshot\":true}\n");
        }
        chosen = SaveSystem::chooseDataRoot(liveCheckout, liveNote);
        check(liveNote.find("19 checkout identities") != std::string::npos,
              "the note counts every undelivered entry");
        check(liveNote.find("11 more") != std::string::npos,
              "the note lists a few entries and counts the rest");

        // Listing Zones re-runs the seed mid-session, from a cwd holding saves/.
        std::error_code cwdEc;
        const fs::path originalCwd = fs::current_path(cwdEc);
        fs::current_path(liveCheckoutRoot, cwdEc);
        SaveSystem::setSaveRoot(liveHome.string());
        (void)SaveSystem::listZoneIdentityRecords();
        SaveSystem::setSaveRoot("");
        fs::current_path(originalCwd, cwdEc);
        check(read(liveHome / "zones/Northern Veil/zone.json").find("mine") != std::string::npos,
              "listing Zones keeps the Person's copy too");
        ::setenv("EARTHCALL_DATA", home.c_str(), 1);
    }

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
