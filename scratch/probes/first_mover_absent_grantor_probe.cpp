#include "Identity/FirstMoverRegister.hpp"
#include "Identity/KeyPair.hpp"

#include <filesystem>
#include <iostream>

int main() {
    using namespace Identity;

    const auto root = std::filesystem::temp_directory_path() /
                      "earthcall_fm_absent_grantor_probe";
    std::filesystem::create_directories(root / "fixtures");

    PrivateKey model = PrivateKey::generate();
    PrivateKey absentGrantor = PrivateKey::generate();

    FirstMover forged;
    forged.id = model.id();
    forged.kind = FirstMover::Kind::Model;
    forged.displayName = "untrusted-model";
    forged.grantedBy = absentGrantor.id();
    forged.scopes = {"fixtures/**"};
    forged.grant = Claim::issue(absentGrantor, forged.id,
                                forged.grantPredicate(), absentGrantor.id(), 1000);

    FirstMoverRegister loaded;
    loaded.setSaveRoot(root);
    loaded.loadFromJson(nlohmann::json{{"movers", {forged.toJson()}}});

    const auto target = root / "fixtures" / "seed.ecsave";
    const bool allowed = loaded.mayWrite(model.id(), target);
    std::cout << loaded.explain(model.id(), target) << '\n';

    std::filesystem::remove_all(root);
    return allowed ? 0 : 1;
}
