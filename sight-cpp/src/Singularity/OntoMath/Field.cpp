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

} // namespace OntoMath
