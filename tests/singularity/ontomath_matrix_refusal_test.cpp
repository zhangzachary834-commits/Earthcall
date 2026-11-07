#include "Singularity/OntoMath/ScalarForm.hpp"

#include <glm/glm.hpp>

#include <cassert>
#include <cstdio>
#include <map>
#include <memory>
#include <string>

namespace {

std::unique_ptr<OntoMath::MathNode> valueLeaf(const char* name) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ValueLeaf;
    n->variableName = name;
    return n;
}

} // namespace

int main() {
    using OntoMath::MathNode;

    // Rung 0 freezes the sovereignty gap before fixing it:
    // PropertyValue CAN carry glm::mat4, and a ValueLeaf can faithfully read it,
    // but MathNode has no Matrix ValueKind or matrix operations yet.
    std::map<std::string, PropertyValue> vars;
    vars.emplace("M", PropertyValue(glm::mat4(1.0f)));
    vars.emplace("N", PropertyValue(glm::mat4(2.0f)));
    vars.emplace("s", PropertyValue(2.0));

    MathNode leaf;
    leaf.op = MathNode::Op::ValueLeaf;
    leaf.variableName = "M";
    const auto leafValue = leaf.evaluate(vars);
    assert(leafValue.has_value());
    assert(std::holds_alternative<glm::mat4>(*leafValue));

    // Ordinary scalar/vector operators MUST NOT accidentally reinterpret a
    // matrix. Today they refuse with nullopt. Rung 1/2 will replace this
    // absence with explicit Matrix typing + authored matrix operations.
    MathNode add;
    add.op = MathNode::Op::Add;
    add.children.push_back(valueLeaf("M"));
    add.children.push_back(valueLeaf("N"));
    assert(!add.evaluate(vars).has_value());

    MathNode scale;
    scale.op = MathNode::Op::Scale;
    scale.children.push_back(valueLeaf("M"));
    scale.children.push_back(valueLeaf("s"));
    assert(!scale.evaluate(vars).has_value());

    MathNode component;
    component.op = MathNode::Op::Component;
    component.stringArg = "x";
    component.children.push_back(valueLeaf("M"));
    assert(!component.evaluate(vars).has_value());

    // There is deliberately no test that codifies glm::inverse(singular) output.
    // Current production call sites do not share a stable singular-matrix
    // semantic contract; platform-dependent NaN/Inf is not mathematics to
    // preserve. The unification plan requires OntoMath to REFUSE singular inverse.
    std::puts("ontomath_matrix_refusal_test: PASS");
    return 0;
}
