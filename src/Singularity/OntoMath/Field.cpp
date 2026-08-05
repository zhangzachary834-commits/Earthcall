#include "Field.hpp"
#include <iostream>

namespace OntoMath {

nlohmann::json ScalarField::toJson() const {
    nlohmann::json j;
    j["mode"] = (mode == EvaluationMode::AST) ? "AST" : "Procedural";
    j["baseDensity"] = baseDensity;
    j["frequency"] = frequency;
    j["amplitude"] = amplitude;
    if (mode == EvaluationMode::AST) {
        j["astDefinition"] = astDefinition.toJson();
    }
    return j;
}

std::shared_ptr<ScalarField> ScalarField::fromJson(const nlohmann::json& j) {
    auto field = std::make_shared<ScalarField>();
    
    std::string m = j.value("mode", "Procedural");
    field->mode = (m == "AST") ? EvaluationMode::AST : EvaluationMode::Procedural;
    
    field->baseDensity = j.value("baseDensity", 1.0f);
    field->frequency = j.value("frequency", 1.0f);
    field->amplitude = j.value("amplitude", 1.0f);
    
    if (j.contains("astDefinition")) {
        field->astDefinition = Piecewise::fromJson(j["astDefinition"]);
    } else {
        field->astDefinition = Piecewise::continuous(
            MathNode::fromLegacyExpression(ScalarForm::constant(0.0))
        );
    }
    return field;
}

nlohmann::json VectorField::toJson() const {
    nlohmann::json j;
    j["mode"] = (mode == EvaluationMode::AST) ? "AST" : "Procedural";
    j["baseFlowX"] = baseFlowX;
    j["baseFlowY"] = baseFlowY;
    j["baseFlowZ"] = baseFlowZ;
    j["frequency"] = frequency;
    j["amplitude"] = amplitude;
    if (mode == EvaluationMode::AST) {
        j["astDefinition"] = astDefinition.toJson();
    }
    return j;
}

std::shared_ptr<VectorField> VectorField::fromJson(const nlohmann::json& j) {
    auto field = std::make_shared<VectorField>();
    
    std::string m = j.value("mode", "Procedural");
    field->mode = (m == "AST") ? EvaluationMode::AST : EvaluationMode::Procedural;
    
    field->baseFlowX = j.value("baseFlowX", 0.0f);
    field->baseFlowY = j.value("baseFlowY", 0.0f);
    field->baseFlowZ = j.value("baseFlowZ", 0.0f);
    field->frequency = j.value("frequency", 1.0f);
    field->amplitude = j.value("amplitude", 1.0f);
    
    if (j.contains("astDefinition")) {
        field->astDefinition = Piecewise::fromJson(j["astDefinition"]);
    } else {
        auto node = std::make_shared<MathNode>();
        node->op = MathNode::Op::VectorConstruct;
        for (int i = 0; i < 3; ++i) {
            auto child = std::make_unique<MathNode>();
            child->op = MathNode::Op::ScalarLeaf;
            child->scalarForm = ScalarForm::constant(0.0);
            node->children.push_back(std::move(child));
        }
        field->astDefinition = Piecewise::continuous(std::move(node));
    }
    return field;
}

} // namespace OntoMath
