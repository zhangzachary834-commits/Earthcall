// migrate_identities — turn a name-addressed world into an identity-bearing one.
//
// Deliberately a tool you run, not a step that happens on load. Migration is
// the one moment where legacy string identifiers are taken at their word and
// signed over to freshly minted keypairs (PersonMigration.hpp, TRUST BOUNDARY),
// and a decision like that should be made on purpose, once, with the report in
// front of you — not silently, every time a world opens.
//
//   migrate_identities <save.json|save.ecsave> [--write]
//
// Without --write nothing is minted and nothing is touched: it only reports
// what migration would do.
#include "Identity/IdentityLedger.hpp"
#include "Identity/KeyStore.hpp"
#include "Identity/PersonMigration.hpp"
#include "Singularity/Storage/SaveSystem.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

int usage(const char* argv0) {
    std::cerr
        << "usage: " << argv0 << " <save-file> [--write]\n\n"
        << "  Reports what identity migration would do. With --write it mints\n"
        << "  keypairs, rewrites the save, and backs up the original first.\n\n"
        << "  --write requires EARTHCALL_KEY_PASSPHRASE to be set: the minted\n"
        << "  private keys are sealed under it, and a default would mean every\n"
        << "  installation shared one.\n";
    return 2;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) return usage(argv[0]);

    const std::string path = argv[1];
    bool write = false;
    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--write") {
            write = true;
        } else {
            std::cerr << "unknown argument: " << arg << "\n";
            return usage(argv[0]);
        }
    }

    if (!std::filesystem::exists(path)) {
        std::cerr << "no such save: " << path << "\n";
        return 1;
    }

    nlohmann::json save = SaveSystem::readSaveData(path);
    if (save.is_null()) {
        std::cerr << "could not read " << path << "\n";
        return 1;
    }

    Identity::IdentityLedger ledger;
    ledger.load();

    if (Identity::isMigrated(save)) {
        std::cout << path << " is already migrated.\n";
        auto broken = Identity::verifyOwnership(save);
        if (broken.empty()) {
            std::cout << "All ownership claims verify.\n";
            return 0;
        }
        std::cout << "OWNERSHIP DOES NOT VERIFY for " << broken.size() << " zone(s):\n";
        for (const auto& z : broken) std::cout << "  " << z << "\n";
        std::cout << "These were edited after migration, or signed by a key this\n"
                     "installation does not hold.\n";
        return 1;
    }

    const auto names = Identity::discoverPersonNames(save, ledger);
    std::cout << "Save: " << path << "\n"
              << "Person names found: " << names.size() << "\n";
    for (const auto& n : names) {
        auto existing = ledger.find(n);
        std::cout << "  " << n << "  ->  "
                  << (existing ? existing->abbreviated() + " (from ledger)"
                               : std::string("a new identity would be minted"))
                  << "\n";
    }

    if (!write) {
        std::cout << "\nDry run: nothing minted, nothing written.\n"
                  << "Re-run with --write to migrate.\n";
        return 0;
    }

    const char* pass = std::getenv("EARTHCALL_KEY_PASSPHRASE");
    if (!pass || !*pass) {
        std::cerr << "\nrefusing to migrate: EARTHCALL_KEY_PASSPHRASE is not set.\n"
                  << "The minted private keys are sealed under it.\n";
        return 1;
    }

    // Back up before rewriting. Migration is idempotent, but it mints keys and
    // rewrites ownership, and the original should stay recoverable.
    const std::string backup = path + ".pre-identity-backup";
    std::error_code ec;
    std::filesystem::copy_file(path, backup,
                               std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        std::cerr << "refusing to migrate: could not back up to " << backup << ": "
                  << ec.message() << "\n";
        return 1;
    }
    std::cout << "\nBacked up original to " << backup << "\n";

    Identity::KeyStore keys;
    Identity::MigrationReport report =
        Identity::migrateSave(save, ledger, keys, pass, std::time(nullptr));

    std::cout << report.summary() << "\n";

    auto broken = Identity::verifyOwnership(save);
    if (!broken.empty()) {
        std::cerr << "migration produced unverifiable ownership; NOT writing.\n";
        return 1;
    }

    // Write beside the original in plain JSON so the result is inspectable --
    // the whole point of this step is that a Person can read what it did.
    const std::string out = path + ".migrated.json";
    std::ofstream os(out, std::ios::trunc);
    if (!os.is_open()) {
        std::cerr << "could not write " << out << "\n";
        return 1;
    }
    os << save.dump(2) << "\n";
    os.close();

    std::cout << "Wrote " << out << "\n"
              << "Keys sealed in " << keys.directory() << "\n"
              << "Ledger at " << ledger.path() << "\n\n"
              << "Review the result, then replace the original when satisfied.\n";
    return 0;
}
