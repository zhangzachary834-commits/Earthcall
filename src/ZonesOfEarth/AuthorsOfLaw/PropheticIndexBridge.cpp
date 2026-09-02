#include "PropheticIndexBridge.hpp"
#include <iostream>

namespace Earthcall {

PropheticIndexBridge::PropheticIndexBridge(Prophetic::Index& indexRef)
    : _index(indexRef) {}

Execution::PropheticIndex::Interference PropheticIndexBridge::queryStructuralDisjointness(
    Earthcall::StringId targetArchetypeId, 
    const PropertyPath& path) const 
{
    (void)targetArchetypeId; (void)path;
    // If the index has fallen or is incomplete, we must assume interference.
    if (_isInvalidated || !_index.complete()) {
        return Execution::PropheticIndex::Interference::Intersects;
    }

    // A structural mutation is anything that alters the memory layout.
    // In Earthcall, this means ActionNode::Kind::AddProperty, RemoveProperty,
    // Spawn, Destroy, etc.
    // The PropheticRete already tags these as `opaqueWrites`.
    
    // Check if ANY law currently in the world performs opaque writes.
    // If so, we cannot mathematically prove the memory layout is stable!
    for (const auto& facts : _index.facts()) {
        if (facts.opaqueWrites) {
            return Execution::PropheticIndex::Interference::Intersects;
        }
    }

    // Mathematical Proof Established: No laws mutate the fundamental memory
    // shape of the world. The JIT may safely drop all Bailout Guards.
    return Execution::PropheticIndex::Interference::Disjoint;
}

void PropheticIndexBridge::invalidate() {
    _isInvalidated = true;
    _index.clear();
}

void PropheticIndexBridge::recalculatePossibilitySpace() {
    // Actually rebuilding the index is handled by LawManager::tick(),
    // so we just clear our invalidated flag and trust the Rete is updated.
    _isInvalidated = false;
}

} // namespace Earthcall
