#include "Singularity/Execution/NativeBytecodeVM.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Core/StringId.hpp"
#include <iostream>
#include <cassert>

using namespace Earthcall;

void testLerpAndScale() {
    std::cout << "[Test 2] VM executing Lerp and Scale actions\n";

    Object obj;
    obj.setDynamicProperty("alpha", 10.0);

    Law law("test-law-2");

    // Create an action model directly since ActionNode::lerp is missing
    ActionModel action;
    action.kind = ActionNode::Kind::Sequence;

    ActionNode lerpNode;
    lerpNode.kind = ActionNode::Kind::Lerp;
    lerpNode.path = PropertyPath::parse("alpha");
    lerpNode.operand = PropertyValue(20.0);
    lerpNode.factor = 0.5;

    ActionNode scaleNode;
    scaleNode.kind = ActionNode::Kind::Scale;
    scaleNode.path = PropertyPath::parse("alpha");
    scaleNode.operand = PropertyValue(2.0);

    action.children.push_back(lerpNode);
    action.children.push_back(scaleNode);

    law.setActionModel(action);

    Execution::NativeBytecodeVM vm;
    auto bytecode = vm.emit(law);

    bool success = vm.execute(bytecode, obj);
    assert(success);

    // alpha starts at 10.0
    // Lerp to 20.0 with factor 0.5 -> 15.0
    // Scale by 2.0 -> 30.0
    auto prop = obj.findProperty(StringInterner::intern("alpha"));
    assert(prop != nullptr);
    assert(std::holds_alternative<double>(prop->value()));
    assert(std::get<double>(prop->value()) == 30.0);

    std::cout << "  ✓ Compiled and Executed Lerp + Scale bytecodes successfully!\n";
}

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

int main() {
    std::cout << "\n=== NativeBytecodeVM Test Suite ===\n\n";

    testBasicSetAndAdd();
    testLerpAndScale();

    std::cout << "\n✓ All NativeBytecodeVM tests passed!\n\n";
    return 0;
}
