#include <unordered_map>
#include "Sdf.hpp"

#include <algorithm>
#include <cmath>
#include <cctype>
#include <cstdlib>

namespace geom {

static const float kPI = 3.14159265358979323846f;

// ---------------------------------------------------------------------------
// Primitive signed-distance functions (local, centred; axis = Z where relevant)
// ---------------------------------------------------------------------------
static float dot2(const glm::vec2& v) { return glm::dot(v, v); }

static float sdSphere(const glm::vec3& p, float r) {
    return glm::length(p) - r;
}

static float sdBox(const glm::vec3& p, const glm::vec3& b) {
    glm::vec3 q = glm::abs(p) - b;
    return glm::length(glm::max(q, glm::vec3(0.0f))) +
           std::min(std::max(q.x, std::max(q.y, q.z)), 0.0f);
}

static float sdRoundBox(const glm::vec3& p, const glm::vec3& b, float r) {
    return sdBox(p, glm::max(b - glm::vec3(r), glm::vec3(0.0f))) - r;
}

static float sdEllipsoid(const glm::vec3& p, const glm::vec3& r) {
    glm::vec3 rr = glm::max(r, glm::vec3(1e-4f));
    float k0 = glm::length(p / rr);
    float k1 = glm::length(p / (rr * rr));
    if (k1 < 1e-8f) return -std::min(rr.x, std::min(rr.y, rr.z));
    return k0 * (k0 - 1.0f) / k1;
}

static float sdCylinder(const glm::vec3& p, float r, float h) {
    glm::vec2 d = glm::abs(glm::vec2(glm::length(glm::vec2(p.x, p.y)), p.z)) - glm::vec2(r, h);
    return std::min(std::max(d.x, d.y), 0.0f) + glm::length(glm::max(d, glm::vec2(0.0f)));
}

// Capped cone along Z: base radius r1 at z=-h, apex (r2=0) at z=+h.
static float sdCone(const glm::vec3& p, float r1, float h) {
    float r2 = 0.0f;
    glm::vec2 q(glm::length(glm::vec2(p.x, p.y)), p.z);
    glm::vec2 k1(r2, h);
    glm::vec2 k2(r2 - r1, 2.0f * h);
    glm::vec2 ca(q.x - std::min(q.x, (q.y < 0.0f) ? r1 : r2), std::abs(q.y) - h);
    glm::vec2 cb = q - k1 + k2 * glm::clamp(glm::dot(k1 - q, k2) / dot2(k2), 0.0f, 1.0f);
    float s = (cb.x < 0.0f && ca.y < 0.0f) ? -1.0f : 1.0f;
    return s * std::sqrt(std::min(dot2(ca), dot2(cb)));
}

static float sdTorus(const glm::vec3& p, float R, float r) {
    glm::vec2 q(glm::length(glm::vec2(p.x, p.y)) - R, p.z);
    return glm::length(q) - r;
}

// --- Implicit expression compiler (recursive descent → RPN) ----------------
namespace {
struct ExprParser {
    const std::string& s;
    size_t i = 0;
    bool ok = true;
    std::vector<SdfToken>& out;
    ExprParser(const std::string& src, std::vector<SdfToken>& o) : s(src), out(o) {}

    void ws() { while (i < s.size() && std::isspace((unsigned char)s[i])) ++i; }
    char cur() { ws(); return i < s.size() ? s[i] : 0; }
    bool eat(char c) { ws(); if (i < s.size() && s[i] == c) { ++i; return true; } return false; }
    void push(SdfToken::Kind k, float v = 0.0f) { SdfToken t; t.kind = k; t.num = v; out.push_back(t); }

    void parseExpr() {
        parseTerm();
        for (;;) { char c = cur();
            if (c == '+') { ++i; parseTerm(); push(SdfToken::Add); }
            else if (c == '-') { ++i; parseTerm(); push(SdfToken::Sub); }
            else break; }
    }
    void parseTerm() {
        parseFactor();
        for (;;) { char c = cur();
            if (c == '*') { ++i; parseFactor(); push(SdfToken::Mul); }
            else if (c == '/') { ++i; parseFactor(); push(SdfToken::Div); }
            else break; }
    }
    void parseFactor() { // ^ is right-associative
        parseUnary();
        if (cur() == '^') { ++i; parseFactor(); push(SdfToken::Pow); }
    }
    void parseUnary() {
        if (cur() == '-') { ++i; parseUnary(); push(SdfToken::Neg); return; }
        if (cur() == '+') { ++i; parseUnary(); return; }
        parsePrimary();
    }
    void parsePrimary() {
        char c = cur();
        if (c == '(') { ++i; parseExpr(); if (!eat(')')) ok = false; return; }
        if (std::isdigit((unsigned char)c) || c == '.') {
            size_t j = i; while (j < s.size() && (std::isdigit((unsigned char)s[j]) || s[j] == '.')) ++j;
            push(SdfToken::Num, std::strtof(s.substr(i, j - i).c_str(), nullptr)); i = j; return;
        }
        if (std::isalpha((unsigned char)c)) {
            size_t j = i; while (j < s.size() && std::isalnum((unsigned char)s[j])) ++j;
            std::string id = s.substr(i, j - i); i = j;
            if (cur() == '(') {
                ++i; parseExpr(); if (!eat(')')) ok = false;
                if (id == "sin") push(SdfToken::Sin); else if (id == "cos") push(SdfToken::Cos);
                else if (id == "tan") push(SdfToken::Tan); else if (id == "sqrt") push(SdfToken::Sqrt);
                else if (id == "abs") push(SdfToken::Abs); else if (id == "exp") push(SdfToken::Exp);
                else if (id == "log" || id == "ln") push(SdfToken::Log); else ok = false;
                return;
            }
            if (id == "x") push(SdfToken::X); else if (id == "y") push(SdfToken::Y);
            else if (id == "z") push(SdfToken::Z); else if (id == "pi") push(SdfToken::Num, 3.14159265358979f);
            else if (id == "e") push(SdfToken::Num, 2.71828182845905f); else ok = false;
            return;
        }
        ok = false;
    }
};
} // namespace

std::vector<SdfToken> compileExpr(const std::string& src) {
    std::vector<SdfToken> out;
    ExprParser p(src, out);
    p.parseExpr();
    p.ws();
    if (!p.ok || p.i < src.size()) out.clear(); // parse error → empty (renders nothing)
    return out;
}

namespace {

// Lift a compiled implicit expression into an OntoMath AST -- or REFUSE.
//
// Returning nullptr is a real answer here, not a failure to try: makeImplicit
// leaves mathNode null, and evalSdf falls back to evalRpn, which computes the
// expression correctly. An APPROXIMATE MathNode is the one outcome that must
// never be produced, because evalSdf and the WGSL emitter both prefer mathNode
// when it exists -- so a wrong tree is not caught by webgpu_sdf_parity_test
// either, which reads the same tree on both sides and agrees with itself.
//
// This is what the first version of this function did, and the shapes it
// silently changed were:
//   x / 2            -> Op::Scale, i.e. x * 2      (there is no division op)
//   x ^ y            -> Op::Scale, i.e. x * y
//   sqrt(e), tan(e), abs(e)  -> e                  (no such token case at all)
//   sin(e) where e is compound -> e                (the call was dropped)
//
// What IS faithfully representable, and so is lifted:
//   numbers, x/y/z, + - *, unary minus,
//   var ^ constant                       -> ScalarForm::variable(var, exp)
//   sin/cos/exp/log OF A BARE VARIABLE   -> ScalarForm::transcendental
//
// Everything else refuses. TransFactor::Kind carries only Sin/Cos/Exp/Ln and
// is append-only and serialized as ints, so widening it to reach tan/sqrt/abs
// is a deliberate ontological act, not something to slip into a converter.
std::shared_ptr<OntoMath::MathNode> rpnToMathNode(const std::vector<SdfToken>& r) {
    if (r.empty()) return nullptr;

    using MN = OntoMath::MathNode;
    const auto scalar = [](double v) {
        auto n = std::make_shared<MN>();
        n->op = MN::Op::ScalarLeaf;
        n->scalarForm = OntoMath::ScalarForm::constant(v);
        return n;
    };
    const auto value = [](const std::string& name) {
        auto n = std::make_shared<MN>();
        n->op = MN::Op::ValueLeaf;
        n->variableName = name;
        return n;
    };
    const auto binary = [](MN::Op op, const std::shared_ptr<MN>& a,
                           const std::shared_ptr<MN>& b) {
        auto n = std::make_shared<MN>();
        n->op = op;
        n->children.push_back(std::make_unique<MN>(*a));
        n->children.push_back(std::make_unique<MN>(*b));
        return n;
    };

    std::vector<std::shared_ptr<MN>> st;
    const auto pop2 = [&st](std::shared_ptr<MN>& a, std::shared_ptr<MN>& b) {
        if (st.size() < 2) return false;      // malformed RPN: refuse, never guess
        b = st.back(); st.pop_back();
        a = st.back(); st.pop_back();
        return true;
    };

    for (const auto& t : r) {
        std::shared_ptr<MN> a, b;
        switch (t.kind) {
            case SdfToken::Num: st.push_back(scalar(t.num)); break;
            case SdfToken::X:   st.push_back(value("x")); break;
            case SdfToken::Y:   st.push_back(value("y")); break;
            case SdfToken::Z:   st.push_back(value("z")); break;

            case SdfToken::Add:
                if (!pop2(a, b)) return nullptr;
                st.push_back(binary(MN::Op::Add, a, b));
                break;
            case SdfToken::Sub:
                if (!pop2(a, b)) return nullptr;
                st.push_back(binary(MN::Op::Sub, a, b));
                break;
            case SdfToken::Mul:
                if (!pop2(a, b)) return nullptr;
                st.push_back(binary(MN::Op::Scale, a, b));
                break;

            case SdfToken::Neg: {
                if (st.empty()) return nullptr;
                a = st.back(); st.pop_back();
                st.push_back(binary(MN::Op::Scale, scalar(-1.0), a));
                break;
            }

            case SdfToken::Div: {
                if (!pop2(a, b)) return nullptr;
                st.push_back(binary(MN::Op::Div, a, b));
                break;
            }

            case SdfToken::Pow: {
                if (!pop2(a, b)) return nullptr;
                if (a->op == MN::Op::ValueLeaf && b->op == MN::Op::ScalarLeaf &&
                    b->scalarForm.terms.size() == 1 && b->scalarForm.terms[0].factors.empty() &&
                    b->scalarForm.terms[0].trans.empty()) {
                    auto n = std::make_shared<MN>();
                    n->op = MN::Op::ScalarLeaf;
                    n->scalarForm = OntoMath::ScalarForm::variable(
                        a->variableName, b->scalarForm.terms[0].coefficient);
                    st.push_back(std::move(n));
                } else {
                    st.push_back(binary(MN::Op::Pow, a, b));
                }
                break;
            }

            case SdfToken::Sin:
            case SdfToken::Cos:
            case SdfToken::Exp:
            case SdfToken::Log: {
                if (st.empty()) return nullptr;
                a = st.back(); st.pop_back();
                if (a->op == MN::Op::ValueLeaf) {
                    OntoMath::TransFactor::Kind kind = OntoMath::TransFactor::Kind::Sin;
                    if (t.kind == SdfToken::Cos)      kind = OntoMath::TransFactor::Kind::Cos;
                    else if (t.kind == SdfToken::Exp) kind = OntoMath::TransFactor::Kind::Exp;
                    else if (t.kind == SdfToken::Log) kind = OntoMath::TransFactor::Kind::Ln;
                    auto n = std::make_shared<MN>();
                    n->op = MN::Op::ScalarLeaf;
                    n->scalarForm = OntoMath::ScalarForm::transcendental(kind, a->variableName);
                    st.push_back(std::move(n));
                } else {
                    return nullptr; // compound argument transcendental: refuse
                }
                break;
            }

            case SdfToken::Abs: {
                if (st.empty()) return nullptr;
                a = st.back(); st.pop_back();
                auto n = std::make_shared<MN>();
                n->op = MN::Op::Abs;
                n->children.push_back(std::make_unique<MN>(*a));
                st.push_back(std::move(n));
                break;
            }

            case SdfToken::Sqrt: {
                if (st.empty()) return nullptr;
                a = st.back(); st.pop_back();
                auto n = std::make_shared<MN>();
                n->op = MN::Op::Sqrt;
                n->children.push_back(std::make_unique<MN>(*a));
                st.push_back(std::move(n));
                break;
            }

            case SdfToken::Tan: {
                if (st.empty()) return nullptr;
                a = st.back(); st.pop_back();
                auto n = std::make_shared<MN>();
                n->op = MN::Op::Tan;
                n->children.push_back(std::make_unique<MN>(*a));
                st.push_back(std::move(n));
                break;
            }

            default:
                return nullptr;
        }
    }
    // A well-formed expression leaves exactly one value on the stack.
    if (st.size() != 1) return nullptr;
    return st.back();
}

} // namespace

SdfNode makeImplicit(const std::string& src) {
    SdfNode n;
    n.op = SdfOp::Leaf;
    n.prim = SdfPrim::Expr;
    n.expr = src;
    n.rpn = compileExpr(src);
    n.mathNode = rpnToMathNode(n.rpn);
    return n;
}

SdfNode makeImplicit(std::shared_ptr<OntoMath::MathNode> node) {
    SdfNode n;
    n.op = SdfOp::Leaf;
    n.prim = SdfPrim::Expr;
    n.mathNode = std::move(node);
    return n;
}

SdfNode makeImplicit(std::shared_ptr<OntoMath::Piecewise> pw) {
    SdfNode n;
    n.op = SdfOp::Leaf;
    n.prim = SdfPrim::Expr;
    n.piecewise = std::move(pw);
    return n;
}

std::shared_ptr<OntoMath::MathNode> SdfNode::toMathNode() const {
    if (op == SdfOp::Union) {
        if (children.size() >= 2 && children[0] && children[1]) {
            auto ca = children[0]->toMathNode();
            auto cb = children[1]->toMathNode();
            if (ca && cb) {
                return std::shared_ptr<OntoMath::MathNode>(OntoMath::MathNode::unionOp(
                    std::make_unique<OntoMath::MathNode>(*ca),
                    std::make_unique<OntoMath::MathNode>(*cb)));
            }
        }
    } else if (op == SdfOp::Intersect) {
        if (children.size() >= 2 && children[0] && children[1]) {
            auto ca = children[0]->toMathNode();
            auto cb = children[1]->toMathNode();
            if (ca && cb) {
                return std::shared_ptr<OntoMath::MathNode>(OntoMath::MathNode::intersectionOp(
                    std::make_unique<OntoMath::MathNode>(*ca),
                    std::make_unique<OntoMath::MathNode>(*cb)));
            }
        }
    } else if (op == SdfOp::Subtract) {
        if (children.size() >= 2 && children[0] && children[1]) {
            auto ca = children[0]->toMathNode();
            auto cb = children[1]->toMathNode();
            if (ca && cb) {
                return std::shared_ptr<OntoMath::MathNode>(OntoMath::MathNode::differenceOp(
                    std::make_unique<OntoMath::MathNode>(*ca),
                    std::make_unique<OntoMath::MathNode>(*cb)));
            }
        }
    } else if (op == SdfOp::SmoothUnion) {
        if (children.size() >= 2 && children[0] && children[1]) {
            auto ca = children[0]->toMathNode();
            auto cb = children[1]->toMathNode();
            if (ca && cb) {
                return std::shared_ptr<OntoMath::MathNode>(OntoMath::MathNode::smoothUnionOp(
                    std::make_unique<OntoMath::MathNode>(*ca),
                    std::make_unique<OntoMath::MathNode>(*cb), t));
            }
        }
    } else if (op == SdfOp::Leaf) {
        if (prim == SdfPrim::Sphere) {
            return std::shared_ptr<OntoMath::MathNode>(OntoMath::MathNode::sphere(dims.x));
        } else if (prim == SdfPrim::Box) {
            return std::shared_ptr<OntoMath::MathNode>(OntoMath::MathNode::box(dims));
        } else if (prim == SdfPrim::Cylinder) {
            return std::shared_ptr<OntoMath::MathNode>(OntoMath::MathNode::cylinder(dims.x, dims.y));
        } else if (prim == SdfPrim::Torus) {
            return std::shared_ptr<OntoMath::MathNode>(OntoMath::MathNode::torus(dims.x, dims.y));
        } else if (prim == SdfPrim::Expr) {
            if (mathNode) return mathNode;
            return rpnToMathNode(rpn);
        }
    }
    // REFUSE, do not substitute. This used to `return sphere(0.5)`, which
    // turned every kind the branches above do not cover -- Cone, Ellipsoid,
    // Ovoid, Paraboloid, RoundedBox, Convex (a whole polyhedron), and the
    // Morph operator -- into a unit sphere, silently and with no diagnostic.
    // A caller that cannot get a faithful AST must learn that, not receive a
    // different shape that evaluates cleanly.
    return nullptr;
}

static float evalRpn(const std::vector<SdfToken>& r, float x, float y, float z) {
    if (r.empty()) return 1e9f; // empty space
    float st[128]; int sp = 0;
    auto push = [&](float v) { if (sp < 128) st[sp++] = v; };
    for (const auto& t : r) {
        switch (t.kind) {
            case SdfToken::Num: push(t.num); break;
            case SdfToken::X: push(x); break;
            case SdfToken::Y: push(y); break;
            case SdfToken::Z: push(z); break;
            case SdfToken::Add: if (sp >= 2) { float b = st[--sp], a = st[--sp]; push(a + b); } break;
            case SdfToken::Sub: if (sp >= 2) { float b = st[--sp], a = st[--sp]; push(a - b); } break;
            case SdfToken::Mul: if (sp >= 2) { float b = st[--sp], a = st[--sp]; push(a * b); } break;
            case SdfToken::Div: if (sp >= 2) { float b = st[--sp], a = st[--sp]; push(std::fabs(b) > 1e-9f ? a / b : 0.0f); } break;
            case SdfToken::Pow: if (sp >= 2) { float b = st[--sp], a = st[--sp]; push(std::pow(a, b)); } break;
            case SdfToken::Neg: if (sp >= 1) st[sp - 1] = -st[sp - 1]; break;
            case SdfToken::Sin: if (sp >= 1) st[sp - 1] = std::sin(st[sp - 1]); break;
            case SdfToken::Cos: if (sp >= 1) st[sp - 1] = std::cos(st[sp - 1]); break;
            case SdfToken::Tan: if (sp >= 1) st[sp - 1] = std::tan(st[sp - 1]); break;
            case SdfToken::Sqrt: if (sp >= 1) st[sp - 1] = std::sqrt(std::max(0.0f, st[sp - 1])); break;
            case SdfToken::Abs: if (sp >= 1) st[sp - 1] = std::fabs(st[sp - 1]); break;
            case SdfToken::Exp: if (sp >= 1) st[sp - 1] = std::exp(st[sp - 1]); break;
            case SdfToken::Log: if (sp >= 1) st[sp - 1] = std::log(std::max(1e-9f, st[sp - 1])); break;
        }
    }
    return sp > 0 ? st[sp - 1] : 1e9f;
}

static float evalLeaf(const SdfNode& n, const glm::vec3& world) {
    const glm::vec3 p = world - n.offset; // leaf placement
    switch (n.prim) {
        case SdfPrim::Sphere:    return sdSphere(p, n.dims.x);
        case SdfPrim::Box:       return sdBox(p, n.dims);
        case SdfPrim::RoundBox:  return sdRoundBox(p, n.dims, n.p0);
        case SdfPrim::Ellipsoid: return sdEllipsoid(p, n.dims);
        case SdfPrim::Cylinder:  return sdCylinder(p, n.dims.x, n.dims.y);
        case SdfPrim::Cone:      return sdCone(p, n.dims.x, n.dims.y);
        case SdfPrim::Torus:     return sdTorus(p, n.dims.x, n.dims.y);
        case SdfPrim::Expr: {
            if (n.mathNode) {
                // Rebuilding this environment per sample was pure overhead:
                // four red-black-tree inserts with string keys, once per SDF
                // sample. Reuse one map per thread -- the keys are created once
                // and every later call only assigns into nodes that already
                // exist, so the steady state allocates nothing.
                //
                // KERNEL SCRATCH (Refusal 6): a reused evaluation buffer, not
                // being state; nothing authored can observe it. Safe to share
                // because MathNode::evaluate never re-enters evalLeaf -- Op::SDF
                // and Op::Gradient recurse inside the MathNode tree, never back
                // out through geom::evalSdf -- so two bindings are never live at
                // once. The piecewise arm below keeps its own fresh map: a Piece
                // can carry a FunctionCall or a Fold, which read the world, and
                // that is not a path this comment can promise never returns here.
                static thread_local std::map<std::string, PropertyValue> vars{
                    {"x", PropertyValue(0.0)},
                    {"y", PropertyValue(0.0)},
                    {"z", PropertyValue(0.0)},
                    {"p", PropertyValue(glm::vec3(0.0f))},
                };
                vars["x"] = PropertyValue(static_cast<double>(p.x));
                vars["y"] = PropertyValue(static_cast<double>(p.y));
                vars["z"] = PropertyValue(static_cast<double>(p.z));
                vars["p"] = PropertyValue(p);
                auto val = n.mathNode->evaluate(vars);
                if (val) {
                    double d = 0.0;
                    if (propertyValueToNumber(*val, d)) return static_cast<float>(d);
                }
            } else if (n.piecewise) {
                std::map<std::string, PropertyValue> vars;
                vars["x"] = PropertyValue(static_cast<double>(p.x));
                vars["y"] = PropertyValue(static_cast<double>(p.y));
                vars["z"] = PropertyValue(static_cast<double>(p.z));
                vars["p"] = PropertyValue(p);
                auto val = n.piecewise->evaluate(vars);
                if (val) {
                    double d = 0.0;
                    if (propertyValueToNumber(*val, d)) return static_cast<float>(d);
                }
            }
            return evalRpn(n.rpn, p.x, p.y, p.z);
        }
        case SdfPrim::Convex: {
            if (n.planes.empty()) return 1e9f;
            float m = -1e9f;
            for (const glm::vec4& pl : n.planes)
                m = std::max(m, glm::dot(glm::vec3(pl), p) - pl.w);
            return m;
        }
    }
    return sdSphere(p, 0.5f);
}

static float smin(float a, float b, float k) {
    if (k <= 1e-5f) return std::min(a, b);
    float h = glm::clamp(0.5f + 0.5f * (b - a) / k, 0.0f, 1.0f);
    return glm::mix(b, a, h) - k * h * (1.0f - h);
}

// ---------------------------------------------------------------------------
// Tree evaluation
// ---------------------------------------------------------------------------
float evalSdf(const SdfNode& n, const glm::vec3& p) {
    if (n.op == SdfOp::Leaf) return evalLeaf(n, p);
    if (n.children.size() < 2) {
        return (n.children.empty() || !n.children[0]) ? 1.0f : evalSdf(*n.children[0], p);
    }
    if (!n.children[0] || !n.children[1]) return 1.0f;
    float a = evalSdf(*n.children[0], p);
    float b = evalSdf(*n.children[1], p);
    switch (n.op) {
        case SdfOp::Morph:       return glm::mix(a, b, glm::clamp(n.t, 0.0f, 1.0f));
        case SdfOp::Union:       return std::min(a, b);
        case SdfOp::Intersect:   return std::max(a, b);
        case SdfOp::Subtract:    return std::max(a, -b);
        case SdfOp::SmoothUnion: return smin(a, b, n.t);
        default:                 return std::min(a, b);
    }
}


OntoMath::Interval evalRange(const SdfNode& n, const glm::vec3& boxMin, const glm::vec3& boxMax) {
    using namespace OntoMath;
    auto retInf = []() { return Interval::infinite(); };
    
    // AABB center and half-diagonal for 1-Lipschitz true-distance leaves
    glm::vec3 c = 0.5f * (boxMin + boxMax);
    glm::vec3 halfSize = 0.5f * (boxMax - boxMin);
    float R = glm::length(halfSize);
    
    switch (n.op) {
        case SdfOp::Leaf: {
            if (n.prim == SdfPrim::Expr) {
                if (n.mathNode) {
                    std::map<std::string, MathNode::RangeValue> vars = {
                        {kAmbientPointVar, MathNode::RangeValue::makeVector(Interval(boxMin.x, boxMax.x), Interval(boxMin.y, boxMax.y), Interval(boxMin.z, boxMax.z))},
                        {"x", MathNode::RangeValue::makeScalar(Interval(boxMin.x, boxMax.x))},
                        {"y", MathNode::RangeValue::makeScalar(Interval(boxMin.y, boxMax.y))},
                        {"z", MathNode::RangeValue::makeScalar(Interval(boxMin.z, boxMax.z))}
                    };
                    auto r = n.mathNode->evalRange(vars);
                    if (r && r->kind == ValueKind::Scalar) return r->scalar;
                }
                return retInf();
            }
            
            // True distance leaves (1-Lipschitz)
            float distAtCenter = evalSdf(n, c);
            return Interval(distAtCenter - R, distAtCenter + R);
        }
        case SdfOp::Morph: {
            if (n.children.size() < 2 || !n.children[0] || !n.children[1]) return retInf();
            Interval a = evalRange(*n.children[0], boxMin, boxMax);
            Interval b = evalRange(*n.children[1], boxMin, boxMax);
            float t = glm::clamp(n.t, 0.0f, 1.0f);
            return Interval(glm::mix(a.lo, b.lo, t), glm::mix(a.hi, b.hi, t));
        }
        case SdfOp::Union: {
            if (n.children.size() < 2 || !n.children[0] || !n.children[1]) return retInf();
            Interval a = evalRange(*n.children[0], boxMin, boxMax);
            Interval b = evalRange(*n.children[1], boxMin, boxMax);
            return Interval(std::min(a.lo, b.lo), std::min(a.hi, b.hi));
        }
        case SdfOp::Intersect: {
            if (n.children.size() < 2 || !n.children[0] || !n.children[1]) return retInf();
            Interval a = evalRange(*n.children[0], boxMin, boxMax);
            Interval b = evalRange(*n.children[1], boxMin, boxMax);
            return Interval(std::max(a.lo, b.lo), std::max(a.hi, b.hi));
        }
        case SdfOp::Subtract: {
            if (n.children.size() < 2 || !n.children[0] || !n.children[1]) return retInf();
            Interval a = evalRange(*n.children[0], boxMin, boxMax);
            Interval b = evalRange(*n.children[1], boxMin, boxMax);
            return Interval(std::max(a.lo, -b.hi), std::max(a.hi, -b.lo));
        }
        case SdfOp::SmoothUnion: {
            if (n.children.size() < 2 || !n.children[0] || !n.children[1]) return retInf();
            Interval a = evalRange(*n.children[0], boxMin, boxMax);
            Interval b = evalRange(*n.children[1], boxMin, boxMax);
            // smin <= min, and below it by at most a k-dependent constant
            // Specifically, for polynomial smin(a,b,k): max difference is k/4
            // So smin >= min(a,b) - k/4
            // Wait, the SDF smooth union uses k directly, max diff is k / 4. 
            // Wait, in evalSdf, smooth min is:
            // float h = glm::clamp( 0.5f + 0.5f*(b-a)/k, 0.0f, 1.0f );
            // return glm::mix(b, a, h) - k*h*(1.0f-h);
            // Max diff is k * 0.5 * 0.5 = k / 4.
            // But we can be looser: just min(lo_a, lo_b) - k, and min(hi_a, hi_b) for upper bound.
            float k = std::max(0.0001f, n.t); // use n.t as k
            return Interval(std::min(a.lo, b.lo) - k, std::min(a.hi, b.hi));
        }
        default:
            return retInf();
    }
}

glm::vec3 sdfNormal(const SdfNode& n, const glm::vec3& p) {
    const float e = 1e-3f;
    glm::vec3 g(evalSdf(n, p + glm::vec3(e, 0, 0)) - evalSdf(n, p - glm::vec3(e, 0, 0)),
                evalSdf(n, p + glm::vec3(0, e, 0)) - evalSdf(n, p - glm::vec3(0, e, 0)),
                evalSdf(n, p + glm::vec3(0, 0, e)) - evalSdf(n, p - glm::vec3(0, 0, e)));
    float len = glm::length(g);
    return len > 1e-8f ? g / len : glm::vec3(0, 1, 0);
}

// ---------------------------------------------------------------------------
// Raycast (sphere tracing)
// ---------------------------------------------------------------------------
// Does this tree contain an implicit-expression leaf? Sphere tracing assumes the
// field is a DISTANCE (1-Lipschitz), so a full step can never overshoot. An
// implicit f(x,y,z)=0 leaf breaks that assumption: it is an iso-surface value that
// can be arbitrarily larger than the true distance, and a full step then tunnels
// straight through the surface. Detecting one lets the marcher damp its steps only
// where it must, instead of halving speed for every ordinary field.
static bool containsExpr(const SdfNode& n) {
    if (n.op == SdfOp::Leaf) return n.prim == SdfPrim::Expr;
    for (const auto& c : n.children) if (c && containsExpr(*c)) return true;
    return false;
}

bool raycastSdf(const SdfNode& n, const glm::vec3& o, const glm::vec3& d,
                float& tHit, glm::vec3& nrm) {
    glm::vec3 dir = glm::normalize(d);
    float t = 0.0f;
    const float maxT = 200.0f;
    // Matches the WebGPU marcher, so a picked point and a rendered point agree
    // about where the surface is.
    const bool gradientStep = containsExpr(n);
    for (int i = 0; i < 256 && t < maxT; ++i) {
        glm::vec3 p = o + dir * t;
        float dist = evalSdf(n, p);
        if (gradientStep) {
            // f/|grad f|: a first-order distance estimate for a field that is only
            // an iso-surface value. Without it the very first step from a typical
            // camera leaps past the whole shape.
            const float e = 1e-3f;
            glm::vec3 g((evalSdf(n, p + glm::vec3(e,0,0)) - evalSdf(n, p - glm::vec3(e,0,0))),
                        (evalSdf(n, p + glm::vec3(0,e,0)) - evalSdf(n, p - glm::vec3(0,e,0))),
                        (evalSdf(n, p + glm::vec3(0,0,e)) - evalSdf(n, p - glm::vec3(0,0,e))));
            g /= (2.0f * e);
            const float gl = glm::length(g);
            if (gl > 1e-6f) dist /= gl;
        }
        if (dist < 1e-4f) {
            tHit = t;
            nrm = sdfNormal(n, p);
            return true;
        }
        t += std::max(dist, 1e-4f);
    }
    return false;
}

// ---------------------------------------------------------------------------
// Tessellation — marching tetrahedra (arbitrary topology).
// ---------------------------------------------------------------------------

// Emit the iso-surface inside one tetrahedron (corners p[4], field values v[4], normals nrm[4]).
static void marchTet(TessMesh& m, const glm::vec3 p[4], const float v[4], const glm::vec3 nrm[4]) {
    int in[4], out[4], ni = 0, no = 0;
    for (int i = 0; i < 4; ++i) (v[i] < 0.0f ? in[ni++] : out[no++]) = i;
    if (ni == 0 || ni == 4) return;

    auto cut = [&](int a, int b) {
        float t = v[a] / (v[a] - v[b]);
        return std::make_pair(p[a] + (p[b] - p[a]) * t, glm::normalize(nrm[a] + (nrm[b] - nrm[a]) * t));
    };
    auto emit = [&](const std::pair<glm::vec3, glm::vec3>& a, 
                    const std::pair<glm::vec3, glm::vec3>& b, 
                    const std::pair<glm::vec3, glm::vec3>& c) {
        glm::vec3 centroidNrm = a.second + b.second + c.second;
        if (glm::dot(centroidNrm, centroidNrm) > 1e-8f) centroidNrm = glm::normalize(centroidNrm);
        else centroidNrm = glm::vec3(0,1,0);
        glm::vec3 face = glm::cross(b.first - a.first, c.first - a.first);
        auto aa = a, bb = b, cc = c;
        if (glm::dot(face, centroidNrm) < 0.0f) std::swap(bb, cc);          // front faces outward
        TessVertex va; va.pos = aa.first; va.normal = aa.second; va.uv = glm::vec2(0.5f);
        TessVertex vb; vb.pos = bb.first; vb.normal = bb.second; vb.uv = glm::vec2(0.5f);
        TessVertex vc; vc.pos = cc.first; vc.normal = cc.second; vc.uv = glm::vec2(0.5f);
        m.tris.push_back(va);
        m.tris.push_back(vb);
        m.tris.push_back(vc);
    };

    if (ni == 1) {
        emit(cut(in[0], out[0]), cut(in[0], out[1]), cut(in[0], out[2]));
    } else if (ni == 3) {
        emit(cut(out[0], in[0]), cut(out[0], in[1]), cut(out[0], in[2]));
    } else { // ni == 2 → quad
        auto ac = cut(in[0], out[0]), ad = cut(in[0], out[1]);
        auto bd = cut(in[1], out[1]), bc = cut(in[1], out[0]);
        emit(ac, ad, bd);
        emit(ac, bd, bc);
    }
}


TessMesh tessellateSdf(const SdfNode& n, const glm::vec3& extent, const glm::ivec3& res) {
    TessMesh m;
    glm::ivec3 N = glm::max(glm::ivec3(4), res);
    glm::vec3 step = 2.0f * extent / glm::vec3(N);
    auto pos = [&](int i, int j, int k) {
        return glm::vec3(-extent.x + i * step.x, -extent.y + j * step.y, -extent.z + k * step.z);
    };

    struct Sample {
        float v;
        glm::vec3 nrm;
    };
    std::unordered_map<uint64_t, Sample> cache;
    auto gi = [&](int i, int j, int k) -> uint64_t { 
        return (static_cast<uint64_t>(i & 0x7FFFFF) << 40) | (static_cast<uint64_t>(j & 0xFFFFF) << 20) | (k & 0xFFFFF);
    };

    auto getV = [&](int i, int j, int k) -> float {
        uint64_t id = gi(i, j, k);
        auto it = cache.find(id);
        if (it != cache.end()) return it->second.v;
        float val = evalSdf(n, pos(i, j, k));
        cache[id] = {val, glm::vec3(0,1,0)};
        return val;
    };

    auto getNormal = [&](int i, int j, int k) -> glm::vec3 {
        // Grid gradient exactly as Phase 3
        const int i0 = i > 0 ? i - 1 : i, i1 = i + 1 <= N.x ? i + 1 : i;
        const int j0 = j > 0 ? j - 1 : j, j1 = j + 1 <= N.y ? j + 1 : j;
        const int k0 = k > 0 ? k - 1 : k, k1 = k + 1 <= N.z ? k + 1 : k;
        glm::vec3 grad(
            i1 > i0 ? (getV(i1,j,k) - getV(i0,j,k)) / ((i1 - i0) * step.x) : 0.0f,
            j1 > j0 ? (getV(i,j1,k) - getV(i,j0,k)) / ((j1 - j0) * step.y) : 0.0f,
            k1 > k0 ? (getV(i,j,k1) - getV(i,j,k0)) / ((k1 - k0) * step.z) : 0.0f);
        float len = glm::length(grad);
        return len > 1e-8f ? grad / len : glm::vec3(0,1,0);
    };

    // Subdivide
    auto subdivide = [&](auto& self, int i0, int i1, int j0, int j1, int k0, int k1) -> void {
        glm::vec3 boxMin = pos(i0, j0, k0);
        glm::vec3 boxMax = pos(i1, j1, k1);
        OntoMath::Interval range = evalRange(n, boxMin, boxMax);
        if (range.lo > 0.0f || range.hi < 0.0f) return;
        
        if (i1 - i0 == 1 && j1 - j0 == 1 && k1 - k0 == 1) {
            // Corner offset per index c: (c&1, (c>>1)&1, (c>>2)&1).
            static const int off[8][3] = { {0,0,0},{1,0,0},{0,1,0},{1,1,0},{0,0,1},{1,0,1},{0,1,1},{1,1,1} };
            static const int tets[6][4] = { {0,7,1,3},{0,7,3,2},{0,7,2,6},{0,7,6,4},{0,7,4,5},{0,7,5,1} };
            
            glm::vec3 cp[8], cn[8]; float cv[8];
            for (int c = 0; c < 8; ++c) {
                int ci = i0 + off[c][0], cj = j0 + off[c][1], ck = k0 + off[c][2];
                cp[c] = pos(ci, cj, ck);
                cv[c] = getV(ci, cj, ck);
            }
            // Now compute normals for the 8 corners
            for (int c = 0; c < 8; ++c) {
                int ci = i0 + off[c][0], cj = j0 + off[c][1], ck = k0 + off[c][2];
                cn[c] = getNormal(ci, cj, ck);
            }
            
            for (int t = 0; t < 6; ++t) {
                glm::vec3 tp[4], tn[4]; float tv[4];
                for (int q = 0; q < 4; ++q) { 
                    tp[q] = cp[tets[t][q]]; 
                    tv[q] = cv[tets[t][q]]; 
                    tn[q] = cn[tets[t][q]]; 
                }
                marchTet(m, tp, tv, tn);
            }
            return;
        }
        
        int imid = i0 + std::max(1, (i1 - i0) / 2);
        int jmid = j0 + std::max(1, (j1 - j0) / 2);
        int kmid = k0 + std::max(1, (k1 - k0) / 2);
        
        if (imid < i1) {
            if (jmid < j1) {
                if (kmid < k1) {
                    self(self, i0, imid, j0, jmid, k0, kmid);
                    self(self, imid, i1, j0, jmid, k0, kmid);
                    self(self, i0, imid, jmid, j1, k0, kmid);
                    self(self, imid, i1, jmid, j1, k0, kmid);
                    self(self, i0, imid, j0, jmid, kmid, k1);
                    self(self, imid, i1, j0, jmid, kmid, k1);
                    self(self, i0, imid, jmid, j1, kmid, k1);
                    self(self, imid, i1, jmid, j1, kmid, k1);
                } else {
                    self(self, i0, imid, j0, jmid, k0, k1);
                    self(self, imid, i1, j0, jmid, k0, k1);
                    self(self, i0, imid, jmid, j1, k0, k1);
                    self(self, imid, i1, jmid, j1, k0, k1);
                }
            } else {
                if (kmid < k1) {
                    self(self, i0, imid, j0, j1, k0, kmid);
                    self(self, imid, i1, j0, j1, k0, kmid);
                    self(self, i0, imid, j0, j1, kmid, k1);
                    self(self, imid, i1, j0, j1, kmid, k1);
                } else {
                    self(self, i0, imid, j0, j1, k0, k1);
                    self(self, imid, i1, j0, j1, k0, k1);
                }
            }
        } else {
            if (jmid < j1) {
                if (kmid < k1) {
                    self(self, i0, i1, j0, jmid, k0, kmid);
                    self(self, i0, i1, jmid, j1, k0, kmid);
                    self(self, i0, i1, j0, jmid, kmid, k1);
                    self(self, i0, i1, jmid, j1, kmid, k1);
                } else {
                    self(self, i0, i1, j0, jmid, k0, k1);
                    self(self, i0, i1, jmid, j1, k0, k1);
                }
            } else {
                if (kmid < k1) {
                    self(self, i0, i1, j0, j1, k0, kmid);
                    self(self, i0, i1, j0, j1, kmid, k1);
                } else {
                    self(self, i0, i1, j0, j1, k0, k1);
                }
            }
        }
    };
    
    subdivide(subdivide, 0, N.x, 0, N.y, 0, N.z);
    
    return m;
}

bool sdfFromComplex(const ComplexShapeData& c, SdfNode& out) {
    const SurfacePatch* side = nullptr;
    int smoothN = 0, planarN = 0;
    for (const auto& p : c.patches) {
        if (p.type == SurfacePatch::Type::Smooth) {
            ++smoothN;
            side = &p;
        } else if (p.type == SurfacePatch::Type::Planar) {
            ++planarN;
        } else {
            return false; // fillet mesh patches are not a single SDF leaf
        }
    }
    if (!side || smoothN != 1) return false;
    const SmoothSurfaceData& s = side->smooth;
    const float r = s.axes.x > 1e-6f ? s.axes.x : 0.5f;
    const float halfH = std::max(std::abs(s.zTrim.x), std::abs(s.zTrim.y));
    if (s.form == SmoothSurfaceData::QuadricForm::CylinderSide && planarN == 2) {
        out = SdfNode::leaf(SdfPrim::Cylinder, glm::vec3(r, halfH, 0.0f));
        return true;
    }
    if (s.form == SmoothSurfaceData::QuadricForm::ConeSide && planarN == 1) {
        out = SdfNode::leaf(SdfPrim::Cone, glm::vec3(r, halfH, 0.0f));
        return true;
    }
    return false;
}

SdfNode sdfFromSmooth(const SmoothSurfaceData& s) {
    if (s.model == SmoothSurfaceData::Model::Parametric) {
        if (s.pkind == SmoothSurfaceData::ParametricKind::Torus) {
            float R = s.params.size() > 0 ? s.params[0] : s.axes.x;
            float r = s.params.size() > 1 ? s.params[1] : 0.15f;
            return SdfNode::leaf(SdfPrim::Torus, glm::vec3(R, r, 0.0f));
        }
        float rad = s.axes.x > 1e-6f ? s.axes.x : 0.5f;
        return SdfNode::leaf(SdfPrim::Sphere, glm::vec3(rad));
    }
    switch (s.form) {
        case SmoothSurfaceData::QuadricForm::Ellipsoid:
            return SdfNode::leaf(SdfPrim::Ellipsoid, s.axes);
        case SmoothSurfaceData::QuadricForm::CylinderSide:
            return SdfNode::leaf(SdfPrim::Cylinder, glm::vec3(s.axes.x, s.zTrim.y, 0.0f));
        case SmoothSurfaceData::QuadricForm::ConeSide:
            return SdfNode::leaf(SdfPrim::Cone, glm::vec3(s.axes.x, s.zTrim.y, 0.0f));
        case SmoothSurfaceData::QuadricForm::Paraboloid:
        case SmoothSurfaceData::QuadricForm::Sphere:
        default:
            return SdfNode::leaf(SdfPrim::Sphere, glm::vec3(s.axes.x > 1e-6f ? s.axes.x : 0.5f));
    }
}

} // namespace geom
