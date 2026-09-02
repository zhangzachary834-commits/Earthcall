#pragma once

#include "NativeBytecodeVM.hpp"
#include "PropheticJIT.hpp"
#include "PropheticIndex.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include <memory>
#include <unordered_map>

namespace Earthcall {
namespace Execution {

// ============================================================================
// ExecutionChannel
//
// The top-level modality interface for Law Execution under Singularity.
// Abstracts the dual-state execution architecture. It orchestrates the flow
// of causality by routing execution through either the NativeBytecodeVM
// or the PropheticJIT.
//
// During stable execution (Disjointness Proven), it routes to JIT Assembly.
// If the Index falls (an opaque structural mutation occurs), it instantly
// routes to the BytecodeVM to preserve frame timing, while signaling the JIT
// to rebuild the executable cache asynchronously.
// ============================================================================
class ExecutionChannel {
public:
    ExecutionChannel();
    ~ExecutionChannel();

    // Invoked by the Zone or EventBus when a Law must fire.
    // The Channel decides, invisibly to the caller, whether to use the
    // unguarded JIT cache or the fallback VM.
    void executeLaw(const class Law& law, Singular& target);

    // Bootstraps the execution engine, initializing the VM and (if available)
    // the LLVM JIT backing.
    void initialize();

    // Recompiles the universe laws. Called on Zone boot or structural shift.
    void warmCaches(const std::vector<class Law>& universeLaws);

    // Forces a hard fallback to the VM and invalidates the JIT cache.
    // Called when the Prophetic Rete detects a fundamental structural shift.
    void triggerStructuralInvalidation();

private:
    std::unique_ptr<NativeBytecodeVM> _vm;
    std::unique_ptr<PropheticJIT> _jit;
    std::unique_ptr<PropheticIndex> _propheticIndex;

    // True if we are currently running in the mathematically proven JIT phase.
    bool _isJITActive = false;
};

} // namespace Execution
} // namespace Earthcall
