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

class NullPropheticJIT : public PropheticJIT {
public:
    ~NullPropheticJIT() override = default;

    NativeLawClosure compileUnguarded(
        const class Law& /*law*/, 
        const PropheticIndex& /*index*/) override
    {
        return nullptr;
    }

    void flushExecutableCache() override {
        // No-op
    }
};

bool PropheticJIT::isSupportedOnHost() {
    return false; // LLVM not linked in this build
}

std::unique_ptr<PropheticJIT> PropheticJIT::create() {
    return std::make_unique<NullPropheticJIT>();
}

#endif

} // namespace Execution
} // namespace Earthcall
