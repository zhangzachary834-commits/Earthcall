// Witness test for FirstMoverRegister boot initialization & disk reload path.
//
// Documenting witness gap:
// Existing unit tests in tests/person/first_mover_test.cpp tested FirstMoverRegister
// in memory with manual trustAuthenticatedPerson() calls or isolated instance objects,
// but never verified the real EngineInit.cpp boot sequence reading from disk via
// SaveSystem::firstMoverRegisterPath() + FirstMoverRegister::loadFromJson().
//
// This test exercises the exact production initialization path from EngineInit.cpp:
//   1. FirstMoverRegister::setSaveRoot(SaveSystem::rootPath())
//   2. Read and parse SaveSystem::firstMoverRegisterPath() ("first_movers.json")
//   3. FirstMoverRegister::loadFromJson(j)
//   4. Re-establish authenticated Person trust root (seedTrustedPersonRoot)
//   5. Assert that First Mover standing and permissions are correctly restored.

#include "Identity/FirstMoverRegister.hpp"
#include "Identity/KeyPair.hpp"
#include "Identity/KeyStore.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "json.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;
using namespace Identity;

int main() {
    const fs::path sandbox = fs::temp_directory_path() / "earthcall_first_mover_boot_test";
    fs::remove_all(sandbox);
    fs::create_directories(sandbox / "saves");

    SaveSystem::setSaveRoot((sandbox / "saves").string());

    // 1. Setup initial state: a Person and a granted First Mover Model
    auto& reg = FirstMoverRegister::instance();
    reg.clear();
    reg.clearAuthenticatedPersons();
    reg.setSaveRoot(sandbox / "saves");

    PrivateKey zachKey = PrivateKey::generate();
    PrivateKey modelKey = PrivateKey::generate();

    assert(reg.trustAuthenticatedPerson(zachKey));
    assert(reg.recognize(zachKey, FirstMover::Kind::Person, modelKey.id(), FirstMover::Kind::Model,
                         "Assistant Model", {"laws/assistant-*/**"}, 1000));

    // Save register to disk (creates saves/identity/first-movers.json or save root path)
    const std::string registerPath = SaveSystem::firstMoverRegisterPath();
    fs::create_directories(fs::path(registerPath).parent_path());
    std::ofstream out(registerPath);
    out << reg.toJson().dump(2);
    out.close();
    assert(fs::exists(registerPath));

    // 2. Simulate process restart: clear in-memory register state
    reg.clear();
    reg.clearAuthenticatedPersons();
    assert(reg.movers().empty());

    // 3. Perform EXACT EngineInit.cpp boot sequence for FirstMoverRegister
    reg.setSaveRoot(SaveSystem::rootPath());

    assert(fs::exists(registerPath));
    std::ifstream in(registerPath);
    nlohmann::json j = nlohmann::json::parse(in, nullptr, /*allow_exceptions=*/false);
    assert(!j.is_discarded());

    reg.loadFromJson(j);

    // Verify state immediately after loadFromJson (before Person authenticates)
    assert(reg.movers().size() == 1);
    assert(reg.standing(modelKey.id()) == Standing::GrantorNotAuthenticated);

    // 4. Authenticate Person at boot (simulating seedTrustedPersonRoot)
    assert(reg.trustAuthenticatedPerson(zachKey));

    // 5. Verify standing and write permissions are fully restored
    assert(reg.standing(modelKey.id()) == Standing::Recognized);
    assert(reg.mayWrite(modelKey.id(), sandbox / "saves" / "laws" / "assistant-1" / "law.ecform"));
    assert(!reg.mayWrite(modelKey.id(), sandbox / "saves" / "laws" / "other" / "law.ecform"));

    // Cleanup
    reg.clear();
    reg.clearAuthenticatedPersons();
    fs::remove_all(sandbox);

    std::cout << "first_mover_register_boot_test: ALL OK\n";
    return 0;
}
