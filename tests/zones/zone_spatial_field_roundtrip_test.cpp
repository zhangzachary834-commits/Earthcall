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
#include "Singularity/Screen/VolumeDensity.hpp"
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
#include <utility>

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

    // V0 density sovereignty: this mathematics is intentionally different from
    // the radiant field AST above. The test will carry both through the real
    // Zone store and prove that neither channel aliases the other.
    auto densityAst = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(
            OntoMath::ScalarForm::variable("z", 0.25, 0.6)));
    const std::string densityAstJson = densityAst.toJson().dump();

    // V1 extinction is intentionally a third scalar truth: it neither aliases
    // source rho nor density D, even when all three share OntoMath Piecewise.
    auto extinctionAst = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(
            OntoMath::ScalarForm::variable("y", 0.4, 0.2)));
    const std::string extinctionAstJson = extinctionAst.toJson().dump();

    // V2 scattering and medium chroma are fourth/fifth truths, independently
    // authored from D, sigma_t, source rho, source chroma, and source alpha.
    auto scatteringAst = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(
            OntoMath::ScalarForm::variable("x", 0.3, 0.4)));
    const std::string scatteringAstJson = scatteringAst.toJson().dump();

    auto scalarMath = [](double value) {
        auto n = std::make_unique<OntoMath::MathNode>();
        n->op = OntoMath::MathNode::Op::ScalarLeaf;
        n->scalarForm.terms.push_back(OntoMath::Term(value));
        return n;
    };
    auto mediumChromaNode = std::make_shared<OntoMath::MathNode>();
    mediumChromaNode->op = OntoMath::MathNode::Op::VectorConstruct;
    mediumChromaNode->children.push_back(scalarMath(1.0));
    mediumChromaNode->children.push_back(scalarMath(0.25));
    mediumChromaNode->children.push_back(scalarMath(0.05));
    auto mediumChromaAst = OntoMath::Piecewise::continuous(mediumChromaNode);
    const std::string mediumChromaAstJson = mediumChromaAst.toJson().dump();

    // V3 phase is a sixth independent medium truth. It may read the medium
    // scattering directions, but it never borrows source angular alpha.
    auto phaseAst = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(
            OntoMath::ScalarForm::constant(1.0).plus(
                OntoMath::ScalarForm::variable(
                    OntoMath::kWiZVar, 1.0, 0.35))));
    const std::string phaseAstJson = phaseAst.toJson().dump();

    auto sourceAngularAst = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(
            OntoMath::ScalarForm::constant(1.0).plus(
                OntoMath::ScalarForm::variable(
                    OntoMath::kOmegaZVar, 1.0, -0.2))));
    const std::string sourceAngularAstJson = sourceAngularAst.toJson().dump();

    // V0c projection witness: sourcehood and mediumhood are independent. This
    // FieldNode has authored density but deliberately has NO light.source.
    {
        geom::FieldNode fogOnly("fog-only-medium");
        fogOnly.origin = glm::vec3(2.0f, 4.0f, 6.0f);
        fogOnly.scale = glm::vec3(8.0f, 10.0f, 12.0f);
        *fogOnly.volumeDensity = densityAst;
        *fogOnly.volumeExtinction = extinctionAst;
        *fogOnly.volumeScattering = scatteringAst;
        *fogOnly.volumeChroma = mediumChromaAst;
        *fogOnly.volumePhase = phaseAst;

        Rendering::VolumeDensityBinding projected;
        check(Rendering::readVolumeDensity(fogOnly, 3.25, 0.125, projected),
              "density-only FieldNode projects as participating medium without light.source");
        check(projected.densityExpr == fogOnly.volumeDensity.get(),
              "volume projection borrows the authored D AST rather than copying or aliasing rho");
        check(projected.extinctionExpr == fogOnly.volumeExtinction.get() &&
                  projected.extinctionRevision != 0,
              "volume projection carries independent authored extinction sigma_t");
        check(projected.scatteringExpr == fogOnly.volumeScattering.get() &&
                  projected.scatteringRevision != 0,
              "volume projection carries independent authored scattering sigma_s");
        check(projected.volumeChromaExpr == fogOnly.volumeChroma.get() &&
                  projected.volumeChromaRevision != 0,
              "volume projection carries independent authored medium chroma C_v");
        check(projected.phaseExpr == fogOnly.volumePhase.get() &&
                  projected.phaseRevision != 0,
              "volume projection carries independent authored phase Phi");
        check(nearf(projected.origin.x, 2.0f) &&
                  nearf(projected.origin.y, 4.0f) &&
                  nearf(projected.origin.z, 6.0f) &&
                  nearf(projected.scale.x, 8.0f) &&
                  nearf(projected.scale.y, 10.0f) &&
                  nearf(projected.scale.z, 12.0f),
              "volume projection preserves authored medium placement and bounds");
        check(std::fabs(projected.temporalCoordinate - 3.25) < 1e-9 &&
                  std::fabs(projected.temporalDelta - 0.125) < 1e-9,
              "volume projection carries its own admitted temporal coordinate");
        PropertyValue lightSource;
        check(!fogOnly.getDynamicProperty("light.source", lightSource),
              "participating medium projection does not fabricate sourcehood");
    }

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

            check(PropertyPath::parse("volume.density.ast").setValue(
                      *root, PropertyValue(densityAstJson)) == PropertyPath::PathResult::Ok,
                  "V0 density AST is independently authored through PropertyPath");
            check(PropertyPath::parse("volume.extinction.ast").setValue(
                      *root, PropertyValue(extinctionAstJson)) == PropertyPath::PathResult::Ok,
                  "V1 extinction AST is independently authored through PropertyPath");
            check(PropertyPath::parse("volume.scattering.ast").setValue(
                      *root, PropertyValue(scatteringAstJson)) == PropertyPath::PathResult::Ok,
                  "V2 scattering AST is independently authored through PropertyPath");
            check(PropertyPath::parse("volume.chroma.ast").setValue(
                      *root, PropertyValue(mediumChromaAstJson)) == PropertyPath::PathResult::Ok,
                  "V2 medium chroma AST is independently authored through PropertyPath");
            check(PropertyPath::parse("volume.phase.ast").setValue(
                      *root, PropertyValue(phaseAstJson)) == PropertyPath::PathResult::Ok,
                  "V3 phase AST is independently authored through PropertyPath");
            check(PropertyPath::parse("light.angular.ast").setValue(
                      *root, PropertyValue(sourceAngularAstJson)) == PropertyPath::PathResult::Ok,
                  "source alpha remains independently authored through its own PropertyPath");

            Property* rhoProperty = root->findProperty("field.ast");
            Property* densityProperty = root->findProperty("volume.density.ast");
            Property* extinctionProperty = root->findProperty("volume.extinction.ast");
            Property* scatteringProperty = root->findProperty("volume.scattering.ast");
            Property* volumeChromaProperty = root->findProperty("volume.chroma.ast");
            Property* phaseProperty = root->findProperty("volume.phase.ast");
            Property* sourceAlphaProperty = root->findProperty("light.angular.ast");
            check(rhoProperty != nullptr && densityProperty != nullptr &&
                      extinctionProperty != nullptr && scatteringProperty != nullptr &&
                      volumeChromaProperty != nullptr && phaseProperty != nullptr &&
                      sourceAlphaProperty != nullptr &&
                      rhoProperty != densityProperty &&
                      rhoProperty != extinctionProperty &&
                      densityProperty != extinctionProperty &&
                      scatteringProperty != densityProperty &&
                      scatteringProperty != extinctionProperty &&
                      volumeChromaProperty != scatteringProperty &&
                      volumeChromaProperty != rhoProperty &&
                      phaseProperty != volumeChromaProperty &&
                      phaseProperty != sourceAlphaProperty &&
                      sourceAlphaProperty != rhoProperty,
                  "rho, D, sigma_t, sigma_s, C_v, Phi, and source alpha are distinct Property beings");

            const PropertyValue phaseBeforeBadWrite =
                phaseProperty ? phaseProperty->value() : PropertyValue(std::string());
            const PropertyValue sourceAlphaBeforeBadPhase =
                sourceAlphaProperty ? sourceAlphaProperty->value() : PropertyValue(std::string());
            check(phaseProperty &&
                      !phaseProperty->setValue(PropertyValue(std::string("{ malformed"))),
                  "malformed V3 phase AST is refused atomically");
            check(phaseProperty && sourceAlphaProperty &&
                      phaseProperty->value() == phaseBeforeBadWrite &&
                      sourceAlphaProperty->value() == sourceAlphaBeforeBadPhase,
                  "refused phase authorship mutates neither Phi nor source alpha");

            const PropertyValue rhoBeforeBadDensity =
                rhoProperty ? rhoProperty->value() : PropertyValue(std::string());
            const PropertyValue densityBeforeBadWrite =
                densityProperty ? densityProperty->value() : PropertyValue(std::string());
            check(densityProperty &&
                      !densityProperty->setValue(PropertyValue(std::string("{ malformed"))),
                  "malformed V0 density AST is refused atomically");
            check(rhoProperty && densityProperty &&
                      rhoProperty->value() == rhoBeforeBadDensity &&
                      densityProperty->value() == densityBeforeBadWrite,
                  "refused density authorship mutates neither density nor source-radiance mathematics");

            const PropertyValue rhoBeforeBadExtinction =
                rhoProperty ? rhoProperty->value() : PropertyValue(std::string());
            const PropertyValue densityBeforeBadExtinction =
                densityProperty ? densityProperty->value() : PropertyValue(std::string());
            const PropertyValue extinctionBeforeBadWrite =
                extinctionProperty ? extinctionProperty->value() : PropertyValue(std::string());
            check(extinctionProperty &&
                      !extinctionProperty->setValue(PropertyValue(std::string("{ malformed"))),
                  "malformed V1 extinction AST is refused atomically");
            check(rhoProperty && densityProperty && extinctionProperty &&
                      rhoProperty->value() == rhoBeforeBadExtinction &&
                      densityProperty->value() == densityBeforeBadExtinction &&
                      extinctionProperty->value() == extinctionBeforeBadWrite,
                  "refused extinction authorship mutates neither rho, D, nor sigma_t");
        }

        auto second = std::make_shared<geom::FieldNode>(zoneId + "_secondRadiantField");
        second->origin = glm::vec3(-5.0f, 3.0f, 9.0f);
        second->setDynamicProperty("light.source", PropertyValue(true));
        second->setDynamicProperty("light.intensity", PropertyValue(0.75f));
        second->field->mode = OntoMath::ScalarField::EvaluationMode::AST;
        second->field->astDefinition = OntoMath::Piecewise::continuous(
            OntoMath::MathNode::fromLegacyExpression(
                OntoMath::ScalarForm::variable("y", 2.0, 1.0)));
        zone->addSpatialField(second);

        check(zone->additionalSpatialFields().size() == 1,
              "Zone directly indexes an additional authored FieldNode");
        bool secondInFormation = false;
        for (Singular* member : zone->formation().getMembers()) {
            if (member == second.get()) secondInFormation = true;
        }
        check(secondInFormation,
              "additional authored FieldNode enters the Zone Formation");

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

        check(zone && zone->additionalSpatialFields().size() == 1,
              "fresh hydration restores the additional FieldNode source collection");
        if (zone && zone->additionalSpatialFields().size() == 1) {
            const auto& second = zone->additionalSpatialFields().front();
            PropertyValue source;
            PropertyValue intensity;
            check(second &&
                      second->getIdentifier() == zoneId + "_secondRadiantField",
                  "additional FieldNode identity survives save -> hydration");
            check(second &&
                      second->getDynamicProperty("light.source", source) &&
                      std::get_if<bool>(&source) && *std::get_if<bool>(&source),
                  "additional source's authored light.source survives persistence");
            check(second &&
                      second->getDynamicProperty("light.intensity", intensity) &&
                      std::get_if<float>(&intensity) &&
                      nearf(*std::get_if<float>(&intensity), 0.75f),
                  "additional source's authored scalar light state survives persistence");
            check(second && nearf(second->origin.x, -5.0f) &&
                      nearf(second->origin.y, 3.0f) && nearf(second->origin.z, 9.0f),
                  "additional source placement survives persistence");

            bool secondInFormation = false;
            if (second) {
                for (Singular* member : zone->formation().getMembers()) {
                    if (member == second.get()) secondInFormation = true;
                }
            }
            check(secondInFormation,
                  "hydrated additional FieldNode is immediately Formation-reachable");
        }
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
            bool sameAst = false;
            if (ast) {
                const PropertyValue astValue = ast->value();
                if (const auto* astText = std::get_if<std::string>(&astValue)) {
                    const auto expected = nlohmann::json::parse(authoredAstJson, nullptr, false);
                    const auto actual = nlohmann::json::parse(*astText, nullptr, false);
                    sameAst = !expected.is_discarded() && !actual.is_discarded()
                           && expected == actual;
                }
            }
            check(sameAst,
                  "the Person-authored OntoMath AST survives save -> fresh hydration");

            Property* density = root->findProperty("volume.density.ast");
            bool sameDensityAst = false;
            if (density) {
                const PropertyValue densityValue = density->value();
                if (const auto* densityText = std::get_if<std::string>(&densityValue)) {
                    const auto expected =
                        nlohmann::json::parse(densityAstJson, nullptr, false);
                    const auto actual =
                        nlohmann::json::parse(*densityText, nullptr, false);
                    sameDensityAst = !expected.is_discarded() && !actual.is_discarded()
                                  && expected == actual;
                }
            }
            check(sameDensityAst,
                  "independent volume.density.ast survives save -> fresh hydration");

            Property* extinction = root->findProperty("volume.extinction.ast");
            bool sameExtinctionAst = false;
            if (extinction) {
                const PropertyValue extinctionValue = extinction->value();
                if (const auto* extinctionText = std::get_if<std::string>(&extinctionValue)) {
                    const auto expected =
                        nlohmann::json::parse(extinctionAstJson, nullptr, false);
                    const auto actual =
                        nlohmann::json::parse(*extinctionText, nullptr, false);
                    sameExtinctionAst = !expected.is_discarded() && !actual.is_discarded()
                                     && expected == actual;
                }
            }
            check(sameExtinctionAst,
                  "independent volume.extinction.ast survives save -> fresh hydration");

            Property* scattering = root->findProperty("volume.scattering.ast");
            bool sameScatteringAst = false;
            if (scattering) {
                const PropertyValue value = scattering->value();
                if (const auto* text = std::get_if<std::string>(&value)) {
                    const auto expected =
                        nlohmann::json::parse(scatteringAstJson, nullptr, false);
                    const auto actual =
                        nlohmann::json::parse(*text, nullptr, false);
                    sameScatteringAst = !expected.is_discarded() && !actual.is_discarded()
                                     && expected == actual;
                }
            }
            check(sameScatteringAst,
                  "independent volume.scattering.ast survives save -> fresh hydration");

            Property* volumeChroma = root->findProperty("volume.chroma.ast");
            bool sameVolumeChromaAst = false;
            if (volumeChroma) {
                const PropertyValue value = volumeChroma->value();
                if (const auto* text = std::get_if<std::string>(&value)) {
                    const auto expected =
                        nlohmann::json::parse(mediumChromaAstJson, nullptr, false);
                    const auto actual =
                        nlohmann::json::parse(*text, nullptr, false);
                    sameVolumeChromaAst = !expected.is_discarded() && !actual.is_discarded()
                                       && expected == actual;
                }
            }
            check(sameVolumeChromaAst,
                  "independent volume.chroma.ast survives save -> fresh hydration");

            Property* phase = root->findProperty("volume.phase.ast");
            bool samePhaseAst = false;
            if (phase) {
                const PropertyValue value = phase->value();
                if (const auto* text = std::get_if<std::string>(&value)) {
                    const auto expected =
                        nlohmann::json::parse(phaseAstJson, nullptr, false);
                    const auto actual =
                        nlohmann::json::parse(*text, nullptr, false);
                    samePhaseAst = !expected.is_discarded() && !actual.is_discarded()
                                && expected == actual;
                }
            }
            check(samePhaseAst,
                  "independent volume.phase.ast survives save -> fresh hydration");

            Property* sourceAlpha = root->findProperty("light.angular.ast");
            bool sameSourceAlphaAst = false;
            if (sourceAlpha) {
                const PropertyValue value = sourceAlpha->value();
                if (const auto* text = std::get_if<std::string>(&value)) {
                    const auto expected =
                        nlohmann::json::parse(sourceAngularAstJson, nullptr, false);
                    const auto actual =
                        nlohmann::json::parse(*text, nullptr, false);
                    sameSourceAlphaAst = !expected.is_discarded() && !actual.is_discarded()
                                     && expected == actual;
                }
            }
            check(sameSourceAlphaAst,
                  "source light.angular.ast survives independently beside medium phase");

            Property* rho = root->findProperty("field.ast");
            check(rho != nullptr && density != nullptr && extinction != nullptr &&
                      scattering != nullptr && volumeChroma != nullptr &&
                      phase != nullptr && sourceAlpha != nullptr &&
                      rho != density && rho != extinction && density != extinction &&
                      scattering != density && scattering != extinction &&
                      volumeChroma != scattering && volumeChroma != rho &&
                      phase != volumeChroma && phase != sourceAlpha,
                  "fresh hydration preserves rho/D/sigma_t/sigma_s/C_v/Phi/source-alpha independence");

            if (rho && density && extinction && scattering && volumeChroma &&
                phase && sourceAlpha) {
                const PropertyValue rhoBeforeDensityRewrite = rho->value();
                const PropertyValue extinctionBeforeDensityRewrite = extinction->value();
                auto replacementDensity = OntoMath::Piecewise::continuous(
                    OntoMath::MathNode::fromLegacyExpression(
                        OntoMath::ScalarForm::variable("y", 0.5, 1.25)));
                check(density->setValue(
                          PropertyValue(replacementDensity.toJson().dump())),
                      "hydrated V0 density accepts a complete Law-style AST replacement");
                check(rho->value() == rhoBeforeDensityRewrite &&
                          extinction->value() == extinctionBeforeDensityRewrite,
                      "rewriting D leaves rho and sigma_t byte-identical");

                const PropertyValue rhoBeforeExtinctionRewrite = rho->value();
                const PropertyValue densityBeforeExtinctionRewrite = density->value();
                auto replacementExtinction = OntoMath::Piecewise::continuous(
                    OntoMath::MathNode::fromLegacyExpression(
                        OntoMath::ScalarForm::variable("x", 1.5, 0.1)));
                check(extinction->setValue(
                          PropertyValue(replacementExtinction.toJson().dump())),
                      "hydrated V1 extinction accepts a complete Law-style AST replacement");
                check(rho->value() == rhoBeforeExtinctionRewrite &&
                          density->value() == densityBeforeExtinctionRewrite,
                      "rewriting sigma_t leaves rho and D byte-identical");

                const PropertyValue rhoBeforeScatteringRewrite = rho->value();
                const PropertyValue densityBeforeScatteringRewrite = density->value();
                const PropertyValue extinctionBeforeScatteringRewrite = extinction->value();
                const PropertyValue chromaBeforeScatteringRewrite = volumeChroma->value();
                auto replacementScattering = OntoMath::Piecewise::continuous(
                    OntoMath::MathNode::fromLegacyExpression(
                        OntoMath::ScalarForm::variable("z", 0.9, 0.05)));
                check(scattering->setValue(
                          PropertyValue(replacementScattering.toJson().dump())),
                      "hydrated V2 scattering accepts a complete Law-style AST replacement");
                check(rho->value() == rhoBeforeScatteringRewrite &&
                          density->value() == densityBeforeScatteringRewrite &&
                          extinction->value() == extinctionBeforeScatteringRewrite &&
                          volumeChroma->value() == chromaBeforeScatteringRewrite,
                      "rewriting sigma_s leaves rho, D, sigma_t, and C_v byte-identical");

                const PropertyValue rhoBeforePhaseRewrite = rho->value();
                const PropertyValue densityBeforePhaseRewrite = density->value();
                const PropertyValue extinctionBeforePhaseRewrite = extinction->value();
                const PropertyValue scatteringBeforePhaseRewrite = scattering->value();
                const PropertyValue chromaBeforePhaseRewrite = volumeChroma->value();
                const PropertyValue sourceAlphaBeforePhaseRewrite = sourceAlpha->value();
                auto replacementPhase = OntoMath::Piecewise::continuous(
                    OntoMath::MathNode::fromLegacyExpression(
                        OntoMath::ScalarForm::constant(0.8).plus(
                            OntoMath::ScalarForm::variable(
                                OntoMath::kWoZVar, 1.0, 0.15))));
                check(phase->setValue(PropertyValue(replacementPhase.toJson().dump())),
                      "hydrated V3 phase accepts a complete Law-style AST replacement");
                check(rho->value() == rhoBeforePhaseRewrite &&
                          density->value() == densityBeforePhaseRewrite &&
                          extinction->value() == extinctionBeforePhaseRewrite &&
                          scattering->value() == scatteringBeforePhaseRewrite &&
                          volumeChroma->value() == chromaBeforePhaseRewrite &&
                          sourceAlpha->value() == sourceAlphaBeforePhaseRewrite,
                      "rewriting Phi leaves rho, D, sigma_t, sigma_s, C_v, and source alpha byte-identical");
            }
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
