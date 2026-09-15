// Regression witness for Zach's 2026-09-14 report that the Perlin Noise Floor
// hills had disappeared. The Zone identity had survived only as
// ShapeKind::Field while its authored SDF tree + extent were missing, so
// Object::from_json produced a Field shell with hasField()==false and the
// renderer had nothing truthful to draw.
//
// This test never writes the authored save. It proves the repaired Zone file
// contains a real Field payload, that the ordinary Object reader rehydrates it
// as a Field, and that the current writer will preserve it on the next save.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "json.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace {
int checks = 0;
int failures = 0;
void check(bool ok, const std::string& message) {
    ++checks;
    std::cout << (ok ? "  ok: " : "  FAILED: ") << message << '\n';
    if (!ok) ++failures;
}
bool near(float a, float b, float eps = 1e-4f) { return std::fabs(a-b) < eps; }
}

int main() {
    std::filesystem::path path = "saves/zones/NoiseFloorWorld/zone.json";
    if (!std::filesystem::exists(path)) path = std::filesystem::path("..") / path;
    check(std::filesystem::exists(path), "restored NoiseFloorWorld Zone identity exists");
    if (!std::filesystem::exists(path)) return 1;

    std::ifstream in(path);
    nlohmann::json zone;
    in >> zone;

    const nlohmann::json* ground = nullptr;
    for (const auto& object : zone["world"]["objects"]) {
        if (object.value("objectID", std::string{}) == "perlin-ground-plane") {
            ground = &object;
            break;
        }
    }
    check(ground != nullptr, "Perlin ground being is present");
    if (!ground) return 1;

    check(ground->value("shapeKind", -1) == static_cast<int>(Object::ShapeKind::Field),
          "Perlin ground keeps Field topology");
    check(ground->contains("field") && (*ground)["field"].is_object(),
          "Zone identity carries the authored SDF tree, not a bare Field shell");
    check(ground->contains("fieldExtent") && (*ground)["fieldExtent"].is_array() &&
              (*ground)["fieldExtent"].size() >= 3,
          "Zone identity carries the Field evaluation extent");
    if (ground->contains("fieldExtent") && (*ground)["fieldExtent"].is_array() &&
        (*ground)["fieldExtent"].size() >= 3) {
        check(near((*ground)["fieldExtent"][0].get<float>(), 1000.0f) &&
              near((*ground)["fieldExtent"][1].get<float>(), 30.0f) &&
              near((*ground)["fieldExtent"][2].get<float>(), 1000.0f),
              "restored extent is the authored 1000 x 30 x 1000 hills domain");
    }

    Object hydrated;
    from_json(*ground, hydrated);
    check(hydrated.hasField(),
          "ordinary Object deserialization restores _hasField=true (renderer can manifest it)");
    const auto extent = hydrated.getFieldExtent();
    check(near(extent.x, 1000.0f) && near(extent.y, 30.0f) && near(extent.z, 1000.0f),
          "hydrated Object retains the authored Field extent");

    nlohmann::json roundTrip;
    to_json(roundTrip, hydrated);
    check(roundTrip.contains("field") && roundTrip.contains("fieldExtent"),
          "current Object writer preserves the SDF payload on the next Zone save");

    std::cout << checks - failures << "/" << checks << " checks passed\n";
    return failures == 0 ? 0 : 1;
}
