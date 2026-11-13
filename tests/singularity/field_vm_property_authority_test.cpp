#include "Singularity/Screen/WebGPU/FieldVmPropertyAuthority.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <cstdio>
#include <map>
#include <memory>
#include <string>

namespace {
int failures = 0;
void check(bool ok, const char* what) {
    std::printf("  %s: %s\n", ok ? "ok" : "FAILED", what);
    if (!ok) ++failures;
}

class TestBeing final : public Singular {
public:
    explicit TestBeing(std::string id) : _id(std::move(id)) {}
    std::string getIdentifier() const override { return _id; }
private:
    std::string _id;
};

std::shared_ptr<OntoMath::MathNode> bind(const char* path) {
    auto n = std::make_shared<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::Bind;
    n->propertyPath = path;
    return n;
}

sdfwgsl::FieldVmDependencyManifest radiusManifest() {
    return sdfwgsl::inspectFieldVmDependencies(
        geom::makeImplicit(bind("radius")));
}

sdfwgsl::FieldVmDependencyContext contextFrom(
    const sdfwgsl::FieldVmCellPropertyEvidence& evidence,
    std::map<std::string, PropertyValue>& values,
    std::map<std::string, std::string>& authorities,
    std::map<std::string, sdfwgsl::FieldVmPropertyChangeRoute>& routes) {
    values["radius"] = evidence.value;
    authorities["radius"] = evidence.authorityIdentity;
    routes["radius"] = evidence.changeRoute;
    sdfwgsl::FieldVmDependencyContext ctx;
    ctx.propertyBindings = &values;
    ctx.propertyBindingAuthorities = &authorities;
    ctx.propertyBindingChangeRoutes = &routes;
    return ctx;
}
} // namespace

int main() {
    std::printf("Running native ValueCell authority evidence tribunal...\n");

    PropertyCellTable::instance().clear();

    TestBeing material("material:A");
    const auto radiusId = Earthcall::StringInterner::intern("radius");
    const CellId originalCell =
        PropertyCellTable::instance().allocateCell(
            PropertyValue(0.5),
            CellOwnershipMode::Shared,
            &material,
            radiusId);
    check(originalCell.valid() &&
              material.bindPropertyCell(
                  radiusId, originalCell, CellOwnershipMode::Shared),
          "fixture binds radius to a live shared ValueCell");

    const auto unproven =
        sdfwgsl::captureFieldVmCellPropertyEvidence(
            material, "radius",
            sdfwgsl::FieldVmCellFeedCoverage::Unproven);
    check(unproven.valid &&
              unproven.revision == 0 &&
              unproven.cellId == originalCell &&
              !unproven.authorityIdentity.empty() &&
              !unproven.changeRoute.completeCoverage,
          "cell capture derives value/provenance without inventing complete change-feed coverage");

    const auto manifest = radiusManifest();
    std::map<std::string, PropertyValue> values;
    std::map<std::string, std::string> authorities;
    std::map<std::string, sdfwgsl::FieldVmPropertyChangeRoute> routes;
    auto ctx = contextFrom(unproven, values, authorities, routes);
    const auto unprovenSnapshot =
        sdfwgsl::snapshotFieldVmDependencies(manifest, ctx);
    const auto unprovenPlan =
        sdfwgsl::planFieldVmInvalidation(manifest, ctx);
    check(unprovenSnapshot.valid &&
              unprovenPlan.valid &&
              unprovenPlan.requirements.size() == 1 &&
              unprovenPlan.requirements[0].mode ==
                  sdfwgsl::FieldVmInvalidationMode::SnapshotRevalidation,
          "native cell provenance alone remains on snapshot-revalidation correctness floor");

    const auto proven =
        sdfwgsl::captureFieldVmCellPropertyEvidence(
            material, "radius",
            sdfwgsl::FieldVmCellFeedCoverage::ProvenComplete);
    values.clear(); authorities.clear(); routes.clear();
    auto provenCtx = contextFrom(proven, values, authorities, routes);
    const auto provenPlan =
        sdfwgsl::planFieldVmInvalidation(manifest, provenCtx);
    check(proven.valid &&
              proven.changeRoute.completeCoverage &&
              provenPlan.valid &&
              provenPlan.requirements[0].mode ==
                  sdfwgsl::FieldVmInvalidationMode::PropertyChangeFeed &&
              sdfwgsl::fieldVmPropertyChangeInvalidates(
                  provenPlan, {"material:A", "radius"}),
          "explicit complete writer-coverage proof enables existing narrow property-change feed");

    const auto beforeMutationIdentity = proven.authorityIdentity;
    check(material.setDynamicProperty("radius", PropertyValue(0.75)),
          "cell-backed dynamic write succeeds through existing mutation path");
    const auto afterMutation =
        sdfwgsl::captureFieldVmCellPropertyEvidence(
            material, "radius",
            sdfwgsl::FieldVmCellFeedCoverage::ProvenComplete);
    check(afterMutation.valid &&
              afterMutation.revision == proven.revision + 1 &&
              afterMutation.authorityIdentity != beforeMutationIdentity,
          "ValueCell revision bump changes native authority identity");
    const double* radiusValue = std::get_if<double>(&afterMutation.value);
    check(radiusValue && *radiusValue == 0.75,
          "captured evidence reads the authoritative cell value itself");

    const auto otherId = Earthcall::StringInterner::intern("other");
    const CellId otherCell =
        PropertyCellTable::instance().allocateCell(
            PropertyValue(1.0),
            CellOwnershipMode::Shared,
            &material,
            otherId);
    check(material.bindPropertyCell(
              otherId, otherCell, CellOwnershipMode::Shared),
          "fixture binds unrelated cell");
    const auto beforeOtherMutation =
        sdfwgsl::captureFieldVmCellPropertyEvidence(
            material, "radius",
            sdfwgsl::FieldVmCellFeedCoverage::ProvenComplete);
    check(material.setDynamicProperty("other", PropertyValue(2.0)),
          "unrelated cell mutation succeeds");
    const auto afterOtherMutation =
        sdfwgsl::captureFieldVmCellPropertyEvidence(
            material, "radius",
            sdfwgsl::FieldVmCellFeedCoverage::ProvenComplete);
    check(beforeOtherMutation.valid && afterOtherMutation.valid &&
              beforeOtherMutation.authorityIdentity ==
                  afterOtherMutation.authorityIdentity,
          "unrelated ValueCell mutation does not perturb radius authority evidence");

    const auto oldEvidence =
        sdfwgsl::captureFieldVmCellPropertyEvidence(
            material, "radius",
            sdfwgsl::FieldVmCellFeedCoverage::ProvenComplete);
    check(material.unbindPropertyCell("radius"),
          "fixture unbinds original radius cell");
    const CellId reboundCell =
        PropertyCellTable::instance().allocateCell(
            oldEvidence.value,
            CellOwnershipMode::Shared,
            &material,
            radiusId);
    check(material.bindPropertyCell(
              radiusId, reboundCell, CellOwnershipMode::Shared),
          "fixture rebinds equal value through a fresh cell generation");
    const auto reboundEvidence =
        sdfwgsl::captureFieldVmCellPropertyEvidence(
            material, "radius",
            sdfwgsl::FieldVmCellFeedCoverage::ProvenComplete);
    check(reboundEvidence.valid &&
              reboundEvidence.cellId != oldEvidence.cellId &&
              reboundEvidence.authorityIdentity !=
                  oldEvidence.authorityIdentity,
          "cell rebind changes native authority even when execution value is equal");
    check(reboundEvidence.cellId.slot == oldEvidence.cellId.slot &&
              reboundEvidence.cellId.generation !=
                  oldEvidence.cellId.generation,
          "reclaimed cell slot is generation-checked across rebind");

    TestBeing inlineOnly("material:inline");
    check(inlineOnly.setDynamicProperty("radius", PropertyValue(0.5)),
          "fixture creates ordinary non-cell-backed dynamic property");
    const auto inlineEvidence =
        sdfwgsl::captureFieldVmCellPropertyEvidence(
            inlineOnly, "radius",
            sdfwgsl::FieldVmCellFeedCoverage::ProvenComplete);
    check(!inlineEvidence.valid &&
              inlineEvidence.error.find("not backed") != std::string::npos,
          "non-cell-backed property refuses native cell authority upgrade");

    const auto missingEvidence =
        sdfwgsl::captureFieldVmCellPropertyEvidence(
            material, "",
            sdfwgsl::FieldVmCellFeedCoverage::ProvenComplete);
    check(!missingEvidence.valid,
          "empty root property cannot mint native authority evidence");

    PropertyCellTable::instance().clear();

    if (failures) {
        std::printf("field_vm_property_authority_test: %d failure(s)\n",
                    failures);
        return 1;
    }
    std::printf("field_vm_property_authority_test: PASS\n");
    return 0;
}
