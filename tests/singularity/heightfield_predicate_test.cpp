// Rendering-optimization Phase C: does geom::isHeightfieldExpr only match the
// decidable Sub(y, h) pattern, and does geom::computeHeightGrid's (hMin,hMax)
// bound ACTUALLY contain the true value of h everywhere in a cell's domain?
//
// The second question is the one that matters: a min/max grid whose bound is
// merely usually-right is not an optimization, it is a bug that deletes real
// geometry a Person could see -- exactly the shape of the campaign's own
// regressions (Bugs.md #12, #15-#20), each of which stayed green on every
// existing test because none of them varied the axis the defect lived on.
// This file's soundness check densely samples real points against the grid's
// own bound rather than trusting the derivation on paper.

#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <map>
#include <memory>
#include <string>

namespace {
int g_failures = 0;

void expect(bool cond, const char* what) {
    if (!cond) { std::printf("  FAILED: %s\n", what); ++g_failures; }
    else std::printf("  ok: %s\n", what);
}

using OntoMath::MathNode;

std::unique_ptr<MathNode> mkLeaf(const std::string& var) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::ValueLeaf;
    n->variableName = var;
    return n;
}
std::unique_ptr<MathNode> mkConst(double v) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::ScalarLeaf;
    n->scalarForm = OntoMath::ScalarForm::constant(v);
    return n;
}
std::unique_ptr<MathNode> mkComponent(std::unique_ptr<MathNode> a, const char* axis) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::Component; n->stringArg = axis;
    n->children.push_back(std::move(a));
    return n;
}
std::unique_ptr<MathNode> mkBinary(MathNode::Op op, std::unique_ptr<MathNode> a, std::unique_ptr<MathNode> b) {
    auto n = std::make_unique<MathNode>();
    n->op = op;
    n->children.push_back(std::move(a));
    n->children.push_back(std::move(b));
    return n;
}
std::unique_ptr<MathNode> mkVec3(std::unique_ptr<MathNode> x, std::unique_ptr<MathNode> y, std::unique_ptr<MathNode> z) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::VectorConstruct;
    n->children.push_back(std::move(x)); n->children.push_back(std::move(y)); n->children.push_back(std::move(z));
    return n;
}
std::unique_ptr<MathNode> mkNoise(std::unique_ptr<MathNode> a) {
    auto n = std::make_unique<MathNode>();
    n->op = MathNode::Op::Noise;
    n->children.push_back(std::move(a));
    return n;
}

// Sub(y, h) wrapping a copy of h, as an SdfNode::Expr leaf -- the shape
// isHeightfieldExpr must recognize.
geom::SdfNode wrapHeightfield(std::unique_ptr<MathNode> h) {
    geom::SdfNode n;
    n.op = geom::SdfOp::Leaf;
    n.prim = geom::SdfPrim::Expr;
    n.mathNode = std::shared_ptr<MathNode>(mkBinary(MathNode::Op::Sub, mkLeaf("y"), std::move(h)).release());
    return n;
}

float frand(float lo, float hi) { return lo + (hi - lo) * (float)std::rand() / (float)RAND_MAX; }

// Densely sample points in the field's local-space box and confirm the grid's
// (hMin,hMax) for the containing cell always contains h's true evaluated
// value. This is the property the WGSL DDA's soundness depends on: if it ever
// fails, the shader will discard a fragment where real geometry exists.
void checkSoundness(const char* label, const MathNode& h, const glm::vec3& halfExtent,
                    int dimX, int dimZ, int samples) {
    geom::HeightGrid grid = geom::computeHeightGrid(h, halfExtent, dimX, dimZ);
    if (grid.dimX == 0) {
        std::printf("  %s: grid refused (dimX=0) -- no acceleration, trivially sound\n", label);
        return;
    }
    const float cellX = (2.0f * halfExtent.x) / grid.dimX;
    const float cellZ = (2.0f * halfExtent.z) / grid.dimZ;
    std::map<std::string, PropertyValue> vars{
        {"x", PropertyValue(0.0)}, {"y", PropertyValue(0.0)},
        {"z", PropertyValue(0.0)}, {"p", PropertyValue(glm::vec3(0.0f))},
    };
    int violations = 0;
    float worstMargin = 1e30f; // how much slack was left at the tightest point (min over samples of min(true-lo, hi-true))
    for (int s = 0; s < samples; ++s) {
        const float x = frand(-halfExtent.x, halfExtent.x);
        const float y = frand(-halfExtent.y, halfExtent.y);
        const float z = frand(-halfExtent.z, halfExtent.z);
        int ix = static_cast<int>((x + halfExtent.x) / cellX);
        int iz = static_cast<int>((z + halfExtent.z) / cellZ);
        if (ix < 0) ix = 0; if (ix >= grid.dimX) ix = grid.dimX - 1;
        if (iz < 0) iz = 0; if (iz >= grid.dimZ) iz = grid.dimZ - 1;
        const glm::vec2 cell = grid.cells[static_cast<size_t>(iz) * grid.dimX + ix];

        vars["x"] = PropertyValue(static_cast<double>(x));
        vars["y"] = PropertyValue(static_cast<double>(y));
        vars["z"] = PropertyValue(static_cast<double>(z));
        vars["p"] = PropertyValue(glm::vec3(x, y, z));
        auto val = h.evaluate(vars);
        if (!val) continue;
        double d = 0.0;
        if (!propertyValueToNumber(*val, d)) continue;
        const float trueVal = static_cast<float>(d);
        if (trueVal < cell.x || trueVal > cell.y) ++violations;
        worstMargin = std::min(worstMargin, std::min(trueVal - cell.x, cell.y - trueVal));
    }
    std::printf("  %s: %d/%d samples out of bound, tightest margin left = %.4f\n",
               label, violations, samples, worstMargin);
    expect(violations == 0, (std::string(label) + ": grid bound contains every sampled true value").c_str());
}

} // namespace

int main() {
    std::srand(42);
    std::printf("=== isHeightfieldExpr: structural pattern match ===\n");
    {
        // Negative: the real authored Perlin-floor field reads the whole point,
        // including p.y. Its outer syntax resembles a heightfield but h is not
        // independent of y, so specialization would change its mathematics.
        auto offset = mkVec3(mkConst(100.0), mkConst(0.0), mkConst(100.0));
        auto arg = mkBinary(MathNode::Op::Scale, mkConst(0.008),
                            mkBinary(MathNode::Op::Add, mkLeaf("p"), std::move(offset)));
        auto h = mkBinary(MathNode::Op::Scale, mkConst(40.0), mkNoise(std::move(arg)));
        geom::SdfNode field = wrapHeightfield(std::move(h));
        const OntoMath::MathNode* outH = nullptr;
        expect(!geom::isHeightfieldExpr(field, &outH) && outH == nullptr,
              "y - 40*noise(p*0.008+offset) is rejected because h reads y");
    }
    {
        // Positive: only x/z components reach Noise, so h is proved independent
        // of y and the 2D min/max grid is admissible.
        auto nx = mkBinary(MathNode::Op::Scale, mkConst(0.008), mkComponent(mkLeaf("p"), "x"));
        auto nz = mkBinary(MathNode::Op::Scale, mkConst(0.008), mkComponent(mkLeaf("p"), "z"));
        auto arg = mkVec3(std::move(nx), mkConst(0.0), std::move(nz));
        auto h = mkBinary(MathNode::Op::Scale, mkConst(40.0), mkNoise(std::move(arg)));
        geom::SdfNode field = wrapHeightfield(std::move(h));
        const OntoMath::MathNode* outH = nullptr;
        expect(geom::isHeightfieldExpr(field, &outH) && outH != nullptr,
              "y - h(x,z) with component-extracted x/z is recognized");
    }
    {
        // Negative: y hidden in a constructed vector component must not escape
        // the dependency proof.
        auto v = mkVec3(mkConst(0.0), mkLeaf("y"), mkConst(0.0));
        auto h = mkComponent(std::move(v), "y");
        geom::SdfNode field = wrapHeightfield(std::move(h));
        expect(!geom::isHeightfieldExpr(field, nullptr),
              "y hidden in VectorConstruct/Component is rejected");
    }
    {
        // Negative: reversed operand order, h - y, is NOT the pattern (that
        // shape means something different -- h is being treated as the "y").
        geom::SdfNode field;
        field.op = geom::SdfOp::Leaf; field.prim = geom::SdfPrim::Expr;
        field.mathNode = std::shared_ptr<MathNode>(
            mkBinary(MathNode::Op::Sub, mkConst(1.0), mkLeaf("y")).release());
        const OntoMath::MathNode* outH = nullptr;
        expect(!geom::isHeightfieldExpr(field, &outH), "1 - y (reversed operands) is rejected");
    }
    {
        // Negative: not even a Sub at the top.
        geom::SdfNode field;
        field.op = geom::SdfOp::Leaf; field.prim = geom::SdfPrim::Expr;
        field.mathNode = std::shared_ptr<MathNode>(mkLeaf("y").release());
        const OntoMath::MathNode* outH = nullptr;
        expect(!geom::isHeightfieldExpr(field, &outH), "a bare 'y' leaf (no Sub) is rejected");
    }
    {
        // Negative: a CSG-combined field (Union of two heightfield-shaped
        // leaves) is not a single Expr leaf, so it must not match either.
        auto h1 = mkConst(1.0);
        auto h2 = mkConst(2.0);
        geom::SdfNode a = wrapHeightfield(std::move(h1));
        geom::SdfNode b = wrapHeightfield(std::move(h2));
        geom::SdfNode u = geom::SdfNode::binary(geom::SdfOp::Union, a, b);
        const OntoMath::MathNode* outH = nullptr;
        expect(!geom::isHeightfieldExpr(u, &outH), "a CSG Union of two heightfield leaves is rejected");
    }

    std::printf("\n=== computeHeightGrid: soundness (bound must contain every true sample) ===\n");
    {
        // The real authored field: h depends on y too (Noise fed the whole
        // point), so the honest bound pays the full y half-extent in slack.
        // This is the addendum's own finding, reproduced here as a locked
        // property rather than left as a one-off measurement.
        auto offset = mkVec3(mkConst(100.0), mkConst(0.0), mkConst(100.0));
        auto arg = mkBinary(MathNode::Op::Scale, mkConst(0.008),
                            mkBinary(MathNode::Op::Add, mkLeaf("p"), std::move(offset)));
        auto h = mkBinary(MathNode::Op::Scale, mkConst(40.0), mkNoise(std::move(arg)));
        checkSoundness("real Perlin-floor field (y-dependent noise arg)", *h,
                       glm::vec3(1000.f, 30.f, 1000.f), 128, 128, 20000);
    }
    {
        // A mathematically valid 2D heightfield can still lack a proved range
        // bound for acceleration. Perlin's former Lipschitz number was
        // empirical, so computeHeightGrid must refuse it rather than using a
        // sampled margin to delete ray segments.
        auto nx = mkBinary(MathNode::Op::Scale, mkConst(0.008), mkComponent(mkLeaf("p"), "x"));
        auto nz = mkBinary(MathNode::Op::Scale, mkConst(0.008), mkComponent(mkLeaf("p"), "z"));
        auto arg = mkVec3(std::move(nx), mkConst(0.0), std::move(nz));
        auto h = mkBinary(MathNode::Op::Scale, mkConst(40.0), mkNoise(std::move(arg)));
        geom::HeightGrid grid = geom::computeHeightGrid(
            *h, glm::vec3(1000.f, 30.f, 1000.f), 128, 128);
        expect(grid.dimX == 0,
              "2D Perlin heightfield refuses grid without a proved Lipschitz bound");
    }
    {
        // An op the Lipschitz estimator does not cover (Pow) must REFUSE
        // (dimX=0), not silently produce an unsound tightened bound.
        auto h = mkBinary(MathNode::Op::Pow, mkComponent(mkLeaf("p"), "x"), mkConst(2.0));
        geom::HeightGrid grid = geom::computeHeightGrid(*h, glm::vec3(10.f), 24, 24);
        expect(grid.dimX == 0, "an unhandled op (Pow) refuses rather than guessing");
    }

    std::printf(g_failures == 0 ? "\nheightfield_predicate_test: ALL OK\n"
                                : "\nheightfield_predicate_test: FAILURES\n");
    return g_failures > 0 ? 1 : 0;
}
