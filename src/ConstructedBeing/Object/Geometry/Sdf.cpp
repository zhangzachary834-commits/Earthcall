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

std::shared_ptr<OntoMath::MathNode> rpnToMathNode(const std::vector<SdfToken>& r) {
    if (r.empty()) return nullptr;
    std::vector<std::shared_ptr<OntoMath::MathNode>> st;
    for (const auto& t : r) {
        switch (t.kind) {
            case SdfToken::Num: {
                auto n = std::make_shared<OntoMath::MathNode>();
                n->op = OntoMath::MathNode::Op::ScalarLeaf;
                n->scalarForm = OntoMath::ScalarForm::constant(t.num);
                st.push_back(std::move(n));
                break;
            }
            case SdfToken::X: {
                auto n = std::make_shared<OntoMath::MathNode>();
                n->op = OntoMath::MathNode::Op::ValueLeaf;
                n->variableName = "x";
                st.push_back(std::move(n));
                break;
            }
            case SdfToken::Y: {
                auto n = std::make_shared<OntoMath::MathNode>();
                n->op = OntoMath::MathNode::Op::ValueLeaf;
                n->variableName = "y";
                st.push_back(std::move(n));
                break;
            }
            case SdfToken::Z: {
                auto n = std::make_shared<OntoMath::MathNode>();
                n->op = OntoMath::MathNode::Op::ValueLeaf;
                n->variableName = "z";
                st.push_back(std::move(n));
                break;
            }
            case SdfToken::Add: {
                if (st.size() >= 2) {
                    auto b = st.back(); st.pop_back();
                    auto a = st.back(); st.pop_back();
                    auto n = std::make_shared<OntoMath::MathNode>();
                    n->op = OntoMath::MathNode::Op::Add;
                    n->children.push_back(std::make_unique<OntoMath::MathNode>(*a));
                    n->children.push_back(std::make_unique<OntoMath::MathNode>(*b));
                    st.push_back(std::move(n));
                }
                break;
            }
            case SdfToken::Sub: {
                if (st.size() >= 2) {
                    auto b = st.back(); st.pop_back();
                    auto a = st.back(); st.pop_back();
                    auto n = std::make_shared<OntoMath::MathNode>();
                    n->op = OntoMath::MathNode::Op::Sub;
                    n->children.push_back(std::make_unique<OntoMath::MathNode>(*a));
                    n->children.push_back(std::make_unique<OntoMath::MathNode>(*b));
                    st.push_back(std::move(n));
                }
                break;
            }
            case SdfToken::Mul: {
                if (st.size() >= 2) {
                    auto b = st.back(); st.pop_back();
                    auto a = st.back(); st.pop_back();
                    auto n = std::make_shared<OntoMath::MathNode>();
                    n->op = OntoMath::MathNode::Op::Scale;
                    n->children.push_back(std::make_unique<OntoMath::MathNode>(*a));
                    n->children.push_back(std::make_unique<OntoMath::MathNode>(*b));
                    st.push_back(std::move(n));
                }
                break;
            }
            case SdfToken::Div: {
                if (st.size() >= 2) {
                    auto b = st.back(); st.pop_back();
                    auto a = st.back(); st.pop_back();
                    auto n = std::make_shared<OntoMath::MathNode>();
                    n->op = OntoMath::MathNode::Op::Scale;
                    n->children.push_back(std::make_unique<OntoMath::MathNode>(*a));
                    n->children.push_back(std::make_unique<OntoMath::MathNode>(*b));
                    st.push_back(std::move(n));
                }
                break;
            }
            case SdfToken::Pow: {
                if (st.size() >= 2) {
                    auto b = st.back(); st.pop_back();
                    auto a = st.back(); st.pop_back();
                    if (a->op == OntoMath::MathNode::Op::ValueLeaf && b->op == OntoMath::MathNode::Op::ScalarLeaf && b->scalarForm.terms.size() == 1) {
                        double exp = b->scalarForm.terms[0].coefficient;
                        auto n = std::make_shared<OntoMath::MathNode>();
                        n->op = OntoMath::MathNode::Op::ScalarLeaf;
                        n->scalarForm = OntoMath::ScalarForm::variable(a->variableName, exp);
                        st.push_back(std::move(n));
                    } else {
                        auto n = std::make_shared<OntoMath::MathNode>();
                        n->op = OntoMath::MathNode::Op::Scale;
                        n->children.push_back(std::make_unique<OntoMath::MathNode>(*a));
                        n->children.push_back(std::make_unique<OntoMath::MathNode>(*b));
                        st.push_back(std::move(n));
                    }
                }
                break;
            }
            case SdfToken::Neg: {
                if (!st.empty()) {
                    auto a = st.back(); st.pop_back();
                    auto neg1 = std::make_shared<OntoMath::MathNode>();
                    neg1->op = OntoMath::MathNode::Op::ScalarLeaf;
                    neg1->scalarForm = OntoMath::ScalarForm::constant(-1.0);
                    auto n = std::make_shared<OntoMath::MathNode>();
                    n->op = OntoMath::MathNode::Op::Scale;
                    n->children.push_back(std::make_unique<OntoMath::MathNode>(*neg1));
                    n->children.push_back(std::make_unique<OntoMath::MathNode>(*a));
                    st.push_back(std::move(n));
                }
                break;
            }
            case SdfToken::Sin: {
                if (!st.empty()) {
                    auto a = st.back(); st.pop_back();
                    if (a->op == OntoMath::MathNode::Op::ValueLeaf) {
                        auto n = std::make_shared<OntoMath::MathNode>();
                        n->op = OntoMath::MathNode::Op::ScalarLeaf;
                        n->scalarForm = OntoMath::ScalarForm::transcendental(OntoMath::TransFactor::Kind::Sin, a->variableName);
                        st.push_back(std::move(n));
                    } else {
                        st.push_back(a);
                    }
                }
                break;
            }
            case SdfToken::Cos: {
                if (!st.empty()) {
                    auto a = st.back(); st.pop_back();
                    if (a->op == OntoMath::MathNode::Op::ValueLeaf) {
                        auto n = std::make_shared<OntoMath::MathNode>();
                        n->op = OntoMath::MathNode::Op::ScalarLeaf;
                        n->scalarForm = OntoMath::ScalarForm::transcendental(OntoMath::TransFactor::Kind::Cos, a->variableName);
                        st.push_back(std::move(n));
                    } else {
                        st.push_back(a);
                    }
                }
                break;
            }
            case SdfToken::Exp: {
                if (!st.empty()) {
                    auto a = st.back(); st.pop_back();
                    if (a->op == OntoMath::MathNode::Op::ValueLeaf) {
                        auto n = std::make_shared<OntoMath::MathNode>();
                        n->op = OntoMath::MathNode::Op::ScalarLeaf;
                        n->scalarForm = OntoMath::ScalarForm::transcendental(OntoMath::TransFactor::Kind::Exp, a->variableName);
                        st.push_back(std::move(n));
                    } else {
                        st.push_back(a);
                    }
                }
                break;
            }
            case SdfToken::Log: {
                if (!st.empty()) {
                    auto a = st.back(); st.pop_back();
                    if (a->op == OntoMath::MathNode::Op::ValueLeaf) {
                        auto n = std::make_shared<OntoMath::MathNode>();
                        n->op = OntoMath::MathNode::Op::ScalarLeaf;
                        n->scalarForm = OntoMath::ScalarForm::transcendental(OntoMath::TransFactor::Kind::Ln, a->variableName);
                        st.push_back(std::move(n));
                    } else {
                        st.push_back(a);
                    }
                }
                break;
            }
            default:
                break;
        }
    }
    return st.empty() ? nullptr : st.back();
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
    return std::shared_ptr<OntoMath::MathNode>(OntoMath::MathNode::sphere(0.5));
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
                std::map<std::string, PropertyValue> vars;
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
static TessVertex mkVert(const SdfNode& n, const glm::vec3& p) {
    TessVertex v; v.pos = p; v.normal = sdfNormal(n, p); v.uv = glm::vec2(0.5f); return v;
}

// Emit the iso-surface inside one tetrahedron (corners p[4], field values v[4]).
static void marchTet(const SdfNode& n, TessMesh& m,
                     const glm::vec3 p[4], const float v[4]) {
    int in[4], out[4], ni = 0, no = 0;
    for (int i = 0; i < 4; ++i) (v[i] < 0.0f ? in[ni++] : out[no++]) = i;
    if (ni == 0 || ni == 4) return;

    auto cut = [&](int a, int b) {
        float t = v[a] / (v[a] - v[b]);
        return p[a] + (p[b] - p[a]) * t;
    };
    auto emit = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c) {
        glm::vec3 centroid = (a + b + c) / 3.0f;
        glm::vec3 nrm = sdfNormal(n, centroid);                   // outward (grad of f)
        glm::vec3 face = glm::cross(b - a, c - a);
        if (glm::dot(face, nrm) < 0.0f) std::swap(b, c);          // front faces outward
        m.tris.push_back(mkVert(n, a));
        m.tris.push_back(mkVert(n, b));
        m.tris.push_back(mkVert(n, c));
    };

    if (ni == 1) {
        glm::vec3 a = cut(in[0], out[0]), b = cut(in[0], out[1]), c = cut(in[0], out[2]);
        emit(a, b, c);
    } else if (ni == 3) {
        glm::vec3 a = cut(out[0], in[0]), b = cut(out[0], in[1]), c = cut(out[0], in[2]);
        emit(a, b, c);
    } else { // ni == 2 → quad
        glm::vec3 ac = cut(in[0], out[0]), ad = cut(in[0], out[1]);
        glm::vec3 bd = cut(in[1], out[1]), bc = cut(in[1], out[0]);
        emit(ac, ad, bd);
        emit(ac, bd, bc);
    }
}

TessMesh tessellateSdf(const SdfNode& n, float extent, int res) {
    TessMesh m;
    int N = std::max(4, res);
    float step = 2.0f * extent / N;
    auto pos = [&](int i, int j, int k) {
        return glm::vec3(-extent + i * step, -extent + j * step, -extent + k * step);
    };
    // Precompute the (N+1)^3 field samples so shared corners are evaluated once.
    int S = N + 1;
    std::vector<float> g(static_cast<size_t>(S) * S * S);
    auto gi = [&](int i, int j, int k) { return (static_cast<size_t>(i) * S + j) * S + k; };
    for (int i = 0; i < S; ++i)
        for (int j = 0; j < S; ++j)
            for (int k = 0; k < S; ++k)
                g[gi(i, j, k)] = evalSdf(n, pos(i, j, k));

    // Corner offset per index c: (c&1, (c>>1)&1, (c>>2)&1).
    static const int off[8][3] = {
        {0,0,0},{1,0,0},{0,1,0},{1,1,0},{0,0,1},{1,0,1},{0,1,1},{1,1,1}
    };
    // 6 tetrahedra sharing the cube's main diagonal (corners 0 and 7).
    static const int tets[6][4] = {
        {0,7,1,3},{0,7,3,2},{0,7,2,6},{0,7,6,4},{0,7,4,5},{0,7,5,1}
    };
    for (int x = 0; x < N; ++x)
    for (int y = 0; y < N; ++y)
    for (int z = 0; z < N; ++z) {
        glm::vec3 cp[8]; float cv[8];
        for (int c = 0; c < 8; ++c) {
            int ci = x + off[c][0], cj = y + off[c][1], ck = z + off[c][2];
            cp[c] = pos(ci, cj, ck);
            cv[c] = g[gi(ci, cj, ck)];
        }
        for (int t = 0; t < 6; ++t) {
            glm::vec3 tp[4]; float tv[4];
            for (int q = 0; q < 4; ++q) { tp[q] = cp[tets[t][q]]; tv[q] = cv[tets[t][q]]; }
            marchTet(n, m, tp, tv);
        }
    }
    return m;
}

} // namespace geom
