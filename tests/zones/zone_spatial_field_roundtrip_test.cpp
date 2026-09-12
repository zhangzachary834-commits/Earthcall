// End-to-end witness for the Zone's continuous mathematical substrate.
//
// A Zone has always owned a geom::FieldNode and admitted it to its Formation,
// but the identity store historically dropped that being at the save boundary.
// This test uses the real ZoneManager persistence/hydration path rather than a
// FieldNode-only round trip so the test cannot agree with itself while boot
// loses the field (ENGINEERING_DISCIPLINE.md: End-to-End Coherence).
//
// Zach's light-authoring direction is the first live consumer: an authored
// `light.source=true` on this FieldNode makes its registered `origin` the
// renderer's persistent world-space light position. EngineRender.cpp is the
// reader of that authored latch; FieldNode::{toJson,applyJson} and
// ZoneSerialization.cpp are its persistence writers/readers.

#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/SaveContext.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

bool nearf(float a, float b, float eps = 1e-4f) {
    return std::fabs(a - b) < eps;
}

std::shared_ptr<Zone> findZone(ZoneManager& mgr, const std::string& id) {
    for (auto& z : mgr.zones()) {
        if (z && z->getIdentifier() == id) return z;
    }
    return nullptr;
}

struct Harness {
    Soul soul;
    Body body;
    Person player;
    Core::Camera camera;
    MouseHandler mouse;
    LawManager laws;
    float color[3] = {1.0f, 1.0f, 1.0f};
    double worldTime = 0.0;
    SaveContext ctx;

    Harness()
        : soul("Player"),
          body("humanoid", "default"),
          player(std::move(soul), std::move(body), "default") {
        ctx.camera = &camera;
        ctx.mouseHandler = &mouse;
        ctx.currentColor = color;
        ctx.person = &player;
        ctx.lawManager = &laws;
        ctx.worldTime = &worldTime;
    }
};

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running Zone spatial FieldNode persistence test...\n";
    std::cout << "============================================================\n";

    const auto sandbox = std::filesystem::temp_directory_path()
        / "earthcall_zone_spatial_field_roundtrip";
    std::filesystem::remove_all(sandbox);
    std::filesystem::create_directories(sandbox / "worlds");
    SaveSystem::setSaveRoot(sandbox.string());

    Harness h;
    const std::string zoneId = "RadiantFieldZone";
    const glm::vec3 authoredOrigin(7.0f, 11.0f, 13.0f);

    auto authoredAst = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(
            OntoMath::ScalarForm::variable("x", 1.0, 3.0)));
    const std::string authoredAstJson = authoredAst.toJson().dump();

    {
        ZoneManager writer;
        auto zone = std::make_shared<Zone>(zoneId, "strict");
        auto* root = zone->spatialRoot();
        check(root != nullptr, "Zone owns a continuous FieldNode root");
        if (root) {
            root->origin = authoredOrigin;
            root->scale = glm::vec3(40.0f, 20.0f, 10.0f);
            root->field->baseDensity = 0.42f;
            root->field->frequency = 3.5f;
            root->field->amplitude = 0.75f;

            // Person/Law-authored vocabulary, not a new C++ Light kind.
            root->setDynamicProperty("light.source", PropertyValue(true));
            root->setDynamicProperty("light.intensity", PropertyValue(2.5f));

            check(PropertyPath::parse("field.ast").setValue(
                      *root, PropertyValue(authoredAstJson)) == PropertyPath::PathResult::Ok,
                  "the radiant field's OntoMath AST is authored through PropertyPath");
        }

        writer.addZone(zone);
        writer.persistZones();
    }

    check(std::filesystem::exists(
              sandbox / "zones" / zoneId / "zone.json"),
          "real Zone identity persistence writes the radiant Zone");

    {
        ZoneManager fresh;
        fresh.hydrateFromZoneStore();
        auto zone = findZone(fresh, zoneId);
        check(zone != nullptr, "fresh boot-style hydration restores the Zone");

        auto* root = zone ? zone->spatialRoot() : nullptr;
        check(root != nullptr, "fresh hydration restores the Zone's FieldNode substrate");
        if (root) {
            check(nearf(root->origin.x, authoredOrigin.x) &&
                      nearf(root->origin.y, authoredOrigin.y) &&
                      nearf(root->origin.z, authoredOrigin.z),
                  "FieldNode origin survives the real identity-store boundary");
            check(nearf(root->scale.x, 40.0f) && nearf(root->scale.y, 20.0f) &&
                      nearf(root->scale.z, 10.0f),
                  "FieldNode scale survives the real identity-store boundary");
            check(nearf(root->field->baseDensity, 0.42f) &&
                      nearf(root->field->frequency, 3.5f) &&
                      nearf(root->field->amplitude, 0.75f),
                  "ScalarField parameters survive the real identity-store boundary");

            PropertyValue source;
            check(root->getDynamicProperty("light.source", source) &&
                      std::get_if<bool>(&source) && *std::get_if<bool>(&source),
                  "authored light.source survives and remains bool-typed");

            PropertyValue intensity;
            check(root->getDynamicProperty("light.intensity", intensity) &&
                      std::get_if<float>(&intensity) &&
                      nearf(*std::get_if<float>(&intensity), 2.5f),
                  "other authored radiant-field vocabulary survives with its type");

            Property* ast = root->findProperty("field.ast");
            const auto* astText = ast ? std::get_if<std::string>(&ast->value()) : nullptr;
            bool sameAst = false;
            if (astText) {
                const auto expected = nlohmann::json::parse(authoredAstJson, nullptr, false);
                const auto actual = nlohmann::json::parse(*astText, nullptr, false);
                sameAst = !expected.is_discarded() && !actual.is_discarded()
                       && expected == actual;
            }
            check(sameAst,
                  "the Person-authored OntoMath AST survives save -> fresh hydration");
        }
    }

    std::filesystem::remove_all(sandbox);
    SaveSystem::setSaveRoot("");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures > 0) {
        std::cout << "zone_spatial_field_roundtrip_test: FAILED\n";
        return 1;
    }
    std::cout << "zone_spatial_field_roundtrip_test: ALL OK\n";
    return 0;
}
