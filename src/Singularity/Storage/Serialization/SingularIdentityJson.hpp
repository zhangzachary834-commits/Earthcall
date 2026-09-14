#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include "json.hpp"

#include <stdexcept>
#include <string>

namespace Singularity::Storage {

inline void writeSingularIdentity(nlohmann::json& json, const Singular& being) {
    if (being.hasPersistableSingularId()) {
        json["singularId"] = being.singularId().toString();
    }
}

// Legacy records legitimately have no SingularId and retain the freshly
// minted in-memory identity until an explicit preservation-first migration.
// An explicit malformed field is never treated as legacy absence.
inline void readSingularIdentity(const nlohmann::json& json, Singular& being) {
    if (!json.contains("singularId")) {
        being.markLegacyIdentityUnpersisted();
        return;
    }
    if (!json["singularId"].is_string()) {
        throw std::invalid_argument("singularId must be a canonical string");
    }
    const std::string text = json["singularId"].get<std::string>();
    const Identity::SingularId id = Identity::SingularId::parse(text);
    if (!id.isValid() || id.toString() != text || !being.restoreSingularId(id)) {
        throw std::invalid_argument("invalid or noncanonical singularId");
    }
}

} // namespace Singularity::Storage
