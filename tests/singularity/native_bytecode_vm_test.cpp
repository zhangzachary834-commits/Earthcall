#include "Singularity/Execution/NativeBytecodeVM.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Person/Body/Body.hpp"
#include "Singularity/Core/StringId.hpp"
#include <iostream>
#include <cassert>

using namespace Earthcall;

void testBasicSetAndAdd() {
    std::cout << "[Test 1] VM executing Set and Add actions\n";

    // Setup an Object directly
    Object obj;
    obj.setDynamicProperty("health", 50.0);

    // Setup a Law with an ActionModel
    Law law("test-law-1");
    // Action: Set "health" to 100, then Add 25 to "health"
    ActionModel action = ActionNode::sequence({
        ActionNode::set("health", 100.0),
        ActionNode::add("health", 25.0)
    });
    law.setActionModel(action);

    // Compile to bytecode
    Execution::NativeBytecodeVM vm;
    auto bytecode = vm.emit(law);

    // Validate compilation produced instructions
    assert(!bytecode.instructions.empty());
    assert(bytecode.instructions.back().op == Execution::NativeBytecodeVM::Opcode::Halt);

    // Execute
    bool success = vm.execute(bytecode, obj);
    assert(success);

    // Verify state mutation (health should be 100 + 25 = 125)
    auto prop = obj.findProperty(StringInterner::intern("health"));
    assert(prop != nullptr);
    assert(std::holds_alternative<double>(prop->value()));
    assert(std::get<double>(prop->value()) == 125.0);

    std::cout << "  ✓ Compiled and Executed Set + Add bytecodes successfully!\n";
}

void testScaleAction() {
    std::cout << "[Test 2] VM executing Scale action\n";

    Object obj;
    obj.setDynamicProperty("speed", 12.0);

    Law law("test-law-scale");
    // Action: Scale "speed" by 2.5 (12.0 * 2.5 = 30.0)
    law.setActionModel(ActionNode::scale("speed", 2.5));

    Execution::NativeBytecodeVM vm;
    auto bytecode = vm.emit(law);
    assert(!bytecode.instructions.empty());

    bool success = vm.execute(bytecode, obj);
    assert(success);

    auto prop = obj.findProperty(StringInterner::intern("speed"));
    assert(prop != nullptr);
    assert(std::holds_alternative<double>(prop->value()));
    assert(std::get<double>(prop->value()) == 30.0);

    // Test scaling an unset property: loadProp returns null, converts to 0.0, 0.0 * 5.0 = 0.0
    Object obj2;
    law.setActionModel(ActionNode::scale("energy", 5.0));
    auto bytecode2 = vm.emit(law);
    success = vm.execute(bytecode2, obj2);
    assert(success);
    auto prop2 = obj2.findProperty(StringInterner::intern("energy"));
    assert(prop2 != nullptr);
    assert(std::holds_alternative<double>(prop2->value()));
    assert(std::get<double>(prop2->value()) == 0.0);

    std::cout << "  ✓ Compiled and Executed Scale bytecodes successfully!\n";
}

void testLerpAction() {
    std::cout << "[Test 3] VM executing Lerp action\n";

    Object obj;
    obj.setDynamicProperty("alpha", 10.0);

    Law law("test-law-lerp");
    ActionNode lerp;
    lerp.kind = ActionNode::Kind::Lerp;
    lerp.path = PropertyPath::parse("alpha");
    lerp.operand = PropertyValue(20.0);
    lerp.factor = 0.25;
    law.setActionModel(lerp);

    Execution::NativeBytecodeVM vm;
    auto bytecode = vm.emit(law);
    assert(bytecode.instructions.size() > 1);
    assert(vm.execute(bytecode, obj));

    auto prop = obj.findProperty(StringInterner::intern("alpha"));
    assert(prop != nullptr);
    assert(std::holds_alternative<double>(prop->value()));
    assert(std::get<double>(prop->value()) == 12.5);

    std::cout << "  ✓ Lerp bytecode preserves authored interpolation semantics!\n";
}

void testSequenceAndParallel() {
    std::cout << "[Test 3] VM executing Sequence and Parallel actions\n";

    Object obj;
    obj.setDynamicProperty("mana", 10.0);
    obj.setDynamicProperty("defense", 5.0);

    // Sequence: Set mana to 50, Add 20 (70), Scale by 0.5 (35)
    Law seqLaw("test-law-seq");
    seqLaw.setActionModel(ActionNode::sequence({
        ActionNode::set("mana", 50.0),
        ActionNode::add("mana", 20.0),
        ActionNode::scale("mana", 0.5)
    }));

    Execution::NativeBytecodeVM vm;
    auto seqBytecode = vm.emit(seqLaw);
    assert(vm.execute(seqBytecode, obj));

    auto manaProp = obj.findProperty(StringInterner::intern("mana"));
    assert(manaProp != nullptr);
    assert(std::get<double>(manaProp->value()) == 35.0);

    // Parallel: Mutate multiple distinct properties
    Law parLaw("test-law-par");
    parLaw.setActionModel(ActionNode::parallel({
        ActionNode::set("x", 1.0),
        ActionNode::set("y", 2.0),
        ActionNode::set("z", 3.0),
        ActionNode::add("defense", 15.0)
    }));

    auto parBytecode = vm.emit(parLaw);
    assert(vm.execute(parBytecode, obj));

    auto xProp = obj.findProperty(StringInterner::intern("x"));
    auto yProp = obj.findProperty(StringInterner::intern("y"));
    auto zProp = obj.findProperty(StringInterner::intern("z"));
    auto defProp = obj.findProperty(StringInterner::intern("defense"));

    assert(xProp && std::get<double>(xProp->value()) == 1.0);
    assert(yProp && std::get<double>(yProp->value()) == 2.0);
    assert(zProp && std::get<double>(zProp->value()) == 3.0);
    assert(defProp && std::get<double>(defProp->value()) == 20.0);

    std::cout << "  ✓ Sequence and Parallel multi-node execution verified!\n";
}

void testDirectInstructionExecution() {
    std::cout << "[Test 4] Direct Opcode instruction execution (Sub, CmpEq, CmpGt, BranchFalse, Jump, Halt)\n";

    Execution::NativeBytecodeVM vm;
    Object obj;
    obj.setDynamicProperty("val", 100.0);

    uint32_t valId = StringInterner::intern("val").value;
    uint32_t resId = StringInterner::intern("res").value;

    // Subtraction: R[0] = 50.0, R[1] = 20.0, R[2] = R[0] - R[1] = 30.0, StoreProp("res", R[2])
    {
        Execution::NativeBytecodeVM::Bytecode bc;
        bc.constants.push_back(50.0);
        bc.constants.push_back(20.0);
        bc.instructions = {
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 0, 0, 0},
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 1, 0, 1},
            {Execution::NativeBytecodeVM::Opcode::Sub, 2, 0, 1},
            {Execution::NativeBytecodeVM::Opcode::StoreProp, 0, 2, resId},
            {Execution::NativeBytecodeVM::Opcode::Halt, 0, 0, 0}
        };

        assert(vm.execute(bc, obj));
        auto prop = obj.findProperty(StringId(resId));
        assert(prop && std::get<double>(prop->value()) == 30.0);
    }

    // CmpEq: R[0] = 42.0, R[1] = 42.0, R[2] = (R[0] == R[1]) -> 1.0
    {
        Execution::NativeBytecodeVM::Bytecode bc;
        bc.constants.push_back(42.0);
        bc.constants.push_back(42.0);
        bc.instructions = {
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 0, 0, 0},
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 1, 0, 1},
            {Execution::NativeBytecodeVM::Opcode::CmpEq, 2, 0, 1},
            {Execution::NativeBytecodeVM::Opcode::StoreProp, 0, 2, resId},
            {Execution::NativeBytecodeVM::Opcode::Halt, 0, 0, 0}
        };

        assert(vm.execute(bc, obj));
        auto prop = obj.findProperty(StringId(resId));
        assert(prop && std::get<double>(prop->value()) == 1.0);
    }

    // CmpGt: R[0] = 10.0, R[1] = 20.0, R[2] = (R[0] > R[1]) -> 0.0
    {
        Execution::NativeBytecodeVM::Bytecode bc;
        bc.constants.push_back(10.0);
        bc.constants.push_back(20.0);
        bc.instructions = {
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 0, 0, 0},
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 1, 0, 1},
            {Execution::NativeBytecodeVM::Opcode::CmpGt, 2, 0, 1},
            {Execution::NativeBytecodeVM::Opcode::StoreProp, 0, 2, resId},
            {Execution::NativeBytecodeVM::Opcode::Halt, 0, 0, 0}
        };

        assert(vm.execute(bc, obj));
        auto prop = obj.findProperty(StringId(resId));
        assert(prop && std::get<double>(prop->value()) == 0.0);
    }

    // BranchFalse: Branch taken (numeric 0.0)
    {
        Execution::NativeBytecodeVM::Bytecode bc;
        bc.constants.push_back(0.0);
        bc.constants.push_back(999.0);
        bc.constants.push_back(777.0);
        bc.instructions = {
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 0, 0, 0},
            {Execution::NativeBytecodeVM::Opcode::BranchFalse, 0, 0, 4},
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 1, 0, 1},
            {Execution::NativeBytecodeVM::Opcode::StoreProp, 0, 1, resId},
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 1, 0, 2},
            {Execution::NativeBytecodeVM::Opcode::StoreProp, 0, 1, resId},
            {Execution::NativeBytecodeVM::Opcode::Halt, 0, 0, 0}
        };
        assert(vm.execute(bc, obj));
        auto prop = obj.findProperty(StringId(resId));
        assert(prop && std::get<double>(prop->value()) == 777.0);
    }

    // BranchFalse: Branch NOT taken (boolean true)
    {
        Execution::NativeBytecodeVM::Bytecode bc;
        bc.constants.push_back(true);
        bc.constants.push_back(111.0);
        bc.constants.push_back(222.0);
        bc.instructions = {
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 0, 0, 0}, // R[0] = true
            {Execution::NativeBytecodeVM::Opcode::BranchFalse, 0, 0, 4}, // if false, jump to 4
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 1, 0, 1}, // R[1] = 111.0
            {Execution::NativeBytecodeVM::Opcode::StoreProp, 0, 1, resId}, // Store 111.0
            {Execution::NativeBytecodeVM::Opcode::Halt, 0, 0, 0} // Halt here
        };
        assert(vm.execute(bc, obj));
        auto prop = obj.findProperty(StringId(resId));
        assert(prop && std::get<double>(prop->value()) == 111.0);
    }

    // Jump, NoOp, & Halt
    {
        Execution::NativeBytecodeVM::Bytecode bc;
        bc.constants.push_back(1234.0);
        bc.instructions = {
            {Execution::NativeBytecodeVM::Opcode::NoOp, 0, 0, 0}, // does nothing
            {Execution::NativeBytecodeVM::Opcode::Jump, 0, 0, 4}, // Jump to idx 4
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 0, 0, 0}, // skipped
            {Execution::NativeBytecodeVM::Opcode::StoreProp, 0, 0, resId}, // skipped
            {Execution::NativeBytecodeVM::Opcode::Halt, 0, 0, 0} // idx 4
        };
        assert(vm.execute(bc, obj));
        auto prop = obj.findProperty(StringId(resId));
        assert(prop && std::get<double>(prop->value()) == 111.0); // remains from previous
    }

    // Multiplication: R[0] = 3.0, R[1] = 4.0, R[2] = R[0] * R[1] = 12.0
    {
        Execution::NativeBytecodeVM::Bytecode bc;
        bc.constants.push_back(3.0);
        bc.constants.push_back(4.0);
        bc.instructions = {
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 0, 0, 0},
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 1, 0, 1},
            {Execution::NativeBytecodeVM::Opcode::Mul, 2, 0, 1},
            {Execution::NativeBytecodeVM::Opcode::StoreProp, 0, 2, resId},
            {Execution::NativeBytecodeVM::Opcode::Halt, 0, 0, 0}
        };
        assert(vm.execute(bc, obj));
        auto prop = obj.findProperty(StringId(resId));
        assert(prop && std::get<double>(prop->value()) == 12.0);
    }

    // CmpEq: String Fallback ("hello" == "hello" -> 1.0)
    {
        Execution::NativeBytecodeVM::Bytecode bc;
        bc.constants.push_back(std::string("hello"));
        bc.constants.push_back(std::string("hello"));
        bc.instructions = {
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 0, 0, 0},
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 1, 0, 1},
            {Execution::NativeBytecodeVM::Opcode::CmpEq, 2, 0, 1},
            {Execution::NativeBytecodeVM::Opcode::StoreProp, 0, 2, resId},
            {Execution::NativeBytecodeVM::Opcode::Halt, 0, 0, 0}
        };
        assert(vm.execute(bc, obj));
        auto prop = obj.findProperty(StringId(resId));
        assert(prop && std::get<double>(prop->value()) == 1.0);
    }

    // CmpGt: True (20.0 > 10.0 -> 1.0)
    {
        Execution::NativeBytecodeVM::Bytecode bc;
        bc.constants.push_back(20.0);
        bc.constants.push_back(10.0);
        bc.instructions = {
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 0, 0, 0},
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 1, 0, 1},
            {Execution::NativeBytecodeVM::Opcode::CmpGt, 2, 0, 1},
            {Execution::NativeBytecodeVM::Opcode::StoreProp, 0, 2, resId},
            {Execution::NativeBytecodeVM::Opcode::Halt, 0, 0, 0}
        };
        assert(vm.execute(bc, obj));
        auto prop = obj.findProperty(StringId(resId));
        assert(prop && std::get<double>(prop->value()) == 1.0);
    }

    std::cout << "  ✓ Direct Opcode execution verified across all tested ops!\n";
}

void testEmptyAndUnauthoredLaws() {
    std::cout << "[Test 5] Empty and unauthored Law compilation\n";

    Execution::NativeBytecodeVM vm;
    Object obj;
    obj.setDynamicProperty("stable", 42.0);

    // Law with no action model
    Law emptyLaw("empty-law");
    auto bc = vm.emit(emptyLaw);

    assert(bc.instructions.size() == 1);
    assert(bc.instructions[0].op == Execution::NativeBytecodeVM::Opcode::Halt);

    assert(vm.execute(bc, obj));
    auto prop = obj.findProperty(StringInterner::intern("stable"));
    assert(prop && std::get<double>(prop->value()) == 42.0);

    std::cout << "  ✓ Empty Law compiles cleanly to Halt and executes as safe no-op!\n";
}

void testVMRegisterIsolation() {
    std::cout << "[Test 6] VM register isolation across multiple runs\n";

    Execution::NativeBytecodeVM vm;
    Object objA;
    Object objB;

    // Run 1 on objA puts large values into registers
    Law lawA("law-A");
    lawA.setActionModel(ActionNode::set("score", 99999.0));
    auto bcA = vm.emit(lawA);
    assert(vm.execute(bcA, objA));

    // Run 2 on objB performs an Add on an unset property without prior Set
    // Should start from default 0.0 + 10.0 = 10.0, NOT 99999.0 + 10.0
    Law lawB("law-B");
    lawB.setActionModel(ActionNode::add("score", 10.0));
    auto bcB = vm.emit(lawB);
    assert(vm.execute(bcB, objB));

    auto propB = objB.findProperty(StringInterner::intern("score"));
    assert(propB != nullptr);
    assert(std::get<double>(propB->value()) == 10.0);

    std::cout << "  ✓ VM registers do not leak state between distinct law executions!\n";
}

void testExecutionOnPersonsAndObjects() {
    std::cout << "[Test 7] Execution against Person and Object (Ontological Invariants)\n";

    Execution::NativeBytecodeVM vm;

    // Refusal 4 & 5: Person strictly represents a human being.
    Person person(Soul("Zachary"), Body::createBasicAvatar("Voxel"), "default");
    person.setDynamicProperty("stamina", 100.0);

    Law staminaLaw("stamina-recovery");
    staminaLaw.setActionModel(ActionNode::add("stamina", 25.0));

    auto bc = vm.emit(staminaLaw);
    assert(vm.execute(bc, person));

    auto staminaProp = person.findProperty(StringInterner::intern("stamina"));
    assert(staminaProp != nullptr);
    assert(std::get<double>(staminaProp->value()) == 125.0);

    std::cout << "  ✓ Bytecode VM successfully mutates Person and Object state conforming to ontology!\n";
}

void testEdgeCasesAndErrors() {
    std::cout << "[Test 8] Edge cases and out-of-bounds safety\n";

    Execution::NativeBytecodeVM vm;
    Object obj;
    uint32_t valId = StringInterner::intern("val").value;

    // 1. LoadImm with OOB index (should safely do nothing to the register)
    {
        Execution::NativeBytecodeVM::Bytecode bc;
        bc.instructions = {
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 0, 0, 999},
            {Execution::NativeBytecodeVM::Opcode::Halt, 0, 0, 0}
        };
        assert(vm.execute(bc, obj)); // should not crash
    }

    // 2. LoadProp of a non-existent property
    {
        Execution::NativeBytecodeVM::Bytecode bc;
        bc.instructions = {
            {Execution::NativeBytecodeVM::Opcode::LoadProp, 0, 0, valId},
            {Execution::NativeBytecodeVM::Opcode::Halt, 0, 0, 0}
        };
        assert(vm.execute(bc, obj));
    }

    // 3. Jump OOB (should safely fall through)
    {
        Execution::NativeBytecodeVM::Bytecode bc;
        bc.instructions = {
            {Execution::NativeBytecodeVM::Opcode::Jump, 0, 0, 999},
            {Execution::NativeBytecodeVM::Opcode::Halt, 0, 0, 0}
        };
        assert(vm.execute(bc, obj));
    }

    // 4. BranchFalse OOB (should safely fall through)
    {
        Execution::NativeBytecodeVM::Bytecode bc;
        bc.constants.push_back(0.0);
        bc.instructions = {
            {Execution::NativeBytecodeVM::Opcode::LoadImm, 0, 0, 0},
            {Execution::NativeBytecodeVM::Opcode::BranchFalse, 0, 0, 999},
            {Execution::NativeBytecodeVM::Opcode::Halt, 0, 0, 0}
        };
        assert(vm.execute(bc, obj));
    }

    std::cout << "  ✓ VM handles edge cases and OOB indices safely!\n";
}

int main() {
    std::cout << "\n=== NativeBytecodeVM Test Suite ===\n\n";

    testBasicSetAndAdd();
    testScaleAction();
    testLerpAction();
    testSequenceAndParallel();
    testDirectInstructionExecution();
    testEmptyAndUnauthoredLaws();
    testVMRegisterIsolation();
    testExecutionOnPersonsAndObjects();
    testEdgeCasesAndErrors();

    std::cout << "\n✓ All NativeBytecodeVM tests passed!\n\n";
    return 0;
}
