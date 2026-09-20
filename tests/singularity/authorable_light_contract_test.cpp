#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include "Singularity/Screen/AuthorableLight.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <variant>

#include <nlohmann/json.hpp>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << '\n';
    } else {
        std::cout << "  ok: " << description << '\n';
    }
}

bool nearf(float a, float b, float eps = 1e-5f) {
    return std::fabs(a - b) <= eps;
}

bool near3(const glm::vec3& a, const glm::vec3& b, float eps = 1e-5f) {
    return nearf(a.x, b.x, eps) && nearf(a.y, b.y, eps) && nearf(a.z, b.z, eps);
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running authored radiant FieldNode contract test...\n";
    std::cout << "============================================================\n";

    geom::FieldNode field("sun.light-field");
    field.origin = glm::vec3(3.0f, 12.0f, -5.0f);

    Rendering::AuthorableLightState state;
    check(!Rendering::readAuthorableLight(field, state),
          "a FieldNode is not secretly a light source without authored light.source");

    field.setDynamicProperty("light.source", PropertyValue(true));
    field.setDynamicProperty("light.enabled", PropertyValue(false));
    field.setDynamicProperty("light.color", PropertyValue(glm::vec3(1.0f, 0.5f, 0.25f)));
    field.setDynamicProperty("light.intensity", PropertyValue(2.0f));
    field.setDynamicProperty("light.ambient", PropertyValue(0.1f));
    field.setDynamicProperty("light.diffuse", PropertyValue(0.75f));
    field.setDynamicProperty("light.specular", PropertyValue(0.5f));
    field.setDynamicProperty("light.attenuation.constant", PropertyValue(1.0f));
    field.setDynamicProperty("light.attenuation.linear", PropertyValue(0.2f));
    field.setDynamicProperty("light.attenuation.quadratic", PropertyValue(0.03f));

    check(Rendering::readAuthorableLight(field, state),
          "authored light.source promotes the continuous field into renderer-consumed radiance");
    check(state.source, "resolved state remembers that the field is a source");
    check(!state.enabled, "light.enabled is authorable and type-preserving");
    check(near3(state.position, field.origin), "registered FieldNode origin is the light placement");
    check(near3(state.color, glm::vec3(1.0f, 0.5f, 0.25f)), "light.color is authorable");
    check(nearf(state.intensity, 2.0f), "light.intensity is authorable");
    check(nearf(state.ambient, 0.1f), "light.ambient is authorable");
    check(nearf(state.diffuse, 0.75f), "light.diffuse is authorable");
    check(nearf(state.specular, 0.5f), "light.specular is authorable");
    check(nearf(state.attenuationConstant, 1.0f) &&
              nearf(state.attenuationLinear, 0.2f) &&
              nearf(state.attenuationQuadratic, 0.03f),
          "attenuation vocabulary resolves without inventing a Light C++ kind");

    check(near3(Rendering::lightAmbientRadiance(state), glm::vec3(0.2f, 0.1f, 0.05f)),
          "ambient renderer radiance is color * intensity * authored ambient coefficient");
    check(near3(Rendering::lightDiffuseRadiance(state), glm::vec3(1.5f, 0.75f, 0.375f)),
          "diffuse renderer radiance is color * intensity * authored diffuse coefficient");
    check(near3(Rendering::lightSpecularRadiance(state), glm::vec3(1.0f, 0.5f, 0.25f)),
          "specular renderer radiance is color * intensity * authored specular coefficient");

    // Hydrate the actual authored Sun Zone spatial root rather than rebuilding
    // a lookalike AST in test code. This witnesses the save -> FieldNode ->
    // OntoMath path that EngineRender consumes.
    {
        namespace fs = std::filesystem;
        const fs::path repoRoot = fs::path(__FILE__).parent_path().parent_path().parent_path();
        std::ifstream in(repoRoot / "saves/zones/Sun/zone.json");
        check(static_cast<bool>(in), "actual Sun Zone save is readable");

        nlohmann::json sun;
        if (in) in >> sun;
        check(sun.contains("spatialRoot"), "Sun save carries a spatial root");

        geom::FieldNode hydrated("sun.light-field.test");
        if (sun.contains("spatialRoot")) hydrated.applyJson(sun["spatialRoot"]);

        check(hydrated.field != nullptr, "Sun spatial root hydrates a scalar field");
        check(hydrated.field &&
              hydrated.field->mode == OntoMath::ScalarField::EvaluationMode::AST,
              "Sun scalar field hydrates in AST mode");
        check(hydrated.field && !hydrated.field->astDefinition.pieces.empty(),
              "Sun scalar field hydrates an authored Piecewise");

        Rendering::AuthorableLightState hydratedLight;
        check(Rendering::readAuthorableLight(hydrated, hydratedLight) && hydratedLight.source,
              "hydrated Sun spatial root retains authored light.source");

        auto eval = [&](double x, double y, double z) -> double {
            if (!hydrated.field) return -1.0;
            std::map<std::string, PropertyValue> vars{
                {"x", PropertyValue(x)},
                {"y", PropertyValue(y)},
                {"z", PropertyValue(z)}
            };
            const auto value = hydrated.field->astDefinition.evaluate(vars);
            if (!value || !std::holds_alternative<double>(*value)) return -1.0;
            return std::get<double>(*value);
        };

        const double nearSource = eval(0.0, 0.0, 0.0);
        const double farther = eval(20.0, 0.0, 0.0);
        check(nearSource > 0.99, "Sun radiance is approximately unit strength at its source");
        check(farther >= 0.0 && farther < nearSource,
              "Sun authored radiance decreases with distance on the CPU");
    }

    // Wrongly typed authored state is visible but not silently guessed into a
    // different value. The resolver keeps its documented default.
    field.setDynamicProperty("light.intensity", PropertyValue(std::string("very bright")));
    Rendering::AuthorableLightState typed;
    check(Rendering::readAuthorableLight(field, typed), "source remains valid after a bad optional value");
    check(nearf(typed.intensity, 1.0f), "wrongly typed intensity is refused rather than coerced by guesswork");

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks - g_failures << "/" << g_checks << " checks passed\n";
    if (g_failures) {
        std::cout << "authorable_light_contract_test: FAILED\n";
        return 1;
    }
    std::cout << "authorable_light_contract_test: ALL OK\n";
    return 0;
}
