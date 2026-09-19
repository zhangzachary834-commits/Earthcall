#pragma once

#include "PropheticIndex.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include <memory>

namespace Earthcall {
namespace Execution {

// ============================================================================
// PropheticJIT
//
// The Engine of Causality (Phase 3). 
// Translates Law definitions directly into native x86_64/ARM machine code.
// Interacts intimately with the PropheticIndex: when disjointness is proven,
// it omits bailout guards, matching C++ structural access speeds exactly.
// ============================================================================
class PropheticJIT {
public:
    virtual ~PropheticJIT() = default;

    // A compiled, callable native function pointer for a Law consequence
    using NativeLawClosure = void(*)(Singular& target);

    // Determines if the host platform supports JIT (W^X memory allocation).
    static bool isSupportedOnHost();

    // Creates an instance of the Prophetic JIT compiler, or a null-safe stub if unsupported.
    static std::unique_ptr<PropheticJIT> create();

    // Compiles a Law to machine code. This is an asynchronous or AOT process.
    // It queries the `index` to determine if Bailout Guards can be safely dropped.
    virtual NativeLawClosure compileUnguarded(
        const class Law& law, 
        const PropheticIndex& index) = 0;

    // Immediately flushes the executable memory cache. Called when the 
    // Prophetic Rete detects an unpredicted structural mutation, rendering 
    // the currently active unguarded assembly unsafe.
    virtual void flushExecutableCache() = 0;
};

} // namespace Execution
} // namespace Earthcall
