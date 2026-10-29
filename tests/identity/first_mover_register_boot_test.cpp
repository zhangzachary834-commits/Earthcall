// Disk round-trip witness for the First Mover Register's boot-load contract.
// This mirrors EngineInit's register-file read and trust-root requirements;
// it is NOT an end-to-end EngineInit/KeyStore-unlock test.

#include "Identity/FirstMoverRegister.hpp"
#include "Identity/KeyPair.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "json.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using namespace Identity;

namespace {
void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}

// Never delete a pre-existing fixed /tmp directory or touch authored saves.
struct IsolatedSaveRoot {
    fs::path path;
    std::string previous = SaveSystem::saveRoot();

    IsolatedSaveRoot() {
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
        const fs::path temp = fs::temp_directory_path();
        for (int attempt = 0; attempt < 64; ++attempt) {
            const fs::path candidate =
                temp / ("earthcall-first-mover-boot-" + std::to_string(stamp) +
                        "-" + std::to_string(attempt));
            if (fs::create_directory(candidate)) {
                path = candidate;
                SaveSystem::setSaveRoot(path.string());
                return;
            }
        }
        throw std::runtime_error("cannot create a fresh isolated save root");
    }

    ~IsolatedSaveRoot() {
        SaveSystem::setSaveRoot(previous);
        if (!path.empty()) {
            std::error_code ec;
            fs::remove_all(path, ec);
        }
    }
};
} // namespace

int main() {
    try {
        IsolatedSaveRoot isolated;
        auto& reg = FirstMoverRegister::instance();
        reg.clear();
        reg.clearAuthenticatedPersons();
        reg.clearActiveMover();
        reg.setSaveRoot(SaveSystem::rootPath());

        const PrivateKey person = PrivateKey::generate();
        const PrivateKey model = PrivateKey::generate();
        require(person.isValid() && model.isValid(), "Ed25519 key generation failed");
        require(reg.trustAuthenticatedPerson(person), "Person root not authenticated");
        require(reg.recognize(person, FirstMover::Kind::Person,
                              model.id(), FirstMover::Kind::Model,
                              "Test model", {"laws/assistant-*/**"}, 1000),
                "signed model grant not created");

        const fs::path registerPath = SaveSystem::firstMoverRegisterPath();
        require(registerPath.lexically_normal() ==
                    (isolated.path / FirstMoverRegister::kRegisterFile).lexically_normal(),
                "register path escaped the isolated save root");
        fs::create_directories(registerPath.parent_path());
        {
            std::ofstream out(registerPath, std::ios::trunc);
            require(out.is_open(), "cannot open register file for writing");
            out << reg.toJson().dump(2);
            out.flush();
            require(out.good(), "register file write failed");
        }
        require(fs::is_regular_file(registerPath), "register file not persisted");

        // Simulate a fresh process: serialized grants must not carry runtime trust.
        reg.clear();
        reg.clearAuthenticatedPersons();
        require(reg.movers().empty(), "old register entries survived reset");
        reg.setSaveRoot(SaveSystem::rootPath());

        nlohmann::json saved;
        {
            std::ifstream in(SaveSystem::firstMoverRegisterPath());
            require(in.is_open(), "boot path did not find register file");
            saved = nlohmann::json::parse(in, nullptr, false);
        }
        require(!saved.is_discarded(), "boot path could not parse register JSON");
        reg.loadFromJson(saved);
        require(reg.movers().size() == 1, "boot path did not restore mover");
        require(reg.standing(model.id()) == Standing::GrantorNotAuthenticated,
                "disk file improperly authenticated its own Person root");

        const fs::path allowed = isolated.path / "laws" / "assistant-1" / "law.ecform";
        const fs::path forbidden = isolated.path / "laws" / "unrelated" / "law.ecform";
        require(!reg.mayWrite(model.id(), allowed),
                "untrusted persisted grant authorized a write");

        // Production seeds an unlocked Person root before loading the register.
        // Re-load with the root present to exercise that ordering too.
        require(reg.trustAuthenticatedPerson(person), "Person reauthentication failed");
        reg.loadFromJson(saved);
        require(reg.standing(model.id()) == Standing::Recognized,
                "signed standing not restored after Person authentication");
        require(reg.mayWrite(model.id(), allowed), "granted write not restored");
        require(!reg.mayWrite(model.id(), forbidden), "out-of-scope write allowed");
        require(!reg.mayWrite(model.id(), isolated.path / "identity" / "first-movers.json"),
                "First Mover could overwrite the protected register");

        // Persisted scope tampering must fail closed even with the Person present.
        auto tampered = saved;
        tampered["movers"][0]["scopes"] = nlohmann::json::array({"**"});
        reg.loadFromJson(tampered);
        require(!reg.mayWrite(model.id(), allowed),
                "tampered serialized scope remained authorized");

        reg.clear();
        reg.clearAuthenticatedPersons();
        reg.clearActiveMover();
        std::cout << "first_mover_register_boot_test: ALL OK\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "first_mover_register_boot_test: FAILED: " << e.what() << '\n';
        return 1;
    }
}
