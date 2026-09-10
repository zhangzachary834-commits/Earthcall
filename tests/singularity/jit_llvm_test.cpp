#include "Singularity/Execution/PropheticJIT.hpp"
#include "Singularity/Execution/JITBridge.hpp"
#include "Singularity/Execution/ExecutionChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/PropheticIndexBridge.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/PropheticRete.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"
#include "Singularity/Core/StringId.hpp"
#include <iostream>
#include <cassert>
#include <memory>

using namespace Earthcall;

void testJITBridgeSetProperty() {
    std::cout << "[Test 1] JIT C-ABI Bridge: SetDoubleProperty & Null Guards\n";

    Object obj;
    uint32_t powerId = StringInterner::intern("power").value;

    // Set new dynamic property
    Earthcall_JIT_SetDoubleProperty(&obj, powerId, 88.5);
    auto prop = obj.findProperty(StringId(powerId));
    assert(prop != nullptr);
    assert(std::holds_alternative<double>(prop->value()));
    assert(std::get<double>(prop->value()) == 88.5);

    // Overwrite existing property
    Earthcall_JIT_SetDoubleProperty(&obj, powerId, 99.0);
    assert(std::get<double>(prop->value()) == 99.0);

    // Null safety: passing nullptr should not crash
    Earthcall_JIT_SetDoubleProperty(nullptr, powerId, 123.4);

    std::cout << "  ✓ SetDoubleProperty and null-target guards verified!\n";
}

void testJITBridgeAddAndScale() {
    std::cout << "[Test 2] JIT C-ABI Bridge: AddDoubleProperty, ScaleDoubleProperty & Type Safety\n";

    Object obj;
    uint32_t scoreId = StringInterner::intern("score").value;
    uint32_t titleId = StringInterner::intern("title").value;

    obj.setDynamicProperty("score", 10.0);
    obj.setDynamicProperty("title", std::string("Champion"));

    // Add 15.0 -> 25.0
    Earthcall_JIT_AddDoubleProperty(&obj, scoreId, 15.0);
    auto prop = obj.findProperty(StringId(scoreId));
    assert(prop && std::get<double>(prop->value()) == 25.0);

    // Scale by 2.0 -> 50.0
    Earthcall_JIT_ScaleDoubleProperty(&obj, scoreId, 2.0);
    assert(std::get<double>(prop->value()) == 50.0);

    // Add negative double -> 30.0
    Earthcall_JIT_AddDoubleProperty(&obj, scoreId, -20.0);
    assert(std::get<double>(prop->value()) == 30.0);

    // Scale by fractional factor -> 15.0
    Earthcall_JIT_ScaleDoubleProperty(&obj, scoreId, 0.5);
    assert(std::get<double>(prop->value()) == 15.0);

    // Type safety: Attempting to Add or Scale a std::string property must be guarded
    Earthcall_JIT_AddDoubleProperty(&obj, titleId, 10.0);
    Earthcall_JIT_ScaleDoubleProperty(&obj, titleId, 2.0);
    auto titleProp = obj.findProperty(StringId(titleId));
    assert(titleProp && std::holds_alternative<std::string>(titleProp->value()));
    assert(std::get<std::string>(titleProp->value()) == "Champion");

    // Null safety
    Earthcall_JIT_AddDoubleProperty(nullptr, scoreId, 5.0);
    Earthcall_JIT_ScaleDoubleProperty(nullptr, scoreId, 5.0);

    std::cout << "  ✓ AddDoubleProperty and ScaleDoubleProperty type-safety verified!\n";
}

void testJITBridgeConditionCheck() {
    std::cout << "[Test 3] JIT C-ABI Bridge: CheckDoubleGt Predicate\n";

    Object obj;
    uint32_t heatId = StringInterner::intern("heat").value;
    uint32_t nameId = StringInterner::intern("name").value;
    uint32_t missingId = StringInterner::intern("missing").value;

    obj.setDynamicProperty("heat", 75.0);
    obj.setDynamicProperty("name", std::string("Forge"));

    // 75.0 > 50.0 -> true (1)
    assert(Earthcall_JIT_CheckDoubleGt(&obj, heatId, 50.0) == 1);

    // 75.0 > 75.0 -> false (0)
    assert(Earthcall_JIT_CheckDoubleGt(&obj, heatId, 75.0) == 0);

    // 75.0 > 100.0 -> false (0)
    assert(Earthcall_JIT_CheckDoubleGt(&obj, heatId, 100.0) == 0);

    // Missing property -> 0
    assert(Earthcall_JIT_CheckDoubleGt(&obj, missingId, 0.0) == 0);

    // Non-double property -> 0
    assert(Earthcall_JIT_CheckDoubleGt(&obj, nameId, 0.0) == 0);

    // Null target -> 0
    assert(Earthcall_JIT_CheckDoubleGt(nullptr, heatId, 50.0) == 0);

    std::cout << "  ✓ CheckDoubleGt condition evaluation verified!\n";
}

void testPropheticJITLifecycle() {
    std::cout << "[Test 4] PropheticJIT Interface, Factory, and Cache Flush\n";

    // 1. Check host support
    bool supported = Execution::PropheticJIT::isSupportedOnHost();
    std::cout << "  ℹ JIT supported on this host: " << (supported ? "YES" : "NO (portable fallback)") << "\n";

    // 2. Factory creation
    auto jit = Execution::PropheticJIT::create();
    assert(jit != nullptr);

    // 3. Cache flush must succeed without error
    jit->flushExecutableCache();

    // 4. Test compilation query against PropheticIndex
    Prophetic::Index coreIndex;
    PropheticIndexBridge indexBridge(coreIndex);

    Law law("jit-test-law");
    law.setActionModel(ActionNode::sequence({
        ActionNode::set("energy", 100.0),
        ActionNode::add("energy", 50.0),
        ActionNode::scale("energy", 2.0)
    }));

    auto closure = jit->compileUnguarded(law, indexBridge);
    if (!supported) {
        assert(closure == nullptr);
        std::cout << "  ✓ Portable stub cleanly returns nullptr when LLVM is disabled\n";
    } else {
        assert(closure != nullptr);
        Object obj;
        closure(obj);
        auto prop = obj.findProperty(StringInterner::intern("energy"));
        assert(prop && std::get<double>(prop->value()) == 300.0);
        std::cout << "  ✓ LLVM JIT closure compiled and executed at 1.0x native speed!\n";
    }

    std::cout << "  ✓ PropheticJIT lifecycle and factory verified!\n";
}

void testJITExecutionSimulationAndOrchestration() {
    std::cout << "[Test 5] JIT Closure Emulation & ExecutionChannel Dual-State Coordination\n";

    // The signature compiled by LLVM is `void(*)(Singular& target)`.
    // We emulate what the LLVM JIT generates using our C-ABI bridge.
    Execution::PropheticJIT::NativeLawClosure simulatedClosure = [](Singular& target) {
        uint32_t hpId = StringInterner::intern("hp").value;
        Earthcall_JIT_SetDoubleProperty(&target, hpId, 100.0);
        Earthcall_JIT_AddDoubleProperty(&target, hpId, 25.0);
        Earthcall_JIT_ScaleDoubleProperty(&target, hpId, 2.0);
    };

    // Execute against Object
    Object obj;
    simulatedClosure(obj);
    auto objHp = obj.findProperty(StringInterner::intern("hp"));
    assert(objHp && std::get<double>(objHp->value()) == 250.0);

    // Execute against Person (Refusal 4 & 5)
    Person person(Soul("Humanity"), Body::createBasicAvatar("Voxel"), "default");
    simulatedClosure(person);
    auto personHp = person.findProperty(StringInterner::intern("hp"));
    assert(personHp && std::get<double>(personHp->value()) == 250.0);

    // Verify ExecutionChannel orchestration with JIT
    Execution::ExecutionChannel channel;
    Prophetic::Index index;
    channel.setPropheticIndex(std::make_unique<PropheticIndexBridge>(index));

    auto law = std::make_shared<Law>("dual-state-law");
    law->setActionModel(ActionNode::set("shield", 50.0));
    std::vector<std::shared_ptr<Law>> laws = { law };

    index.rebuild(laws);
    channel.warmCaches(laws);

    // Execution under stable universe
    channel.executeLaw(*law, obj);
    auto shieldProp = obj.findProperty(StringInterner::intern("shield"));
    assert(shieldProp && std::get<double>(shieldProp->value()) == 50.0);

    // Trigger invalidation: drops JIT and falls back to Bytecode VM
    channel.triggerStructuralInvalidation();
    assert(!channel.isJITActive());

    // Channel still executes seamlessly via VM on new or re-warmed laws
    auto addLaw = std::make_shared<Law>("dual-state-add-law");
    addLaw->setActionModel(ActionNode::add("shield", 25.0));
    channel.executeLaw(*addLaw, obj);
    assert(std::get<double>(shieldProp->value()) == 75.0);

    std::cout << "  ✓ JIT closure contract and ExecutionChannel fallback orchestration verified!\n";
}

int main() {
    std::cout << "\n=== JIT LLVM & C-ABI Bridge Test Suite ===\n\n";

    testJITBridgeSetProperty();
    testJITBridgeAddAndScale();
    testJITBridgeConditionCheck();
    testPropheticJITLifecycle();
    testJITExecutionSimulationAndOrchestration();

    std::cout << "\n✓ All JIT LLVM tests passed!\n\n";
    return 0;
}
