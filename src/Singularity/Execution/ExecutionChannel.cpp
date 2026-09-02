#include "ExecutionChannel.hpp"

namespace Earthcall {
namespace Execution {

ExecutionChannel::ExecutionChannel() {
    // Instantiate actual implementations of VM, JIT, and Index here.
}

ExecutionChannel::~ExecutionChannel() = default;

void ExecutionChannel::initialize() {
    // Setup modality
}

void ExecutionChannel::executeLaw(const class Law& law, Singular& target) {
    (void)law;
    (void)target;
    // In actual implementation:
    // if (_isJITActive && _jitCache.has(law.id)) {
    //     _jitCache.get(law.id)(target);
    // } else {
    //     _vm->execute(_vmCache.get(law.id), target);
    // }
}

void ExecutionChannel::warmCaches(const std::vector<class Law>& universeLaws) {
    (void)universeLaws;
}

void ExecutionChannel::triggerStructuralInvalidation() {
    _isJITActive = false;
    if (_jit) {
        _jit->flushExecutableCache();
    }
    if (_propheticIndex) {
        _propheticIndex->invalidate();
    }
}

} // namespace Execution
} // namespace Earthcall
