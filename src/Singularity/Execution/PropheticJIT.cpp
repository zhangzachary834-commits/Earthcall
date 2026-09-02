#include "PropheticJIT.hpp"

// ============================================================================
// PropheticJIT
//
// This file acts as the anchor for the JIT interface. The actual LLVM implementation
// is compiled conditionally if the LLVM backend is enabled in CMake.
// If disabled, this simply returns false for isSupportedOnHost().
// ============================================================================

namespace Earthcall {
namespace Execution {

#ifndef EARTHCALL_ENABLE_LLVM

bool PropheticJIT::isSupportedOnHost() {
    return false; // LLVM not linked in this build
}

PropheticJIT::NativeLawClosure PropheticJIT::compileUnguarded(
    const class Law& /*law*/, 
    const PropheticIndex& /*index*/) 
{
    return nullptr;
}

void PropheticJIT::flushExecutableCache() {
    // No-op
}

#endif

} // namespace Execution
} // namespace Earthcall
