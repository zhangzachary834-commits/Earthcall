// Test: the geometry <-> OntoMath seam.
//
// Guards the defects found auditing d4514815 ("Integrate OntoMath into geometry
// & SDF"). Every one of them was a SILENT wrong answer rather than a crash, and
// the suite was green for four of them, so each check below names the behaviour
// it protects rather than just the function.

#include "ConstructedBeing/Object/Geometry/Patch.hpp"
#include "ConstructedBeing/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Object/Geometry/SdfJson.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <cmath>
#include <cstdio>
#include <string>

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) { ++g_failures; std::printf("  FAILED: %s\n", what.c_str()); return; }
    std::printf("  ok: %s\n", what.c_str());
}

bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

double evalNum(const OntoMath::MathNode& n, const std::map<std::string, PropertyValue>& vars,
               bool& ok) {
    auto v = n.evaluate(vars);
    double d = 0.0;
    ok = v && propertyValueToNumber(*v, d);
    return d;
}

} // namespace

int main() {
    std::printf("Running geometry <-> OntoMath seam test...\n");

    // -----------------------------------------------------------------------
    // 1. ONE scalar type. Op::Length/Dot/Distance returned glm's float while
    //    ScalarLeaf returns double, and Add/Sub strict-matched double on both
    //    operands -- so Sub(Length(p), r) was nullopt for every input, which is
    //    what made every MathNode::sphere undefined.
    // -----------------------------------------------------------------------
    {
        std::map<std::string, PropertyValue> vars;
        vars["p"] = PropertyValue(glm::vec3(2.0f, 0.0f, 0.0f));
        auto sphere = OntoMath::MathNode::sphere(2.0, "p");
        bool ok = false;
        const double at2 = evalNum(*sphere, vars, ok);
        check(ok && std::fabs(at2) < 1e-9, "sphere(2) evaluates to 0 on its own surface");

        vars["p"] = PropertyValue(glm::vec3(5.0f, 0.0f, 0.0f));
        const double at5 = evalNum(*sphere, vars, ok);
        check(ok && std::fabs(at5 - 3.0) < 1e-9, "sphere(2) evaluates to 3 at distance 5");

        // A float arriving from anywhere -- every PropertyRef<T, float> on a
        // being -- must not poison ordinary arithmetic.
        auto add = std::make_unique<OntoMath::MathNode>();
        add->op = OntoMath::MathNode::Op::Add;
        auto lhs = std::make_unique<OntoMath::MathNode>();
        lhs->op = OntoMath::MathNode::Op::ValueLeaf;
        lhs->variableName = "f";
        auto rhs = std::make_unique<OntoMath::MathNode>();
        rhs->op = OntoMath::MathNode::Op::ScalarLeaf;
        rhs->scalarForm = OntoMath::ScalarForm::constant(1.0);
        add->children.push_back(std::move(lhs));
        add->children.push_back(std::move(rhs));
        std::map<std::string, PropertyValue> fv;
        fv["f"] = PropertyValue(2.0f);          // the float alternative, deliberately
        const double sum = evalNum(*add, fv, ok);
        check(ok && std::fabs(sum - 3.0) < 1e-9, "Add coerces a float operand against a double");
    }

    // -----------------------------------------------------------------------
    // 2. The RPN lift REFUSES what it cannot say, and evalSdf stays correct.
    //    evalSdf and the WGSL emitter both prefer mathNode when it exists, so
    //    an approximate lift is a wrong shape that webgpu_sdf_parity_test
    //    cannot catch -- it reads the same tree on both sides.
    // -----------------------------------------------------------------------
    {
        struct Case { const char* expr; bool liftable; glm::vec3 at; float expect; };
        const Case cases[] = {
            {"x + y",                      true,  {0.3f, 0.4f, 0.0f}, 0.7f},
            {"x * y",                      true,  {0.3f, 0.4f, 0.0f}, 0.12f},
            {"x^3",                        true,  {0.3f, 0.4f, 0.0f}, 0.027f},
            {"sin(x)",                     true,  {0.3f, 0.4f, 0.0f}, std::sin(0.3f)},
            {"x/2",                        false, {0.3f, 0.4f, 0.0f}, 0.15f},
            {"sqrt(x*x + y*y + z*z) - 0.55", false, {0.3f, 0.4f, 0.0f}, -0.05f},
            {"sin(x*2)",                   false, {0.3f, 0.4f, 0.0f}, std::sin(0.6f)},
            {"tan(x)",                     false, {0.3f, 0.4f, 0.0f}, std::tan(0.3f)},
            {"abs(x)",                     false, {-0.3f, 0.0f, 0.0f}, 0.3f},
        };
        for (const auto& c : cases) {
            geom::SdfNode n = geom::makeImplicit(c.expr);
            check((n.mathNode != nullptr) == c.liftable,
                  std::string("\"") + c.expr + "\" " +
                      (c.liftable ? "lifts to an OntoMath AST" : "REFUSES to lift (no faithful op)"));
            check(nearf(geom::evalSdf(n, c.at), c.expect),
                  std::string("\"") + c.expr + "\" still evaluates correctly");
        }
    }

    // -----------------------------------------------------------------------
    // 3. Unimplemented primitives refuse instead of returning a sphere.
    //    box discarded two half-extents, cylinder its height, torus its minor
    //    radius, smoothUnion its blend -- all four silently WERE spheres.
    // -----------------------------------------------------------------------
    {
        check(OntoMath::MathNode::box(glm::vec3(0.5f)) == nullptr,
              "MathNode::box refuses rather than returning a sphere");
        check(OntoMath::MathNode::cylinder(0.5, 2.0) == nullptr,
              "MathNode::cylinder refuses rather than discarding its height");
        check(OntoMath::MathNode::torus(0.5, 0.1) == nullptr,
              "MathNode::torus refuses rather than discarding its minor radius");
        check(OntoMath::MathNode::smoothUnionOp(OntoMath::MathNode::sphere(1.0),
                                                OntoMath::MathNode::sphere(2.0), 0.3) == nullptr,
              "MathNode::smoothUnionOp refuses rather than dropping its blend");
        check(OntoMath::MathNode::sphere(1.0) != nullptr, "sphere is genuinely implemented");
    }

    // -----------------------------------------------------------------------
    // 4. SdfNode::toMathNode refuses kinds it cannot express. It used to
    //    return sphere(0.5) for all of them.
    // -----------------------------------------------------------------------
    {
        geom::SdfNode cone  = geom::SdfNode::leaf(geom::SdfPrim::Cone, glm::vec3(0.25f));
        geom::SdfNode conv  = geom::SdfNode::convex({glm::vec4(1, 0, 0, 0.2f)});
        geom::SdfNode morph = geom::SdfNode::binary(geom::SdfOp::Morph, cone, conv, 0.5f);
        geom::SdfNode box   = geom::SdfNode::leaf(geom::SdfPrim::Box, glm::vec3(0.5f));
        check(cone.toMathNode()  == nullptr, "toMathNode refuses a Cone");
        check(conv.toMathNode()  == nullptr, "toMathNode refuses a Convex polyhedron");
        check(morph.toMathNode() == nullptr, "toMathNode refuses a Morph");
        check(box.toMathNode()   == nullptr, "toMathNode refuses a Box while box() is unimplemented");
        geom::SdfNode sph = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(0.5f));
        check(sph.toMathNode() != nullptr, "toMathNode lifts a Sphere");
    }

    // -----------------------------------------------------------------------
    // 5. Both OntoMath arms of SdfNode survive a save/load round trip.
    //    piecewise was constructible but never serialized: the shape came back
    //    as empty space (evalRpn on an empty rpn returns 1e9).
    // -----------------------------------------------------------------------
    {
        geom::SdfNode withMath = geom::makeImplicit("x + y");
        geom::SdfNode backM = geom::sdfFromJson(geom::sdfToJson(withMath));
        check(backM.mathNode != nullptr, "mathNode survives a JSON round trip");

        auto pw = std::make_shared<OntoMath::Piecewise>(OntoMath::Piecewise::continuous(
            OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::variable("x"))));
        geom::SdfNode withPw = geom::makeImplicit(pw);
        const float before = geom::evalSdf(withPw, glm::vec3(0.4f, 0.0f, 0.0f));
        geom::SdfNode backP = geom::sdfFromJson(geom::sdfToJson(withPw));
        check(backP.piecewise != nullptr, "piecewise survives a JSON round trip");
        check(nearf(before, 0.4f), "the piecewise shape evaluates before saving");
        check(nearf(geom::evalSdf(backP, glm::vec3(0.4f, 0.0f, 0.0f)), before),
              "the piecewise shape evaluates the same after loading");
    }

    // -----------------------------------------------------------------------
    // 6. Hoisting the patch derivatives out of the per-vertex loop did not
    //    change the normals it produces (1530 ms -> 7.2 ms for one 24x24 patch).
    // -----------------------------------------------------------------------
    {
        geom::BezierPatch p = geom::makeBezierGrid(3, 3, 0.5f);
        const geom::PatchDerivatives d = geom::patchDerivatives(p);
        bool allMatch = true;
        for (float u = 0.0f; u <= 1.0f; u += 0.25f) {
            for (float v = 0.0f; v <= 1.0f; v += 0.25f) {
                const glm::vec3 hoisted = geom::bezierNormal(p, d, u, v);
                const glm::vec3 direct  = geom::bezierNormal(p, u, v);
                if (!nearf(hoisted.x, direct.x) || !nearf(hoisted.y, direct.y) ||
                    !nearf(hoisted.z, direct.z)) {
                    allMatch = false;
                }
            }
        }
        check(allMatch, "hoisted and per-sample symbolic normals agree across the patch");
        const glm::vec3 n = geom::bezierNormal(p, d, 0.5f, 0.5f);
        check(nearf(glm::length(n), 1.0f), "the symbolic normal is a unit vector");
    }

    std::printf(g_failures == 0 ? "geometry_ontomath_test: ALL OK\n"
                                : "geometry_ontomath_test: FAILURES\n");
    return g_failures > 0 ? 1 : 0;
}
