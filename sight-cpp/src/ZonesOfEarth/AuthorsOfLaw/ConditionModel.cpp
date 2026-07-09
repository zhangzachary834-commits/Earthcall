#include "ConditionModel.hpp"

#include "Form/Object/Geometry/SdfJson.hpp"
#include "Form/Singular/Property/PropertyValueJson.hpp"

#include <cmath>
#include <utility>

namespace {

const char* opName(ConditionNode::Op op) {
    switch (op) {
        case ConditionNode::Op::Eq: return "==";
        case ConditionNode::Op::Ne: return "!=";
        case ConditionNode::Op::Lt: return "<";
        case ConditionNode::Op::Le: return "<=";
        case ConditionNode::Op::Gt: return ">";
        case ConditionNode::Op::Ge: return ">=";
        case ConditionNode::Op::Near: return "near";
        case ConditionNode::Op::InRange: return "in-range";
    }
    return "?";
}

} // namespace

nlohmann::json ConditionNode::toJson() const {
    nlohmann::json j{{"kind", static_cast<int>(kind)}};
    switch (kind) {
        case Kind::Compare:
            j["path"] = path.toString();
            j["op"] = static_cast<int>(op);
            j["operand"] = propertyValueToJson(operand);
            if (!operandPath.empty()) j["operandPath"] = operandPath.toString();
            if (op == Op::Near) j["tolerance"] = tolerance;
            if (op == Op::InRange) {
                j["lo"] = propertyValueToJson(lo);
                j["hi"] = propertyValueToJson(hi);
            }
            break;
        case Kind::InRegion:
            j["region"] = geom::sdfToJson(region);
            j["probe"] = probe.empty() ? "position" : probe.toString();
            break;
        case Kind::Related:
            j["relationType"] = relationType;
            j["otherId"] = otherId;
            break;
        case Kind::All:
        case Kind::Any:
        case Kind::Not: {
            nlohmann::json kids = nlohmann::json::array();
            for (const auto& c : children) kids.push_back(c.toJson());
            j["children"] = kids;
            break;
        }
    }
    return j;
}

ConditionNode ConditionNode::fromJson(const nlohmann::json& j) {
    ConditionNode n;
    n.kind = static_cast<Kind>(j.value("kind", 0));
    if (j.contains("path")) n.path = PropertyPath::parse(j["path"].get<std::string>());
    n.op = static_cast<Op>(j.value("op", 0));
    if (j.contains("operand")) n.operand = propertyValueFromJson(j["operand"]);
    if (j.contains("operandPath")) n.operandPath = PropertyPath::parse(j["operandPath"].get<std::string>());
    n.tolerance = j.value("tolerance", 0.0);
    if (j.contains("lo")) n.lo = propertyValueFromJson(j["lo"]);
    if (j.contains("hi")) n.hi = propertyValueFromJson(j["hi"]);
    if (j.contains("region")) n.region = geom::sdfFromJson(j["region"]);
    if (j.contains("probe")) n.probe = PropertyPath::parse(j["probe"].get<std::string>());
    n.relationType = j.value("relationType", std::string());
    n.otherId = j.value("otherId", std::string());
    if (j.contains("children")) {
        for (const auto& c : j["children"]) n.children.push_back(fromJson(c));
    }
    return n;
}

ECA::ConditionPredicate ConditionNode::compile() const {
    switch (kind) {
        case Kind::Compare: {
            const PropertyPath lhsPath = path;
            const Op o = op;
            const PropertyValue rhsLiteral = operand;
            const PropertyPath rhsPath = operandPath;
            const double tol = tolerance;
            const PropertyValue vlo = lo, vhi = hi;
            return [lhsPath, o, rhsLiteral, rhsPath, tol, vlo, vhi](
                       const ECA::Event&, const Singular& target) {
                Singular& t = const_cast<Singular&>(target);
                PropertyValue lhs;
                if (!lhsPath.getValue(t, lhs)) return false;
                PropertyValue rhs = rhsLiteral;
                if (!rhsPath.empty() && !rhsPath.getValue(t, rhs)) return false;

                double a = 0.0, b = 0.0;
                const bool numeric =
                    propertyValueToNumber(lhs, a) && propertyValueToNumber(rhs, b);
                switch (o) {
                    case Op::Eq: return numeric ? a == b : lhs == rhs;
                    case Op::Ne: return numeric ? a != b : !(lhs == rhs);
                    case Op::Lt: return numeric && a < b;
                    case Op::Le: return numeric && a <= b;
                    case Op::Gt: return numeric && a > b;
                    case Op::Ge: return numeric && a >= b;
                    case Op::Near: return numeric && std::fabs(a - b) <= tol;
                    case Op::InRange: {
                        double l = 0.0, h = 0.0;
                        return numeric && propertyValueToNumber(vlo, l) &&
                               propertyValueToNumber(vhi, h) && a >= l && a <= h;
                    }
                }
                return false;
            };
        }
        case Kind::InRegion: {
            const geom::SdfNode r = region;   // deep copy — the law owns its criterion
            const PropertyPath pr = probe.empty() ? PropertyPath::parse("position") : probe;
            return [r, pr](const ECA::Event&, const Singular& target) {
                PropertyValue v;
                if (!pr.getValue(const_cast<Singular&>(target), v)) return false;
                const glm::vec3* point = std::get_if<glm::vec3>(&v);
                return point && geom::evalSdf(r, *point) < 0.0f;
            };
        }
        case Kind::Related: {
            // Recorded but not yet resolvable: relation-graph lookup arrives
            // with the event/Rete wiring (commit 4). Until then a Related
            // condition never passes — laws must not fire on unproven relations.
            return [](const ECA::Event&, const Singular&) { return false; };
        }
        case Kind::All:
        case Kind::Any:
        case Kind::Not: {
            std::vector<ECA::ConditionPredicate> compiled;
            compiled.reserve(children.size());
            for (const auto& c : children) compiled.push_back(c.compile());
            const Kind k = kind;
            return [compiled, k](const ECA::Event& e, const Singular& target) {
                if (k == Kind::Not) {
                    return !compiled.empty() && !compiled[0](e, target);
                }
                if (k == Kind::All) {
                    for (const auto& p : compiled) {
                        if (!p || !p(e, target)) return false;
                    }
                    return true;
                }
                for (const auto& p : compiled) {           // Any
                    if (p && p(e, target)) return true;
                }
                return false;
            };
        }
    }
    return [](const ECA::Event&, const Singular&) { return false; };
}

std::string ConditionNode::describe() const {
    switch (kind) {
        case Kind::Compare: {
            std::string rhs = operandPath.empty() ? std::string("value") : operandPath.toString();
            double n = 0.0;
            if (operandPath.empty() && propertyValueToNumber(operand, n)) {
                rhs = std::to_string(n);
            }
            return path.toString() + " " + opName(op) + " " + rhs;
        }
        case Kind::InRegion:
            return "in-region(" + (probe.empty() ? std::string("position") : probe.toString()) + ")";
        case Kind::Related:
            return "related(" + relationType + ", " + otherId + ")";
        case Kind::All: return "all(" + std::to_string(children.size()) + ")";
        case Kind::Any: return "any(" + std::to_string(children.size()) + ")";
        case Kind::Not: return "not(...)";
    }
    return "condition";
}

ConditionNode ConditionNode::compare(const std::string& dottedPath, Op op, PropertyValue rhs) {
    ConditionNode n;
    n.kind = Kind::Compare;
    n.path = PropertyPath::parse(dottedPath);
    n.op = op;
    n.operand = std::move(rhs);
    return n;
}

ConditionNode ConditionNode::comparePaths(const std::string& dottedPath, Op op,
                                          const std::string& rhsPath) {
    ConditionNode n;
    n.kind = Kind::Compare;
    n.path = PropertyPath::parse(dottedPath);
    n.op = op;
    n.operandPath = PropertyPath::parse(rhsPath);
    return n;
}

ConditionNode ConditionNode::inRegion(geom::SdfNode region, const std::string& probePath) {
    ConditionNode n;
    n.kind = Kind::InRegion;
    n.region = std::move(region);
    n.probe = PropertyPath::parse(probePath);
    return n;
}

ConditionNode ConditionNode::all(std::vector<ConditionNode> children) {
    ConditionNode n;
    n.kind = Kind::All;
    n.children = std::move(children);
    return n;
}

ConditionNode ConditionNode::any(std::vector<ConditionNode> children) {
    ConditionNode n;
    n.kind = Kind::Any;
    n.children = std::move(children);
    return n;
}

ConditionNode ConditionNode::negate(ConditionNode child) {
    ConditionNode n;
    n.kind = Kind::Not;
    n.children.push_back(std::move(child));
    return n;
}
