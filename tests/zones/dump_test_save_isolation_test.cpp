#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Person/Person.hpp"
#include "support/test_save_helper.hpp"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << '\n';
    } else {
        std::cout << "  ok: " << description << '\n';
    }
}

bool hasFileWithStem(const std::filesystem::path& dir, const std::string& stem) {
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec)) return false;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (entry.is_regular_file() && entry.path().stem() == stem) return true;
    }
    return false;
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running dump_test_save isolation test...\n";
    std::cout << "============================================================\n";

    // 1. Semantics 1 & 2: When SaveSystem::saveRoot() is empty or default ("saves"),
    // dump_test_save redirects writes to OS temp `earthcall_test_dumps` tree and
    // restores prior saveRoot afterwards.
    SaveSystem::setSaveRoot("");
    const std::string priorRootEmpty = SaveSystem::saveRoot();

    Zone testWorld1("dump_iso_1", "default");
    LawManager lawManager1;
    Soul soul1("Tester1");
    Body body1("humanoid", "default");
    Person player1(std::move(soul1), std::move(body1), "default");

    std::filesystem::path realSavesTests = std::filesystem::current_path() / "saves" / "tests";
    std::error_code ec;

    dump_test_save("dump_iso_test_1", testWorld1, lawManager1, player1);

    check(SaveSystem::saveRoot() == priorRootEmpty, "Prior empty save root restored exactly after dump_test_save");
    check(!hasFileWithStem(realSavesTests, "dump_iso_test_1"), "dump_test_save did not write into repository saves/ tree");

    std::filesystem::path tmpDumpsFolder1 = std::filesystem::temp_directory_path() / "earthcall_test_dumps" / "tests";
    check(hasFileWithStem(tmpDumpsFolder1, "dump_iso_test_1"), "dump_test_save wrote to OS temp earthcall_test_dumps tree");

    // Also test when SaveSystem::saveRoot() is explicitly "saves"
    SaveSystem::setSaveRoot("saves");
    const std::string priorRootDefault = SaveSystem::saveRoot();

    Zone testWorld2("dump_iso_2", "default");
    LawManager lawManager2;
    Soul soul2("Tester2");
    Body body2("humanoid", "default");
    Person player2(std::move(soul2), std::move(body2), "default");

    dump_test_save("dump_iso_test_2", testWorld2, lawManager2, player2);

    check(SaveSystem::saveRoot() == priorRootDefault, "Prior 'saves' save root restored exactly after dump_test_save");
    check(!hasFileWithStem(realSavesTests, "dump_iso_test_2"), "dump_test_save with 'saves' root did not write into repository saves/ tree");

    check(hasFileWithStem(tmpDumpsFolder1, "dump_iso_test_2"), "dump_test_save with 'saves' root wrote to OS temp earthcall_test_dumps tree");

    // 3. Semantics 3: If a non-default explicit save root is already configured,
    // dump_test_save respects it instead of redirecting elsewhere.
    const auto customSandbox = std::filesystem::temp_directory_path() / "earthcall_custom_save_root_sandbox";
    std::filesystem::remove_all(customSandbox, ec);
    std::filesystem::create_directories(customSandbox, ec);

    SaveSystem::setSaveRoot(customSandbox.string());
    const std::string priorCustomRoot = SaveSystem::saveRoot();

    Zone testWorld3("dump_iso_3", "default");
    LawManager lawManager3;
    Soul soul3("Tester3");
    Body body3("humanoid", "default");
    Person player3(std::move(soul3), std::move(body3), "default");

    dump_test_save("dump_iso_test_3", testWorld3, lawManager3, player3);

    check(SaveSystem::saveRoot() == priorCustomRoot, "Prior custom save root restored exactly after dump_test_save");

    std::filesystem::path customDumpsFolder = customSandbox / "tests";
    check(hasFileWithStem(customDumpsFolder, "dump_iso_test_3"), "dump_test_save respected explicit custom save root");

    std::filesystem::remove_all(customSandbox, ec);
    std::filesystem::remove_all(std::filesystem::temp_directory_path() / "earthcall_test_dumps", ec);
    SaveSystem::setSaveRoot("");

    std::cout << "============================================================\n";
    std::cout << (g_failures == 0 ? "PASS" : "FAIL") << ": " << g_checks
              << " checks, " << g_failures << " failure(s)\n";
    std::cout << "============================================================\n";
    return g_failures == 0 ? 0 : 1;
}
