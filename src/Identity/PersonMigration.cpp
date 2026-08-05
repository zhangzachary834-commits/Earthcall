#include "Identity/PersonMigration.hpp"

#include "Identity/Claim.hpp"

#include <algorithm>
#include <set>
#include <sstream>

namespace Identity {

namespace {

constexpr int kMigrationVersion = 1;
constexpr const char* kMarker = "identityMigration";

std::string stringField(const nlohmann::json& j, const char* key) {
    auto it = j.find(key);
    if (it == j.end() || !it->is_string()) return {};
    return it->get<std::string>();
}

} // namespace

std::string MigrationReport::summary() const {
    std::ostringstream os;
    if (!ran) {
        os << "already migrated; nothing to do";
        return os.str();
    }
    os << identitiesMinted << " identity(ies) minted, " << identitiesReused
       << " reused from ledger; " << ownersRewritten << " owner(s), "
       << deletabilityRewritten << " deletability entr(ies), " << lawAuthorsRewritten
       << " law author(s) rewritten";
    for (const auto& w : warnings) os << "\n  warning: " << w;
    return os.str();
}

bool isMigrated(const nlohmann::json& save) {
    auto it = save.find(kMarker);
    return it != save.end() && it->is_object();
}

std::vector<std::string> discoverPersonNames(const nlohmann::json& save,
                                             const IdentityLedger& ledger) {
    std::set<std::string> names;

    auto zones = save.find("zones");
    if (zones != save.end() && zones->is_array()) {
        for (const auto& z : *zones) {
            if (!z.is_object()) continue;

            // Zone ownership is a Person by definition.
            const std::string owner = stringField(z, "owner");
            if (!owner.empty()) names.insert(owner);

            // Deletability is keyed per Person.
            auto del = z.find("deletable");
            if (del != z.end() && del->is_object()) {
                for (auto it = del->begin(); it != del->end(); ++it) {
                    if (!it.key().empty()) names.insert(it.key());
                }
            }
        }
    }

    // A name already established as a Person stays one, so law authorship can
    // be migrated for it without guessing.
    for (const auto& [name, id] : ledger.entries()) {
        (void)id;
        names.insert(name);
    }

    return std::vector<std::string>(names.begin(), names.end());
}

namespace {

// Rewrite law author/target lists, but only for names we know are Persons.
int rewriteLawAuthors(nlohmann::json& save,
                      const std::map<std::string, std::string>& nameToId,
                      std::vector<std::string>& warnings) {
    int rewritten = 0;

    auto migrateList = [&](nlohmann::json& list) {
        if (!list.is_array()) return;
        for (auto& entry : list) {
            if (!entry.is_string()) continue;
            auto it = nameToId.find(entry.get<std::string>());
            if (it == nameToId.end()) continue; // not a known Person: leave alone
            entry = it->second;
            ++rewritten;
        }
    };

    auto authored = save.find("authoredLaws");
    if (authored == save.end() || !authored->is_object()) return 0;

    auto laws = authored->find("laws");
    if (laws == authored->end() || !laws->is_array()) return 0;

    for (auto& law : *laws) {
        if (!law.is_object()) continue;
        // Only authors. targets[] and conditionSubjects[] address beings the
        // law acts ON, which are usually Objects -- rewriting a coincidental
        // name collision there would repoint the law at nothing.
        auto authors = law.find("authors");
        if (authors != law.end()) migrateList(*authors);
    }

    if (rewritten > 0) {
        warnings.push_back(
            "law authors were rewritten by name match; verify the Law Author window "
            "shows each law still authored");
    }
    return rewritten;
}

} // namespace

MigrationReport migrateSave(nlohmann::json& save,
                            IdentityLedger& ledger,
                            KeyStore& keys,
                            const std::string& passphrase,
                            int64_t at) {
    MigrationReport report;

    if (!save.is_object()) {
        report.warnings.push_back("save is not an object; nothing migrated");
        return report;
    }
    if (isMigrated(save)) return report; // ran stays false

    report.ran = true;

    const std::vector<std::string> names = discoverPersonNames(save, ledger);

    // Resolve every Person name to an identity first, so a failure part-way
    // does not leave the save half-rewritten.
    std::map<std::string, std::string> nameToId;
    std::map<std::string, SingularId> nameToKeyId;
    for (const auto& name : names) {
        const bool known = ledger.find(name).has_value();
        auto id = ledger.resolveOrMint(name, keys, passphrase);
        if (!id) {
            report.warnings.push_back("could not mint an identity for '" + name +
                                      "'; its references were left as-is");
            continue;
        }
        nameToId[name] = id->toString();
        nameToKeyId[name] = *id;
        if (known) {
            ++report.identitiesReused;
        } else {
            ++report.identitiesMinted;
        }
        report.personNames.push_back(name);
    }

    // Rewrite zone ownership into signed claims.
    auto zones = save.find("zones");
    if (zones != save.end() && zones->is_array()) {
        for (auto& z : *zones) {
            if (!z.is_object()) continue;

            const std::string owner = stringField(z, "owner");
            auto owned = nameToId.find(owner);
            if (!owner.empty() && owned != nameToId.end()) {
                const SingularId ownerId = nameToKeyId[owner];

                // The zone needs a stable identity of its own to be the subject
                // of the claim. Zones were addressed by name too, so mint one
                // and keep the name as a label.
                SingularId zoneId = SingularId::parse(stringField(z, "zoneId"));
                if (!zoneId.isValid()) {
                    zoneId = SingularId::mintOpaque();
                    z["zoneId"] = zoneId.toString();
                }

                auto key = keys.load(ownerId, passphrase);
                if (!key) {
                    report.warnings.push_back(
                        "owner '" + owner + "' could not be unsealed; ownership of zone '" +
                        stringField(z, "name") + "' left unsigned");
                } else {
                    // THE trust-on-first-migration moment: the world's existing
                    // assertion is signed over to the freshly minted identity.
                    Claim ownsIt = Claim::issue(*key, zoneId, "owns", ownerId, at);
                    z["ownerId"] = ownerId.toString();
                    z["ownerClaim"] = ownsIt.toJson();
                    // The bare string goes. Leaving it would preserve exactly
                    // the forgeable path this migration exists to remove.
                    z.erase("owner");
                    ++report.ownersRewritten;
                }
            }

            auto del = z.find("deletable");
            if (del != z.end() && del->is_object()) {
                nlohmann::json rebuilt = nlohmann::json::object();
                for (auto it = del->begin(); it != del->end(); ++it) {
                    auto mapped = nameToId.find(it.key());
                    if (mapped == nameToId.end()) {
                        rebuilt[it.key()] = it.value(); // unknown: leave untouched
                    } else {
                        rebuilt[mapped->second] = it.value();
                        ++report.deletabilityRewritten;
                    }
                }
                *del = rebuilt;
            }
        }
    }

    report.lawAuthorsRewritten = rewriteLawAuthors(save, nameToId, report.warnings);

    // The marker makes migration idempotent and records what was trusted.
    nlohmann::json migratedNames = nlohmann::json::object();
    for (const auto& [name, id] : nameToId) migratedNames[name] = id;

    save[kMarker] = {
        {"version", kMigrationVersion},
        {"at", at},
        {"trustedNames", migratedNames},
        {"note",
         "Legacy string identifiers were taken at their word once, here, and "
         "signed over to minted keypairs. Ownership now requires a verifiable "
         "claim; editing ownerId or ownerClaim invalidates it."},
    };

    ledger.save();
    return report;
}

std::vector<std::string> verifyOwnership(const nlohmann::json& save) {
    std::vector<std::string> broken;

    auto zones = save.find("zones");
    if (zones == save.end() || !zones->is_array()) return broken;

    for (const auto& z : *zones) {
        if (!z.is_object()) continue;

        auto claimIt = z.find("ownerClaim");
        if (claimIt == z.end()) continue; // unowned, or never migrated

        const std::string zoneName = stringField(z, "name");
        Claim claim = Claim::fromJson(*claimIt);

        if (!claim.verify()) {
            broken.push_back(zoneName);
            continue;
        }

        // A valid signature over the wrong subject is still wrong: the claim
        // must be about THIS zone and name THIS owner.
        const SingularId zoneId = SingularId::parse(stringField(z, "zoneId"));
        const SingularId ownerId = SingularId::parse(stringField(z, "ownerId"));
        if (!zoneId.isValid() || claim.subject() != zoneId ||
            !ownerId.canAuthenticate() || claim.issuer() != ownerId ||
            claim.predicate() != "owns") {
            broken.push_back(zoneName);
        }
    }
    return broken;
}

} // namespace Identity
