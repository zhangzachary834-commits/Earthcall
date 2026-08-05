#include "ConditionModel.hpp"

#include "Form/Object/Geometry/SdfJson.hpp"
#include "Form/Object/Formation/Formation.hpp"
#include "Form/Object/Object.hpp"
#include "Form/Singular/Property/PropertyValueJson.hpp"
#include "Law.hpp"
#include "LawAuditLogger.hpp"
#include "Person/Person.hpp"
#include "Relation/Relation.hpp"
#include "Universe.hpp"
#include "ZonesOfEarth/Physics/CollisionDispatcher.hpp"
#include "ZonesOfEarth/World/World.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Singularity/Language/Lexeme.hpp"

#include <algorithm>
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

// Honest C++ instanceof — the ontology's kinds checked at runtime.
bool matchesKindImpl(const Singular& being, ConditionNode::BeingKind kind) {
    switch (kind) {
        case ConditionNode::BeingKind::AnyBeing:  return true;
        case ConditionNode::BeingKind::Object:    return dynamic_cast<const Object*>(&being) != nullptr;
        case ConditionNode::BeingKind::Person:    return dynamic_cast<const Person*>(&being) != nullptr;
        case ConditionNode::BeingKind::Relation:  return dynamic_cast<const Relation*>(&being) != nullptr;
        case ConditionNode::BeingKind::Formation: return dynamic_cast<const Formation*>(&being) != nullptr;
        case ConditionNode::BeingKind::Law:       return dynamic_cast<const Law*>(&being) != nullptr;
        case ConditionNode::BeingKind::World:     return dynamic_cast<const World*>(&being) != nullptr;
        case ConditionNode::BeingKind::Zone:      return dynamic_cast<const Zone*>(&being) != nullptr;
        case ConditionNode::BeingKind::Lexeme:    return dynamic_cast<const Singularity::Language::Lexeme*>(&being) != nullptr;
    }
    return false;
}

const char* beingKindName(ConditionNode::BeingKind kind) {
    switch (kind) {
        case ConditionNode::BeingKind::AnyBeing:  return "being";
        case ConditionNode::BeingKind::Object:    return "Object";
        case ConditionNode::BeingKind::Person:    return "Person";
        case ConditionNode::BeingKind::Relation:  return "Relation";
        case ConditionNode::BeingKind::Formation: return "Formation";
        case ConditionNode::BeingKind::Law:       return "Law";
        case ConditionNode::BeingKind::World:     return "World";
        case ConditionNode::BeingKind::Zone:      return "Zone";
        case ConditionNode::BeingKind::Lexeme:    return "Lexeme";
    }
    return "?";
}

// A participant token names a being: "@event.subject" / "@event.object"
// resolve through the application-event context; anything else is a being id
// looked up in the Universe. Unproven = nullptr — a condition never passes
// and an action never acts on a referent the world cannot produce.
Singular* resolveParticipantToken(const std::string& token) {
    if (token.empty()) return nullptr;
    if (token == "@event.subject" || token == "@event.object") {
        if (!Universe::instance().hasApplicationEvent()) return nullptr;
        return token == "@event.subject"
                   ? Universe::instance().applicationEventSubject()
                   : Universe::instance().applicationEventObject();
    }
    for (Singular* being : Universe::instance().beings()) {
        if (being && being->getIdentifier() == token) return being;
    }
    return nullptr;
}

// Kinds this build can actually evaluate. Deliberately a whitelist of the
// live values rather than a `< count` range check: 12 and 13 are retired and
// must keep failing this test even though they sit inside the range.
bool isKnownConditionKind(int raw) {
    switch (static_cast<ConditionNode::Kind>(raw)) {
        case ConditionNode::Kind::Compare:
        case ConditionNode::Kind::InRegion:
        case ConditionNode::Kind::Related:
        case ConditionNode::Kind::All:
        case ConditionNode::Kind::Any:
        case ConditionNode::Kind::Not:
        case ConditionNode::Kind::Zone:
        case ConditionNode::Kind::IsKind:
        case ConditionNode::Kind::Identity:
        case ConditionNode::Kind::ForAny:
        case ConditionNode::Kind::ForAll:
        case ConditionNode::Kind::Overlaps:
            return true;
        // Unsupported is where unknown kinds LAND; it is never a stored value
        // an author picked, so it does not read back as known.
        case ConditionNode::Kind::Unsupported:
            return false;
    }
    return false;
}

} // namespace

nlohmann::json ConditionNode::toJson() const {
    // A kind this build cannot read is handed back exactly as it arrived.
    // Re-serializing it from our own fields would drop every payload key we
    // have no slot for, so merely OPENING a world in this build would destroy
    // law text written by another one.
    if (kind == Kind::Unsupported) {
        if (unsupported) return *unsupported;
        return nlohmann::json{{"kind", static_cast<int>(Kind::Unsupported)}};
    }

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
        case Kind::Overlaps:
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
        case Kind::Zone:
            j["function"] = zoneFunction.toJson();
            j["bindings"] = mathBindingsToJson(bindings);
            if (!std::holds_alternative<std::monostate>(lo)) j["lo"] = propertyValueToJson(lo);
            if (!std::holds_alternative<std::monostate>(hi)) j["hi"] = propertyValueToJson(hi);
            break;
        case Kind::IsKind:
            j["beingKind"] = static_cast<int>(beingKind);
            break;
        case Kind::Identity:
            j["otherId"] = otherId;
            break;
        case Kind::ForAny:
        case Kind::ForAll: {
            j["beingKind"] = static_cast<int>(beingKind);
            if (!exceptIds.empty()) j["except"] = exceptIds;
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

    // The stored kind is an ARBITRARY int from a file, not a Kind. Casting it
    // unchecked let a retired or future kind through as an out-of-range enum
    // that no switch matched — so it compiled to a silent constant false and
    // re-serialized as a stripped husk. Park it as Unsupported instead, keep
    // the JSON, and say so out loud.
    const int rawKind = j.value("kind", 0);
    if (!isKnownConditionKind(rawKind)) {
        n.kind = Kind::Unsupported;
        n.unsupported = std::make_shared<nlohmann::json>(j);
        ECA::LawAuditLogger::instance().log(
            "CONDITION",
            "Condition kind " + std::to_string(rawKind) +
                " is not supported by this build - the condition will never hold. "
                "Kinds 12 and 13 were the retired pair quantifiers; model pairs "
                "as Relations in the graph instead.",
            {{"kind", rawKind}});
        return n;
    }
    n.kind = static_cast<Kind>(rawKind);

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
    if (j.contains("function")) n.zoneFunction = OntoMath::Piecewise::fromJson(j["function"]);
    if (j.contains("bindings")) n.bindings = mathBindingsFromJson(j["bindings"]);
    n.beingKind = static_cast<BeingKind>(j.value("beingKind", 0));
    if (j.contains("except")) n.exceptIds = j["except"].get<std::vector<std::string>>();
    if (j.contains("children")) {
        for (const auto& c : j["children"]) n.children.push_back(fromJson(c));
    }
    return n;
}

bool ConditionNode::matchesKind(const Singular& being, BeingKind kind) {
    return matchesKindImpl(being, kind);
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
            const std::string desc = this->describe();
            return [lhsPath, o, rhsLiteral, rhsPath, tol, vlo, vhi, desc](
                       const ECA::Event&, const Singular& target) {
                Singular& t = const_cast<Singular&>(target);
                PropertyValue lhs;
                if (!lawGetValue(t, lhsPath, lhs)) {
                    ECA::LawAuditLogger::instance().log("CONDITION", "Condition Evaluated [FAIL - Property Not Found]: " + desc, {
                        {"targetId", t.getIdentifier()}, {"result", false}
                    });
                    return false;
                }
                PropertyValue rhs = rhsLiteral;
                if (!rhsPath.empty() && !lawGetValue(t, rhsPath, rhs)) {
                    ECA::LawAuditLogger::instance().log("CONDITION", "Condition Evaluated [FAIL - RHS Property Not Found]: " + desc, {
                        {"targetId", t.getIdentifier()}, {"result", false}
                    });
                    return false;
                }
                
                std::string lhsStr = "(unknown)";
                if (const std::string* s = std::get_if<std::string>(&lhs)) lhsStr = *s;
                else if (const double* d = std::get_if<double>(&lhs)) lhsStr = std::to_string(*d);

                double a = 0.0, b = 0.0;
                const bool numeric =
                    propertyValueToNumber(lhs, a) && propertyValueToNumber(rhs, b);
                
                bool res = false;
                switch (o) {
                    case Op::Eq: res = (numeric ? a == b : lhs == rhs); break;
                    case Op::Ne: res = (numeric ? a != b : !(lhs == rhs)); break;
                    case Op::Lt: res = (numeric && a < b); break;
                    case Op::Le: res = (numeric && a <= b); break;
                    case Op::Gt: res = (numeric && a > b); break;
                    case Op::Ge: res = (numeric && a >= b); break;
                    case Op::Near: res = (numeric && std::fabs(a - b) <= tol); break;
                    case Op::InRange: {
                        double l = 0.0, h = 0.0;
                        res = (numeric && propertyValueToNumber(vlo, l) &&
                               propertyValueToNumber(vhi, h) && a >= l && a <= h);
                        break;
                    }
                }
                std::string logMsg = "Condition Evaluated [" + std::string(res ? "PASS" : "FAIL") + "]: " + desc;
                if (!res) {
                    logMsg += " (LHS was: " + lhsStr + ")";
                }
                
                ECA::LawAuditLogger::instance().log("CONDITION", logMsg, {
                    {"targetId", t.getIdentifier()}, {"result", res}
                });
                return res;
            };
        }
        case Kind::InRegion: {
            const geom::SdfNode r = region;   // deep copy — the law owns its criterion
            const PropertyPath pr = probe.empty() ? PropertyPath::parse("position") : probe;
            return [r, pr](const ECA::Event&, const Singular& target) {
                PropertyValue v;
                if (!lawGetValue(const_cast<Singular&>(target), pr, v)) return false;
                const glm::vec3* point = std::get_if<glm::vec3>(&v);
                return point && geom::evalSdf(r, *point) < 0.0f;
            };
        }
        case Kind::Related: {
            // The graph-shaped condition: true when the subject participates
            // in a relation from the Universe's relation graph. Empty
            // relationType = any type; empty otherId = related to ANYONE;
            // otherId may also be "@event.subject" / "@event.object" — the
            // triggering event's participants, resolved at evaluation time
            // ("on collision, IF the subject is related to the one it hit").
            // Direction is honored: a directed relation satisfies only its
            // source ("a owns b" makes related(owns, b) true OF a, not of b).
            // No provider = no proven relations: never passes.
            const std::string type = relationType;
            const std::string otherSpec = otherId;
            return [type, otherSpec](const ECA::Event&, const Singular& subject) {
                std::string other = otherSpec;
                if (otherSpec == "@event.subject" || otherSpec == "@event.object") {
                    if (!Universe::instance().hasApplicationEvent()) return false;
                    Singular* participant =
                        otherSpec == "@event.subject"
                            ? Universe::instance().applicationEventSubject()
                            : Universe::instance().applicationEventObject();
                    if (!participant) return false;   // unproven referent
                    other = participant->getIdentifier();
                }
                const std::string id = subject.getIdentifier();
                for (const Relation* rel : Universe::instance().relations()) {
                    if (!rel) continue;
                    if (!type.empty() && rel->type != type) continue;
                    if (other.empty()) {
                        if (rel->directed ? rel->entityA == id : rel->involves(id)) {
                            return true;
                        }
                        continue;
                    }
                    if (rel->isBetween(id, other)) return true;
                }
                return false;
            };
        }
        case Kind::Overlaps: {
            // Geometric contact, answered by the engine's collision test —
            // a first mover shrunk to a pure predicate. Both participants
            // must be spatial Objects and PROVEN (an absent other never
            // touches anything).
            const std::string other = otherId;
            return [other](const ECA::Event&, const Singular& subject) {
                Singular* resolved = resolveParticipantToken(other);
                if (!resolved || resolved == &subject) return false;
                const auto* a = dynamic_cast<const Object*>(&subject);
                const auto* b = dynamic_cast<const Object*>(resolved);
                if (!a || !b) return false;
                return Physics::dispatchCollision(*a, *b).hit;
            };
        }
        case Kind::Zone: {
            // The satisfaction zone of an authored function: read every bound
            // variable off the subject, evaluate the (piecewise, multivariate)
            // expression exactly, and test the authored bounds. Undefined math
            // — an unbound variable or a point outside every piece — is never
            // satisfied.
            const OntoMath::Piecewise f = zoneFunction;
            const MathBindings binds = bindings;
            const PropertyValue zlo = lo, zhi = hi;
            return [f, binds, zlo, zhi](const ECA::Event&, const Singular& target) {
                auto vars = readMathBindings(const_cast<Singular&>(target), binds);
                if (!vars) return false;
                std::map<std::string, PropertyValue> pVars;
                for (const auto& [k, v] : *vars) pVars[k] = PropertyValue(v);
                const auto valProp = f.evaluate(pVars, &target);
                std::optional<double> value;
                if (valProp && std::holds_alternative<double>(*valProp)) value = std::get<double>(*valProp);
                if (!value) return false;
                double bound = 0.0;
                if (propertyValueToNumber(zlo, bound) && *value < bound) return false;
                if (propertyValueToNumber(zhi, bound) && *value > bound) return false;
                return true;
            };
        }
        case Kind::All:
        case Kind::Any:
        case Kind::Not: {
            std::vector<ECA::ConditionPredicate> compiled;
            compiled.reserve(children.size());
            for (const auto& c : children) compiled.push_back(c.compile());
            const Kind k = kind;
            const std::string desc = this->describe();
            return [compiled, k, desc](const ECA::Event& e, const Singular& target) {
                bool res = false;
                if (k == Kind::Not) {
                    res = !compiled.empty() && !compiled[0](e, target);
                } else if (k == Kind::All) {
                    res = true;
                    for (const auto& p : compiled) {
                        if (!p || !p(e, target)) { res = false; break; }
                    }
                } else {
                    for (const auto& p : compiled) {           // Any
                        if (p && p(e, target)) { res = true; break; }
                    }
                }
                ECA::LawAuditLogger::instance().log("CONDITION", "Logic Node Evaluated [" + std::string(res ? "PASS" : "FAIL") + "]: " + desc, {
                    {"targetId", target.getIdentifier()}, {"result", res}
                });
                return res;
            };
        }
        case Kind::IsKind: {
            // Runtime instanceof: is the subject this kind of being?
            const BeingKind k = beingKind;
            return [k](const ECA::Event&, const Singular& target) {
                return ConditionNode::matchesKind(target, k);
            };
        }
        case Kind::Identity: {
            // This one specific being, and no other.
            const std::string id = otherId;
            return [id](const ECA::Event&, const Singular& target) {
                return !id.empty() && target.getIdentifier() == id;
            };
        }
        case Kind::ForAny:
        case Kind::ForAll: {
            // First-order quantification over the Universe: the inner
            // condition runs with each INSTANCE as its subject, not the
            // law's subject. Exceptions carve out named beings. ForAll over
            // an empty domain is vacuously true (mathematics, honored);
            // ForAny over it is false.
            const BeingKind k = beingKind;
            const std::vector<std::string> except = exceptIds;
            const bool isAll = kind == Kind::ForAll;
            ECA::ConditionPredicate inner =
                children.empty() ? ECA::ConditionPredicate{} : children[0].compile();
            return [k, except, isAll, inner](const ECA::Event& e, const Singular&) {
                for (Singular* being : Universe::instance().beings()) {
                    if (!being || !ConditionNode::matchesKind(*being, k)) continue;
                    if (std::find(except.begin(), except.end(), being->getIdentifier()) !=
                        except.end()) {
                        continue;
                    }
                    const bool holds = !inner || inner(e, *being);
                    if (isAll && !holds) return false;
                    if (!isAll && holds) return true;
                }
                return isAll;
            };
        }
        case Kind::Unsupported: {
            // Never satisfied — but it says so every time it is asked, so a
            // law that stopped firing after a version change is traceable
            // instead of just mysteriously inert.
            const int raw = unsupported ? unsupported->value("kind", -1) : -1;
            return [raw](const ECA::Event&, const Singular& target) {
                ECA::LawAuditLogger::instance().log(
                    "CONDITION",
                    "Condition Evaluated [FAIL - Unsupported Kind]: kind " +
                        std::to_string(raw) + " cannot be evaluated by this build",
                    {{"targetId", target.getIdentifier()}, {"result", false}});
                return false;
            };
        }
    }
    return [](const ECA::Event&, const Singular&) { return false; };
}

void ConditionNode::collectPaths(std::vector<PropertyPath>& out) const {
    const auto add = [&out](const PropertyPath& p) {
        if (!p.empty()) out.push_back(p);
    };
    switch (kind) {
        case Kind::Compare:
            add(path);
            add(operandPath);
            break;
        case Kind::InRegion:
            // An empty probe defaults to "position" at compile time; name it
            // explicitly here so the filter sees what the closure will read.
            out.push_back(probe.empty() ? PropertyPath::parse("position") : probe);
            break;
        case Kind::Zone:
            for (const auto& b : bindings) add(b.second);
            break;
        case Kind::ForAny:
        case Kind::ForAll:
            // The inner condition is about the INSTANCES, not the subject.
            return;
        default:
            // IsKind / Identity / Related / Overlaps ask about the being
            // itself, not about any property it carries.
            break;
    }
    for (const auto& c : children) c.collectPaths(out);
}

std::string ConditionNode::describe() const {
    switch (kind) {
        case Kind::Compare: {
            std::string rhs = operandPath.empty() ? std::string("value") : operandPath.toString();
            if (operandPath.empty()) {
                if (std::holds_alternative<std::string>(operand)) {
                    rhs = "\"" + std::get<std::string>(operand) + "\"";
                } else if (std::holds_alternative<bool>(operand)) {
                    rhs = std::get<bool>(operand) ? "true" : "false";
                } else {
                    double n = 0.0;
                    if (propertyValueToNumber(operand, n)) {
                        rhs = std::to_string(n);
                        rhs.erase(rhs.find_last_not_of('0') + 1, std::string::npos);
                        if (rhs.back() == '.') rhs.pop_back();
                    }
                }
            }
            return path.toString() + " " + opName(op) + " " + rhs;
        }
        case Kind::InRegion:
            return "in-region(" + (probe.empty() ? std::string("position") : probe.toString()) + ")";
        case Kind::Related:
            return "related" + (relationType.empty() ? "" : "[" + relationType + "]") +
                   " to " + (otherId.empty() ? "anyone" : otherId);
        case Kind::Overlaps:
            return "overlaps " + (otherId.empty() ? std::string("?") : otherId);
        case Kind::All: return "all(" + std::to_string(children.size()) + ")";
        case Kind::Any: return "any(" + std::to_string(children.size()) + ")";
        case Kind::Not: return "not(...)";
        case Kind::Zone: {
            std::string range;
            double bound = 0.0;
            if (propertyValueToNumber(lo, bound)) range += std::to_string(bound);
            range += "..";
            if (propertyValueToNumber(hi, bound)) range += std::to_string(bound);
            return "zone(" + zoneFunction.print() + " in [" + range + "])";
        }
        case Kind::IsKind:
            return std::string("is-a(") + beingKindName(beingKind) + ")";
        case Kind::Identity:
            return "is(" + (otherId.empty() ? std::string("?") : otherId) + ")";
        case Kind::ForAny:
        case Kind::ForAll: {
            std::string out = kind == Kind::ForAny ? "for-any " : "for-all ";
            out += beingKindName(beingKind);
            if (!exceptIds.empty()) {
                out += " except " + std::to_string(exceptIds.size());
            }
            out += ": ";
            out += children.empty() ? "true" : children[0].describe();
            return out;
        }
        case Kind::Unsupported:
            return "unsupported condition (kind " +
                   std::to_string(unsupported ? unsupported->value("kind", -1) : -1) +
                   ") - never holds";
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

ConditionNode ConditionNode::zone(OntoMath::Piecewise function, MathBindings bindings,
                                  PropertyValue zoneLo, PropertyValue zoneHi) {
    ConditionNode n;
    n.kind = Kind::Zone;
    n.zoneFunction = std::move(function);
    n.bindings = std::move(bindings);
    n.lo = std::move(zoneLo);
    n.hi = std::move(zoneHi);
    return n;
}

ConditionNode ConditionNode::isKind(BeingKind kind) {
    ConditionNode n;
    n.kind = Kind::IsKind;
    n.beingKind = kind;
    return n;
}

ConditionNode ConditionNode::identity(const std::string& beingId) {
    ConditionNode n;
    n.kind = Kind::Identity;
    n.otherId = beingId;
    return n;
}

ConditionNode ConditionNode::related(const std::string& type, const std::string& otherId) {
    ConditionNode n;
    n.kind = Kind::Related;
    n.relationType = type;
    n.otherId = otherId;
    return n;
}

ConditionNode ConditionNode::overlaps(const std::string& otherToken) {
    ConditionNode n;
    n.kind = Kind::Overlaps;
    n.otherId = otherToken;
    return n;
}

ConditionNode ConditionNode::forAny(BeingKind kind, ConditionNode inner,
                                    std::vector<std::string> exceptions) {
    ConditionNode n;
    n.kind = Kind::ForAny;
    n.beingKind = kind;
    n.exceptIds = std::move(exceptions);
    n.children.push_back(std::move(inner));
    return n;
}

ConditionNode ConditionNode::forAll(BeingKind kind, ConditionNode inner,
                                    std::vector<std::string> exceptions) {
    ConditionNode n;
    n.kind = Kind::ForAll;
    n.beingKind = kind;
    n.exceptIds = std::move(exceptions);
    n.children.push_back(std::move(inner));
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
