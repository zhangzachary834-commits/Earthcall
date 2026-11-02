// Primordial Spinning Cube resurrection witness.
//
// This test guards the historical correction, not merely file presence:
//   * the resurrected being is a Field, never ShapeKind::Cube;
//   * its cubical form is authored OntoMath/SDF math;
//   * no six-slot faceColors payload is part of the authored record;
//   * its six visible colour regions come from Material::colorExpr;
//   * its spin is a persisted authored Law with three distinct authored rates.
//
// This is still a MACHINE witness. A Person must open the world and see the
// ancestor rotate; "a green suite is not a witness" remains in force.

#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <variant>

namespace {

int checks = 0;
int failures = 0;

void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) {
        ++failures;
        std::cout << "  FAILED: " << what << "\n";
    } else {
        std::cout << "  ok: " << what << "\n";
    }
}

std::map<std::string, PropertyValue> vars(double x, double y, double z, double t = 0.0) {
    return {
        {"x", PropertyValue(x)},
        {"y", PropertyValue(y)},
        {"z", PropertyValue(z)},
        {"p", PropertyValue(glm::vec3(static_cast<float>(x),
                                      static_cast<float>(y),
                                      static_cast<float>(z)))},
        {OntoMath::kTimeVar, PropertyValue(t)},
    };
}

bool evalColor(const OntoMath::Piecewise& pw, double x, double y, double z, glm::vec3& out) {
    const auto value = pw.evaluate(vars(x, y, z));
    if (!value) return false;
    const auto* colour = std::get_if<glm::vec3>(&*value);
    if (!colour) return false;
    out = *colour;
    return true;
}

bool near(float a, float b, float eps = 1e-4f) {
    return std::abs(a - b) <= eps;
}

bool near3(const glm::vec3& a, const glm::vec3& b, float eps = 1e-3f) {
    return near(a.x, b.x, eps) && near(a.y, b.y, eps) && near(a.z, b.z, eps);
}

} // namespace

int main() {
    namespace fs = std::filesystem;
    const fs::path root = fs::path(__FILE__).parent_path().parent_path().parent_path();
    const fs::path zonePath = root / "saves/zones/Primordial Spinning Cube/zone.json";
    const fs::path lawPath = root / "saves/laws/law-primordial-cube-spin/law.json";
    const fs::path worldPath = root / "saves/worlds/primordial_spinning_cube.json";

    std::cout << "============================================================\n";
    std::cout << "Primordial Spinning Cube — authored resurrection witness\n";
    std::cout << "============================================================\n";

    const auto zone = SaveSystem::readSaveData(zonePath.string());
    check(!zone.empty(), "the resurrected Zone exists and is readable");
    if (zone.empty()) return 1;

    check(zone.value("identifier", "") == "Primordial Spinning Cube",
          "Zone identity is the Primordial Spinning Cube");
    check(zone.value("owner", "") == "Zach",
          "the Zone records the commissioning Person as owner");
    check(zone.contains("lawRefs") && zone["lawRefs"].is_array()
              && zone["lawRefs"].size() == 1
              && zone["lawRefs"][0] == "law-primordial-cube-spin",
          "the Zone explicitly references its authored spin Law");

    const auto& objects = zone.at("world").at("objects");
    check(objects.size() == 1, "the Zone contains one visible ancestor");
    if (objects.empty()) return 1;
    const auto& record = objects[0];

    check(record.value("objectID", "") == "primordial.spinning-cube",
          "the ancestor has a stable identifier");
    check(record.value("shapeKind", -1) == static_cast<int>(Object::ShapeKind::Field),
          "the ancestor is ShapeKind::Field, not ShapeKind::Cube");
    check(record.value("shapeKind", -1) != static_cast<int>(Object::ShapeKind::Cube),
          "the old Cube primitive is not the source of form");
    check(!record.contains("faceColors"),
          "the authored record contains no primordial faceColors[6] payload");
    check(record.contains("field") && record["field"].is_object()
              && record["field"].value("prim", -1) == static_cast<int>(geom::SdfPrim::Expr),
          "the visible form is an authored SdfPrim::Expr");

    // Hydrate through the production Object codec, then evaluate the authored
    // form independently at canonical points. h = 1.5 m.
    Object cube;
    from_json(record, cube);
    check(cube.getShapeKind() == Object::ShapeKind::Field,
          "production Object hydration preserves the Field representation");
    check(cube.hasField(), "the hydrated ancestor carries authored field mathematics");
    if (cube.hasField()) {
        const auto& sdf = cube.getFieldData();
        check(sdf.prim == geom::SdfPrim::Expr && sdf.mathNode != nullptr,
              "the hydrated Field is executable OntoMath, not an empty shell");

        const float center = geom::evalSdf(sdf, glm::vec3(0.0f, 0.0f, 0.0f));
        const float faceX = geom::evalSdf(sdf, glm::vec3(1.5f, 0.0f, 0.0f));
        const float faceY = geom::evalSdf(sdf, glm::vec3(0.0f, -1.5f, 0.0f));
        const float outside = geom::evalSdf(sdf, glm::vec3(2.0f, 0.0f, 0.0f));
        check(near(center, -1.5f), "center evaluates inside at -1.5 m");
        check(near(faceX, 0.0f) && near(faceY, 0.0f),
              "orthogonal visible sides emerge as zero sets of the authored equation");
        check(near(outside, 0.5f), "a point 0.5 m beyond +X evaluates outside by 0.5");
    }

    const auto& props = record.at("authoredProperties");
    check(props.contains("primordial.form")
              && props["primordial.form"].value("v", "") ==
                 "max(abs(x)-h, abs(y)-h, abs(z)-h)",
          "the being names its own authored mathematical form");
    check(props.contains("primordial.spin.xDegreesPerSecond")
              && props.contains("primordial.spin.yDegreesPerSecond")
              && props.contains("primordial.spin.zDegreesPerSecond"),
          "all three spin rates are persisted as in-world authored properties");

    const char* paletteKeys[] = {
        "primordial.palette.posX", "primordial.palette.negX",
        "primordial.palette.posY", "primordial.palette.negY",
        "primordial.palette.posZ", "primordial.palette.negZ",
    };
    bool allPalette = true;
    for (const char* key : paletteKeys) allPalette = allPalette && props.contains(key);
    check(allPalette, "six historical colour regions are authored properties, not Object anatomy");

    check(zone.contains("materials") && zone["materials"].is_array()
              && zone["materials"].size() == 1,
          "the Zone carries the Material being its ancestor names");
    if (zone.contains("materials") && zone["materials"].is_array() && !zone["materials"].empty()) {
        Material material = Material::fromJson(zone["materials"][0]);
        check(material.colorExpr != nullptr && !material.colorExpr->pieces.empty(),
              "the Material carries authored colorExpr mathematics");
        if (material.colorExpr && !material.colorExpr->pieces.empty()) {
            struct Probe { glm::vec3 p; glm::vec3 expected; const char* name; };
            const Probe probes[] = {
                {{ 1.5f, 0.0f, 0.0f}, {0.96f, 0.22f, 0.20f}, "+X red"},
                {{-1.5f, 0.0f, 0.0f}, {0.18f, 0.72f, 0.98f}, "-X cyan"},
                {{ 0.0f, 1.5f, 0.0f}, {0.98f, 0.78f, 0.16f}, "+Y gold"},
                {{ 0.0f,-1.5f, 0.0f}, {0.48f, 0.25f, 0.92f}, "-Y purple"},
                {{ 0.0f, 0.0f, 1.5f}, {0.24f, 0.88f, 0.40f}, "+Z green"},
                {{ 0.0f, 0.0f,-1.5f}, {0.96f, 0.42f, 0.82f}, "-Z pink"},
            };
            for (const auto& probe : probes) {
                glm::vec3 got(0.0f);
                const bool ok = evalColor(*material.colorExpr,
                                          probe.p.x, probe.p.y, probe.p.z, got);
                check(ok && near3(got, probe.expected, 0.01f),
                      std::string(probe.name) + " is produced by colorExpr");
            }
        }
    }

    // The motion must be saved meaning, not Object::angularVelocity (which is
    // currently registered but not an Object persistence field).
    const auto law = SaveSystem::readSaveData(lawPath.string());
    check(!law.empty(), "the authored spin Law exists and is readable");
    if (!law.empty()) {
        const auto& body = law.at("law");
        check(body.value("activation", -1) == 1,
              "spin Law is WhileTrue");
        check(body.at("conditionModel").value("path", "") == "primordialCube",
              "spin Law selects the authored ancestor property");
        const auto& action = body.at("actionModel");
        check(action.value("kind", -1) == 8 && action.value("path", "") == "rotation",
              "spin Law Maps the ordinary rotation Property");
        const auto& bindings = action.at("bindings");
        check(bindings.value("t", "") == "time"
                  && bindings.value("sx", "") == "primordial.spin.xDegreesPerSecond"
                  && bindings.value("sy", "") == "primordial.spin.yDegreesPerSecond"
                  && bindings.value("sz", "") == "primordial.spin.zDegreesPerSecond",
              "time and all three authored spin rates are explicit Law bindings");

        const auto& children = action.at("function").at("pieces")[0].at("mathNode").at("children");
        check(children.size() == 3
                  && children[0]["scalarForm"]["terms"][0]["factors"].contains("sx")
                  && children[1]["scalarForm"]["terms"][0]["factors"].contains("sy")
                  && children[2]["scalarForm"]["terms"][0]["factors"].contains("sz"),
              "each rotation component uses its own authored axis rate");
    }

    const auto world = SaveSystem::readSaveData(worldPath.string());
    check(!world.empty() && world.value("currentZoneId", "") == "Primordial Spinning Cube",
          "a launchable witness world opens on the resurrected ancestor");

    std::cout << "------------------------------------------------------------\n";
    std::cout << checks - failures << "/" << checks << " machine checks passed.\n";
    std::cout << "PERSON WITNESS STILL REQUIRED: open the world and see it spin.\n";
    std::cout << "A green suite is not a witness.\n";
    return failures == 0 ? 0 : 1;
}
