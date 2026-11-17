// Material-as-a-being test (OPENGL_MIGRATION_PLAN.md, Milestone 2).
//
// The decision was that a Material is a Singular being, not a render-layer struct.
// That claim only means something if two things hold:
//   1. A Material is LAW-ADDRESSABLE — its fields resolve through PropertyPath and
//      can be read and driven exactly like an Object's position. This is the whole
//      reason to make it a being: a Law can change a material's colour.
//   2. MaterialManager owns the beings the way RelationManager owns relations:
//      a default always resolves, references resolve by identifier, and the set
//      survives a JSON round-trip.
// Gate B pins a stronger compatibility boundary: fresh material.default authors
// the current matte law, while old response-less saved Materials migrate once to
// explicit legacy compatibility ASTs so their appearance does not silently change.

#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Material/MaterialResponseDefaults.hpp"
#include "ConstructedBeing/Material/MaterialPersistenceMigration.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <map>
#include <set>
#include <stdexcept>

static bool nearf(float a, float b, float eps = 1e-5f) { return std::fabs(a - b) < eps; }

int main() {
    // --- 1. A Material is a legible, drivable being --------------------------
    {
        Material m("clay");
        assert(m.getIdentifier() == "material.clay");

        // The vocabulary is discoverable (authoring UIs list these).
        assert(!m.listProperties().empty());

        // A "Law" reads a field.
        PropertyValue out;
        assert(PropertyPath::parse("shininess").getValue(m, out) == PropertyPath::PathResult::Ok);
        assert(nearf(std::get<float>(out), 32.0f));

        // A "Law" drives a field — the payoff of material-as-being.
        assert(PropertyPath::parse("shininess").setValue(m, PropertyValue(8.0f)) == PropertyPath::PathResult::Ok);
        assert(nearf(m.shininess, 8.0f));

        assert(PropertyPath::parse("baseColor").setValue(
            m, PropertyValue(glm::vec3(0.6f, 0.3f, 0.1f))) == PropertyPath::PathResult::Ok);
        assert(nearf(m.baseColor.r, 0.6f) && nearf(m.baseColor.g, 0.3f) && nearf(m.baseColor.b, 0.1f));
        std::printf("  being:   material.clay is law-addressable (shininess, baseColor driven)\n");
    }

    // --- 1b. Rung 9 response is independent authored Material truth -----------
    {
        Material m("response_clay");
        assert(m.findProperty("responseExpr") != nullptr);
        assert(m.findProperty("environmentResponseExpr") != nullptr);
        assert(!m.responseExpr);
        assert(!m.environmentResponseExpr);
        const uint32_t colorRevisionBefore = m.getRevision();
        const uint32_t responseRevisionBefore = m.getResponseRevision();

        auto root = std::make_shared<OntoMath::MathNode>();
        root->op = OntoMath::MathNode::Op::VectorConstruct;
        for (double value : {0.25, 0.5, 0.75}) {
            auto c = std::make_unique<OntoMath::MathNode>();
            c->op = OntoMath::MathNode::Op::ScalarLeaf;
            c->scalarForm = OntoMath::ScalarForm::constant(value);
            root->children.push_back(std::move(c));
        }
        OntoMath::Piecewise response = OntoMath::Piecewise::continuous(root);
        const std::string serialized = response.toJson().dump();

        assert(PropertyPath::parse("responseExpr").setValue(
                   m, PropertyValue(serialized)) == PropertyPath::PathResult::Ok);
        assert(m.responseExpr);
        assert(m.getResponseRevision() == responseRevisionBefore + 1);
        assert(m.getRevision() == colorRevisionBefore);
        assert(m.responseExpr->toJson() == response.toJson());

        const nlohmann::json saved = m.toJson();
        assert(saved.contains("responseExpr"));
        Material restored = Material::fromJson(saved);
        assert(restored.responseExpr);
        assert(restored.responseExpr->toJson() == response.toJson());

        const uint32_t responseRevisionAfter = m.getResponseRevision();
        assert(PropertyPath::parse("responseExpr").setValue(
                   m, PropertyValue(std::string("{not-json"))) != PropertyPath::PathResult::Ok);
        assert(m.getResponseRevision() == responseRevisionAfter);
        assert(m.responseExpr->toJson() == response.toJson());

        std::printf("  rung9:    responseExpr is Law-addressable, persisted, and revision-independent\n");
    }

    // --- 1c. AD-21 Bind reads ordinary Material PropertyPaths -----------------
    {
        Material m("bound_clay");
        m.shininess = 24.0f;

        auto bind = std::make_shared<OntoMath::MathNode>();
        bind->op = OntoMath::MathNode::Op::Bind;
        bind->propertyPath = "shininess";

        auto value = bind->evaluate({}, &m);
        double numeric = 0.0;
        assert(value && propertyValueToNumber(*value, numeric));
        assert(nearf(static_cast<float>(numeric), 24.0f));

        std::set<std::string> paths;
        bind->collectPropertyBindings(paths);
        assert(paths.size() == 1 && paths.count("shininess") == 1);

        const nlohmann::json encoded = bind->toJson();
        assert(encoded["op"] == 42 && encoded["path"] == "shininess");
        auto restored = OntoMath::MathNode::fromJson(encoded);
        assert(restored && restored->op == OntoMath::MathNode::Op::Bind);
        assert(restored->propertyPath == "shininess");

        assert(PropertyPath::parse("shininess").setValue(
                   m, PropertyValue(7.0f)) == PropertyPath::PathResult::Ok);
        value = restored->evaluate({}, &m);
        assert(value && propertyValueToNumber(*value, numeric));
        assert(nearf(static_cast<float>(numeric), 7.0f));

        OntoMath::MathNode unresolved;
        unresolved.op = OntoMath::MathNode::Op::Bind;
        unresolved.propertyPath = "does.not.exist";
        assert(!unresolved.evaluate({}, &m));

        // Dynamic authored parameters are first-class Material state too. A
        // response Bind that survives only until save/load is not persistence.
        assert(m.setDynamicProperty("roughness", PropertyValue(0.42f)));
        auto roughBind = std::make_unique<OntoMath::MathNode>();
        roughBind->op = OntoMath::MathNode::Op::Bind;
        roughBind->propertyPath = "roughness";
        auto roughResponse = std::make_shared<OntoMath::MathNode>();
        roughResponse->op = OntoMath::MathNode::Op::VectorConstruct;
        roughResponse->children.push_back(std::move(roughBind));
        for (int i = 0; i < 2; ++i) {
            auto zero = std::make_unique<OntoMath::MathNode>();
            zero->op = OntoMath::MathNode::Op::ScalarLeaf;
            zero->scalarForm = OntoMath::ScalarForm::constant(0.0);
            roughResponse->children.push_back(std::move(zero));
        }
        m.responseExpr = std::make_shared<OntoMath::Piecewise>(
            OntoMath::Piecewise::continuous(roughResponse));
        m.bumpResponseRevision();

        const nlohmann::json boundSaved = m.toJson();
        assert(boundSaved.contains("authoredProperties"));
        assert(boundSaved["authoredProperties"].contains("roughness"));
        Material boundRestored = Material::fromJson(boundSaved);
        PropertyValue roughness;
        assert(boundRestored.getDynamicProperty("roughness", roughness));
        double roughNumber = 0.0;
        assert(propertyValueToNumber(roughness, roughNumber));
        assert(nearf(static_cast<float>(roughNumber), 0.42f));
        assert(boundRestored.responseExpr);
        auto rebound = boundRestored.responseExpr->evaluate({}, &boundRestored);
        assert(rebound && std::holds_alternative<glm::vec3>(*rebound));
        assert(nearf(std::get<glm::vec3>(*rebound).x, 0.42f));

        std::printf("  bind:     OntoMath Bind reads live/persisted Material PropertyPaths\n");
    }

    // --- 2. Gate B: fresh matte default vs one-time legacy migration ----------
    {
        MaterialManager mm;
        auto def = mm.defaultMaterial();
        assert(def);
        assert(def->getIdentifier() == "material.default");
        assert(def->responseExpr &&
               "fresh material.default must own the current authored matte law");
        assert(def->environmentResponseExpr &&
               "fresh material.default must own current environment response");

        // Historical compatibility coefficients remain stored/Law-addressable,
        // but the fresh default response must not Bind them.
        assert(nearf(def->ambient, 0.2f));
        assert(nearf(def->diffuse, 0.8f));
        assert(nearf(def->specular, 1.0f));
        assert(nearf(def->shininess, 32.0f));
        std::set<std::string> freshBindings;
        for (const auto& piece : def->responseExpr->pieces) {
            if (piece.mathNode) piece.mathNode->collectPropertyBindings(freshBindings);
            if (piece.whereLEZero) piece.whereLEZero->collectPropertyBindings(freshBindings);
        }
        assert(freshBindings.empty() &&
               "fresh default must not resurrect legacy diffuse/specular/shininess Bind inputs");

        const std::map<std::string, PropertyValue> matteVars{
            {OntoMath::kSurfaceNormalVar,
             PropertyValue(glm::vec3(0.0f, 0.0f, 1.0f))},
            {OntoMath::kSurfaceColorVar,
             PropertyValue(glm::vec3(0.5f, 0.25f, 1.0f))},
            {OntoMath::kIncidentRadianceVar,
             PropertyValue(glm::vec3(0.25f, 0.10f, 0.05f))},
            {OntoMath::kWiXVar, PropertyValue(0.0)},
            {OntoMath::kWiYVar, PropertyValue(0.0)},
            {OntoMath::kWiZVar, PropertyValue(-1.0)}
        };
        const auto matteValue = def->responseExpr->evaluate(matteVars, def.get());
        assert(matteValue && std::holds_alternative<glm::vec3>(*matteValue));
        const glm::vec3 matteRgb = std::get<glm::vec3>(*matteValue);
        assert(nearf(matteRgb.x, 0.125f) &&
               nearf(matteRgb.y, 0.025f) &&
               nearf(matteRgb.z, 0.05f));

        const std::map<std::string, PropertyValue> envVars{
            {OntoMath::kSurfaceColorVar,
             PropertyValue(glm::vec3(0.5f, 0.25f, 1.0f))},
            {OntoMath::kEnvironmentRadianceVar,
             PropertyValue(glm::vec3(1.0f, 0.5f, 0.25f))}
        };
        const auto envValue =
            def->environmentResponseExpr->evaluate(envVars, def.get());
        assert(envValue && std::holds_alternative<glm::vec3>(*envValue));
        const glm::vec3 envRgb = std::get<glm::vec3>(*envValue);
        assert(nearf(envRgb.x, 0.5f) &&
               nearf(envRgb.y, 0.125f) &&
               nearf(envRgb.z, 0.25f));

        // Old save: no responseExpr and no omission marker. Hydration migrates
        // once to explicit legacy compatibility ASTs.
        nlohmann::json oldSaved{
            {"name", "legacy_clay"},
            {"ambient", 0.2f},
            {"diffuse", 0.8f},
            {"specular", 1.0f},
            {"shininess", 32.0f}
        };
        Material legacy = Material::fromJson(oldSaved);
        assert(legacy.responseExpr);
        assert(legacy.environmentResponseExpr);

        std::set<std::string> legacyBindings;
        for (const auto& piece : legacy.responseExpr->pieces) {
            if (piece.mathNode) piece.mathNode->collectPropertyBindings(legacyBindings);
            if (piece.whereLEZero) piece.whereLEZero->collectPropertyBindings(legacyBindings);
        }
        assert(legacyBindings.size() == 3 &&
               legacyBindings.count("diffuse") == 1 &&
               legacyBindings.count("specular") == 1 &&
               legacyBindings.count("shininess") == 1);

        const std::map<std::string, PropertyValue> legacyVars{
            {OntoMath::kSurfaceNormalVar,
             PropertyValue(glm::vec3(0.0f, 0.0f, 1.0f))},
            {OntoMath::kSurfaceColorVar,
             PropertyValue(glm::vec3(0.5f, 0.25f, 1.0f))},
            {OntoMath::kIncidentRadianceVar, PropertyValue(glm::vec3(1.0f))},
            {OntoMath::kWiXVar, PropertyValue(0.0)},
            {OntoMath::kWiYVar, PropertyValue(0.0)},
            {OntoMath::kWiZVar, PropertyValue(-1.0)},
            {OntoMath::kWoXVar, PropertyValue(0.0)},
            {OntoMath::kWoYVar, PropertyValue(0.0)},
            {OntoMath::kWoZVar, PropertyValue(1.0)},
            {OntoMath::kLegacyDiffuseIncidentVar,
             PropertyValue(glm::vec3(0.25f, 0.10f, 0.05f))},
            {OntoMath::kLegacySpecularIncidentVar,
             PropertyValue(glm::vec3(0.40f, 0.20f, 0.05f))}
        };
        const auto legacyValue =
            legacy.responseExpr->evaluate(legacyVars, &legacy);
        assert(legacyValue && std::holds_alternative<glm::vec3>(*legacyValue));
        const glm::vec3 legacyRgb = std::get<glm::vec3>(*legacyValue);
        assert(nearf(legacyRgb.x, 0.50f) &&
               nearf(legacyRgb.y, 0.22f) &&
               nearf(legacyRgb.z, 0.09f));

        const auto migratedSaved = legacy.toJson();
        assert(migratedSaved.contains("responseExpr"));
        assert(migratedSaved.contains("environmentResponseExpr"));
        assert(!migratedSaved.contains(
            MaterialResponseDefaults::kOmissionSemanticsKey));

        // Current material: response omission intentionally means inheritance.
        Material currentInherited("current_inherited");
        assert(!currentInherited.responseExpr);
        const auto currentSaved = currentInherited.toJson();
        assert(currentSaved.value(
                   MaterialResponseDefaults::kOmissionSemanticsKey,
                   std::string()) ==
               MaterialResponseDefaults::kInheritDefaultV1);
        Material currentRestored = Material::fromJson(currentSaved);
        assert(!currentRestored.responseExpr);
        assert(!currentRestored.environmentResponseExpr);

        // Unknown future omission semantics must refuse rather than silently
        // reinterpret a save as either legacy or current.
        nlohmann::json unknown = currentSaved;
        unknown[MaterialResponseDefaults::kOmissionSemanticsKey] =
            "future-policy-v999";
        bool refusedUnknown = false;
        try {
            (void)Material::fromJson(unknown);
        } catch (const std::invalid_argument&) {
            refusedUnknown = true;
        }
        assert(refusedUnknown);

        // The GLOBAL manager may not infer old-world fallback semantics from a
        // missing/default record. It always reinstates the CURRENT matte default;
        // document-local persistence migration owns old-world preservation.
        MaterialManager legacySnapshot;
        legacySnapshot.loadFromJson(nlohmann::json::array({
            nlohmann::json{{"name", "old_stone"}}
        }));
        auto canonicalDefault = legacySnapshot.defaultMaterial();
        assert(canonicalDefault && canonicalDefault->responseExpr);
        std::set<std::string> canonicalBindings;
        for (const auto& piece : canonicalDefault->responseExpr->pieces) {
            if (piece.mathNode)
                piece.mathNode->collectPropertyBindings(canonicalBindings);
            if (piece.whereLEZero)
                piece.whereLEZero->collectPropertyBindings(canonicalBindings);
        }
        assert(canonicalBindings.empty() &&
               "global fallback became legacy merely because a snapshot omitted default");

        // A concrete old Zone document gets a collision-free scoped legacy
        // default and only its own default references are rewritten.
        nlohmann::json oldZone{
            {"identifier", "Old Zone / 成都"},
            {"materials", nlohmann::json::array({
                nlohmann::json{
                    {"name", "default"},
                    {"ambient", 0.2f},
                    {"diffuse", 0.8f},
                    {"specular", 1.0f},
                    {"shininess", 32.0f}
                },
                nlohmann::json{{"name", "stone"}}
            })},
            {"world", {
                {"objects", nlohmann::json::array({
                    nlohmann::json{
                        {"objectID", "old-default-object"},
                        {"materialId", "material.default"}
                    },
                    nlohmann::json{
                        {"objectID", "named-object"},
                        {"materialId", "material.stone"}
                    }
                })}
            }}
        };
        const auto scoped =
            MaterialPersistenceMigration::migrateLegacyDefaultDocument(
                oldZone, "zone:Old Zone / 成都");
        assert(scoped.migrated && !scoped.synthesized);
        assert(scoped.scopedMaterialId != "material.default");
        assert(scoped.scopedMaterialId ==
               MaterialPersistenceMigration::scopedLegacyDefaultId(
                   "zone:Old Zone / 成都"));
        assert(scoped.document["world"]["objects"][0]["materialId"] ==
               scoped.scopedMaterialId);
        assert(scoped.document["world"]["objects"][1]["materialId"] ==
               "material.stone");
        assert(MaterialPersistenceMigration::materialIdentity(
                   scoped.document["materials"][0]) ==
               scoped.scopedMaterialId);

        // Full identity encoding, not a finite hash: another owning identity
        // must produce a different compatibility Material.
        assert(MaterialPersistenceMigration::scopedLegacyDefaultId(
                   "zone:Old Zone / 成都") !=
               MaterialPersistenceMigration::scopedLegacyDefaultId(
                   "zone:Other Zone / 成都"));

        legacySnapshot.mergeFromJson(scoped.document["materials"]);
        auto scopedLegacy =
            legacySnapshot.get(scoped.scopedMaterialId);
        assert(scopedLegacy && scopedLegacy->responseExpr);
        std::set<std::string> scopedBindings;
        for (const auto& piece : scopedLegacy->responseExpr->pieces) {
            if (piece.mathNode)
                piece.mathNode->collectPropertyBindings(scopedBindings);
            if (piece.whereLEZero)
                piece.whereLEZero->collectPropertyBindings(scopedBindings);
        }
        assert(scopedBindings.count("specular") == 1);
        canonicalBindings.clear();
        for (const auto& piece : legacySnapshot.defaultMaterial()->responseExpr->pieces) {
            if (piece.mathNode)
                piece.mathNode->collectPropertyBindings(canonicalBindings);
        }
        assert(canonicalBindings.empty() &&
               "scoped legacy Zone poisoned canonical material.default");

        // Migration is one-way/idempotent: once scoped, no default reference
        // remains to migrate again.
        const auto scopedAgain =
            MaterialPersistenceMigration::migrateLegacyDefaultDocument(
                scoped.document, "zone:Old Zone / 成都");
        assert(!scopedAgain.migrated);

        // Pre-material legacy documents still preserve appearance: a default
        // object with no materials bag gets a synthesized scoped compatibility
        // Material.
        nlohmann::json preMaterialZone{
            {"identifier", "Pre Materials"},
            {"objects", nlohmann::json::array({
                nlohmann::json{
                    {"objectID", "pre-material-object"}
                    // Deliberately NO materialId: Object::from_json historically
                    // interprets this omission as material.default.
                }
            })}
        };
        const auto synthesized =
            MaterialPersistenceMigration::migrateLegacyDefaultDocument(
                preMaterialZone, "zone:Pre Materials");
        assert(synthesized.migrated && synthesized.synthesized);
        assert(synthesized.document["materials"].size() == 1);
        assert(synthesized.document["objects"][0]["materialId"] ==
               synthesized.scopedMaterialId &&
               "pre-material omission was reinterpreted as current matte default");

        // Gate-A-era explicit legacy defaults are recognized too; otherwise a
        // save written after the first migration could re-poison the global
        // default on its second load.
        nlohmann::json gateADefault{
            {"name", "default"},
            {"responseExpr",
             MaterialResponseDefaults::makeLegacyDirectResponse()->toJson()}
            // Deliberately no environmentResponseExpr: once the direct AST is
            // recognized as the exact historical default, Gate B restores the
            // matching historical environment law while scoping it.
        };
        nlohmann::json gateAZone{
            {"identifier", "Gate A Old Zone"},
            {"materials", nlohmann::json::array({gateADefault})},
            {"objects", nlohmann::json::array({
                nlohmann::json{
                    {"objectID", "gate-a-object"},
                    {"materialId", "material.default"}
                }
            })}
        };
        const auto gateAScoped =
            MaterialPersistenceMigration::migrateLegacyDefaultDocument(
                gateAZone, "zone:Gate A Old Zone");
        assert(gateAScoped.migrated &&
               gateAScoped.document["objects"][0]["materialId"] ==
                   gateAScoped.scopedMaterialId);
        assert(gateAScoped.document["materials"][0].contains(
                   "environmentResponseExpr") &&
               "Gate-A explicit legacy default lost historical environment law");

        // A shared root named material.default is an ambiguous historical
        // authority at this helper layer: the root may carry custom old
        // coefficients. Refuse rather than synthesize generic legacy values.
        nlohmann::json sharedDefaultZone{
            {"identifier", "Shared Default Root"},
            {"materialRefs",
             nlohmann::json::array({"material.default"})},
            {"objects", nlohmann::json::array({
                nlohmann::json{
                    {"objectID", "shared-default-object"},
                    {"materialId", "material.default"}
                }
            })}
        };
        bool refusedSharedDefaultRoot = false;
        try {
            (void)MaterialPersistenceMigration::migrateLegacyDefaultDocument(
                sharedDefaultZone, "zone:Shared Default Root");
        } catch (const std::invalid_argument&) {
            refusedSharedDefaultRoot = true;
        }
        assert(refusedSharedDefaultRoot &&
               "ambiguous shared legacy default root was replaced with invented coefficients");

        // A current explicit matte default is NOT legacy evidence and must not
        // be renamed by the Gate B migrator.
        nlohmann::json currentZone{
            {"identifier", "Current Zone"},
            {"materials", nlohmann::json::array({
                nlohmann::json{
                    {"name", "default"},
                    {"responseExpr",
                     MaterialResponseDefaults::makeCurrentDefaultDirectResponse()->toJson()},
                    {"environmentResponseExpr",
                     MaterialResponseDefaults::makeCurrentDefaultEnvironmentResponse()->toJson()}
                }
            })},
            {"objects", nlohmann::json::array({
                nlohmann::json{
                    {"objectID", "current-object"},
                    {"materialId", "material.default"}
                }
            })}
        };
        const auto currentScoped =
            MaterialPersistenceMigration::migrateLegacyDefaultDocument(
                currentZone, "zone:Current Zone");
        assert(!currentScoped.migrated);

        std::printf("  gate-b: matte global default + scoped legacy document migration\n");
    }

    // --- 3. Manager ownership, resolution, and the undeletable default -------
    {
        MaterialManager mm;
        auto clay = mm.create("clay");
        clay->baseColor = glm::vec3(0.6f, 0.3f, 0.1f);

        // create is idempotent by name (no duplicate beings).
        assert(mm.create("clay") == clay);

        // Resolve by full identifier and by bare name.
        assert(mm.get("material.clay") == clay);
        assert(mm.get("clay") == clay);

        // A dangling reference resolves to the default, never nullptr.
        assert(mm.get("material.nope") == nullptr);
        assert(mm.resolveOrDefault("material.nope") == mm.defaultMaterial());

        // The default is load-bearing and cannot be removed.
        assert(!mm.remove("material.default"));
        assert(mm.remove("material.clay"));
        assert(mm.get("clay") == nullptr);

        // An UNSCOPED legacy default may not replace canonical global state.
        // Persistence must transplant it first.
        const std::size_t beforeMergeCount = mm.getAll().size();
        const std::string defaultBefore =
            mm.defaultMaterial()->responseExpr->toJson().dump();
        mm.mergeFromJson(nlohmann::json::array({
            nlohmann::json{{"name", "default"}}
        }));
        assert(mm.getAll().size() == beforeMergeCount);
        assert(mm.defaultMaterial()->responseExpr->toJson().dump() ==
               defaultBefore);

        // Current explicit authored default state may still be merged normally.
        nlohmann::json customDefault{
            {"name", "default"},
            {"baseColor", {0.2f, 0.3f, 0.4f}},
            {"responseExpr",
             MaterialResponseDefaults::makeCurrentDefaultDirectResponse()->toJson()},
            {"environmentResponseExpr",
             MaterialResponseDefaults::makeCurrentDefaultEnvironmentResponse()->toJson()}
        };
        mm.mergeFromJson(nlohmann::json::array({customDefault}));
        assert(mm.getAll().size() == beforeMergeCount);
        assert(nearf(mm.defaultMaterial()->baseColor.r, 0.2f));

        // Already-deserialized Material objects can enter through shared roots
        // and stored Singular restoration, bypassing JSON mergeFromJson. The
        // object-level registry boundary must enforce the same law.
        auto directLegacyDefault = std::make_shared<Material>("default");
        directLegacyDefault->responseExpr =
            MaterialResponseDefaults::makeLegacyDirectResponse();
        directLegacyDefault->environmentResponseExpr =
            MaterialResponseDefaults::makeLegacyEnvironmentResponse();
        directLegacyDefault->baseColor = glm::vec3(0.91f, 0.01f, 0.02f);
        mm.add(directLegacyDefault);
        assert(nearf(mm.defaultMaterial()->baseColor.r, 0.2f) &&
               "direct legacy default Material bypass poisoned canonical fallback");

        auto responseLessDefault = std::make_shared<Material>("default");
        responseLessDefault->baseColor = glm::vec3(0.88f, 0.02f, 0.03f);
        mm.add(responseLessDefault);
        assert(nearf(mm.defaultMaterial()->baseColor.r, 0.2f) &&
               "response-less direct default Material bypass poisoned canonical fallback");

        // Same refusal must hold for REPLACE/snapshot ingestion. A default that
        // says "inherit material.default" would be self-referential, not a
        // lawful canonical default.
        nlohmann::json selfInheritingDefault{
            {"name", "default"},
            {MaterialResponseDefaults::kOmissionSemanticsKey,
             MaterialResponseDefaults::kInheritDefaultV1},
            {"baseColor", {0.77f, 0.01f, 0.02f}}
        };
        mm.loadFromJson(nlohmann::json::array({selfInheritingDefault}));
        assert(mm.defaultMaterial() && mm.defaultMaterial()->responseExpr);
        assert(nearf(mm.defaultMaterial()->baseColor.r, 1.0f) &&
               "snapshot installed response-less self-inheriting material.default");

        auto directCurrentDefault = std::make_shared<Material>("default");
        directCurrentDefault->baseColor = glm::vec3(0.15f, 0.25f, 0.35f);
        directCurrentDefault->responseExpr =
            MaterialResponseDefaults::makeCurrentDefaultDirectResponse();
        directCurrentDefault->environmentResponseExpr =
            MaterialResponseDefaults::makeCurrentDefaultEnvironmentResponse();
        mm.add(directCurrentDefault);
        assert(nearf(mm.defaultMaterial()->baseColor.r, 0.15f) &&
               "explicit current default Material could not replace canonical state");

        std::printf("  manager: JSON and direct-object legacy defaults refused; current authored default remains replaceable\n");
    }

    // --- 4. Serialization round-trip -----------------------------------------
    {
        MaterialManager mm;
        auto clay = mm.create("clay");
        clay->baseColor = glm::vec3(0.6f, 0.3f, 0.1f);
        clay->shininess = 8.0f;

        MaterialManager loaded;
        loaded.loadFromJson(mm.toJson());

        auto rt = loaded.get("clay");
        assert(rt);
        assert(nearf(rt->baseColor.r, 0.6f) && nearf(rt->shininess, 8.0f));
        assert(!rt->responseExpr &&
               "current response omission must survive round-trip as inheritance");
        assert(loaded.defaultMaterial());
        assert(loaded.defaultMaterial()->responseExpr);
        std::set<std::string> roundTripDefaultBindings;
        for (const auto& piece : loaded.defaultMaterial()->responseExpr->pieces) {
            if (piece.mathNode)
                piece.mathNode->collectPropertyBindings(roundTripDefaultBindings);
        }
        assert(roundTripDefaultBindings.empty() &&
               "fresh matte default must survive manager round-trip");
        std::printf("  serial: current inheritance and matte default round-trip through JSON\n");
    }

    // --- 5. Direct Material methods and faceTextures unit tests --------------
    {
        // Default constructor & getters/setters
        Material m_def;
        assert(m_def.name() == "default");
        assert(m_def.getIdentifier() == "material.default");
        assert(nearf(m_def.baseColor.r, 1.0f) && nearf(m_def.baseColor.g, 1.0f) && nearf(m_def.baseColor.b, 1.0f));
        assert(nearf(m_def.opacity, 1.0f));
        assert(nearf(m_def.shininess, 32.0f));
        assert(nearf(m_def.specular, 1.0f));
        assert(nearf(m_def.ambient, 0.2f));
        assert(nearf(m_def.diffuse, 0.8f));
        assert(m_def.faceTextures.empty());

        m_def.setName("stone");
        assert(m_def.name() == "stone");
        assert(m_def.getIdentifier() == "material.stone");

        // initFaceTextures
        m_def.initFaceTextures(6, 32, 32);
        assert(m_def.faceTextures.size() == 6);
        assert(m_def.faceTextures[0].width == 32);
        assert(m_def.faceTextures[0].height == 32);
        assert(m_def.faceTextures[0].pixels.size() == 32 * 32 * 4);

        // Idempotency check: calling initFaceTextures with same count leaves existing textures untouched
        m_def.faceTextures[0].pixels[0] = 0xAA;
        m_def.initFaceTextures(6, 64, 64);
        assert(m_def.faceTextures[0].pixels[0] == 0xAA);
        assert(m_def.faceTextures[0].width == 32);

        // Changing count re-initialises
        m_def.initFaceTextures(4, 16, 16);
        assert(m_def.faceTextures.size() == 4);
        assert(m_def.faceTextures[0].width == 16);

        std::printf("  material: default state, name setter, initFaceTextures tested\n");
    }

    // --- 6. Property registration completeness --------------------------------
    {
        Material m("prop_test");
        assert(m.findProperty("baseColor") != nullptr);
        assert(m.findProperty("opacity") != nullptr);
        assert(m.findProperty("shininess") != nullptr);
        assert(m.findProperty("specular") != nullptr);
        assert(m.findProperty("ambient") != nullptr);
        assert(m.findProperty("diffuse") != nullptr);
        assert(m.findProperty("colorExpr") != nullptr);
        assert(m.findProperty("responseExpr") != nullptr);
        assert(m.findProperty("environmentResponseExpr") != nullptr);
        assert(m.findProperty("nonexistent") == nullptr);
        std::printf("  properties: Material response channels are registered and accessible\n");
    }

    // --- 7. Material serialization edge cases and faceTextures ----------------
    {
        Material m("painted");
        m.initFaceTextures(2, 8, 8);
        m.faceTextures[0].pixels[0] = 128; // modified pixel

        nlohmann::json j = m.toJson();
        assert(j["name"] == "painted");
        assert(j.contains("faceTextures"));
        assert(j["faceTextures"].is_array());
        assert(j["faceTextures"].size() == 2);

        Material restored = Material::fromJson(j);
        assert(restored.name() == "painted");
        assert(restored.faceTextures.size() == 2);
        assert(restored.faceTextures[0].width == 8);
        assert(restored.faceTextures[0].height == 8);
        assert(restored.faceTextures[0].pixels.size() == 8 * 8 * 4);
        assert(restored.faceTextures[0].pixels[0] == 128);

        // JSON missing optional fields falls back to defaults
        nlohmann::json empty_j = nlohmann::json::object();
        Material fallback = Material::fromJson(empty_j);
        assert(fallback.name() == "default");
        assert(nearf(fallback.opacity, 1.0f));
        assert(nearf(fallback.shininess, 32.0f));

        // Invalid texture size handling
        nlohmann::json invalid_tex_j = {
            {"name", "invalid_tex"},
            {"faceTextures", {{{"width", -10}, {"height", 64}}}}
        };
        Material inv_m = Material::fromJson(invalid_tex_j);
        assert(inv_m.faceTextures.empty());

        std::printf("  serialization: faceTextures round-trip and fallback defaults tested\n");
    }

    std::printf("material_being_test: ALL OK\n");
    return 0;
}
