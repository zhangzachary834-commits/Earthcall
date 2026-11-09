#include "Singularity/Screen/WebGPU/FieldVmDependencies.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"

#include <cstdio>
#include <limits>
#include <memory>

namespace {
int failures = 0;
void check(bool ok, const char* what) {
    std::printf("  %s: %s\n", ok ? "ok" : "FAILED", what);
    if (!ok) ++failures;
}

std::shared_ptr<OntoMath::MathNode> scalar(double v) {
    auto n = std::make_shared<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ScalarLeaf;
    n->scalarForm = OntoMath::ScalarForm::constant(v);
    return n;
}

std::shared_ptr<OntoMath::MathNode> bind(const char* path) {
    auto n = std::make_shared<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::Bind;
    n->propertyPath = path;
    return n;
}

std::shared_ptr<OntoMath::MathNode> value(const char* name) {
    auto n = std::make_shared<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ValueLeaf;
    n->variableName = name;
    return n;
}

bool has(const sdfwgsl::FieldVmDependencyManifest& m,
         sdfwgsl::FieldVmDependencyKind kind,
         const std::string& key) {
    for (const auto& d : m.dependencies)
        if (d.kind == kind && d.key == key) return true;
    return false;
}
} // namespace

int main() {
    std::printf("Running Field-VM dependency preflight tribunal...\n");

    // One authored AST; two runtime snapshots. Structure must not mutate merely
    // because the authoritative property value changes.
    auto add = std::make_shared<OntoMath::MathNode>();
    add->op = OntoMath::MathNode::Op::Add;
    add->children = {value("x"), bind("radius")};
    const auto boundField = geom::makeImplicit(add);

    const auto manifest = sdfwgsl::inspectFieldVmDependencies(boundField);
    check(manifest.structurallyValid() &&
              manifest.dependencies.size() == 1 &&
              has(manifest, sdfwgsl::FieldVmDependencyKind::PropertyPath,
                  "radius"),
          "Bind is exposed as PropertyPath dependency; local x is not external");

    std::map<std::string, PropertyValue> valuesA{
        {"radius", PropertyValue(0.5)}
    };
    std::map<std::string, PropertyValue> valuesB{
        {"radius", PropertyValue(0.9)}
    };
    sdfwgsl::FieldVmDependencyContext ctxA;
    ctxA.propertyBindings = &valuesA;
    sdfwgsl::FieldVmDependencyContext ctxB;
    ctxB.propertyBindings = &valuesB;

    const auto readyA = sdfwgsl::assessFieldVmDependencies(manifest, ctxA);
    const auto readyB = sdfwgsl::assessFieldVmDependencies(manifest, ctxB);
    check(readyA.ready && readyB.ready &&
              manifest.dependencies ==
                  sdfwgsl::inspectFieldVmDependencies(boundField).dependencies,
          "value-only binding changes leave structural dependency manifest unchanged");

    const auto snapshotA =
        sdfwgsl::snapshotFieldVmDependencies(manifest, ctxA);
    const auto snapshotB =
        sdfwgsl::snapshotFieldVmDependencies(manifest, ctxB);
    check(snapshotA.valid && snapshotB.valid &&
              snapshotA.identity != snapshotB.identity,
          "same AST with changed executable binding value gets a different exact runtime snapshot");

    const auto missing =
        sdfwgsl::assessFieldVmDependencies(manifest, {});
    check(!missing.ready && missing.missing.size() == 1 &&
              missing.missing[0] == "radius",
          "missing PropertyPath provider is explicit unresolved evidence");

    std::map<std::string, PropertyValue> nanValues{
        {"radius", PropertyValue(
            std::numeric_limits<double>::quiet_NaN())}
    };
    sdfwgsl::FieldVmDependencyContext nanCtx;
    nanCtx.propertyBindings = &nanValues;
    const auto nonFinite =
        sdfwgsl::assessFieldVmDependencies(manifest, nanCtx);
    check(!nonFinite.ready && nonFinite.nonFinite.size() == 1 &&
              nonFinite.nonFinite[0] == "radius",
          "non-finite bound value revokes readiness instead of becoming field truth");
    check(!sdfwgsl::snapshotFieldVmDependencies(
              manifest, nanCtx).valid,
          "non-finite binding cannot mint an executable dependency snapshot");

    std::map<std::string, PropertyValue> hugeValues{
        {"radius", PropertyValue(1e300)}
    };
    sdfwgsl::FieldVmDependencyContext hugeCtx;
    hugeCtx.propertyBindings = &hugeValues;
    const auto huge =
        sdfwgsl::assessFieldVmDependencies(manifest, hugeCtx);
    check(!huge.ready && huge.outOfFloat32Range.size() == 1 &&
              huge.outOfFloat32Range[0] == "radius" &&
              !sdfwgsl::snapshotFieldVmDependencies(
                  manifest, hugeCtx).valid,
          "finite double outside float32 VM range refuses before snapshot identity");

    std::map<std::string, PropertyValue> stringValues{
        {"radius", PropertyValue(std::string("large"))}
    };
    sdfwgsl::FieldVmDependencyContext stringCtx;
    stringCtx.propertyBindings = &stringValues;
    const auto unsupported =
        sdfwgsl::assessFieldVmDependencies(manifest, stringCtx);
    check(!unsupported.ready && unsupported.unsupported.size() == 1,
          "unsupported bound PropertyValue type remains explicit refusal");

    const auto namedField = geom::makeImplicit(value("temperature"));
    const auto namedManifest =
        sdfwgsl::inspectFieldVmDependencies(namedField);
    check(has(namedManifest, sdfwgsl::FieldVmDependencyKind::NamedValue,
              "temperature"),
          "unbound ValueLeaf is named as query-context dependency");

    std::map<std::string, PropertyValue> namedValues{
        {"temperature", PropertyValue(12.0)}
    };
    sdfwgsl::FieldVmDependencyContext namedCtx;
    namedCtx.namedValues = &namedValues;
    check(sdfwgsl::assessFieldVmDependencies(
              namedManifest, namedCtx).ready,
          "explicit finite named-value provider satisfies preflight without granting VM admission");

    // World/context dependencies are distinct from local pure mathematical
    // control. None are silently replaced by an invented subject or world.
    auto pw = std::make_shared<OntoMath::Piecewise>();
    OntoMath::Piecewise::Piece piece;
    piece.mathNode = scalar(-1.0);
    piece.whereLEZero = scalar(-1.0);
    piece.guard = std::make_shared<ConditionNode>();
    piece.call = std::make_shared<OntoMath::FunctionCall>();
    piece.call->function = "field.helper";
    piece.fold = std::make_shared<OntoMath::Fold>();
    pw->pieces.push_back(piece);

    const auto contextManifest =
        sdfwgsl::inspectFieldVmDependencies(geom::makeImplicit(pw));
    check(has(contextManifest, sdfwgsl::FieldVmDependencyKind::PureGuard,
              "piece[0].whereLEZero") &&
              has(contextManifest, sdfwgsl::FieldVmDependencyKind::WorldGuard,
                  "piece[0].guard") &&
              has(contextManifest, sdfwgsl::FieldVmDependencyKind::FunctionCall,
                  "piece[0].call") &&
              has(contextManifest, sdfwgsl::FieldVmDependencyKind::WorldFold,
                  "piece[0].fold"),
          "Piecewise preflight distinguishes local control from external authorities");

    const auto absentContext =
        sdfwgsl::assessFieldVmDependencies(contextManifest, {});
    check(!absentContext.ready &&
              absentContext.unresolvedContext.size() == 3,
          "world guard/call/fold remain unresolved while pure guard needs no invented subject");

    sdfwgsl::FieldVmDependencyContext fullContext;
    fullContext.subjectAvailable = true;
    fullContext.worldConditionContextAvailable = true;
    fullContext.functionRegistryAvailable = true;
    fullContext.worldEnumerationAvailable = true;
    check(sdfwgsl::assessFieldVmDependencies(
              contextManifest, fullContext).ready,
          "explicit context availability satisfies preflight only");

    const auto unstamped =
        sdfwgsl::snapshotFieldVmDependencies(contextManifest, fullContext);
    check(!unstamped.valid &&
              unstamped.missingAuthorityIdentity.size() == 4,
          "available external contexts without exact authority identities cannot mint a reusable snapshot");

    fullContext.subjectIdentity = "singular:subject-A@revision-7";
    fullContext.worldConditionContextIdentity = "condition-world@revision-21";
    fullContext.functionRegistryIdentity = "function-registry@revision-3";
    fullContext.worldEnumerationIdentity = "world-enumeration@revision-9";
    const auto contextSnapshotA =
        sdfwgsl::snapshotFieldVmDependencies(contextManifest, fullContext);
    check(contextSnapshotA.valid,
          "explicit authority identities mint a valid dependency snapshot");

    auto subjectB = fullContext;
    subjectB.subjectIdentity = "singular:subject-B@revision-7";
    const auto contextSnapshotB =
        sdfwgsl::snapshotFieldVmDependencies(contextManifest, subjectB);
    check(contextSnapshotB.valid &&
              contextSnapshotA.identity != contextSnapshotB.identity,
          "different subject identity invalidates retained runtime dependency evidence");

    auto worldB = fullContext;
    worldB.worldConditionContextIdentity = "condition-world@revision-22";
    const auto contextSnapshotWorldB =
        sdfwgsl::snapshotFieldVmDependencies(contextManifest, worldB);
    check(contextSnapshotWorldB.valid &&
              contextSnapshotA.identity != contextSnapshotWorldB.identity,
          "changed world-condition authority revision changes exact snapshot identity");

    const auto contextSnapshotA2 =
        sdfwgsl::snapshotFieldVmDependencies(contextManifest, fullContext);
    check(contextSnapshotA2.valid &&
              contextSnapshotA.identity == contextSnapshotA2.identity,
          "identical dependency evidence reproduces the exact same snapshot identity");

    const auto emptyBindField = geom::makeImplicit(bind(""));
    const auto emptyBind =
        sdfwgsl::inspectFieldVmDependencies(emptyBindField);
    check(!emptyBind.structurallyValid() &&
              !sdfwgsl::assessFieldVmDependencies(emptyBind, {}).ready,
          "malformed empty Bind refuses before any runtime provider can bless it");

    if (failures) {
        std::printf("field_vm_dependency_test: %d failure(s)\n", failures);
        return 1;
    }
    std::printf("field_vm_dependency_test: PASS\n");
    return 0;
}
