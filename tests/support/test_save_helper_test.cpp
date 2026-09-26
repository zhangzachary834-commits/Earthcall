#include "support/test_save_helper.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Person/Person.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <stdexcept>

int main() {
    std::cout << "Running test_save_helper_test...\n";

    Zone testWorld("test_world", "test");
    LawManager testLawManager;
    Soul soul("TestPlayer");
    Body body("humanoid", "default");
    Person testPlayer(std::move(soul), std::move(body), "default");

    auto tempDumps = std::filesystem::temp_directory_path() / "earthcall_test_dumps";

    // 1. Test empty/default root spellings redirect dumps to temp storage and restore exact root
    const auto absoluteDefault = std::filesystem::absolute("saves").lexically_normal().string();
    const std::vector<std::string> defaultSpellings = {
        "", "saves", "./saves", "saves/", "./saves/", "saves/.", "saves/./",
        "scratch/../saves", absoluteDefault
    };

    for (std::size_t i = 0; i < defaultSpellings.size(); ++i) {
        const auto& spelling = defaultSpellings[i];
        std::filesystem::remove_all(tempDumps);
        SaveSystem::setSaveRoot(spelling);
        const std::string testName = "test_dump_default_" + std::to_string(i);

        dump_test_save(testName, testWorld, testLawManager, testPlayer);

        // Assert nothing written to ./saves/tests/
        assert(!std::filesystem::exists("saves/tests/" + testName + ".json"));
        assert(!std::filesystem::exists("saves/tests/" + testName + ".ecform"));

        // Positively assert created under /tmp/earthcall_test_dumps/tests/
        assert(std::filesystem::exists(tempDumps / "tests" / (testName + ".json")) ||
               std::filesystem::exists(tempDumps / "tests" / (testName + ".ecform")));

        // Positively assert SaveSystem::saveRoot() restored to exact original spelling
        assert(SaveSystem::saveRoot() == spelling);
    }

    // 2. Test custom save root is honored and restored
    auto customRoot = std::filesystem::temp_directory_path() / "earthcall_custom_test_root";
    std::filesystem::remove_all(customRoot);
    SaveSystem::setSaveRoot(customRoot.string());
    dump_test_save("test_dump_custom", testWorld, testLawManager, testPlayer);
    assert(std::filesystem::exists(customRoot / "tests" / "test_dump_custom.json") ||
           std::filesystem::exists(customRoot / "tests" / "test_dump_custom.ecform"));
    assert(SaveSystem::saveRoot() == customRoot.string());
    std::filesystem::remove_all(customRoot);

    // A custom path that lexically collapses through ".." must remain custom;
    // only the actual authored saves path is the default. Keep this entirely in
    // the OS temp tree so the witness cannot create repo-relative artifacts.
    auto cwdLikeBase = std::filesystem::temp_directory_path() / "earthcall_cwd_like_root";
    auto cwdLikeChild = cwdLikeBase / "child";
    std::filesystem::remove_all(cwdLikeBase);
    std::filesystem::create_directories(cwdLikeChild);
    const std::string cwdLikeSpelling = (cwdLikeChild / "..").string();
    SaveSystem::setSaveRoot(cwdLikeSpelling);
    dump_test_save("test_dump_cwd_like", testWorld, testLawManager, testPlayer);
    assert(std::filesystem::exists(cwdLikeBase / "tests" / "test_dump_cwd_like.json") ||
           std::filesystem::exists(cwdLikeBase / "tests" / "test_dump_cwd_like.ecform"));
    assert(SaveSystem::saveRoot() == cwdLikeSpelling);
    std::filesystem::remove_all(cwdLikeBase);

    // 3. Test explicit filepathOverride is honored using temp root and causes no repo pollution
    SaveSystem::setSaveRoot(customRoot.string());
    auto explicitOverride = std::filesystem::temp_directory_path() / "earthcall_explicit_override" / "custom_dump.json";
    std::filesystem::remove_all(explicitOverride.parent_path());
    dump_test_save("test_dump_override", testWorld, testLawManager, testPlayer, explicitOverride.string());
    assert(std::filesystem::exists(explicitOverride) ||
           std::filesystem::exists(explicitOverride.parent_path() / "custom_dump.ecform"));
    assert(SaveSystem::saveRoot() == customRoot.string());
    assert(!std::filesystem::exists("custom_spelling_root"));
    std::filesystem::remove_all(explicitOverride.parent_path());

    // 4. Test the production RAII guard through the real helper. Inject a
    // failure only after dump_test_save() has switched the process-global
    // SaveSystem root, then prove the exact original spelling is restored.
    SaveSystem::setSaveRoot("saves/");
    {
        bool threw = false;
        try {
            dump_test_save(
                "test_failing_dump", testWorld, testLawManager, testPlayer, "",
                [] { throw std::runtime_error("intentional post-root-switch failure"); });
        } catch (const std::runtime_error&) {
            threw = true;
        }
        assert(threw);
        assert(SaveSystem::saveRoot() == "saves/");
    }

    SaveSystem::setSaveRoot("");
    std::cout << "test_save_helper_test: ALL OK\n";
    return 0;
}
