#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

[[noreturn]] void fail(const char* expression, const char* file, int line) {
    std::cerr << "CHECK failed: " << expression << " at " << file << ':' << line << '\n';
    std::exit(EXIT_FAILURE);
}

#define CHECK(expr) do { if (!(expr)) fail(#expr, __FILE__, __LINE__); } while (false)

void objectIdentityRoundTripsBesideSlug() {
    Object source("zone-piece");
    const Identity::SingularId original = source.singularId();

    nlohmann::json json = source;
    CHECK(json.at("objectID") == "zone-piece");
    CHECK(json.at("singularId") == original.toString());

    Object loaded("temporary-slug");
    from_json(json, loaded);
    CHECK(loaded.singularId() == original);
    CHECK(loaded.getIdentifier() == "zone-piece");

    loaded.setObjectID("renamed-law-address");
    CHECK(loaded.getIdentifier() == "renamed-law-address");
    CHECK(loaded.singularId() == original);
}

void materialIdentityRoundTripsAndLegacyStillReads() {
    Material source("clay");
    const Identity::SingularId original = source.singularId();
    nlohmann::json json = source.toJson();
    Material loaded = Material::fromJson(json);
    CHECK(loaded.singularId() == original);
    CHECK(loaded.getIdentifier() == "material.clay");

    json.erase("singularId");
    Material legacy = Material::fromJson(json);
    CHECK(legacy.singularId().isValid());
    CHECK(legacy.singularId() != original);
    CHECK(legacy.getIdentifier() == "material.clay");
    CHECK(!legacy.hasPersistableSingularId());
    CHECK(!legacy.toJson().contains("singularId"));
}

void explicitBadIdentityRefuses() {
    nlohmann::json json = Material("clay").toJson();
    json["singularId"] = "ec1:not-canonical";
    bool refused = false;
    try {
        (void)Material::fromJson(json);
    } catch (const std::invalid_argument&) {
        refused = true;
    }
    CHECK(refused);
}

void copyAndMoveHaveBeingSemantics() {
    Material original("clay");
    Material copied(original);
    CHECK(copied.singularId() != original.singularId());

    const Identity::SingularId copiedId = copied.singularId();
    Material moved(std::move(copied));
    CHECK(moved.singularId() == copiedId);

    Material assigned("target");
    const Identity::SingularId targetId = assigned.singularId();
    assigned = original;
    CHECK(assigned.singularId() == targetId);
}

void singularIdPropertyIsDiscoverableAndReadOnly() {
    Material material("clay");
    Property* property = material.findProperty("singularId");
    CHECK(property != nullptr);
    CHECK(std::get<std::string>(property->value()) == material.singularId().toString());
    CHECK(!property->setValue(std::string("ec1:aaaaaaaaaaaaaaaaaaaaaaaaaa")));
}

} // namespace

int main() {
    objectIdentityRoundTripsBesideSlug();
    materialIdentityRoundTripsAndLegacyStillReads();
    explicitBadIdentityRefuses();
    copyAndMoveHaveBeingSemantics();
    singularIdPropertyIsDiscoverableAndReadOnly();
    std::cout << "singular_persistence_id_test: all checks passed\n";
    return EXIT_SUCCESS;
}
