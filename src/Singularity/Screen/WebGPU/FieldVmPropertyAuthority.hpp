#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Property/PropertyCell.hpp"
#include "Singularity/Screen/WebGPU/FieldVmDependencies.hpp"

#include <string>

namespace sdfwgsl {

// Coverage is an explicit proof supplied by the caller. Merely finding a
// ValueCell is not enough: some direct C++ access paths can bypass the ordinary
// PropertyPath/change-feed boundary.
enum class FieldVmCellFeedCoverage : unsigned {
    Unproven = 0,
    ProvenComplete = 1,
};

struct FieldVmCellPropertyEvidence {
    bool valid = false;
    PropertyValue value;
    std::string authorityIdentity;
    FieldVmPropertyChangeRoute changeRoute;
    CellId cellId{};
    uint64_t revision = 0;
    CellOwnershipMode ownershipMode = CellOwnershipMode::InlineValue;
    std::string error;
};

// Capture one ROOT property whose current authority is a live ValueCell.
//
// This is derived execution evidence only:
//   - value comes from the cell itself;
//   - authority identity includes stable bearer/root + slot/generation/revision
//     + ownership mode;
//   - narrow change-feed invalidation is enabled ONLY when the caller proves
//     complete writer coverage.
//
// A nested Bind may use this evidence when its authoritative root is this cell,
// but this helper does not guess PropertyPath root resolution.
inline FieldVmCellPropertyEvidence captureFieldVmCellPropertyEvidence(
    const Singular& bearer,
    const std::string& rootProperty,
    FieldVmCellFeedCoverage coverage =
        FieldVmCellFeedCoverage::Unproven) {
    FieldVmCellPropertyEvidence out;

    if (rootProperty.empty()) {
        out.error = "cell authority capture requires a non-empty root property";
        return out;
    }

    const std::string bearerIdentity = bearer.getIdentifier();
    if (bearerIdentity.empty()) {
        out.error = "cell authority capture requires stable bearer identity";
        return out;
    }

    if (!bearer.hasPropertyCell(rootProperty)) {
        out.error = "property is not backed by a live ValueCell binding";
        return out;
    }

    const CellId id = bearer.getPropertyCell(rootProperty);
    if (!id.valid()) {
        out.error = "property cell id is invalid";
        return out;
    }

    const ValueCell* cell = PropertyCellTable::instance().getCell(id);
    if (!cell) {
        out.error = "property cell binding is expired";
        return out;
    }

    out.cellId = id;
    out.revision = cell->revision;
    out.ownershipMode = bearer.getPropertyCellMode(rootProperty);
    out.value = cell->value;

    std::string identity;
    const auto add = [&](const std::string& part) {
        detail::appendFieldVmIdentityPart(identity, part);
    };
    add("field-vm-property-cell-authority:v1");
    add(bearerIdentity);
    add(rootProperty);
    add(std::to_string(id.slot));
    add(std::to_string(id.generation));
    add(std::to_string(cell->revision));
    add(std::to_string(static_cast<unsigned>(out.ownershipMode)));

    out.authorityIdentity = std::move(identity);
    out.changeRoute.bearerIdentity = bearerIdentity;
    out.changeRoute.rootProperty = rootProperty;
    out.changeRoute.completeCoverage =
        coverage == FieldVmCellFeedCoverage::ProvenComplete;
    out.valid = true;
    return out;
}

} // namespace sdfwgsl
