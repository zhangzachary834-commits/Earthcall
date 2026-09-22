#include "ExecutionChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

namespace Earthcall {
namespace Execution {

ExecutionChannel::ExecutionChannel()
    : _jit(PropheticJIT::create()) {}
ExecutionChannel::~ExecutionChannel() = default;

void ExecutionChannel::setPropheticIndex(std::unique_ptr<PropheticIndex> index) {
    _propheticIndex = std::move(index);
}

void ExecutionChannel::executeLaw(const class Law& law, Singular& target) {
    // If the index has fallen, we MUST fallback to the VM immediately.
    // In a fully integrated system, the JIT would query disjointness per-path,
    // but at the Channel orchestration level, if the Index is totally invalid,
    // we drop the whole JIT state to avoid executing stale assembly.
    if (_isJITActive && _propheticIndex) {
        // Example check: query the index for structural stability
        // (In reality, JIT cache invalidation handles this, but for Phase 4 orchestration
        // we simulate the fallback logic).
        PropertyPath dummyPath = PropertyPath::parse(""); // Checking global stability
        auto interference = _propheticIndex->queryStructuralDisjointness(Earthcall::StringId(0), dummyPath);
        if (interference == PropheticIndex::Interference::Intersects) {
            triggerStructuralInvalidation();
        }
    }

    bool executedViaJIT = false;
    if (_isJITActive) {
        auto it = _jitCache.find(law.getIdentifier());
        if (it != _jitCache.end() && it->second) {
            it->second(target);
            executedViaJIT = true;
        }
    }

    if (!executedViaJIT) {
        auto it = _vmCache.find(law.getIdentifier());
        if (it != _vmCache.end()) {
            _vm.execute(it->second, target);
        } else {
            // Compile the bytecode just-in-time if not found
            NativeBytecodeVM::Bytecode code = _vm.emit(law);
            _vm.execute(code, target);
            _vmCache[law.getIdentifier()] = std::move(code);
        }
    }
}

void ExecutionChannel::warmCaches(const std::vector<std::shared_ptr<class Law>>& universeLaws) {
    _vmCache.clear();
    _jitCache.clear();
    for (const auto& law : universeLaws) {
        _vmCache[law->getIdentifier()] = _vm.emit(*law);
        if (_jit && _propheticIndex && PropheticJIT::isSupportedOnHost()) {
            auto closure = _jit->compileUnguarded(*law, *_propheticIndex);
            if (closure) {
                _jitCache[law->getIdentifier()] = closure;
            }
        }
    }

    // Attempt to promote to JIT if structurally stable
    if (_propheticIndex) {
        PropertyPath dummyPath = PropertyPath::parse("");
        auto interference = _propheticIndex->queryStructuralDisjointness(Earthcall::StringId(0), dummyPath);
        if (interference == PropheticIndex::Interference::Disjoint) {
            _isJITActive = true;
        } else {
            _isJITActive = false;
        }
    }
}

void ExecutionChannel::triggerStructuralInvalidation() {
    _isJITActive = false;
    _jitCache.clear();
    if (_jit) {
        _jit->flushExecutableCache();
    }
    if (_propheticIndex) {
        _propheticIndex->invalidate();
    }
}

} // namespace Execution
} // namespace Earthcall
