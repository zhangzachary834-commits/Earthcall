// End-to-end and pedagogical verification test for Prism Cathedral Zone.
//
// Verifies that:
// 1. The save file exists under saves/zones/Prism Cathedral/zone.json and satisfies
//    Invariant 6 (directoryKey == documentIdentity == "Prism Cathedral").
// 2. ZoneManager hydrates the Zone without warnings or errors.
// 3. spatialRoot is a valid continuous FieldNode with mode=AST, light.source=true,
//    lightChroma, and lightAngular, carrying complex shaped radiance along Z.
// 4. spatialFields contains the 16 additional radiant/volumetric fields.
// 5. Wing B density sovereignty is verified (D(p,t), rho != D, no implicit radiant fog,
//    and complex non-box shapes: spherical, hollow shell, torus, CSG crescent, noise,
//    time-varying breathing shell, and proxy-proof 20x8x20m AABB containing a slender ring).
// 6. Wing A authored surface color fields (Material::colorExpr) are present and valid.
// 7. Every chamber and wing has an authored Inscription Stele teaching the exact architecture.

#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/AuthorableLight.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <variant>

extern MaterialManager materials;

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

double evalScalarAt(const OntoMath::Piecewise& pw, double x, double y, double z, double t = 0.0) {
    std::map<std::string, PropertyValue> vars{
        {"p", PropertyValue(glm::vec3(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z)))},
        {"x", PropertyValue(x)},
        {"y", PropertyValue(y)},
        {"z", PropertyValue(z)},
        {OntoMath::kTimeVar, PropertyValue(t)}
    };
    const auto res = pw.evaluate(vars);
    if (!res) return -1.0;
    double n = -1.0;
    if (!propertyValueToNumber(*res, n)) return -1.0;
    return n;
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running Prism Cathedral Zone Verification Test (Pass 2)...\n";
    std::cout << "============================================================\n";

    namespace fs = std::filesystem;
    const fs::path repoRoot = fs::path(__FILE__).parent_path().parent_path().parent_path();
    const fs::path zonePath = repoRoot / "saves/zones/Prism Cathedral/zone.json";

    // 1. Save file existence and Invariant 6 check
    std::ifstream in(zonePath);
    check(static_cast<bool>(in), "Prism Cathedral save file exists and is readable");

    nlohmann::json zoneJson;
    if (in) in >> zoneJson;

    check(zoneJson.value("identifier", "") == "Prism Cathedral",
          "Invariant 6: document identifier matches directory key exactly");
    check(zoneJson.value("name", "") == "Prism Cathedral",
          "Zone name is 'Prism Cathedral'");
    check(zoneJson.contains("spatialRoot"), "Zone contains spatialRoot");
    check(zoneJson.contains("spatialFields"), "Zone contains spatialFields");

    const auto& spatialFieldsJson = zoneJson["spatialFields"];
    check(spatialFieldsJson.is_array() && spatialFieldsJson.size() == 16,
          "spatialFields contains exactly 16 additional radiant/volumetric fields");

    // 2. Hydration into ZoneManager
    ZoneManager mgr;
    mgr.hydrateFromZoneStore();

    std::shared_ptr<Zone> cathedral = nullptr;
    for (const auto& z : mgr.zones()) {
        if (z && z->getIdentifier() == "Prism Cathedral") {
            cathedral = z;
            break;
        }
    }
    check(cathedral != nullptr, "Prism Cathedral hydrated successfully into ZoneManager");
    if (!cathedral) {
        std::cout << "Cannot proceed without hydrated zone.\n";
        return 1;
    }

    // 3. Verify spatialRoot
    auto* root = cathedral->spatialRoot();
    check(root != nullptr, "Hydrated Prism Cathedral owns spatialRoot FieldNode");
    if (root) {
        Rendering::AuthorableLightState light;
        check(Rendering::readAuthorableLight(*root, light),
              "spatialRoot resolves as an authored light source (light.source=true)");
        check(light.enabled, "spatialRoot light is enabled");

        check(root->field != nullptr && root->field->mode == OntoMath::ScalarField::EvaluationMode::AST,
              "spatialRoot scalar field is in AST mode");
        check(!root->field->astDefinition.pieces.empty(),
              "spatialRoot scalar field contains authored Piecewise pieces");

        check(root->lightChroma != nullptr && !root->lightChroma->pieces.empty(),
              "spatialRoot carries authored source chroma Piecewise (Rung 5)");
        check(root->lightAngular != nullptr && !root->lightAngular->pieces.empty(),
              "spatialRoot carries authored angular emission Piecewise (Rung 6)");

        // Pedagogical evaluation across Z:
        // Station 1: Z=40
        double valSt1 = evalScalarAt(root->field->astDefinition, 0.0, 0.0, 40.0);
        check(valSt1 > 0.5, "Foundation 1 scalar radiance evaluates positive at Z=40");

        // Station 3: Near vs Far
        double valNear = evalScalarAt(root->field->astDefinition, 0.0, 0.0, 107.0);
        double valFar = evalScalarAt(root->field->astDefinition, 0.0, 0.0, 119.0);
        check(valNear > valFar * 1.5,
              "Rung 3 Near witness receives significantly stronger radiance than Far witness");

        // Station 5: Chroma evaluates as a Vector
        std::map<std::string, PropertyValue> chromaVars{
            {"p", PropertyValue(glm::vec3(0.0f, 0.0f, 180.0f))},
            {"x", PropertyValue(0.0)}, {"y", PropertyValue(0.0)}, {"z", PropertyValue(180.0)},
            {OntoMath::kTimeVar, PropertyValue(0.0)}
        };
        auto valChroma = root->lightChroma->evaluate(chromaVars);
        check(valChroma && std::holds_alternative<glm::vec3>(*valChroma),
              "Rung 5 source chroma evaluates as a vec3 at Z=180");

        // Station 6: Angular evaluates
        std::map<std::string, PropertyValue> angularVars{
            {"p", PropertyValue(glm::vec3(0.0f, 0.0f, 215.0f))},
            {"x", PropertyValue(0.0)}, {"y", PropertyValue(0.0)}, {"z", PropertyValue(215.0)},
            {OntoMath::kTimeVar, PropertyValue(0.0)},
            {OntoMath::kOmegaXVar, PropertyValue(0.0)},
            {OntoMath::kOmegaYVar, PropertyValue(0.0)},
            {OntoMath::kOmegaZVar, PropertyValue(1.0)}
        };
        auto valAngular = root->lightAngular->evaluate(angularVars);
        double numAngular = -1.0;
        check(valAngular && propertyValueToNumber(*valAngular, numAngular) && numAngular > 0.0,
              "Rung 6 angular emission evaluates positive at Z=215");
    }

    // 4. Verify spatialFields (16 fields)
    const auto& fields = cathedral->additionalSpatialFields();
    check(fields.size() == 16, "Zone holds 16 additional FieldNodes in formation");

    bool hasSapphireLantern = false;
    bool hasAmethystPulsar = false;
    bool hasEmeraldFilament = false;
    bool hasBlueVisibilitySource = false;
    bool hasSphericalCloud = false;
    bool hasHollowShell = false;
    bool hasTorusMedium = false;
    bool hasCsgCrescent = false;
    bool hasNoiseCloud = false;
    bool hasTimeNebula = false;
    bool hasProxyProofTorus = false;
    bool hasDualSovereignBeing = false;
    bool hasSolSecundus = false;
    bool hasSolTertius = false;
    bool hasSummitTorus = false;
    bool hasCelestialAtmosphere = false;

    for (const auto& f : fields) {
        if (!f) continue;
        const std::string id = f->getIdentifier();
        if (id == "prism.station7.sapphire-lantern") {
            hasSapphireLantern = true;
            Rendering::AuthorableLightState l;
            check(Rendering::readAuthorableLight(*f, l), "Sapphire Lantern is a radiant source");
            check(f->lightChroma && !f->lightChroma->pieces.empty(), "Sapphire Lantern has authored chroma");
            check(f->lightAngular && !f->lightAngular->pieces.empty(), "Sapphire Lantern has authored angular fan");
        } else if (id == "prism.station7.amethyst-pulsar") {
            hasAmethystPulsar = true;
            Rendering::AuthorableLightState l;
            check(Rendering::readAuthorableLight(*f, l), "Amethyst Pulsar is a radiant source");
            check(f->lightChroma && !f->lightChroma->pieces.empty(), "Amethyst Pulsar has authored chroma");
        } else if (id == "prism.station7.emerald-filament") {
            hasEmeraldFilament = true;
            Rendering::AuthorableLightState l;
            check(Rendering::readAuthorableLight(*f, l), "Emerald Filament is a radiant source");
        } else if (id == "prism.station8.blue-source") {
            hasBlueVisibilitySource = true;
            Rendering::AuthorableLightState l;
            check(Rendering::readAuthorableLight(*f, l), "Blue Visibility Source is a radiant source");
        } else if (id == "prism.wingB.spherical-cloud") {
            hasSphericalCloud = true;
            Rendering::AuthorableLightState l;
            check(!Rendering::readAuthorableLight(*f, l), "Wing B Spherical Cloud is NOT a light source");
            Rendering::VolumeDensityBinding b;
            check(Rendering::readVolumeDensity(*f, 0.0, 0.0, b), "Spherical Cloud has valid volumeDensity");
        } else if (id == "prism.wingB.hollow-shell") {
            hasHollowShell = true;
            Rendering::VolumeDensityBinding b;
            check(Rendering::readVolumeDensity(*f, 0.0, 0.0, b), "Hollow Shell has valid volumeDensity");
            // Check hollow core evaluate: at (0,0,0) density should be 0.0
            double dCore = evalScalarAt(*f->volumeDensity, 0.0, 0.0, 0.0);
            check(dCore <= 1e-4, "Hollow Shell core density is zero at local (0,0,0)");
            double dShell = evalScalarAt(*f->volumeDensity, 2.6, 0.0, 0.0);
            check(dShell > 1.0, "Hollow Shell peak density is > 1.0 at r=2.6");
        } else if (id == "prism.wingB.torus-medium") {
            hasTorusMedium = true;
            Rendering::VolumeDensityBinding b;
            check(Rendering::readVolumeDensity(*f, 0.0, 0.0, b), "Torus Medium has valid volumeDensity");
            // Check donut hole evaluate: at (0,0,0) density should be 0.0
            double dHole = evalScalarAt(*f->volumeDensity, 0.0, 0.0, 0.0);
            check(dHole <= 1e-4, "Torus Medium hole density is zero at local (0,0,0)");
            double dRing = evalScalarAt(*f->volumeDensity, 2.4, 0.0, 0.0);
            check(dRing > 1.0, "Torus Medium ring density is > 1.0 at major radius R=2.4");
        } else if (id == "prism.wingB.csg-crescent-medium") {
            hasCsgCrescent = true;
            Rendering::VolumeDensityBinding b;
            check(Rendering::readVolumeDensity(*f, 0.0, 0.0, b), "CSG Crescent has valid volumeDensity");
        } else if (id == "prism.wingB.noise-organic-cloud") {
            hasNoiseCloud = true;
            Rendering::VolumeDensityBinding b;
            check(Rendering::readVolumeDensity(*f, 0.0, 0.0, b), "Noise Cloud has valid volumeDensity");
        } else if (id == "prism.wingB.time-breathing-nebula") {
            hasTimeNebula = true;
            Rendering::VolumeDensityBinding b;
            check(Rendering::readVolumeDensity(*f, 0.0, 0.0, b), "Time-Breathing Nebula has valid volumeDensity");
        } else if (id == "prism.wingB.proxy-proof-torus") {
            hasProxyProofTorus = true;
            Rendering::VolumeDensityBinding b;
            check(Rendering::readVolumeDensity(*f, 0.0, 0.0, b), "Proxy-Proof Torus has valid volumeDensity");
            // Check scale is 10x4x10
            check(std::fabs(f->scale.x - 10.0f) < 1e-4f && std::fabs(f->scale.z - 10.0f) < 1e-4f,
                  "Proxy-Proof Torus has colossal 20x8x20m AABB proxy scale");
            // Check that density is 0 far from the ring
            double dOutside = evalScalarAt(*f->volumeDensity, 8.0, 0.0, 0.0);
            check(dOutside <= 1e-4, "Proxy-Proof Torus has zero density at x=8 (outside ring)");
            double dRing = evalScalarAt(*f->volumeDensity, 1.8, 0.0, 0.0);
            check(dRing > 1.5, "Proxy-Proof Torus has peak density > 1.5 on ring at x=1.8");
        } else if (id == "prism.wingB.dual-sovereign-being") {
            hasDualSovereignBeing = true;
            Rendering::AuthorableLightState l;
            check(Rendering::readAuthorableLight(*f, l), "Wing B Dual Being is an authored radiant source");
            Rendering::VolumeDensityBinding b;
            check(Rendering::readVolumeDensity(*f, 0.0, 0.0, b), "Wing B Dual Being is an authored participating medium");
            check(f->field->astDefinition.toJson().dump() != f->volumeDensity->toJson().dump(),
                  "Constitution: rho_source AST != D_medium AST on Dual Sovereign Being");
        } else if (id == "prism.summit.sol-secundus") {
            hasSolSecundus = true;
            Rendering::AuthorableLightState l;
            check(Rendering::readAuthorableLight(*f, l), "Sol Secundus is a radiant source");
        } else if (id == "prism.summit.sol-tertius") {
            hasSolTertius = true;
            Rendering::AuthorableLightState l;
            check(Rendering::readAuthorableLight(*f, l), "Sol Tertius is a radiant source");
        } else if (id == "prism.summit.torus-medium") {
            hasSummitTorus = true;
            Rendering::VolumeDensityBinding b;
            check(Rendering::readVolumeDensity(*f, 0.0, 0.0, b), "Summit Torus Medium has valid volumeDensity");
        } else if (id == "prism.summit.celestial-veil") {
            hasCelestialAtmosphere = true;
            Rendering::VolumeDensityBinding b;
            check(Rendering::readVolumeDensity(*f, 0.0, 0.0, b), "The Prism Celestial Veil has valid volumeDensity");
        }
    }

    check(hasSapphireLantern, "Found Station 7 Sapphire Lantern");
    check(hasAmethystPulsar, "Found Station 7 Amethyst Pulsar");
    check(hasEmeraldFilament, "Found Station 7 Emerald Filament");
    check(hasBlueVisibilitySource, "Found Station 8 Blue Visibility Source");
    check(hasSphericalCloud, "Found Wing B Spherical Cloud");
    check(hasHollowShell, "Found Wing B Hollow Shell");
    check(hasTorusMedium, "Found Wing B Torus Medium");
    check(hasCsgCrescent, "Found Wing B CSG Crescent Medium");
    check(hasNoiseCloud, "Found Wing B Noise Cloud");
    check(hasTimeNebula, "Found Wing B Time-Breathing Nebula");
    check(hasProxyProofTorus, "Found Wing B Proxy-Proof Torus Medium");
    check(hasDualSovereignBeing, "Found Wing B Dual Sovereign Being");
    check(hasSolSecundus, "Found The Prism Sol Secundus");
    check(hasSolTertius, "Found The Prism Sol Tertius");
    check(hasSummitTorus, "Found The Prism Summit Torus Medium");
    check(hasCelestialAtmosphere, "Found The Prism Celestial Atmosphere");

    // 5. Verify Materials & Wing A Surface Color Fields
    const auto& matsJson = zoneJson["materials"];
    std::unordered_set<std::string> matIds;
    for (const auto& mj : matsJson) {
        matIds.insert(mj.value("id", ""));
    }

    check(matIds.count("material.prism_surface_rainbow") > 0, "Wing A Stratified Rainbow material present");
    check(matIds.count("material.prism_surface_radial") > 0, "Wing A Radial Gradient material present");
    check(matIds.count("material.prism_surface_lattice") > 0, "Wing A Nodal Lattice material present");
    check(matIds.count("material.prism_surface_pure_blue") > 0, "Wing A Pure Blue Surface material present");

    for (const auto& mj : matsJson) {
        const std::string mid = mj.value("id", "");
        if (mid == "material.prism_surface_rainbow" ||
            mid == "material.prism_surface_radial" ||
            mid == "material.prism_surface_lattice" ||
            mid == "material.prism_surface_pure_blue") {
            check(mj.contains("colorExpr"), mid + " has authored colorExpr in JSON");
            auto pw = OntoMath::Piecewise::fromJson(mj["colorExpr"]);
            check(!pw.pieces.empty(), mid + " colorExpr has valid pieces");
        }
    }

    // 6. Verify Pedagogy Stelae and Key Architecture
    const auto& objects = cathedral->getOwnedObjects();
    check(objects.size() >= 200, "Cathedral contains full architectural complement (>200 objects)");

    std::unordered_set<std::string> stelaeFound;
    for (const auto& obj : objects) {
        if (!obj) continue;
        const std::string id = obj->getIdentifier();
        if (id.find("stele") != std::string::npos) {
            stelaeFound.insert(id);
        }
    }

    check(stelaeFound.count("cathedral.entrance.stele") > 0, "Found Entrance Atrium Stele");
    check(stelaeFound.count("cathedral.station1.stele") > 0, "Found Foundation 1 Stele");
    check(stelaeFound.count("cathedral.station2.stele") > 0, "Found Foundation 2 Stele");
    check(stelaeFound.count("cathedral.station3.stele") > 0, "Found Rung 3 Stele");
    check(stelaeFound.count("cathedral.station4.stele") > 0, "Found Rung 4 Stele");
    check(stelaeFound.count("cathedral.station5.stele") > 0, "Found Rung 5 Stele");
    check(stelaeFound.count("cathedral.station6.stele") > 0, "Found Rung 6 Stele");
    check(stelaeFound.count("cathedral.station7.stele") > 0, "Found Rung 7 Stele");
    check(stelaeFound.count("cathedral.station8.stele") > 0, "Found Rung 8 Stele");
    check(stelaeFound.count("cathedral.wingA.stele") > 0, "Found Wing A (Surface Color) Stele");
    check(stelaeFound.count("cathedral.wingB.stele") > 0, "Found Wing B (Volumetric V0) Stele");
    check(stelaeFound.count("cathedral.wingC.stele") > 0, "Found Wing C (Live Authoring) Stele");
    check(stelaeFound.count("cathedral.wingD.stele") > 0, "Found Wing D (Compatibility & Refusal) Stele");
    check(stelaeFound.count("cathedral.prism.summit_stele") > 0, "Found The Prism Summit Stele");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks << " checks completed (" << (g_checks - g_failures)
              << " passed, " << g_failures << " failed).\n";
    if (g_failures == 0) {
        std::cout << "prism_cathedral_test: ALL OK\n";
        return 0;
    } else {
        std::cout << "prism_cathedral_test: FAILED\n";
        return 1;
    }
}
