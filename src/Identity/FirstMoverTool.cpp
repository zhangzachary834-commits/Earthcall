// earthcall_first_mover -- a Person recognises a First Mover, and a mover
// proves it is itself.
//
// Recognition is a covenant, not a file write (FIRST_MOVER_AUTHORING.md 8d),
// so the register is changed only here, by a Person who unlocks their own key
// in front of it. No engine path and no mover scope can write
// saves/identity/first-movers.json.
//
//   earthcall_first_mover mint --name "Claude Sonnet 4.5"
//       Mint a mover keypair, sealed in the KeyStore (~/.earthcall/identity)
//       under EARTHCALL_MOVER_PASSPHRASE. Prints the mover id.
//
//   earthcall_first_mover grant --mover <id> --name <label> --scope <glob>...
//       The Person grants standing. EARTHCALL_KEY_PASSPHRASE must unlock the
//       Person's key (the single keyed profile in saves/persons, or the one
//       EARTHCALL_PERSON_ID names). Re-granting replaces the scopes.
//
//   earthcall_first_mover revoke --mover <id>
//       The granting Person withdraws standing.
//
//   earthcall_first_mover list
//       Every mover, its scopes, and whether it would stand if its Person
//       authenticated (quarantine is reported; nothing is hidden).
//
//   earthcall_first_mover sign-challenge --mover <id> --challenge-id <c>
//                                        --nonce <n> --connection <k>
//       The MCP bridge's signer: builds the session transcript itself (it
//       will sign nothing else) and prints the hex signature. Needs
//       EARTHCALL_MOVER_PASSPHRASE. The private key never leaves this process.
//
// Every command accepts --saves <dir> (default: ./saves).
//
// Built 2026-09-24 by Claude Opus 5.5 at Zach's request, implementing phases
// 5.3 and 7 of Sol's docs/plans/MCP_FIRST_MOVER_GOVERNANCE_IMPLEMENTATION_PLAN_
// 2026-09-18.md, so Claude Sonnet 4.5 could be granted standing to act in
// Earthcall through MCP.
#include "Identity/FirstMoverRegister.hpp"
#include "Identity/KeyPair.hpp"
#include "Identity/KeyStore.hpp"
#include "Singularity/Storage/SaveSystem.hpp"

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <optional>
#include <set>

namespace fs = std::filesystem;
using namespace Identity;

namespace {

struct Args {
    std::string command;
    std::map<std::string, std::string> one;
    std::vector<std::string> scopes;
};

int usage() {
    std::cerr <<
        "usage:\n"
        "  earthcall_first_mover mint   --name <label>\n"
        "  earthcall_first_mover grant  --mover <id> --name <label> --scope <glob> [--scope <glob>...]\n"
        "                               [--kind model|person] [--person <person-id>]\n"
        "  earthcall_first_mover revoke --mover <id> [--person <person-id>]\n"
        "  earthcall_first_mover list\n"
        "  earthcall_first_mover sign-challenge --mover <id> --challenge-id <c> --nonce <n> --connection <k>\n"
        "  (all accept --saves <dir>, default ./saves)\n\n"
        "  EARTHCALL_KEY_PASSPHRASE    unlocks the granting Person's key (grant, revoke)\n"
        "  EARTHCALL_MOVER_PASSPHRASE  seals / unlocks the mover's key (mint, sign-challenge)\n";
    return 2;
}

std::optional<Args> parse(int argc, char** argv) {
    if (argc < 2) return std::nullopt;
    Args a;
    a.command = argv[1];
    for (int i = 2; i < argc; ++i) {
        const std::string k = argv[i];
        if (k.rfind("--", 0) != 0 || i + 1 >= argc) return std::nullopt;
        const std::string v = argv[++i];
        if (k == "--scope") a.scopes.push_back(v);
        else a.one[k.substr(2)] = v;
    }
    return a;
}

std::string get(const Args& a, const char* key) {
    auto it = a.one.find(key);
    return it == a.one.end() ? std::string{} : it->second;
}

const char* env(const char* name) {
    const char* v = std::getenv(name);
    return (v && *v) ? v : nullptr;
}

fs::path savesDir(const Args& a) {
    const std::string s = get(a, "saves");
    std::error_code ec;
    fs::path p = fs::absolute(s.empty() ? "saves" : s, ec);
    return p;
}

// The Person whose key must unlock: explicit, or the single keyed profile.
// Never guessed among several.
std::optional<SingularId> personId(const Args& a) {
    std::string chosen = get(a, "person");
    if (chosen.empty() && env("EARTHCALL_PERSON_ID")) chosen = env("EARTHCALL_PERSON_ID");
    if (!chosen.empty()) {
        auto id = SingularId::parse(chosen);
        if (!id.canAuthenticate()) return std::nullopt;
        return id;
    }
    std::vector<SingularId> keyed;
    for (const auto& info : SaveSystem::listWorlds(SaveSystem::SaveType::PERSON)) {
        auto profile = SaveSystem::readSaveData(info.path);
        if (!profile.is_object() || !profile.contains("personId") || !profile["personId"].is_string()) continue;
        auto id = SingularId::parse(profile["personId"].get<std::string>());
        if (id.canAuthenticate()) keyed.push_back(id);
    }
    if (keyed.size() == 1) return keyed.front();
    if (keyed.empty()) {
        std::cerr << "No keyed Person profile in saves/persons. A Person gets a key once, on purpose:\n"
                     "  EARTHCALL_MIGRATE_PERSON_IDENTITY=1 EARTHCALL_KEY_PASSPHRASE=... ./Earthcall.command\n";
    } else {
        std::cerr << "Several keyed Person profiles; pass --person <id> (refusing to guess).\n";
    }
    return std::nullopt;
}

// Unlock the Person's key and seat them as the register's trusted root.
std::optional<PrivateKey> authenticatePerson(const Args& a, FirstMoverRegister& reg) {
    const char* pass = env("EARTHCALL_KEY_PASSPHRASE");
    if (!pass) {
        std::cerr << "refused: EARTHCALL_KEY_PASSPHRASE is not set; only the Person can grant.\n";
        return std::nullopt;
    }
    auto id = personId(a);
    if (!id) return std::nullopt;
    KeyStore keys;
    auto key = keys.load(*id, pass);
    if (!key || key->id() != *id) {
        std::cerr << "refused: the Person key " << id->abbreviated()
                  << " did not unlock with EARTHCALL_KEY_PASSPHRASE.\n";
        return std::nullopt;
    }
    reg.trustAuthenticatedPerson(*key);
    return key;
}

nlohmann::json readRegister(const fs::path& path) {
    std::error_code ec;
    if (!fs::exists(path, ec)) return nlohmann::json{{"movers", nlohmann::json::array()}};
    std::ifstream in(path);
    auto j = nlohmann::json::parse(in, nullptr, false);
    if (j.is_discarded()) throw std::runtime_error("register is not valid JSON: " + path.string());
    return j;
}

std::set<std::string> moverIds(const nlohmann::json& j) {
    std::set<std::string> out;
    if (j.contains("movers") && j["movers"].is_array()) {
        for (const auto& m : j["movers"]) {
            if (m.is_object() && m.contains("id") && m["id"].is_string()) out.insert(m["id"].get<std::string>());
        }
    }
    return out;
}

// Patch, never regenerate (CLAUDE.md, Save files are sacred): the new file is
// the old register plus one edit. Stage it, prove no other mover was erased,
// keep the old file as a backup, then rename atomically.
bool writeRegister(const fs::path& path, const nlohmann::json& before, const nlohmann::json& after,
                   const std::string& removedId) {
    std::set<std::string> expected = moverIds(before);
    if (!removedId.empty()) expected.erase(removedId);
    const std::set<std::string> got = moverIds(after);
    for (const auto& id : expected) {
        if (!got.count(id)) {
            std::cerr << "refused: the edit would erase mover " << id << "; nothing written.\n";
            return false;
        }
    }
    std::error_code ec;
    fs::create_directories(path.parent_path(), ec);
    const fs::path staged = path.string() + ".staged";
    {
        std::ofstream out(staged, std::ios::trunc);
        out << after.dump(2) << "\n";
        if (!out) {
            std::cerr << "refused: could not stage " << staged << "\n";
            return false;
        }
    }
    if (moverIds(readRegister(staged)) != got) {
        std::cerr << "refused: staged register did not read back identically.\n";
        fs::remove(staged, ec);
        return false;
    }
    if (fs::exists(path, ec)) {
        const auto stamp = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        const fs::path backup = path.parent_path().parent_path() / "backups" /
                                ("first-movers-" + std::to_string(stamp) + ".json");
        fs::create_directories(backup.parent_path(), ec);
        fs::copy_file(path, backup, fs::copy_options::overwrite_existing, ec);
        if (ec) {
            std::cerr << "refused: could not back up the current register: " << ec.message() << "\n";
            fs::remove(staged, ec);
            return false;
        }
        std::cout << "backed up previous register to " << backup.string() << "\n";
    }
    fs::rename(staged, path, ec);
    if (ec) {
        std::cerr << "refused: atomic rename failed: " << ec.message() << "\n";
        return false;
    }
    return true;
}

int64_t nowSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

int cmdMint(const Args& a) {
    const char* pass = env("EARTHCALL_MOVER_PASSPHRASE");
    if (!pass) {
        std::cerr << "refused: EARTHCALL_MOVER_PASSPHRASE is not set; the mover's key is sealed under it.\n";
        return 1;
    }
    const std::string name = get(a, "name");
    PrivateKey key = PrivateKey::generate();
    KeyStore keys;
    if (!keys.store(key, pass)) {
        std::cerr << "failed: KeyStore refused to store the key.\n";
        return 1;
    }
    const std::string id = key.id().toString();
    std::cout << id << "\n";
    std::cerr << "Minted First Mover key" << (name.empty() ? "" : " for '" + name + "'") << ".\n"
              << "  sealed in " << keys.directory().string() << " (never in saves/)\n"
              << "  it has NO standing until a Person grants it:\n"
              << "    EARTHCALL_KEY_PASSPHRASE=... earthcall_first_mover grant --mover " << id
              << " --name \"" << name << "\" --scope \"laws/**\" ...\n"
              << "  so the MCP bridge can sign without a plaintext passphrase, store it in the macOS Keychain:\n"
              << "    security add-generic-password -s earthcall-first-mover -a " << id << " -w\n";
    return 0;
}

int cmdGrant(const Args& a, bool revoke) {
    const auto mover = SingularId::parse(get(a, "mover"));
    if (!mover.canAuthenticate()) {
        std::cerr << "refused: --mover must be a cryptographic mover id (from `mint`).\n";
        return 1;
    }
    FirstMoverRegister reg;
    reg.setSaveRoot(savesDir(a));
    auto person = authenticatePerson(a, reg);
    if (!person) return 1;

    const fs::path path = SaveSystem::firstMoverRegisterPath();
    const nlohmann::json before = readRegister(path);
    reg.loadFromJson(before);

    if (revoke) {
        if (!reg.revoke(*person, mover)) {
            std::cerr << "refused: " << mover.abbreviated()
                      << " is not registered, or was granted by a different Person.\n";
            return 1;
        }
    } else {
        const std::string kind = get(a, "kind");
        const auto moverKind = (kind == "person") ? FirstMover::Kind::Person : FirstMover::Kind::Model;
        if (a.scopes.empty()) {
            std::cerr << "note: no --scope given; the mover will be recognised but may change nothing.\n";
        }
        if (!reg.recognize(*person, FirstMover::Kind::Person, mover, moverKind,
                           get(a, "name"), a.scopes, nowSeconds())) {
            std::cerr << "refused: recognition failed (self-attestation, or an unauthenticated grantor).\n";
            return 1;
        }
    }

    if (!writeRegister(path, before, reg.toJson(), revoke ? mover.toString() : std::string{})) return 1;
    std::cout << (revoke ? "Revoked " : "Granted ") << mover.abbreviated() << " in " << path
              << ", authored by Person " << person->id().abbreviated() << ".\n";
    if (!revoke) {
        for (const auto& s : a.scopes) std::cout << "  scope: " << s << "\n";
        std::cout << "It stands only in sessions where this Person unlocks their key at boot "
                     "(EARTHCALL_KEY_PASSPHRASE).\n";
    }
    return 0;
}

int cmdList(const Args& a) {
    FirstMoverRegister reg;
    reg.setSaveRoot(savesDir(a));
    if (env("EARTHCALL_KEY_PASSPHRASE")) (void)authenticatePerson(a, reg);
    reg.loadFromJson(readRegister(SaveSystem::firstMoverRegisterPath()));
    if (reg.movers().empty()) {
        std::cout << "(no First Movers registered)\n";
        return 0;
    }
    for (const auto& m : reg.movers()) {
        std::cout << m->displayName << "  " << m->id.toString() << "\n"
                  << "  kind: " << m->propKind() << "   granted by: " << m->grantedBy.abbreviated() << "\n"
                  << "  standing: " << standingCode(reg.standing(m->id))
                  << (reg.isQuarantined(m->id) ? "  (QUARANTINED)" : "") << "\n";
        for (const auto& s : m->scopes) std::cout << "  scope: " << s << "\n";
    }
    return 0;
}

int cmdSign(const Args& a) {
    const char* pass = env("EARTHCALL_MOVER_PASSPHRASE");
    const auto mover = SingularId::parse(get(a, "mover"));
    const std::string challengeId = get(a, "challenge-id");
    const std::string nonce = get(a, "nonce");
    const std::string connection = get(a, "connection");
    if (!pass || !mover.canAuthenticate() || challengeId.empty() || nonce.empty() || connection.empty()) {
        std::cerr << "refused: sign-challenge needs EARTHCALL_MOVER_PASSPHRASE, --mover, "
                     "--challenge-id, --nonce and --connection.\n";
        return 1;
    }
    KeyStore keys;
    auto key = keys.load(mover, pass);
    if (!key || key->id() != mover) {
        std::cerr << "refused: mover key did not unlock.\n";
        return 1;
    }
    std::cout << hexEncode(key->sign(foreignSessionTranscript(challengeId, nonce, connection, mover))) << "\n";
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    auto args = parse(argc, argv);
    if (!args) return usage();
    SaveSystem::setSaveRoot(savesDir(*args).string());
    try {
        if (args->command == "mint") return cmdMint(*args);
        if (args->command == "grant") return cmdGrant(*args, false);
        if (args->command == "revoke") return cmdGrant(*args, true);
        if (args->command == "list") return cmdList(*args);
        if (args->command == "sign-challenge") return cmdSign(*args);
    } catch (const std::exception& e) {
        std::cerr << "failed: " << e.what() << "\n";
        return 1;
    }
    return usage();
}
