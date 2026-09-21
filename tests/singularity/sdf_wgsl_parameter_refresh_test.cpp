// Regression witness for SDF WGSL structure/value separation.
//
// A value-only edit must not require shader-source regeneration. collectParams()
// follows compile()'s exact parameter traversal order and must therefore produce
// the same parameter block a full compile would have produced for the changed
// values, while the WGSL source itself remains byte-identical.
//
// This test covers both ordinary CSG parameters and the special analytic-gradient
// Perlin path, because the latter has its own traversal order.

#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

int failures = 0;

void check(bool ok, const char* what) {
    std::printf("  %s: %s\n", ok ? "ok" : "FAILED", what);
    if (!ok) ++failures;
}

bool sameFloats(const std::vector<float>& a, const std::vector<float>& b) {
    return a == b;
}

std::unique_ptr<OntoMath::MathNode> number(double value) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ScalarLeaf;
    n->scalarForm.terms.push_back(OntoMath::Term(value));
    return n;
}

std::unique_ptr<OntoMath::MathNode> variable(const std::string& name) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ValueLeaf;
    n->variableName = name;
    return n;
}

std::unique_ptr<OntoMath::MathNode> vector3(double x, double y, double z) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::VectorConstruct;
    n->children.push_back(number(x));
    n->children.push_back(number(y));
    n->children.push_back(number(z));
    return n;
}

std::shared_ptr<OntoMath::MathNode> terrainMath(double amplitude) {
    auto y = variable("y");
    auto p = variable("p");

    auto plus = std::make_unique<OntoMath::MathNode>();
    plus->op = OntoMath::MathNode::Op::Add;
    plus->children.push_back(std::move(p));
    plus->children.push_back(vector3(100.0, 0.0, 100.0));

    auto scaledPoint = std::make_unique<OntoMath::MathNode>();
    scaledPoint->op = OntoMath::MathNode::Op::Scale;
    scaledPoint->children.push_back(number(0.008));
    scaledPoint->children.push_back(std::move(plus));

    auto noise = std::make_unique<OntoMath::MathNode>();
    noise->op = OntoMath::MathNode::Op::Noise;
    noise->children.push_back(std::move(scaledPoint));

    auto scaledNoise = std::make_unique<OntoMath::MathNode>();
    scaledNoise->op = OntoMath::MathNode::Op::Scale;
    scaledNoise->children.push_back(number(amplitude));
    scaledNoise->children.push_back(std::move(noise));

    auto root = std::make_unique<OntoMath::MathNode>();
    root->op = OntoMath::MathNode::Op::Sub;
    root->children.push_back(std::move(y));
    root->children.push_back(std::move(scaledNoise));
    return std::shared_ptr<OntoMath::MathNode>(root.release());
}

} // namespace

int main() {
    std::printf("Running SDF WGSL parameter refresh test...\n");

    // ---------------------------------------------------------------------
    // 1. Ordinary CSG: blend and child offset are parameter values, not WGSL
    //    structure. Recollection must exactly match a full compile.
    // ---------------------------------------------------------------------
    {
        auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.8f));
        auto torus = geom::SdfNode::leaf(geom::SdfPrim::Torus,
                                         glm::vec3(1.1f, 0.22f, 0.0f));
        auto field = geom::SdfNode::binary(geom::SdfOp::SmoothUnion,
                                           sphere, torus, 0.17f);

        const sdfwgsl::Program before = sdfwgsl::compile(field);
        check(before.ok, "initial CSG program compiles");

        field.t = 0.63f;
        field.children[1]->offset = glm::vec3(0.3f, -0.1f, 0.2f);

        const sdfwgsl::ParameterBlock refreshed = sdfwgsl::collectParams(field);
        const sdfwgsl::Program after = sdfwgsl::compile(field);

        check(refreshed.ok, "CSG parameter-only recollection succeeds");
        check(after.ok, "mutated CSG full compile succeeds");
        check(before.wgsl == after.wgsl,
              "CSG value edits leave WGSL byte-identical");
        check(!sameFloats(before.params, after.params),
              "CSG value edits change the parameter block");
        check(sameFloats(refreshed.values, after.params),
              "CSG recollection exactly matches full-compile parameters");

        field.op = geom::SdfOp::Union;
        const sdfwgsl::Program structural = sdfwgsl::compile(field);
        check(structural.ok, "structurally-mutated CSG compiles");
        check(after.wgsl != structural.wgsl,
              "CSG operator edit changes WGSL structure");
    }

    // ---------------------------------------------------------------------
    // 2. Analytic gradients are a property of supported mathematics, not of
    //    Noise specifically. length(p)-r is differentiable by the same jet
    //    machinery and must not fall back to finite differences.
    // ---------------------------------------------------------------------
    {
        auto length = std::make_unique<OntoMath::MathNode>();
        length->op = OntoMath::MathNode::Op::Length;
        length->children.push_back(variable("p"));

        auto radiusField = std::make_unique<OntoMath::MathNode>();
        radiusField->op = OntoMath::MathNode::Op::Sub;
        radiusField->children.push_back(std::move(length));
        radiusField->children.push_back(number(0.75));

        geom::SdfNode implicit = geom::makeImplicit(
            std::shared_ptr<OntoMath::MathNode>(radiusField.release()));
        const sdfwgsl::Program compiled = sdfwgsl::compile(implicit);

        check(compiled.ok, "non-noise differentiable Expr compiles");
        check(compiled.wgsl.find("fn sdfEvalGrad") != std::string::npos,
              "non-noise differentiable Expr emits analytic value+gradient");
    }

    // ---------------------------------------------------------------------
    // 3. Analytic Perlin-gradient path: compile() traverses the root through
    //    emitMathNodeGrad instead of ordinary emitNode. collectParams() must
    //    preserve that special ordering exactly.
    // ---------------------------------------------------------------------
    {
        geom::SdfNode terrain = geom::makeImplicit(terrainMath(40.0));
        terrain.offset = glm::vec3(2.0f, 3.0f, 4.0f);

        const sdfwgsl::Program before = sdfwgsl::compile(terrain);
        check(before.ok, "analytic Perlin program compiles");
        check(before.needsGradientStep, "Perlin expression uses gradient-corrected marcher");

        // Root is Sub(y, Scale(amplitude, Noise(...))).
        auto& amplitudeNode = terrain.mathNode->children[1]->children[0];
        amplitudeNode->scalarForm.terms[0].coefficient = 55.0;
        terrain.offset = glm::vec3(-5.0f, 1.5f, 8.0f);

        const sdfwgsl::ParameterBlock refreshed = sdfwgsl::collectParams(terrain);
        const sdfwgsl::Program after = sdfwgsl::compile(terrain);

        check(refreshed.ok, "analytic Perlin parameter recollection succeeds");
        check(after.ok, "mutated analytic Perlin full compile succeeds");
        check(before.wgsl == after.wgsl,
              "analytic Perlin value edits leave WGSL byte-identical");
        check(!sameFloats(before.params, after.params),
              "analytic Perlin value edits change parameter values");
        check(sameFloats(refreshed.values, after.params),
              "analytic Perlin recollection exactly matches full-compile parameters");
    }

    // ---------------------------------------------------------------------
    // 4. Authored radiance uses the SAME OntoMath emitter and parameter
    //    traversal as geometry/material math. A value-only radiance edit must
    //    recollect to the exact full-compile buffer without changing WGSL.
    // ---------------------------------------------------------------------
    {
        auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));

        auto radianceNode = std::shared_ptr<OntoMath::MathNode>(number(0.75).release());
        OntoMath::Piecewise radiance;
        radiance.pieces.push_back({
            false, false, 0.0, 0.0, true, true,
            radianceNode, nullptr, nullptr, nullptr, nullptr, nullptr
        });

        const sdfwgsl::Program before =
            sdfwgsl::compile(sphere, nullptr, nullptr, &radiance);
        const sdfwgsl::ScalarExpressionLayout layoutBefore =
            sdfwgsl::inspectScalarExpression(&radiance);
        check(before.ok, "authored radiance program compiles");
        check(layoutBefore.ok, "authored radiance structure inspection succeeds");
        check(before.wgsl.find("fn lightRadiance(p: vec3<f32>) -> f32") != std::string::npos,
              "authored radiance emits the shared OntoMath WGSL function");

        radianceNode->scalarForm.terms[0].coefficient = 0.25;
        const sdfwgsl::ParameterBlock refreshed =
            sdfwgsl::collectParams(sphere, nullptr, nullptr, &radiance);
        const sdfwgsl::Program after =
            sdfwgsl::compile(sphere, nullptr, nullptr, &radiance);
        const sdfwgsl::ScalarExpressionLayout layoutAfter =
            sdfwgsl::inspectScalarExpression(&radiance);

        check(refreshed.ok, "radiance parameter recollection succeeds");
        check(layoutAfter.ok, "mutated radiance structure inspection succeeds");
        check(layoutBefore.structure == layoutAfter.structure,
              "numeric radiance edit preserves emitted structure identity");
        check(layoutBefore.parameterCount == layoutAfter.parameterCount,
              "numeric radiance edit preserves parameter layout");
        check(after.ok, "mutated radiance full compile succeeds");
        check(before.wgsl == after.wgsl,
              "radiance value edit leaves WGSL byte-identical");
        check(!sameFloats(before.params, after.params),
              "radiance value edit changes the parameter block");
        check(sameFloats(refreshed.values, after.params),
              "radiance recollection exactly matches full-compile parameters");

        auto add = std::make_shared<OntoMath::MathNode>();
        add->op = OntoMath::MathNode::Op::Add;
        add->children.push_back(number(0.10));
        add->children.push_back(number(0.15));
        radiance.pieces[0].mathNode = add;
        const sdfwgsl::ScalarExpressionLayout structural =
            sdfwgsl::inspectScalarExpression(&radiance);
        check(structural.ok, "structurally changed radiance remains compilable");
        check(structural.structure != layoutAfter.structure,
              "radiance operator-tree edit changes emitted structure identity");

        auto unsupported = std::make_shared<OntoMath::MathNode>();
        unsupported->op = OntoMath::MathNode::Op::Raycast;
        radiance.pieces[0].mathNode = unsupported;
        const sdfwgsl::ScalarExpressionLayout refused =
            sdfwgsl::inspectScalarExpression(&radiance);
        check(!refused.ok && !refused.error.empty(),
              "unsupported authored radiance refuses during structure inspection");

        const sdfwgsl::Program legacy = sdfwgsl::compile(sphere);
        check(legacy.ok, "legacy no-radiance program still compiles");
        check(legacy.wgsl.find("fn lightRadiance(p: vec3<f32>) -> f32") != std::string::npos,
              "no-radiance source still exposes the common lightRadiance seam");
    }

    // ---------------------------------------------------------------------
    // 5. Rung 4 world time is an ambient input, not an authored parameter.
    //    A rho(p,t) expression must compile to the shared Universe-time
    //    uniform and therefore require no parameter slot or per-frame WGSL
    //    regeneration merely because t advances.
    // ---------------------------------------------------------------------
    {
        auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));

        auto timeNode = std::shared_ptr<OntoMath::MathNode>(
            variable(OntoMath::kTimeVar).release());
        OntoMath::Piecewise timedRadiance =
            OntoMath::Piecewise::continuous(timeNode);

        const sdfwgsl::ScalarExpressionLayout unboundLayout =
            sdfwgsl::inspectScalarExpression(&timedRadiance);
        const sdfwgsl::ScalarExpressionLayout layout =
            sdfwgsl::inspectScalarExpression(&timedRadiance, true);
        const sdfwgsl::Program timed =
            sdfwgsl::compile(sphere, nullptr, nullptr, &timedRadiance);

        check(!unboundLayout.ok &&
                  unboundLayout.error.find("does not bind world time") != std::string::npos,
              "t refuses in a shader expression context that did not opt into world time");
        check(layout.ok, "rho(p,t) structure inspection succeeds");
        check(timed.ok, "rho(p,t) WGSL compilation succeeds");
        check(layout.parameterCount == 0,
              "world time consumes no authored parameter slot");
        check(timed.wgsl.find("u.time.x") != std::string::npos,
              "canonical t binds to the shared SDF world-time uniform");

        auto scalarTime = std::make_shared<OntoMath::MathNode>();
        scalarTime->op = OntoMath::MathNode::Op::ScalarLeaf;
        scalarTime->scalarForm.terms.push_back(
            OntoMath::Term(2.0, {{OntoMath::kTimeVar, 1.0}}));
        timedRadiance.pieces[0].mathNode = scalarTime;
        const sdfwgsl::Program scalarTimed =
            sdfwgsl::compile(sphere, nullptr, nullptr, &timedRadiance);
        check(scalarTimed.ok && scalarTimed.wgsl.find("u.time.x") != std::string::npos,
              "ScalarForm factors may use the same canonical t binding");
    }

    if (failures) {
        std::printf("sdf_wgsl_parameter_refresh_test: %d failure(s)\n", failures);
        return 1;
    }
    std::printf("sdf_wgsl_parameter_refresh_test: PASS\n");
    return 0;
}
