#pragma once

#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include <vector>

namespace Earthcall {
namespace Execution {

// ============================================================================
// PropheticIndex
//
// The mathematical shield for the JIT compiler. This interface performs
// Ahead-Of-Time abstract interpretation of the Rete graph. It determines
// the exact possibility space of structural changes (AddProperty, RemoveProperty)
// that could occur during the execution of a given Law.
//
// By proving disjointness here, the JIT is granted mathematical permission
// to drop Bailout Guards and emit unguarded 1.0x C++ machine code.
// ============================================================================
class PropheticIndex {
public:
    enum class Interference {
        // Mathematical proof established: no other law can mutate the layout of
        // this target while this code executes. The JIT may drop all shape guards.
        Disjoint,

        // A structural mutation is possible, or the target is modified by an
        // opaque pathway (e.g. Foreign/Script injection). The JIT must emit
        // bailout guards (Paranoia Tax) or fall back to the VM.
        Intersects
    };

    // Queries the index: "If I access `path` on `targetArchetypeId`, is there
    // any possibility that a structural change intersects this access?"
    virtual Interference queryStructuralDisjointness(
        Earthcall::StringId targetArchetypeId, 
        const PropertyPath& path) const = 0;

    // Called when a Law is authored or structurally changed, requiring the
    // possibility space to be recalculated.
    virtual void invalidate() = 0;

    // Rebuilds the disjointness proofs for the current universe state.
    virtual void recalculatePossibilitySpace() = 0;

    virtual ~PropheticIndex() = default;
};

} // namespace Execution
} // namespace Earthcall
