#pragma once

#include "Singularity/Execution/PropheticIndex.hpp"
#include "PropheticRete.hpp"

namespace Earthcall {

// ============================================================================
// PropheticIndexBridge
//
// Bridges the ontological Prophetic::Index (which understands Law semantics)
// to the Execution Channel (which only understands raw machine instructions).
// Implements the mathematical shield interface that grants the JIT permission
// to drop Bailout Guards.
// ============================================================================
class PropheticIndexBridge : public Execution::PropheticIndex {
public:
    explicit PropheticIndexBridge(Prophetic::Index& indexRef);
    ~PropheticIndexBridge() override = default;

    Execution::PropheticIndex::Interference queryStructuralDisjointness(
        Earthcall::StringId targetArchetypeId, 
        const PropertyPath& path) const override;

    void invalidate() override;
    void recalculatePossibilitySpace() override;

private:
    Prophetic::Index& _index;
    bool _isInvalidated = false;
};

} // namespace Earthcall
