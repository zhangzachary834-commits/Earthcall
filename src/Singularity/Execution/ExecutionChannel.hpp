#pragma once

#include "NativeBytecodeVM.hpp"
#include "PropheticJIT.hpp"
#include "PropheticIndex.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include <memory>
#include <unordered_map>
#include <vector>

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

    void setPropheticIndex(std::unique_ptr<PropheticIndex> index);

    // Invoked by the Zone or EventBus when a Law must fire.
    // The Channel decides, invisibly to the caller, whether to use the
    // unguarded JIT cache or the fallback VM.
    void executeLaw(const class Law& law, Singular& target);

    // Recompiles the universe laws into Bytecode.
    // (In the future, also triggers the background LLVM thread to build unguarded assembly).
    void warmCaches(const std::vector<std::shared_ptr<class Law>>& universeLaws);

    // Forces a hard fallback to the VM and invalidates the JIT cache.
    // Called when the Prophetic Rete detects a fundamental structural shift.
    void triggerStructuralInvalidation();

    // Check if the engine is currently running in mathematically-proven 1.0x native mode.
    bool isJITActive() const { return _isJITActive; }

private:
    NativeBytecodeVM _vm;
    std::unique_ptr<PropheticJIT> _jit;
    std::unique_ptr<PropheticIndex> _propheticIndex;

    // Cache of pre-compiled bytecodes
    std::unordered_map<std::string, NativeBytecodeVM::Bytecode> _vmCache;

    // True if we are currently running in the mathematically proven JIT phase.
    bool _isJITActive = false;
};

} // namespace Execution
} // namespace Earthcall
