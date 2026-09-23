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
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"

#include <cmath>
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
    // 5. Rung 4's admitted temporal coordinate is an ambient input, not an
    //    authored parameter. rho(p,t) must compile to the shared temporal
    //    uniform without knowing which Timeline supplied t, and therefore
    //    requires no parameter slot or per-frame WGSL regeneration.
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
                  unboundLayout.error.find("does not bind the temporal coordinate") != std::string::npos,
              "t refuses in a shader expression context that did not opt into time");
        check(layout.ok, "rho(p,t) structure inspection succeeds");
        check(timed.ok, "rho(p,t) WGSL compilation succeeds");
        check(layout.parameterCount == 0,
              "temporal coordinate consumes no authored parameter slot");
        check(timed.wgsl.find("u.radianceTime.x") != std::string::npos,
              "canonical t binds to the shared SDF temporal uniform");

        auto scalarTime = std::make_shared<OntoMath::MathNode>();
        scalarTime->op = OntoMath::MathNode::Op::ScalarLeaf;
        scalarTime->scalarForm.terms.push_back(
            OntoMath::Term(2.0, {{OntoMath::kTimeVar, 1.0}}));
        timedRadiance.pieces[0].mathNode = scalarTime;
        const sdfwgsl::Program scalarTimed =
            sdfwgsl::compile(sphere, nullptr, nullptr, &timedRadiance);
        check(scalarTimed.ok && scalarTimed.wgsl.find("u.radianceTime.x") != std::string::npos,
              "ScalarForm factors may use the same canonical t binding");

        // Piecewise applicability must use the same admitted coordinate. Before
        // Rung 4 this emitter recognized only x/y/z and silently used 0.0 for
        // every other inputVariable, which would make a bounded rho(t) choose
        // the wrong branch while still producing valid WGSL.
        OntoMath::Piecewise boundedTime =
            OntoMath::Piecewise::continuous(
                std::shared_ptr<OntoMath::MathNode>(number(1.0).release()));
        boundedTime.inputVariable = OntoMath::kTimeVar;
        boundedTime.pieces[0].hasLo = true;
        boundedTime.pieces[0].lo = 0.25;
        boundedTime.pieces[0].hasHi = true;
        boundedTime.pieces[0].hi = 0.75;

        const auto boundedUnbound =
            sdfwgsl::inspectScalarExpression(&boundedTime);
        const auto boundedLayout =
            sdfwgsl::inspectScalarExpression(&boundedTime, true);
        const auto boundedProgram =
            sdfwgsl::compile(sphere, nullptr, nullptr, &boundedTime);

        check(!boundedUnbound.ok,
              "bounded rho(t) refuses when temporal coordinate is not admitted");
        check(boundedLayout.ok && boundedProgram.ok,
              "bounded rho(t) compiles when temporal coordinate is admitted");
        check(boundedProgram.wgsl.find("u.radianceTime.x >=") != std::string::npos &&
                  boundedProgram.wgsl.find("u.radianceTime.x <=") != std::string::npos,
              "Piecewise t bounds read the admitted Timeline coordinate");
    }

    // ---------------------------------------------------------------------
    // 6. Rung 5: chi(p,t)->vec3 is independent authored source chroma.
    //    Numeric edits refresh parameters; structure edits compile; t is an
    //    ambient source coordinate; absence remains legacy light.color.
    // ---------------------------------------------------------------------
    {
        auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));
        auto rhoNode = std::shared_ptr<OntoMath::MathNode>(number(1.0).release());
        OntoMath::Piecewise rho = OntoMath::Piecewise::continuous(rhoNode);

        auto chiNode = std::shared_ptr<OntoMath::MathNode>(vector3(1.0, 0.25, 0.0).release());
        OntoMath::Piecewise chi = OntoMath::Piecewise::continuous(chiNode);

        const auto legacyLayout = sdfwgsl::inspectVectorExpression(nullptr, true);
        const auto layoutBefore = sdfwgsl::inspectVectorExpression(&chi, true);
        const auto before = sdfwgsl::compile(sphere, nullptr, nullptr, &rho, &chi);
        check(legacyLayout.ok &&
                  legacyLayout.structure.find("legacy-chroma:light.color") != std::string::npos,
              "absent chi has explicit legacy light.color structural identity");
        check(layoutBefore.ok, "authored chi vector structure inspection succeeds");
        check(before.ok && before.wgsl.find("fn lightChroma(p: vec3<f32>) -> vec3<f32>") != std::string::npos,
              "authored chi lowers through the production OntoMath WGSL emitter");
        check(before.wgsl.find("const HAS_AUTHORED_CHROMA: bool = true") != std::string::npos,
              "authored chi selects the separated source-chroma lighting path");

        // VALUE ONLY: mutate the red component's ScalarLeaf coefficient.
        chiNode->children[0]->scalarForm.terms[0].coefficient = 0.2;
        const auto layoutAfter = sdfwgsl::inspectVectorExpression(&chi, true);
        const auto refreshed = sdfwgsl::collectParams(sphere, nullptr, nullptr, &rho, &chi);
        const auto after = sdfwgsl::compile(sphere, nullptr, nullptr, &rho, &chi);
        check(layoutAfter.ok && layoutAfter.structure == layoutBefore.structure,
              "numeric chi edit preserves vector structure identity");
        check(layoutAfter.parameterCount == layoutBefore.parameterCount,
              "numeric chi edit preserves vector parameter layout");
        check(after.ok && before.wgsl == after.wgsl,
              "numeric chi edit leaves WGSL byte-identical");
        check(!sameFloats(before.params, after.params),
              "numeric chi edit changes authored parameter data");
        check(refreshed.ok && sameFloats(refreshed.values, after.params),
              "chi parameter recollection exactly matches full compile");

        // STRUCTURE/TIME: chi=(t,0,0). t is admitted but takes no authored slot.
        auto timeVector = std::make_shared<OntoMath::MathNode>();
        timeVector->op = OntoMath::MathNode::Op::VectorConstruct;
        timeVector->children.push_back(variable(OntoMath::kTimeVar));
        timeVector->children.push_back(number(0.0));
        timeVector->children.push_back(number(0.0));
        chi.pieces[0].mathNode = timeVector;
        const auto timedUnbound = sdfwgsl::inspectVectorExpression(&chi, false);
        const auto timedLayout = sdfwgsl::inspectVectorExpression(&chi, true);
        const auto timed = sdfwgsl::compile(sphere, nullptr, nullptr, &rho, &chi);
        check(!timedUnbound.ok,
              "timed chi refuses in an expression context that did not admit source time");
        check(timedLayout.ok && timedLayout.structure != layoutAfter.structure,
              "structural chi edit changes emitted structure identity");
        check(timed.ok && timed.wgsl.find("u.radianceTime.x") != std::string::npos,
              "chi(p,t) binds the same admitted radiance-source Timeline coordinate");

        // An authored scalar is NOT silently accepted as RGB merely because the
        // caller expected chroma.
        OntoMath::Piecewise scalarChi = OntoMath::Piecewise::continuous(
            std::shared_ptr<OntoMath::MathNode>(number(0.5).release()));
        const auto wrongType = sdfwgsl::inspectVectorExpression(&scalarChi, true);
        check(!wrongType.ok && wrongType.error.find("Vector") != std::string::npos,
              "non-vector authored chi refuses instead of falling back to a color");

        auto badVector = std::make_shared<OntoMath::MathNode>();
        badVector->op = OntoMath::MathNode::Op::VectorConstruct;
        auto raycast = std::make_unique<OntoMath::MathNode>();
        raycast->op = OntoMath::MathNode::Op::Raycast;
        badVector->children.push_back(std::move(raycast));
        badVector->children.push_back(number(0.0));
        badVector->children.push_back(number(0.0));
        OntoMath::Piecewise unsupportedChi = OntoMath::Piecewise::continuous(badVector);
        const auto refused = sdfwgsl::inspectVectorExpression(&unsupportedChi, true);
        check(!refused.ok && refused.error.find("Raycast") != std::string::npos,
              "unsupported authored chroma math refuses explicitly");

        const auto legacy = sdfwgsl::compile(sphere, nullptr, nullptr, &rho, nullptr);
        check(legacy.ok &&
                  legacy.wgsl.find("const HAS_AUTHORED_CHROMA: bool = false") != std::string::npos,
              "source without chi retains the exact legacy-color compatibility branch");
    }


    // ---------------------------------------------------------------------
    // 7. Rung 6: alpha(p,omega,t)->scalar is independent authored angular
    //    emission. omega is admitted ONLY here and means normalized world-space
    //    source -> receiver direction at the production shader seam.
    // ---------------------------------------------------------------------
    {
        auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));
        OntoMath::Piecewise rho = OntoMath::Piecewise::continuous(
            std::shared_ptr<OntoMath::MathNode>(number(1.0).release()));
        OntoMath::Piecewise chi = OntoMath::Piecewise::continuous(
            std::shared_ptr<OntoMath::MathNode>(vector3(1.0, 1.0, 1.0).release()));

        auto coefficient = number(-1.0);
        auto omegaZ = variable(OntoMath::kOmegaZVar);
        auto lobe = std::make_shared<OntoMath::MathNode>();
        lobe->op = OntoMath::MathNode::Op::Scale;
        lobe->children.push_back(std::move(coefficient));
        lobe->children.push_back(std::move(omegaZ));
        OntoMath::Piecewise alpha = OntoMath::Piecewise::continuous(lobe);

        const auto legacyLayout = sdfwgsl::inspectAngularExpression(nullptr);
        const auto layoutBefore = sdfwgsl::inspectAngularExpression(&alpha);
        const auto before =
            sdfwgsl::compile(sphere, nullptr, nullptr, &rho, &chi, &alpha);

        check(legacyLayout.ok && !legacyLayout.readsOmega &&
                  legacyLayout.structure.find("legacy-angular:1.0") != std::string::npos,
              "absent alpha has explicit multiplicative-identity structure");
        check(layoutBefore.ok && layoutBefore.readsOmega,
              "authored directional alpha records that its structure reads omega");
        check(before.ok &&
                  before.wgsl.find("fn lightAngular(p: vec3<f32>, omega: vec3<f32>) -> f32") != std::string::npos &&
                  before.wgsl.find("omega.z") != std::string::npos &&
                  before.wgsl.find("sourceDelta / directionLength") != std::string::npos,
              "alpha lowers through production WGSL with normalized source-to-receiver omega");
        check(before.wgsl.find("const HAS_AUTHORED_ANGULAR: bool = true") != std::string::npos &&
                  before.wgsl.find("const ANGULAR_READS_OMEGA: bool = true") != std::string::npos,
              "production shader exposes authored/directional angular structure explicitly");

        // VALUE ONLY: keep Scale(number, omega.z), change only its coefficient.
        lobe->children[0]->scalarForm.terms[0].coefficient = -0.25;
        const auto layoutAfter = sdfwgsl::inspectAngularExpression(&alpha);
        const auto refreshed =
            sdfwgsl::collectParams(sphere, nullptr, nullptr, &rho, &chi, &alpha);
        const auto after =
            sdfwgsl::compile(sphere, nullptr, nullptr, &rho, &chi, &alpha);
        check(layoutAfter.ok && layoutAfter.structure == layoutBefore.structure &&
                  layoutAfter.readsOmega == layoutBefore.readsOmega,
              "numeric alpha edit preserves angular structure identity");
        check(layoutAfter.parameterCount == layoutBefore.parameterCount,
              "numeric alpha edit preserves angular parameter layout");
        check(after.ok && before.wgsl == after.wgsl,
              "numeric alpha edit leaves WGSL byte-identical");
        check(!sameFloats(before.params, after.params),
              "numeric alpha edit changes authored parameter data");
        check(refreshed.ok && sameFloats(refreshed.values, after.params),
              "alpha parameter recollection exactly matches full compile");

        // STRUCTURE + TIME: alpha = -omega.z * cos(t). The temporal coordinate
        // is ambient; its value is not baked into WGSL or the parameter buffer.
        auto cosine = std::make_unique<OntoMath::MathNode>();
        cosine->op = OntoMath::MathNode::Op::ScalarLeaf;
        cosine->scalarForm =
            OntoMath::ScalarForm::transcendental(OntoMath::TransFactor::Kind::Cos,
                                                 OntoMath::kTimeVar);
        auto omegaZTimed = variable(OntoMath::kOmegaZVar);
        auto directionalCos = std::make_unique<OntoMath::MathNode>();
        directionalCos->op = OntoMath::MathNode::Op::Scale;
        directionalCos->children.push_back(std::move(omegaZTimed));
        directionalCos->children.push_back(std::move(cosine));
        auto rotating = std::make_shared<OntoMath::MathNode>();
        rotating->op = OntoMath::MathNode::Op::Scale;
        rotating->children.push_back(number(-1.0));
        rotating->children.push_back(std::move(directionalCos));
        alpha.pieces[0].mathNode = rotating;

        const auto timedLayout = sdfwgsl::inspectAngularExpression(&alpha);
        const auto timed =
            sdfwgsl::compile(sphere, nullptr, nullptr, &rho, &chi, &alpha);
        check(timedLayout.ok && timedLayout.readsOmega &&
                  timedLayout.structure != layoutAfter.structure,
              "structural timed-alpha edit advances angular structure identity");
        check(timed.ok &&
                  timed.wgsl.find("u.radianceTime.x") != std::string::npos &&
                  timed.wgsl.find("omega.z") != std::string::npos,
              "alpha(omega,t) binds both the source Timeline and canonical omega");

        // omega must not leak into rho: the same authored variable outside the
        // angular context is a refusal, not a fabricated zero or direction.
        OntoMath::Piecewise illegalRho = OntoMath::Piecewise::continuous(
            std::shared_ptr<OntoMath::MathNode>(
                variable(OntoMath::kOmegaXVar).release()));
        const auto omegaOutsideAngular =
            sdfwgsl::inspectScalarExpression(&illegalRho, true);
        check(!omegaOutsideAngular.ok &&
                  omegaOutsideAngular.error.find("does not bind omega") != std::string::npos,
              "omega refuses outside the admitted angular-radiance context");

        auto raycast = std::make_shared<OntoMath::MathNode>();
        raycast->op = OntoMath::MathNode::Op::Raycast;
        alpha.pieces[0].mathNode = raycast;
        const auto refused = sdfwgsl::inspectAngularExpression(&alpha);
        check(!refused.ok && refused.error.find("Raycast") != std::string::npos,
              "unsupported authored angular math refuses explicitly");

        const auto legacy =
            sdfwgsl::compile(sphere, nullptr, nullptr, &rho, &chi, nullptr);
        check(legacy.ok &&
                  legacy.wgsl.find("const HAS_AUTHORED_ANGULAR: bool = false") != std::string::npos &&
                  legacy.wgsl.find("return 1.0;") != std::string::npos,
              "source without alpha retains exact multiplicative-identity compatibility");
    }

    // ---------------------------------------------------------------------
    // 8. Rung 7: multiple source ASTs remain independent and are composed
    //    above the source invariants. Numeric edits refresh the packed values;
    //    changing emitted structure changes WGSL. Source time is per-source.
    // ---------------------------------------------------------------------
    {
        auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));

        auto rho0Node = std::shared_ptr<OntoMath::MathNode>(number(1.0).release());
        auto rho1Node = std::shared_ptr<OntoMath::MathNode>(number(0.5).release());
        OntoMath::Piecewise rho0 = OntoMath::Piecewise::continuous(rho0Node);
        OntoMath::Piecewise rho1 = OntoMath::Piecewise::continuous(rho1Node);

        auto chi0Node = std::shared_ptr<OntoMath::MathNode>(vector3(1.0, 0.0, 0.0).release());
        auto chi1Node = std::shared_ptr<OntoMath::MathNode>(vector3(0.0, 0.0, 1.0).release());
        OntoMath::Piecewise chi0 = OntoMath::Piecewise::continuous(chi0Node);
        OntoMath::Piecewise chi1 = OntoMath::Piecewise::continuous(chi1Node);

        Rendering::RadianceSourceBinding s0;
        s0.radianceExpr = &rho0;
        s0.chromaExpr = &chi0;
        s0.temporalCoordinate = 0.25;

        Rendering::RadianceSourceBinding s1;
        s1.radianceExpr = &rho1;
        s1.chromaExpr = &chi1;
        s1.temporalCoordinate = 0.75;

        std::vector<Rendering::RadianceSourceBinding> sources{s0, s1};

        const auto before =
            sdfwgsl::compile(sphere, nullptr, nullptr, nullptr, nullptr, nullptr, &sources);
        check(before.ok, "two-source WGSL compilation succeeds");
        check(before.wgsl.find("@group(0) @binding(2) var<storage, read> RS") != std::string::npos,
              "multi-source WGSL admits a dedicated authored-source storage binding");
        check(before.wgsl.find("fn lightRadiance_0") != std::string::npos &&
                  before.wgsl.find("fn lightRadiance_1") != std::string::npos,
              "each source keeps its own rho function instead of enumerating the world inside one AST");
        check(before.wgsl.find("ambientTerm +=") != std::string::npos &&
                  before.wgsl.find("diffuseTerm +=") != std::string::npos,
              "source emission is aggregated additively above the individual invariants");

        // VALUE ONLY: same ScalarLeaf structure on source 1.
        rho1Node->scalarForm.terms[0].coefficient = 0.2;
        const auto refreshed =
            sdfwgsl::collectParams(sphere, nullptr, nullptr, nullptr, nullptr, nullptr, &sources);
        const auto valueEdited =
            sdfwgsl::compile(sphere, nullptr, nullptr, nullptr, nullptr, nullptr, &sources);
        check(refreshed.ok && valueEdited.ok,
              "multi-source numeric parameter refresh succeeds");
        check(before.wgsl == valueEdited.wgsl,
              "numeric edit in one source leaves multi-source WGSL byte-identical");
        check(!sameFloats(before.params, valueEdited.params),
              "numeric edit in one source changes packed authored parameters");
        check(sameFloats(refreshed.values, valueEdited.params),
              "multi-source parameter recollection exactly matches full compile");

        // STRUCTURE ONLY: source 1 becomes Add(number, number).
        auto add = std::make_shared<OntoMath::MathNode>();
        add->op = OntoMath::MathNode::Op::Add;
        add->children.push_back(number(0.1));
        add->children.push_back(number(0.1));
        rho1.pieces[0].mathNode = add;
        const auto structureEdited =
            sdfwgsl::compile(sphere, nullptr, nullptr, nullptr, nullptr, nullptr, &sources);
        check(structureEdited.ok && structureEdited.wgsl != valueEdited.wgsl,
              "structural edit in one source changes the composed WGSL structure");

        // Per-source relative time: only source 1 reads t, and it must bind that
        // source's own record rather than the historical global radianceTime.
        auto timed = std::make_shared<OntoMath::MathNode>();
        timed->op = OntoMath::MathNode::Op::ValueLeaf;
        timed->variableName = OntoMath::kTimeVar;
        rho1.pieces[0].mathNode = timed;
        const auto timedProgram =
            sdfwgsl::compile(sphere, nullptr, nullptr, nullptr, nullptr, nullptr, &sources);
        check(timedProgram.ok &&
                  timedProgram.wgsl.find("RS[1u].time.x") != std::string::npos,
              "source 1 temporal mathematics reads source 1's relative Timeline coordinate");
    }

    // ---------------------------------------------------------------------
    // V0. Density sovereignty: D(p,t) is an explicit compiler input with its
    //     own structure/value identity and its own temporal coordinate.
    //     It must never borrow rho merely because both are scalar Piecewise.
    // ---------------------------------------------------------------------
    {
        auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));

        auto rhoNode = std::shared_ptr<OntoMath::MathNode>(number(0.9).release());
        auto densityNode = std::shared_ptr<OntoMath::MathNode>(number(0.2).release());
        OntoMath::Piecewise rho = OntoMath::Piecewise::continuous(rhoNode);
        OntoMath::Piecewise density = OntoMath::Piecewise::continuous(densityNode);

        const std::string rhoBeforeDensityEdit = rho.toJson().dump();
        const auto densityLayoutBefore = sdfwgsl::inspectDensityExpression(&density);
        const auto before =
            sdfwgsl::compile(sphere, nullptr, nullptr, &rho, nullptr, nullptr, nullptr, &density,
                             sdfwgsl::DensityInputKind::Authored);

        check(densityLayoutBefore.ok,
              "V0 authored density structure inspection succeeds");
        check(before.ok &&
                  before.wgsl.find("fn volumeDensityEval(p: vec3<f32>) -> f32") != std::string::npos &&
                  before.wgsl.find("let density = volumeDensityEval(p)") != std::string::npos,
              "V0 density lowers through an explicitly named participating-medium evaluator");
        check(before.wgsl.find("fn fieldEval(") == std::string::npos,
              "new V0 shaders no longer expose generic fieldEval as density ontology");
        check(before.wgsl.find("let sample_t = t;") != std::string::npos &&
                  before.wgsl.find("max(min(t, maxDist) - sample_t, 0.0)") != std::string::npos &&
                  before.wgsl.find("first_density_t = sample_t") != std::string::npos,
              "V0 transport integrates the actual bounded marched interval from the sampled coordinate");
        check(before.wgsl.find("max(abs(d), current_eps)") == std::string::npos,
              "V0 transport no longer reinterprets SDF magnitude as optical path length");

        // VALUE ONLY: alter D while rho remains byte-identical.
        densityNode->scalarForm.terms[0].coefficient = 0.45;
        const auto densityLayoutAfter = sdfwgsl::inspectDensityExpression(&density);
        const auto refreshed =
            sdfwgsl::collectParams(sphere, nullptr, nullptr, &rho, nullptr, nullptr, nullptr, &density,
                                 sdfwgsl::DensityInputKind::Authored);
        const auto after =
            sdfwgsl::compile(sphere, nullptr, nullptr, &rho, nullptr, nullptr, nullptr, &density,
                             sdfwgsl::DensityInputKind::Authored);

        check(densityLayoutAfter.ok &&
                  densityLayoutAfter.structure == densityLayoutBefore.structure &&
                  densityLayoutAfter.parameterCount == densityLayoutBefore.parameterCount,
              "numeric D edit preserves density structure and parameter layout");
        check(after.ok && before.wgsl == after.wgsl,
              "numeric D edit leaves WGSL byte-identical");
        check(!sameFloats(before.params, after.params),
              "numeric D edit changes only packed authored parameter data");
        check(refreshed.ok && sameFloats(refreshed.values, after.params),
              "density parameter recollection exactly matches full compile");
        check(rho.toJson().dump() == rhoBeforeDensityEdit,
              "editing D leaves rho byte-identical");

        // Explicit density must outrank the old generic FieldNode density path.
        geom::FieldNode legacy("legacy-density-projection");
        legacy.field->mode = OntoMath::ScalarField::EvaluationMode::Procedural;
        legacy.field->baseDensity = 8.0f;
        legacy.field->frequency = 3.0f;
        legacy.field->amplitude = 2.0f;
        const auto explicitOverLegacy =
            sdfwgsl::compile(sphere, &legacy, nullptr, &rho, nullptr, nullptr, nullptr, &density,
                             sdfwgsl::DensityInputKind::Authored);
        check(explicitOverLegacy.ok &&
                  explicitOverLegacy.wgsl.find("V0: explicit authored D(p,t)") != std::string::npos &&
                  explicitOverLegacy.wgsl.find("rawDensity") == std::string::npos,
              "explicit volume.density.ast outranks legacy generic-field density");

        const auto legacyOnly = sdfwgsl::compile(sphere, &legacy);
        check(legacyOnly.ok &&
                  legacyOnly.wgsl.find("LEGACY procedural density projection") != std::string::npos,
              "legacy generic density remains quarantined as an explicit compatibility path");

        const auto explicitNone =
            sdfwgsl::compile(sphere, &legacy, nullptr, &rho, nullptr, nullptr, nullptr,
                             nullptr, sdfwgsl::DensityInputKind::None);
        check(explicitNone.ok &&
                  explicitNone.wgsl.find("LEGACY procedural density projection") == std::string::npos &&
                  explicitNone.wgsl.find("return 0.0;") != std::string::npos,
              "explicit no-medium state cannot reinterpret generic field.ast as density");

        // TIME: rho(t) and D(t) receive distinct ambient coordinates.
        OntoMath::Piecewise timedRho = OntoMath::Piecewise::continuous(
            std::shared_ptr<OntoMath::MathNode>(
                variable(OntoMath::kTimeVar).release()));
        OntoMath::Piecewise timedDensity = OntoMath::Piecewise::continuous(
            std::shared_ptr<OntoMath::MathNode>(
                variable(OntoMath::kTimeVar).release()));
        const auto timedDensityLayout = sdfwgsl::inspectDensityExpression(&timedDensity);
        const auto timed =
            sdfwgsl::compile(sphere, nullptr, nullptr, &timedRho, nullptr, nullptr, nullptr, &timedDensity,
                             sdfwgsl::DensityInputKind::Authored);
        check(timedDensityLayout.ok && timed.ok,
              "D(p,t) is admitted through the production OntoMath emitter");
        check(timed.wgsl.find("u.radianceTime.x") != std::string::npos &&
                  timed.wgsl.find("u.volumeTime.x") != std::string::npos,
              "rho(t) and D(t) bind independent renderer temporal coordinates");

        auto raycast = std::make_shared<OntoMath::MathNode>();
        raycast->op = OntoMath::MathNode::Op::Raycast;
        OntoMath::Piecewise unsupportedDensity =
            OntoMath::Piecewise::continuous(raycast);
        const auto refusedDensity =
            sdfwgsl::inspectDensityExpression(&unsupportedDensity);
        check(!refusedDensity.ok &&
                  refusedDensity.error.find("Raycast") != std::string::npos,
              "unsupported authored density math refuses instead of fabricating empty medium");

        // V0c dedicated composite shader: density is no longer piggy-backed on
        // a surface shader. It samples finished scene depth, owns no frag_depth,
        // and binds each medium's relative Timeline independently.
        const auto volumeBefore = sdfwgsl::compileVolume(&density);
        const auto volumeLayoutBefore = sdfwgsl::inspectDensityExpression(&density);
        check(volumeBefore.ok &&
                  volumeBefore.wgsl.find("texture_depth_2d") != std::string::npos &&
                  volumeBefore.wgsl.find("textureLoad(sceneDepthTex") != std::string::npos,
              "dedicated volume shader samples the finished opaque depth texture");
        check(volumeBefore.wgsl.find("@builtin(frag_depth)") == std::string::npos,
              "participating-medium composite owns no opaque fragment depth");

        auto dedicatedTimeNode = std::make_shared<OntoMath::MathNode>();
        dedicatedTimeNode->op = OntoMath::MathNode::Op::ValueLeaf;
        dedicatedTimeNode->variableName = OntoMath::kTimeVar;
        OntoMath::Piecewise dedicatedTimedDensity =
            OntoMath::Piecewise::continuous(dedicatedTimeNode);
        const auto timedVolume = sdfwgsl::compileVolume(&dedicatedTimedDensity);
        check(timedVolume.ok &&
                  timedVolume.wgsl.find("instances[g_instIdx].time.x") != std::string::npos,
              "dedicated D(p,t) volume shader reads the current medium's relative Timeline coordinate");

        check(volumeBefore.wgsl.find("worldP - inst.origin.xyz") != std::string::npos,
              "D(p,t) receives FieldNode-local offset coordinates");
        check(volumeBefore.wgsl.find("opaqueT") != std::string::npos &&
                  volumeBefore.wgsl.find("t1 = min(t1") != std::string::npos,
              "volume integration is truncated at finished scene depth");

        densityNode->scalarForm.terms[0].coefficient = 0.7;
        const auto volumeLayoutAfter = sdfwgsl::inspectDensityExpression(&density);
        const auto volumeParams = sdfwgsl::collectVolumeParams(&density);
        const auto volumeAfter = sdfwgsl::compileVolume(&density);
        check(volumeLayoutAfter.ok &&
                  volumeLayoutAfter.structure == volumeLayoutBefore.structure &&
                  volumeBefore.wgsl == volumeAfter.wgsl,
              "numeric D edit preserves dedicated volume shader structure");
        check(volumeParams.ok &&
                  sameFloats(volumeParams.values, volumeAfter.params) &&
                  !sameFloats(volumeBefore.params, volumeAfter.params),
              "numeric D edit refreshes dedicated volume parameters without shader regeneration");

        const auto refusedVolume = sdfwgsl::compileVolume(&unsupportedDensity);
        check(!refusedVolume.ok &&
                  refusedVolume.error.find("Raycast") != std::string::npos,
              "dedicated volume shader refuses unsupported density math with no stale fallback");

        auto noiseNode = std::make_shared<OntoMath::MathNode>();
        noiseNode->op = OntoMath::MathNode::Op::Noise;
        auto noisePoint = std::make_shared<OntoMath::MathNode>();
        noisePoint->op = OntoMath::MathNode::Op::ValueLeaf;
        noisePoint->variableName = OntoMath::kAmbientPointVar;
        noiseNode->children.push_back(std::make_unique<OntoMath::MathNode>(*noisePoint));
        OntoMath::Piecewise noiseDensity =
            OntoMath::Piecewise::continuous(noiseNode);
        const auto noiseVolume = sdfwgsl::compileVolume(&noiseDensity);
        check(noiseVolume.ok &&
                  noiseVolume.wgsl.find("cnoise3(") != std::string::npos &&
                  noiseVolume.wgsl.find("fn cnoise3(P: vec3<f32>) -> f32") != std::string::npos,
              "dedicated volume shader compiles noise expressions and defines cnoise3 in scope");
    }

    // ---------------------------------------------------------------------
    // V1. Extinction sovereignty: sigma_t(p,t) is independently authored.
    //     Absence preserves exact pre-V1 sigma_t = 0.5 * D compatibility.
    // ---------------------------------------------------------------------
    {
        auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));
        auto densityNode = std::shared_ptr<OntoMath::MathNode>(number(0.8).release());
        auto extinctionNode = std::shared_ptr<OntoMath::MathNode>(number(0.05).release());
        OntoMath::Piecewise density = OntoMath::Piecewise::continuous(densityNode);
        OntoMath::Piecewise extinction = OntoMath::Piecewise::continuous(extinctionNode);

        const std::string densityBeforeExtinctionEdit = density.toJson().dump();
        const auto compatibility = sdfwgsl::compileVolume(&density);
        check(compatibility.ok &&
                  compatibility.wgsl.find("return compatibilityDensity * 0.5") != std::string::npos,
              "V1 absence preserves the exact pre-V1 0.5*D extinction contract");

        const auto extinctionLayoutBefore =
            sdfwgsl::inspectExtinctionExpression(&extinction);
        const auto authored =
            sdfwgsl::compileVolume(&density, &extinction);
        const auto genericAuthored =
            sdfwgsl::compile(sphere, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                             &density, sdfwgsl::DensityInputKind::Authored, &extinction);
        check(extinctionLayoutBefore.ok && authored.ok && genericAuthored.ok,
              "authored sigma_t lowers through both volume renderer seams");
        check(authored.wgsl.find("fn volumeExtinctionEval") != std::string::npos &&
                  authored.wgsl.find("compatibilityDensity * 0.5") == std::string::npos &&
                  genericAuthored.wgsl.find("V1: explicit authored sigma_t(p,t)") != std::string::npos,
              "authored extinction replaces the fossil rather than multiplying or aliasing D");

        // VALUE ONLY: sigma_t changes while D remains byte-identical.
        extinctionNode->scalarForm.terms[0].coefficient = 3.0;
        const auto extinctionLayoutAfter =
            sdfwgsl::inspectExtinctionExpression(&extinction);
        const auto refreshed =
            sdfwgsl::collectVolumeParams(&density, &extinction);
        const auto valueEdited =
            sdfwgsl::compileVolume(&density, &extinction);
        check(extinctionLayoutAfter.ok &&
                  extinctionLayoutAfter.structure == extinctionLayoutBefore.structure &&
                  extinctionLayoutAfter.parameterCount == extinctionLayoutBefore.parameterCount,
              "numeric sigma_t edit preserves extinction structure and parameter layout");
        check(valueEdited.ok && authored.wgsl == valueEdited.wgsl,
              "numeric sigma_t edit leaves dedicated volume WGSL byte-identical");
        check(refreshed.ok && sameFloats(refreshed.values, valueEdited.params) &&
                  !sameFloats(authored.params, valueEdited.params),
              "numeric sigma_t edit refreshes only packed medium parameters");
        check(density.toJson().dump() == densityBeforeExtinctionEdit,
              "editing sigma_t leaves D byte-identical");

        // STRUCTURE: change sigma_t from ScalarLeaf to Add without touching D.
        auto extinctionAdd = std::make_shared<OntoMath::MathNode>();
        extinctionAdd->op = OntoMath::MathNode::Op::Add;
        extinctionAdd->children.push_back(number(1.5));
        extinctionAdd->children.push_back(number(1.5));
        extinction.pieces[0].mathNode = extinctionAdd;
        const auto extinctionStructuralLayout =
            sdfwgsl::inspectExtinctionExpression(&extinction);
        const auto structureEdited =
            sdfwgsl::compileVolume(&density, &extinction);
        check(extinctionStructuralLayout.ok &&
                  extinctionStructuralLayout.structure != extinctionLayoutAfter.structure &&
                  structureEdited.ok && structureEdited.wgsl != valueEdited.wgsl,
              "structural sigma_t edit advances only extinction shader structure");
        check(density.toJson().dump() == densityBeforeExtinctionEdit,
              "structural sigma_t edit still leaves D byte-identical");

        // TIME: sigma_t(p,t) shares the admitted medium coordinate, without
        // inventing an ExtinctionTimeline kind or mutating authored structure.
        auto extinctionTimeNode = std::make_shared<OntoMath::MathNode>();
        extinctionTimeNode->op = OntoMath::MathNode::Op::ValueLeaf;
        extinctionTimeNode->variableName = OntoMath::kTimeVar;
        OntoMath::Piecewise timedExtinction =
            OntoMath::Piecewise::continuous(extinctionTimeNode);
        const auto cpuTimedExtinction =
            timedExtinction.evaluate({{OntoMath::kTimeVar, PropertyValue(2.25)}});
        double cpuSigmaT = -1.0;
        check(cpuTimedExtinction &&
                  propertyValueToNumber(*cpuTimedExtinction, cpuSigmaT) &&
                  cpuSigmaT == 2.25,
              "sigma_t(p,t) remains ordinary CPU-evaluable OntoMath truth");

        const auto timedExtinctionLayout =
            sdfwgsl::inspectExtinctionExpression(&timedExtinction);
        const auto timedExtinctionProgram =
            sdfwgsl::compileVolume(&density, &timedExtinction);
        check(timedExtinctionLayout.ok && timedExtinctionProgram.ok &&
                  timedExtinctionProgram.wgsl.find("instances[g_instIdx].time.x") != std::string::npos,
              "the same sigma_t(p,t) lowers to WGSL with the admitted medium Timeline coordinate");

        auto raycast = std::make_shared<OntoMath::MathNode>();
        raycast->op = OntoMath::MathNode::Op::Raycast;
        OntoMath::Piecewise unsupportedExtinction =
            OntoMath::Piecewise::continuous(raycast);
        const auto refusedExtinction =
            sdfwgsl::inspectExtinctionExpression(&unsupportedExtinction);
        const auto refusedVolume =
            sdfwgsl::compileVolume(&density, &unsupportedExtinction);
        check(!refusedExtinction.ok && !refusedVolume.ok &&
                  refusedExtinction.error.find("Raycast") != std::string::npos &&
                  refusedVolume.error.find("Raycast") != std::string::npos,
              "unsupported authored extinction refuses instead of using stale or compatibility sigma_t");
    }

    // ---------------------------------------------------------------------
    // V2. Scattering/chroma sovereignty: sigma_s(p,t) and C_v(p,t) are
    //     independently authored while D and sigma_t remain unchanged.
    // ---------------------------------------------------------------------
    {
        auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));
        auto densityNode = std::shared_ptr<OntoMath::MathNode>(number(0.8).release());
        auto extinctionNode = std::shared_ptr<OntoMath::MathNode>(number(0.4).release());
        auto scatteringNode = std::shared_ptr<OntoMath::MathNode>(number(0.2).release());
        auto chromaNode = std::shared_ptr<OntoMath::MathNode>(vector3(1.0, 0.2, 0.1).release());
        OntoMath::Piecewise density = OntoMath::Piecewise::continuous(densityNode);
        OntoMath::Piecewise extinction = OntoMath::Piecewise::continuous(extinctionNode);
        OntoMath::Piecewise scattering = OntoMath::Piecewise::continuous(scatteringNode);
        OntoMath::Piecewise volumeChroma = OntoMath::Piecewise::continuous(chromaNode);

        const std::string densityTruth = density.toJson().dump();
        const std::string extinctionTruth = extinction.toJson().dump();

        const auto compatibility = sdfwgsl::compileVolume(&density, &extinction);
        check(compatibility.ok &&
                  compatibility.wgsl.find("return compatibilityDensity;") != std::string::npos &&
                  compatibility.wgsl.find("return vec3<f32>(1.0);") != std::string::npos,
              "V2 absence preserves exact sigma_s=D and neutral-white compatibility");

        const auto scatteringLayoutBefore =
            sdfwgsl::inspectScatteringExpression(&scattering);
        const auto chromaLayoutBefore =
            sdfwgsl::inspectVolumeChromaExpression(&volumeChroma);
        const auto authored =
            sdfwgsl::compileVolume(&density, &extinction, &scattering, &volumeChroma);
        const auto genericAuthored =
            sdfwgsl::compile(sphere, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                             &density, sdfwgsl::DensityInputKind::Authored, &extinction,
                             &scattering, &volumeChroma);
        check(scatteringLayoutBefore.ok && chromaLayoutBefore.ok &&
                  authored.ok && genericAuthored.ok,
              "authored sigma_s and C_v lower through both volume renderer seams");
        check(authored.wgsl.find("fn volumeScatteringEval") != std::string::npos &&
                  authored.wgsl.find("fn volumeChromaEval") != std::string::npos &&
                  authored.wgsl.find("var volumetricScatter = vec3<f32>(0.0)") != std::string::npos &&
                  genericAuthored.wgsl.find("var volumetric_scatter = vec3<f32>(0.0)") != std::string::npos,
              "V2 transport accumulates authored medium chroma as vector radiance");

        // VALUE ONLY: sigma_s changes while D and sigma_t remain byte-identical.
        scatteringNode->scalarForm.terms[0].coefficient = 0.65;
        const auto scatteringLayoutAfter =
            sdfwgsl::inspectScatteringExpression(&scattering);
        const auto scatteringRefreshed =
            sdfwgsl::collectVolumeParams(&density, &extinction, &scattering, &volumeChroma);
        const auto scatteringEdited =
            sdfwgsl::compileVolume(&density, &extinction, &scattering, &volumeChroma);
        check(scatteringLayoutAfter.ok &&
                  scatteringLayoutAfter.structure == scatteringLayoutBefore.structure &&
                  scatteringEdited.ok && scatteringEdited.wgsl == authored.wgsl &&
                  scatteringRefreshed.ok &&
                  sameFloats(scatteringRefreshed.values, scatteringEdited.params) &&
                  !sameFloats(authored.params, scatteringEdited.params),
              "numeric sigma_s edit refreshes parameters without shader regeneration");
        check(density.toJson().dump() == densityTruth &&
                  extinction.toJson().dump() == extinctionTruth,
              "editing sigma_s leaves D and sigma_t byte-identical");

        // VALUE ONLY: C_v changes independently from sigma_s.
        const std::string scatteringTruth = scattering.toJson().dump();
        chromaNode->children[1]->scalarForm.terms[0].coefficient = 0.9;
        const auto chromaLayoutAfter =
            sdfwgsl::inspectVolumeChromaExpression(&volumeChroma);
        const auto chromaRefreshed =
            sdfwgsl::collectVolumeParams(&density, &extinction, &scattering, &volumeChroma);
        const auto chromaEdited =
            sdfwgsl::compileVolume(&density, &extinction, &scattering, &volumeChroma);
        check(chromaLayoutAfter.ok &&
                  chromaLayoutAfter.structure == chromaLayoutBefore.structure &&
                  chromaEdited.ok && chromaEdited.wgsl == scatteringEdited.wgsl &&
                  chromaRefreshed.ok &&
                  sameFloats(chromaRefreshed.values, chromaEdited.params) &&
                  !sameFloats(scatteringEdited.params, chromaEdited.params),
              "numeric C_v edit refreshes parameters without shader regeneration");
        check(scattering.toJson().dump() == scatteringTruth,
              "editing C_v leaves sigma_s byte-identical");

        // STRUCTURE: sigma_s changes shape without changing D.
        auto scatteringAdd = std::make_shared<OntoMath::MathNode>();
        scatteringAdd->op = OntoMath::MathNode::Op::Add;
        scatteringAdd->children.push_back(number(0.2));
        scatteringAdd->children.push_back(number(0.45));
        scattering.pieces[0].mathNode = scatteringAdd;
        const auto scatteringStructuralLayout =
            sdfwgsl::inspectScatteringExpression(&scattering);
        const auto scatteringStructural =
            sdfwgsl::compileVolume(&density, &extinction, &scattering, &volumeChroma);
        check(scatteringStructuralLayout.ok &&
                  scatteringStructuralLayout.structure != scatteringLayoutAfter.structure &&
                  scatteringStructural.ok && scatteringStructural.wgsl != chromaEdited.wgsl,
              "structural sigma_s edit recompiles scattering structure independently");
        check(density.toJson().dump() == densityTruth,
              "structural sigma_s edit still leaves D byte-identical");

        // TIME: both V2 channels use the admitted medium Timeline coordinate.
        auto scatteringTimeNode = std::make_shared<OntoMath::MathNode>();
        scatteringTimeNode->op = OntoMath::MathNode::Op::ValueLeaf;
        scatteringTimeNode->variableName = OntoMath::kTimeVar;
        OntoMath::Piecewise timedScattering =
            OntoMath::Piecewise::continuous(scatteringTimeNode);
        auto timedChromaNode = std::make_shared<OntoMath::MathNode>();
        timedChromaNode->op = OntoMath::MathNode::Op::VectorConstruct;
        timedChromaNode->children.push_back(variable(OntoMath::kTimeVar));
        timedChromaNode->children.push_back(number(0.0));
        timedChromaNode->children.push_back(number(1.0));
        OntoMath::Piecewise timedChroma =
            OntoMath::Piecewise::continuous(timedChromaNode);
        const auto timedScatteringLayout =
            sdfwgsl::inspectScatteringExpression(&timedScattering);
        const auto timedChromaLayout =
            sdfwgsl::inspectVolumeChromaExpression(&timedChroma);
        const auto timedProgram =
            sdfwgsl::compileVolume(&density, &extinction, &timedScattering, &timedChroma);
        check(timedScatteringLayout.ok && timedChromaLayout.ok && timedProgram.ok &&
                  timedProgram.wgsl.find("instances[g_instIdx].time.x") != std::string::npos,
              "sigma_s(p,t) and C_v(p,t) share the admitted medium Timeline coordinate");

        // REFUSAL: authored unsupported/wrongly typed V2 channels may not fall
        // back to D or white.
        auto raycast = std::make_shared<OntoMath::MathNode>();
        raycast->op = OntoMath::MathNode::Op::Raycast;
        OntoMath::Piecewise unsupportedScattering =
            OntoMath::Piecewise::continuous(raycast);
        OntoMath::Piecewise scalarAsChroma =
            OntoMath::Piecewise::continuous(
                std::shared_ptr<OntoMath::MathNode>(number(1.0).release()));
        const auto refusedScattering =
            sdfwgsl::compileVolume(&density, &extinction, &unsupportedScattering, &volumeChroma);
        const auto refusedChroma =
            sdfwgsl::compileVolume(&density, &extinction, &scattering, &scalarAsChroma);
        check(!refusedScattering.ok &&
                  refusedScattering.error.find("Raycast") != std::string::npos,
              "unsupported sigma_s refuses instead of reverting to compatibility D");
        check(!refusedChroma.ok &&
                  refusedChroma.error.find("volume chroma") != std::string::npos,
              "invalid C_v refuses instead of reverting to compatibility white");
    }

    // ---------------------------------------------------------------------
    // V3. Phase sovereignty: Phi(p,wi,wo,t) is independent medium truth.
    //     Absent Phi preserves the literal V2 accumulation path.
    // ---------------------------------------------------------------------
    {
        auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));
        auto densityNode = std::shared_ptr<OntoMath::MathNode>(number(0.8).release());
        auto extinctionNode = std::shared_ptr<OntoMath::MathNode>(number(0.4).release());
        auto scatteringNode = std::shared_ptr<OntoMath::MathNode>(number(0.3).release());
        auto chromaNode = std::shared_ptr<OntoMath::MathNode>(vector3(0.2, 0.8, 1.0).release());
        OntoMath::Piecewise density = OntoMath::Piecewise::continuous(densityNode);
        OntoMath::Piecewise extinction = OntoMath::Piecewise::continuous(extinctionNode);
        OntoMath::Piecewise scattering = OntoMath::Piecewise::continuous(scatteringNode);
        OntoMath::Piecewise volumeChroma = OntoMath::Piecewise::continuous(chromaNode);

        const std::string densityTruth = density.toJson().dump();
        const std::string extinctionTruth = extinction.toJson().dump();
        const std::string scatteringTruth = scattering.toJson().dump();
        const std::string chromaTruth = volumeChroma.toJson().dump();

        const auto noPhase =
            sdfwgsl::compileVolume(&density, &extinction, &scattering, &volumeChroma);
        check(noPhase.ok &&
                  noPhase.wgsl.find("const HAS_AUTHORED_VOLUME_PHASE: bool = false") !=
                      std::string::npos &&
                  noPhase.wgsl.find(
                      "mediumChroma * (scattering / extinction) * (oldT - transmittance)") !=
                      std::string::npos,
              "absent Phi keeps the literal V2 scattering accumulation path");

        // Phi = 1 + g * wi.z. Scalar-by-scalar multiplication belongs inside
        // ScalarForm's exact algebra; MathNode::Scale is vector/scalar only and
        // there is deliberately no separate MathNode::Mul vocabulary.
        auto weightedWi = std::make_unique<OntoMath::MathNode>();
        weightedWi->op = OntoMath::MathNode::Op::ScalarLeaf;
        weightedWi->scalarForm =
            OntoMath::ScalarForm::variable(OntoMath::kWiZVar, 1.0, 0.35);
        OntoMath::MathNode* gainPtr = weightedWi.get();
        auto phaseRoot = std::make_shared<OntoMath::MathNode>();
        phaseRoot->op = OntoMath::MathNode::Op::Add;
        phaseRoot->children.push_back(number(1.0));
        phaseRoot->children.push_back(std::move(weightedWi));
        OntoMath::Piecewise phase = OntoMath::Piecewise::continuous(phaseRoot);

        const auto phaseLayoutBefore = sdfwgsl::inspectPhaseExpression(&phase);
        const auto authored =
            sdfwgsl::compileVolume(
                &density, &extinction, &scattering, &volumeChroma, &phase);
        const auto genericAuthored =
            sdfwgsl::compile(
                sphere, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                &density, sdfwgsl::DensityInputKind::Authored, &extinction,
                &scattering, &volumeChroma, &phase);
        check(phaseLayoutBefore.ok && phaseLayoutBefore.readsWi &&
                  !phaseLayoutBefore.readsWo && authored.ok && genericAuthored.ok &&
                  authored.wgsl.find("fn volumePhaseEval") != std::string::npos &&
                  authored.wgsl.find("const VOLUME_PHASE_READS_WI: bool = true") !=
                      std::string::npos,
              "authored Phi lowers independently through both volume renderer seams");

        const auto cpuPhase = phase.evaluate({
            {OntoMath::kWiXVar, PropertyValue(0.0)},
            {OntoMath::kWiYVar, PropertyValue(0.0)},
            {OntoMath::kWiZVar, PropertyValue(1.0)},
            {OntoMath::kWoXVar, PropertyValue(0.0)},
            {OntoMath::kWoYVar, PropertyValue(0.0)},
            {OntoMath::kWoZVar, PropertyValue(-1.0)},
            {OntoMath::kTimeVar, PropertyValue(2.0)}
        });
        double cpuPhi = -1.0;
        check(cpuPhase && propertyValueToNumber(*cpuPhase, cpuPhi) &&
                  std::abs(cpuPhi - 1.35) < 1e-9,
              "Phi(p,wi,wo,t) remains ordinary CPU-evaluable OntoMath truth");

        // VALUE ONLY: change g, preserving exact AST shape and sibling channels.
        gainPtr->scalarForm.terms[0].coefficient = 0.8;
        const auto phaseLayoutAfter = sdfwgsl::inspectPhaseExpression(&phase);
        const auto refreshed =
            sdfwgsl::collectVolumeParams(
                &density, &extinction, &scattering, &volumeChroma, &phase);
        const auto valueEdited =
            sdfwgsl::compileVolume(
                &density, &extinction, &scattering, &volumeChroma, &phase);
        check(phaseLayoutAfter.ok &&
                  phaseLayoutAfter.structure == phaseLayoutBefore.structure &&
                  valueEdited.ok && valueEdited.wgsl == authored.wgsl &&
                  refreshed.ok &&
                  sameFloats(refreshed.values, valueEdited.params) &&
                  !sameFloats(authored.params, valueEdited.params),
              "numeric Phi edit refreshes parameters without WGSL regeneration");
        check(density.toJson().dump() == densityTruth &&
                  extinction.toJson().dump() == extinctionTruth &&
                  scattering.toJson().dump() == scatteringTruth &&
                  volumeChroma.toJson().dump() == chromaTruth,
              "rewriting Phi leaves D, sigma_t, sigma_s and C_v byte-identical");

        // STRUCTURE: change only the directional dependency wi.z -> wo.z.
        auto directionalStructure = std::make_shared<OntoMath::MathNode>();
        directionalStructure->op = OntoMath::MathNode::Op::Add;
        directionalStructure->children.push_back(number(1.0));
        directionalStructure->children.push_back(variable(OntoMath::kWoZVar));
        phase.pieces[0].mathNode = directionalStructure;
        const auto phaseStructuralLayout = sdfwgsl::inspectPhaseExpression(&phase);
        const auto structureEdited =
            sdfwgsl::compileVolume(
                &density, &extinction, &scattering, &volumeChroma, &phase);
        check(phaseStructuralLayout.ok && !phaseStructuralLayout.readsWi &&
                  phaseStructuralLayout.readsWo &&
                  phaseStructuralLayout.structure != phaseLayoutAfter.structure &&
                  structureEdited.ok && structureEdited.wgsl != valueEdited.wgsl,
              "structural Phi edit recompiles only the phase shader structure");

        // TIME: Phi may consume the admitted medium Timeline without AST rewrite.
        auto timedPhaseRoot = std::make_shared<OntoMath::MathNode>();
        timedPhaseRoot->op = OntoMath::MathNode::Op::Add;
        timedPhaseRoot->children.push_back(number(0.25));
        timedPhaseRoot->children.push_back(variable(OntoMath::kTimeVar));
        OntoMath::Piecewise timedPhase =
            OntoMath::Piecewise::continuous(timedPhaseRoot);
        const auto timedPhaseLayout = sdfwgsl::inspectPhaseExpression(&timedPhase);
        const auto timedPhaseProgram =
            sdfwgsl::compileVolume(
                &density, &extinction, &scattering, &volumeChroma, &timedPhase);
        const auto cpuTimedPhase =
            timedPhase.evaluate({{OntoMath::kTimeVar, PropertyValue(0.75)}});
        double cpuTimedPhi = -1.0;
        check(timedPhaseLayout.ok && timedPhaseProgram.ok &&
                  timedPhaseProgram.wgsl.find("instances[g_instIdx].time.x") !=
                      std::string::npos &&
                  cpuTimedPhase &&
                  propertyValueToNumber(*cpuTimedPhase, cpuTimedPhi) &&
                  std::abs(cpuTimedPhi - 1.0) < 1e-9,
              "Phi timeline input lowers to WGSL and evaluates on CPU without structural mutation");

        // CONTEXT REFUSAL: wi is admitted only by phase, never ordinary scalar fields.
        OntoMath::Piecewise wiOnly =
            OntoMath::Piecewise::continuous(
                std::shared_ptr<OntoMath::MathNode>(
                    variable(OntoMath::kWiXVar).release()));
        const auto wiOutsidePhase = sdfwgsl::inspectScalarExpression(&wiOnly, true);
        check(!wiOutsidePhase.ok &&
                  wiOutsidePhase.error.find("wi") != std::string::npos,
              "wi cannot leak from V3 phase into ordinary field expression contexts");

        // AUTHORED REFUSAL: unsupported phase is named and cannot fall back to Phi=1.
        auto raycast = std::make_shared<OntoMath::MathNode>();
        raycast->op = OntoMath::MathNode::Op::Raycast;
        OntoMath::Piecewise unsupportedPhase =
            OntoMath::Piecewise::continuous(raycast);
        const auto refusedPhaseLayout =
            sdfwgsl::inspectPhaseExpression(&unsupportedPhase);
        const auto refusedPhaseProgram =
            sdfwgsl::compileVolume(
                &density, &extinction, &scattering, &volumeChroma, &unsupportedPhase);
        check(!refusedPhaseLayout.ok && !refusedPhaseProgram.ok &&
                  refusedPhaseLayout.error.find("Raycast") != std::string::npos &&
                  refusedPhaseProgram.error.find("volume phase") != std::string::npos,
              "unsupported authored Phi refuses instead of reverting to isotropic identity");
    }

    // ---------------------------------------------------------------------
    // V4. Emission sovereignty: E_v(p,omega,t) is independent medium truth.
    //     It emits vec3 radiance even when no external source illuminates it.
    // ---------------------------------------------------------------------
    {
        auto densityNode = std::shared_ptr<OntoMath::MathNode>(number(0.8).release());
        auto extinctionNode = std::shared_ptr<OntoMath::MathNode>(number(0.4).release());
        auto scatteringNode = std::shared_ptr<OntoMath::MathNode>(number(0.3).release());
        auto chromaNode = std::shared_ptr<OntoMath::MathNode>(vector3(0.2, 0.8, 1.0).release());
        auto emissionNode = std::shared_ptr<OntoMath::MathNode>(vector3(0.1, 0.5, 1.0).release());
        OntoMath::Piecewise density = OntoMath::Piecewise::continuous(densityNode);
        OntoMath::Piecewise extinction = OntoMath::Piecewise::continuous(extinctionNode);
        OntoMath::Piecewise scattering = OntoMath::Piecewise::continuous(scatteringNode);
        OntoMath::Piecewise volumeChroma = OntoMath::Piecewise::continuous(chromaNode);
        OntoMath::Piecewise emission = OntoMath::Piecewise::continuous(emissionNode);

        const std::string densityTruth = density.toJson().dump();
        const std::string extinctionTruth = extinction.toJson().dump();
        const std::string scatteringTruth = scattering.toJson().dump();
        const std::string chromaTruth = volumeChroma.toJson().dump();

        const auto emissionLayoutBefore = sdfwgsl::inspectEmissionExpression(&emission);
        const auto authored = sdfwgsl::compileVolume(
            &density, &extinction, &scattering, &volumeChroma, nullptr, &emission);
        check(emissionLayoutBefore.ok && authored.ok &&
                  authored.wgsl.find("fn volumeEmissionEval") != std::string::npos &&
                  authored.wgsl.find("HAS_AUTHORED_VOLUME_EMISSION") != std::string::npos,
              "authored E_v lowers through the dedicated volume renderer");

        // SELF-EMISSION: this compiled transport has no source-radiance input at all.
        // The emitted term must enter the medium accumulation independently.
        check(authored.wgsl.find("volumeEmissionEval") != std::string::npos &&
                  authored.wgsl.find("emission") != std::string::npos,
              "E_v remains present in transport without any external illumination channel");

        // VALUE ONLY: change one E_v coefficient; siblings remain byte-identical.
        emissionNode->children[1]->scalarForm.terms[0].coefficient = 0.9;
        const auto emissionLayoutAfter = sdfwgsl::inspectEmissionExpression(&emission);
        const auto refreshed = sdfwgsl::collectVolumeParams(
            &density, &extinction, &scattering, &volumeChroma, nullptr, &emission);
        const auto valueEdited = sdfwgsl::compileVolume(
            &density, &extinction, &scattering, &volumeChroma, nullptr, &emission);
        check(emissionLayoutAfter.ok &&
                  emissionLayoutAfter.structure == emissionLayoutBefore.structure &&
                  valueEdited.ok && valueEdited.wgsl == authored.wgsl &&
                  refreshed.ok && sameFloats(refreshed.values, valueEdited.params) &&
                  !sameFloats(authored.params, valueEdited.params),
              "numeric E_v edit refreshes parameters without WGSL regeneration");
        check(density.toJson().dump() == densityTruth &&
                  extinction.toJson().dump() == extinctionTruth &&
                  scattering.toJson().dump() == scatteringTruth &&
                  volumeChroma.toJson().dump() == chromaTruth,
              "rewriting E_v leaves D, sigma_t, sigma_s and C_v byte-identical");

        // STRUCTURE + DIRECTION: omega belongs to medium emission and means sample -> eye.
        auto directionalEmission = std::make_shared<OntoMath::MathNode>();
        directionalEmission->op = OntoMath::MathNode::Op::VectorConstruct;
        directionalEmission->children.push_back(variable(OntoMath::kOmegaXVar));
        directionalEmission->children.push_back(number(0.25));
        directionalEmission->children.push_back(number(0.75));
        emission.pieces[0].mathNode = directionalEmission;
        const auto directionalLayout = sdfwgsl::inspectEmissionExpression(&emission);
        const auto structureEdited = sdfwgsl::compileVolume(
            &density, &extinction, &scattering, &volumeChroma, nullptr, &emission);
        check(directionalLayout.ok && directionalLayout.readsOmega &&
                  directionalLayout.structure != emissionLayoutAfter.structure &&
                  structureEdited.ok && structureEdited.wgsl != valueEdited.wgsl,
              "structural/directional E_v edit changes emission program identity");

        // TIME: E_v consumes the medium Timeline without AST rewrite.
        auto timedEmissionNode = std::make_shared<OntoMath::MathNode>();
        timedEmissionNode->op = OntoMath::MathNode::Op::VectorConstruct;
        timedEmissionNode->children.push_back(variable(OntoMath::kTimeVar));
        timedEmissionNode->children.push_back(number(0.0));
        timedEmissionNode->children.push_back(number(1.0));
        OntoMath::Piecewise timedEmission = OntoMath::Piecewise::continuous(timedEmissionNode);
        const auto timedLayout = sdfwgsl::inspectEmissionExpression(&timedEmission);
        const auto timedProgram = sdfwgsl::compileVolume(
            &density, &extinction, &scattering, &volumeChroma, nullptr, &timedEmission);
        check(timedLayout.ok && timedProgram.ok &&
                  timedProgram.wgsl.find("instances[g_instIdx].time.x") != std::string::npos,
              "E_v(p,omega,t) lowers the admitted medium Timeline independently");

        // REFUSAL: unsupported authored emission cannot silently become black/absent.
        auto raycast = std::make_shared<OntoMath::MathNode>();
        raycast->op = OntoMath::MathNode::Op::Raycast;
        OntoMath::Piecewise unsupportedEmission = OntoMath::Piecewise::continuous(raycast);
        const auto refusedLayout = sdfwgsl::inspectEmissionExpression(&unsupportedEmission);
        const auto refusedProgram = sdfwgsl::compileVolume(
            &density, &extinction, &scattering, &volumeChroma, nullptr, &unsupportedEmission);
        check(!refusedLayout.ok && !refusedProgram.ok &&
                  refusedProgram.error.find("volume emission") != std::string::npos,
              "unsupported authored E_v refuses instead of falling back to absent emission");
    }

    // 9. Rung 8: visibility is derived transport below source authorship.
    //    The shader must expose an exact V=1 compatibility gate and multiply
    //    each source's direct radiance AFTER rho*chi*alpha composition.
    // ---------------------------------------------------------------------
    {
        auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));
        auto rho0Node = std::shared_ptr<OntoMath::MathNode>(number(1.0).release());
        auto rho1Node = std::shared_ptr<OntoMath::MathNode>(number(1.0).release());
        OntoMath::Piecewise rho0 = OntoMath::Piecewise::continuous(rho0Node);
        OntoMath::Piecewise rho1 = OntoMath::Piecewise::continuous(rho1Node);

        const auto oneSource = sdfwgsl::compile(sphere, nullptr, nullptr, &rho0);
        check(oneSource.ok &&
                  oneSource.wgsl.find("fn sourceVisibility") != std::string::npos,
              "Rung-8 shader exposes derived geometric visibility transport");
        check(oneSource.wgsl.find("u.lightControl.y < 0.5") != std::string::npos &&
                  oneSource.wgsl.find("return 1.0") != std::string::npos,
              "visibility-disabled compatibility path is explicit V=1");
        check(oneSource.wgsl.find(
                  "let directRadiance = shapedRadiance * pathVisibility") != std::string::npos,
              "one-source direct transport multiplies visibility after authored emission");

        Rendering::RadianceSourceBinding s0;
        s0.radianceExpr = &rho0;
        Rendering::RadianceSourceBinding s1;
        s1.radianceExpr = &rho1;
        std::vector<Rendering::RadianceSourceBinding> sources{s0, s1};
        const auto multiSource =
            sdfwgsl::compile(sphere, nullptr, nullptr, nullptr, nullptr, nullptr, &sources);
        check(multiSource.ok &&
                  multiSource.wgsl.find(
                      "sourceVisibility(pf, nf, source.position.xyz)") != std::string::npos,
              "each composed source derives visibility from its own source-receiver path");
        check(multiSource.wgsl.find(
                  "diff * directRadiance") != std::string::npos &&
                  multiSource.wgsl.find(
                      "specShape * directRadiance") != std::string::npos,
              "multi-source diffuse/specular transport consumes per-source visibility");
        check(multiSource.wgsl.find(
                  "ambientTerm += inst.shading.x") != std::string::npos,
              "legacy ambient compatibility remains outside direct-path visibility");
    }

    // Prism integration: V_transport and D_medium coexist without semantic aliasing.
    // The combined shader may contain both sourceVisibility() and volumeDensityEval(),
    // but visibility is a geometry query; participating-medium D is not promoted
    // into a binary opaque blocker merely because both are transport phenomena.
    {
        auto bridgeSphere =
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));
        auto bridgeRhoNode =
            std::shared_ptr<OntoMath::MathNode>(number(1.0).release());
        auto bridgeDensityNode =
            std::shared_ptr<OntoMath::MathNode>(number(0.3).release());
        OntoMath::Piecewise bridgeRho =
            OntoMath::Piecewise::continuous(bridgeRhoNode);
        OntoMath::Piecewise bridgeDensity =
            OntoMath::Piecewise::continuous(bridgeDensityNode);

        const auto bridge = sdfwgsl::compile(
            bridgeSphere, nullptr, nullptr, &bridgeRho, nullptr, nullptr, nullptr,
            &bridgeDensity, sdfwgsl::DensityInputKind::Authored);
        check(bridge.ok &&
                  bridge.wgsl.find("fn sourceVisibility") != std::string::npos &&
                  bridge.wgsl.find("fn volumeDensityEval") != std::string::npos &&
                  bridge.wgsl.find(
                      "let directRadiance = shapedRadiance * pathVisibility") != std::string::npos,
              "combined V+D shader preserves distinct visibility and density channels");

        const auto visibilityStart = bridge.wgsl.find("fn sourceVisibility");
        const auto visibilityEnd =
            visibilityStart == std::string::npos
                ? std::string::npos
                : bridge.wgsl.find("@fragment", visibilityStart);
        const bool visibilityIsGeometryOnly =
            visibilityStart != std::string::npos &&
            visibilityEnd != std::string::npos &&
            bridge.wgsl.substr(visibilityStart, visibilityEnd - visibilityStart)
                    .find("volumeDensityEval") == std::string::npos;
        check(visibilityIsGeometryOnly,
              "Rung-8 V does not reinterpret participating-medium D as opaque geometry");
    }

    // ---------------------------------------------------------------------
    // 10. Rendering relevance economics: the benchmark-only NO-PROOF topology
    //     must be a structural subtraction, never a semantic rewrite.
    //
    //     This CPU witness runs before the expensive native performance job:
    //     - default compile() must remain exactly proof-capable;
    //     - NO-PROOF must preserve the authored parameter block and storage ABI;
    //     - executable rangeCandidate/traversal code must actually be absent;
    //     - the same subtraction must hold in the multi-source marcher branch.
    // ---------------------------------------------------------------------
    {
        geom::SdfNode terrain = geom::makeImplicit(terrainMath(40.0));

        const auto production = sdfwgsl::compile(terrain);
        sdfwgsl::CompileOptions explicitDefaultOptions;
        explicitDefaultOptions.emitRangeTraversal = true;
        const auto explicitDefault =
            sdfwgsl::compileWithOptions(terrain, explicitDefaultOptions);

        sdfwgsl::CompileOptions noProofOptions;
        noProofOptions.emitRangeTraversal = false;
        const auto noProof =
            sdfwgsl::compileWithOptions(terrain, noProofOptions);

        check(production.ok && explicitDefault.ok && noProof.ok,
              "proof-capable and NO-PROOF compiler topologies both compile");
        check(production.wgsl == explicitDefault.wgsl &&
                  sameFloats(production.params, explicitDefault.params),
              "default compile() remains byte-identical to explicit proof-capable topology");
        check(sameFloats(production.params, noProof.params),
              "NO-PROOF preserves the exact authored parameter block");
        check(noProof.wgsl.find("reserved0: u32") != std::string::npos &&
                  noProof.wgsl.find("reserved3: u32") != std::string::npos,
              "NO-PROOF preserves SdfInstanceData byte stride with neutral reserved slots");
        check(production.wgsl.find("fn rangeCandidate(") != std::string::npos &&
                  production.wgsl.find("inst.rangeTraversalEnabled != 0u") != std::string::npos &&
                  production.wgsl.find("rangeProofWords") != std::string::npos,
              "proof-capable topology contains executable proof state and storage");
        check(noProof.wgsl.find("fn rangeCandidate(") == std::string::npos &&
                  noProof.wgsl.find("rangeProof") == std::string::npos &&
                  noProof.wgsl.find("rangeTraversal") == std::string::npos &&
                  noProof.wgsl.find("@group(1) @binding(2)") == std::string::npos,
              "NO-PROOF structurally removes proof functions, fields, branch, and storage binding");

        auto rho0Node =
            std::shared_ptr<OntoMath::MathNode>(number(1.0).release());
        auto rho1Node =
            std::shared_ptr<OntoMath::MathNode>(number(0.8).release());
        OntoMath::Piecewise rho0 =
            OntoMath::Piecewise::continuous(rho0Node);
        OntoMath::Piecewise rho1 =
            OntoMath::Piecewise::continuous(rho1Node);
        Rendering::RadianceSourceBinding source0;
        source0.radianceExpr = &rho0;
        Rendering::RadianceSourceBinding source1;
        source1.radianceExpr = &rho1;
        std::vector<Rendering::RadianceSourceBinding> sources{
            source0, source1};

        const auto multiProof = sdfwgsl::compile(
            terrain, nullptr, nullptr, nullptr, nullptr, nullptr, &sources);
        const auto multiNoProof = sdfwgsl::compileWithOptions(
            terrain, noProofOptions, nullptr, nullptr, nullptr, nullptr,
            nullptr, &sources);

        check(multiProof.ok && multiNoProof.ok,
              "multi-source proof-capable and NO-PROOF topologies both compile");
        check(sameFloats(multiProof.params, multiNoProof.params),
              "multi-source NO-PROOF preserves the exact parameter block");
        check(multiProof.wgsl.find("fn rangeCandidate(") != std::string::npos &&
                  multiNoProof.wgsl.find("fn rangeCandidate(") == std::string::npos &&
                  multiNoProof.wgsl.find("rangeProof") == std::string::npos &&
                  multiNoProof.wgsl.find("rangeTraversal") == std::string::npos &&
                  multiNoProof.wgsl.find("@group(1) @binding(2)") == std::string::npos,
              "multi-source NO-PROOF removes the same proof WGSL surface");
    }

    if (failures) {
        std::printf("sdf_wgsl_parameter_refresh_test: %d failure(s)\n", failures);
        return 1;
    }
    std::printf("sdf_wgsl_parameter_refresh_test: PASS\n");
    return 0;
}
