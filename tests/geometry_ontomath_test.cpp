// Test: the geometry <-> OntoMath seam.
//
// Guards the defects found auditing d4514815 ("Integrate OntoMath into geometry
// & SDF"). Every one of them was a SILENT wrong answer rather than a crash, and
// the suite was green for four of them, so each check below names the behaviour
// it protects rather than just the function.

#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Object/Geometry/SmoothSurface.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
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

    // -----------------------------------------------------------------------
    // 7. RUNG 5 -- the sculpted geometry is reachable and GOVERNABLE by law.
    //    shape.* already exposed the parametric radii; the SDF tree behind a
    //    Field and the control net behind a Patch reached no law at all, which
    //    is the gap SDF_BEZIER_SHAPE_GENERATOR_LAW_REPLICATION.md names.
    // -----------------------------------------------------------------------
    {
        Object o("geometry-governance-probe");

        const auto get = [&o](const char* path, PropertyValue& out) {
            return PropertyPath::parse(path).getValue(o, out) == PropertyPath::PathResult::Ok;
        };
        const auto set = [&o](const char* path, const PropertyValue& v) {
            return PropertyPath::parse(path).setValue(o, v) == PropertyPath::PathResult::Ok;
        };

        // Every path resolves on a bare object -- the picker probes a prototype
        // to decide a path's type, so "reads as nothing" is not an option.
        const char* paths[] = {"field.extent", "field.op", "field.prim", "field.dims",
                               "field.offset", "field.p0", "field.p1", "field.blend",
                               "field.expr", "patch.degreeU", "patch.degreeV",
                               "patch.controlCount", "patch.ctrl.0", "patch.ctrl.15"};
        bool allResolve = true;
        for (const char* path : paths) {
            PropertyValue v;
            if (!get(path, v)) { allResolve = false; std::printf("    (%s did not resolve)\n", path); }
        }
        check(allResolve, "every field.* and patch.* path resolves on a bare Object");

        // Authoring an implicit surface AS LAW TEXT: writing the expression
        // reshapes the being.
        check(set("field.expr", PropertyValue(std::string("x*x + y*y + z*z - 0.25"))),
              "field.expr accepts an implicit expression");
        check(o.hasField(), "writing field.expr gives the object a Field shape");
        check(nearf(geom::evalSdf(o.getFieldData(), glm::vec3(0.5f, 0.0f, 0.0f)), 0.0f),
              "the authored field evaluates as its expression says");
        check(!set("field.expr", PropertyValue(std::string("x +* )"))),
              "an unparseable expression is REFUSED, leaving the old shape standing");
        check(nearf(geom::evalSdf(o.getFieldData(), glm::vec3(0.5f, 0.0f, 0.0f)), 0.0f),
              "the shape survived the refused write");

        // A law growing a radius, and breathing a blend.
        check(set("field.dims", PropertyValue(glm::vec3(0.75f))), "field.dims is writable");
        PropertyValue dims;
        check(get("field.dims", dims) && std::get<glm::vec3>(dims).x == 0.75f,
              "field.dims reads back what law wrote");
        check(set("field.blend", PropertyValue(0.25)), "field.blend is writable");
        PropertyValue blend;
        check(get("field.blend", blend) && nearf(static_cast<float>(std::get<float>(blend)), 0.25f),
              "field.blend reads back what law wrote");
        check(set("field.extent", PropertyValue(2.0)) && nearf(o.getFieldExtent(), 2.0f),
              "field.extent is writable and reaches the object");

        // The Bezier control net: the Bernstein coefficients themselves.
        o.setBezierPatch(geom::makeBezierGrid(3, 3, 0.5f));
        PropertyValue count, du;
        check(get("patch.controlCount", count) && std::get<int>(count) == 16,
              "patch.controlCount reports the live net");
        check(get("patch.degreeU", du) && std::get<int>(du) == 3, "patch.degreeU reports the live net");
        check(!set("patch.degreeU", PropertyValue(4)),
              "patch.degreeU is read-only (a degree change is an elevation, not an assignment)");

        const glm::vec3 moved(9.0f, 8.0f, 7.0f);
        check(set("patch.ctrl.5", PropertyValue(moved)), "a control point is writable by law");
        PropertyValue back;
        check(get("patch.ctrl.5", back) && std::get<glm::vec3>(back) == moved,
              "the control point reads back what law wrote");
        check(o.getPatchControlLocal(5) == moved,
              "the write reached the patch data, not just the property");

        // The surface actually followed -- a law that animates a control point
        // must move the geometry, not desynchronise a cached mesh.
        const geom::PatchDerivatives d2 = geom::patchDerivatives(o.getPatchData());
        const glm::vec3 n2 = geom::bezierNormal(o.getPatchData(), d2, 0.5f, 0.5f);
        check(nearf(glm::length(n2), 1.0f), "the deformed surface still has a unit normal");
        check(!nearf(geom::evalBezier(o.getPatchData(), 0.5f, 0.5f).x, 0.0f, 1e-3f),
              "moving a control point moved the surface");
    }

    // -----------------------------------------------------------------------
    // 8. RUNG 5, second half -- a FieldNode's AST is reachable, and a concept
    //    carries the mathematics it captured.
    // -----------------------------------------------------------------------
    {
        geom::FieldNode fn("ast-probe");
        PropertyValue before;
        check(PropertyPath::parse("field.ast").getValue(fn, before) == PropertyPath::PathResult::Ok,
              "field.ast resolves on a FieldNode");

        auto authored = OntoMath::Piecewise::continuous(
            OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::variable("x", 1.0, 3.0)));
        const std::string doc = authored.toJson().dump();
        check(PropertyPath::parse("field.ast").setValue(fn, PropertyValue(doc)) ==
                  PropertyPath::PathResult::Ok,
              "field.ast accepts an authored AST");
        PropertyValue after;
        PropertyPath::parse("field.ast").getValue(fn, after);
        check(std::get<std::string>(after) == doc, "field.ast reads back the AST law wrote");
        check(PropertyPath::parse("field.ast").setValue(fn, PropertyValue(std::string("{not json"))) !=
                  PropertyPath::PathResult::Ok,
              "a malformed AST document is REFUSED, not half-applied");
        PropertyValue survived;
        PropertyPath::parse("field.ast").getValue(fn, survived);
        check(std::get<std::string>(survived) == doc, "the AST survived the refused write");

        // A concept must carry the mathematics, not just the pose: a captured
        // field/patch that reinstantiated as a bare cube would make set-to-set
        // creation lossy exactly where the geometry is most authored.
        Object sculpted("sculpted-source");
        sculpted.setFieldShape(geom::makeImplicit("x*x + y*y + z*z - 0.25"), 1.5f);
        auto concept = ObjectConcept::captureFrom({&sculpted}, "concept-sculpted");
        auto born = concept->instantiate(glm::mat4(1.0f),
                                         static_cast<const std::vector<Singular*>*>(nullptr));
        check(born.size() == 1 && born[0] && born[0]->hasField(),
              "a captured Field shape is reinstantiated as a Field, not a cube");
        if (!born.empty() && born[0]) {
            check(nearf(geom::evalSdf(born[0]->getFieldData(), glm::vec3(0.5f, 0.0f, 0.0f)), 0.0f),
                  "the newborn's field evaluates as the source's did");
            check(nearf(born[0]->getFieldExtent(), 1.5f), "the field extent came across too");
        }
    }

    // -----------------------------------------------------------------------
    // 9. RUNG 3 -- the quadric matrix path is held accountable to OntoMath.
    //    The rung is not "make raycast walk an AST"; it is "the mathematics is
    //    authored in OntoMath and the channel merely evaluates it". So: for
    //    every quadric the factories make, the fast matrix path must agree with
    //    ScalarForm::derivative and with the symbolic coefficient extraction.
    //    If they ever diverge, the matrix has started deciding what the thing
    //    is, and that is the divergence this rung exists to prevent.
    // -----------------------------------------------------------------------
    {
        struct Q { const char* name; glm::mat4 m; };
        const Q quadrics[] = {
            {"sphere",     geom::Quadric::sphere(0.5f)},
            {"ellipsoid",  geom::Quadric::ellipsoid(0.5f, 0.3f, 0.8f)},
            {"cylinder",   geom::Quadric::cylinder(0.4f)},
            {"cone",       geom::Quadric::cone(1.3f)},
            {"paraboloid", geom::Quadric::paraboloid(2.0f)},
        };
        const glm::vec3 samples[] = {
            {0.3f, 0.4f, 0.1f}, {-0.2f, 0.7f, -0.5f}, {1.1f, -0.3f, 0.9f}};

        bool gradsAgree = true, coeffsAgree = true, roundTrips = true;
        for (const Q& q : quadrics) {
            const OntoMath::ScalarForm f = geom::Quadric::toScalarForm(q.m);

            // The polynomial and the matrix are the same object.
            const glm::mat4 back = geom::Quadric::fromScalarForm(f);
            for (int r = 0; r < 4; ++r)
                for (int c = 0; c < 4; ++c)
                    if (!nearf(q.m[c][r], back[c][r], 1e-4f)) roundTrips = false;

            for (const glm::vec3& p : samples) {
                const glm::vec3 fast = geom::Quadric::gradient(q.m, p);
                const glm::vec3 symbolic = geom::Quadric::gradientFromForm(f, p);
                if (!nearf(fast.x, symbolic.x, 1e-3f) ||
                    !nearf(fast.y, symbolic.y, 1e-3f) ||
                    !nearf(fast.z, symbolic.z, 1e-3f)) {
                    gradsAgree = false;
                    std::printf("    (%s gradient at (%.2f,%.2f,%.2f): matrix (%.4f,%.4f,%.4f) "
                                "vs symbolic (%.4f,%.4f,%.4f))\n",
                                q.name, p.x, p.y, p.z, fast.x, fast.y, fast.z,
                                symbolic.x, symbolic.y, symbolic.z);
                }
            }

            // raycast's A/B/C against the same coefficients taken symbolically.
            const glm::vec3 o(0.0f, 0.0f, -3.0f);
            const glm::vec3 d = glm::normalize(glm::vec3(0.1f, 0.05f, 1.0f));
            double A = 0, B = 0, C = 0;
            if (!geom::Quadric::raycastCoefficientsFromForm(f, o, d, A, B, C)) {
                coeffsAgree = false;
            } else {
                const glm::vec4 P(o, 1.0f), D(d, 0.0f);
                const double mA = glm::dot(D, q.m * D);
                const double mB = 2.0 * glm::dot(P, q.m * D);
                const double mC = glm::dot(P, q.m * P);
                if (std::fabs(mA - A) > 1e-4 || std::fabs(mB - B) > 1e-4 ||
                    std::fabs(mC - C) > 1e-4) {
                    coeffsAgree = false;
                    std::printf("    (%s coeffs: matrix %.4f/%.4f/%.4f vs symbolic %.4f/%.4f/%.4f)\n",
                                q.name, mA, mB, mC, A, B, C);
                }
            }
        }
        check(roundTrips,  "every quadric round-trips matrix -> ScalarForm -> matrix");
        check(gradsAgree,  "the matrix gradient equals ScalarForm::derivative for every quadric");
        check(coeffsAgree, "raycast's A/B/C equal the symbolic coefficients of f(o + t*d)");
    }

    std::printf(g_failures == 0 ? "geometry_ontomath_test: ALL OK\n"
                                : "geometry_ontomath_test: FAILURES\n");
    return g_failures > 0 ? 1 : 0;
}
