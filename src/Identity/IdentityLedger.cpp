#include "Identity/IdentityLedger.hpp"

#include "json.hpp"

#include <fstream>
#include <iostream>

#if !defined(_WIN32)
#include <sys/stat.h>
#endif

namespace Identity {

namespace {
constexpr int kFormatVersion = 1;
}

std::filesystem::path IdentityLedger::defaultPath() {
    // Beside the keys, for the same reason: never in the repo, never in saves/.
    return KeyStore::defaultDirectory() / "migration-ledger.json";
}

IdentityLedger::IdentityLedger(std::filesystem::path path) : _path(std::move(path)) {}

bool IdentityLedger::load() {
    _byName.clear();

    std::ifstream in(_path);
    if (!in.is_open()) return false; // absent is normal on a first run

    nlohmann::json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        std::cerr << "[IdentityLedger] malformed ledger at " << _path << ": " << e.what() << "\n";
        return false;
    }
    if (!j.is_object()) return false;

    auto entries = j.find("entries");
    if (entries == j.end() || !entries->is_object()) return false;

    for (auto it = entries->begin(); it != entries->end(); ++it) {
        if (!it.value().is_string()) continue;
        SingularId id = SingularId::parse(it.value().get<std::string>());
        // A malformed entry is dropped rather than repaired: guessing what a
        // corrupted identity mapping was meant to say is how one being
        // silently becomes another.
        if (id.canAuthenticate()) _byName[it.key()] = id;
    }
    return true;
}

bool IdentityLedger::save() const {
    std::error_code ec;
    std::filesystem::create_directories(_path.parent_path(), ec);

    nlohmann::json entries = nlohmann::json::object();
    for (const auto& [name, id] : _byName) entries[name] = id.toString();

    nlohmann::json j = {{"version", kFormatVersion}, {"entries", entries}};

    std::ofstream out(_path, std::ios::trunc);
    if (!out.is_open()) {
        std::cerr << "[IdentityLedger] cannot write " << _path << "\n";
        return false;
    }
    out << j.dump(2) << "\n";
    out.close();

#if !defined(_WIN32)
    // Not secret -- it holds only public ids -- but it decides which identity a
    // name resolves to, so it is not world-writable either.
    ::chmod(_path.c_str(), 0600);
#endif
    return true;
}

std::optional<SingularId> IdentityLedger::find(const std::string& legacyName) const {
    auto it = _byName.find(legacyName);
    if (it == _byName.end()) return std::nullopt;
    return it->second;
}

void IdentityLedger::record(const std::string& legacyName, const SingularId& id) {
    if (legacyName.empty() || !id.canAuthenticate()) return;
    _byName[legacyName] = id;
}

std::optional<SingularId> IdentityLedger::resolveOrMint(const std::string& legacyName,
                                                        KeyStore& keys,
                                                        const std::string& passphrase) {
    if (legacyName.empty()) return std::nullopt;

    if (auto existing = find(legacyName)) {
        // Only reuse it if we still hold the signing half. A ledger entry whose
        // key is gone would hand back an identity that can no longer sign, and
        // every claim made under it afterwards would fail to issue.
        if (keys.contains(*existing)) return existing;
        std::cerr << "[IdentityLedger] ledger names " << legacyName
                  << " but its key is missing from the store; refusing to remint\n";
        return std::nullopt;
    }

    PrivateKey key = PrivateKey::generate();
    if (!keys.store(key, passphrase)) {
        std::cerr << "[IdentityLedger] could not seal a key for " << legacyName << "\n";
        return std::nullopt;
    }

    const SingularId id = key.id();
    _byName[legacyName] = id;
    return id;
}

} // namespace Identity
