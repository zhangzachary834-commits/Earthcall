#include "PropheticRete.hpp"

#include "Law.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <tuple>

namespace Prophetic {

using OntoMath::Interval;

namespace {

std::string formatNumber(double v) {
    if (!std::isfinite(v)) return v > 0 ? "+inf" : "-inf";
    std::string s = std::to_string(v);
    if (s.find('.') != std::string::npos) {
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (!s.empty() && s.back() == '.') s.pop_back();
    }
    return s;
}

// The boolean lattice seen as numbers. Bool is arithmetic everywhere in this
// codebase (propertyValueToNumber accepts it), so a condition may legitimately
// compare a bool property against 0 or 1 — and refusing to relate the two
// lattices would make that comparison unprovable in either direction.
Interval boolAsInterval(bool maybeTrue, bool maybeFalse) {
    if (maybeTrue && maybeFalse) return Interval(0.0f, 1.0f);
    if (maybeTrue) return Interval(1.0f);
    if (maybeFalse) return Interval(0.0f);
    return Interval(1.0f, 0.0f);   // empty
}

// Stable authored-branch provenance. These ids are DERIVED from the authored
// node text rather than persisted as runtime state: recompilation may rebuild
// closures and vectors, but the same authored branch has the same canonical
// JSON and therefore the same id across recompile and save/load.
std::string stableBranchId(const char* prefix, const nlohmann::json& authored) {
    const std::string text = authored.dump();
    std::uint64_t hash = 1469598103934665603ull; // FNV-1a 64
    for (const unsigned char ch : text) {
        hash ^= static_cast<std::uint64_t>(ch);
        hash *= 1099511628211ull;
    }
    std::ostringstream out;
    out << prefix << '-' << std::hex << std::setw(16) << std::setfill('0') << hash;
    return out.str();
}

std::string actionBranchId(const ActionNode& node) {
    return stableBranchId("a", node.toJson());
}

std::string conditionBranchId(const ConditionNode& node) {
    return stableBranchId("c", node.toJson());
}

} // namespace

// ---------------------------------------------------------------------------
// Range — the abstract value lattice
// ---------------------------------------------------------------------------

Range Range::fromInterval(Interval i) {
    Range r;
    if (i.empty()) { r.kind = Kind::Bottom; return r; }
    // An interval with no finite side constrains nothing; say so in the
    // lattice rather than carrying a bound that reads as information.
    if (!std::isfinite(i.lo) && !std::isfinite(i.hi)) { r.kind = Kind::Top; return r; }
    r.kind = Kind::Number;
    r.number = i;
    return r;
}

Range Range::boolean(bool canBeTrue, bool canBeFalse) {
    Range r;
    if (!canBeTrue && !canBeFalse) { r.kind = Kind::Bottom; return r; }
    r.kind = Kind::Boolean;
    r.maybeTrue = canBeTrue;
    r.maybeFalse = canBeFalse;
    return r;
}

Range Range::text(std::set<std::string> possibilities) {
    Range r;
    if (possibilities.empty()) { r.kind = Kind::Bottom; return r; }
    r.kind = Kind::Text;
    r.texts = std::move(possibilities);
    return r;
}

Range Range::ofValue(const PropertyValue& v) {
    if (std::holds_alternative<bool>(v)) {
        const bool b = std::get<bool>(v);
        return boolean(b, !b);
    }
    if (std::holds_alternative<std::string>(v)) {
        return text({std::get<std::string>(v)});
    }
    double n = 0.0;
    if (propertyValueToNumber(v, n)) {
        return fromInterval(Interval(static_cast<float>(n)));
    }
    // monostate, vec3, mat4, a being pointer, a list/dict/field — this lattice
    // holds none of them, and a lattice that pretends otherwise is exactly the
    // unsound narrowing the header refuses.
    return top();
}

bool Range::mayIntersect(const Range& other) const {
    if (isBottom() || other.isBottom()) return false;
    if (isTop() || other.isTop()) return true;

    if (kind == Kind::Number && other.kind == Kind::Number) {
        return number.overlaps(other.number);
    }
    if (kind == Kind::Boolean && other.kind == Kind::Boolean) {
        return (maybeTrue && other.maybeTrue) || (maybeFalse && other.maybeFalse);
    }
    if (kind == Kind::Boolean && other.kind == Kind::Number) {
        return boolAsInterval(maybeTrue, maybeFalse).overlaps(other.number);
    }
    if (kind == Kind::Number && other.kind == Kind::Boolean) {
        return number.overlaps(boolAsInterval(other.maybeTrue, other.maybeFalse));
    }
    if (kind == Kind::Text && other.kind == Kind::Text) {
        if (textMaybeOther || other.textMaybeOther) return true;
        for (const auto& s : texts) {
            if (other.texts.count(s)) return true;
        }
        return false;
    }
    // Kinds this does not know how to relate. "Possibly" is the only safe
    // answer; see the header's ONE RULE.
    return true;
}

Range Range::joined(const Range& other) const {
    if (isBottom()) return other;
    if (other.isBottom()) return *this;
    if (isTop() || other.isTop()) return top();
    if (kind != other.kind) return top();

    switch (kind) {
        case Kind::Number: return fromInterval(number.joined(other.number));
        case Kind::Boolean:
            return boolean(maybeTrue || other.maybeTrue, maybeFalse || other.maybeFalse);
        case Kind::Text: {
            Range r = *this;
            r.texts.insert(other.texts.begin(), other.texts.end());
            r.textMaybeOther = textMaybeOther || other.textMaybeOther;
            return r;
        }
        default: return top();
    }
}

Range Range::met(const Range& other) const {
    if (isTop()) return other;
    if (other.isTop()) return *this;
    if (isBottom() || other.isBottom()) return bottom();
    // Different lattice kinds: an intersection this cannot compute must be
    // reported as UNCONSTRAINED, never as empty. met() narrows a demand, and
    // a demand narrowed to Bottom by accident is a law silently ruled out.
    if (kind != other.kind) return top();

    switch (kind) {
        case Kind::Number: return fromInterval(number.met(other.number));
        case Kind::Boolean:
            return boolean(maybeTrue && other.maybeTrue, maybeFalse && other.maybeFalse);
        case Kind::Text: {
            if (textMaybeOther || other.textMaybeOther) return top();
            std::set<std::string> both;
            for (const auto& s : texts) {
                if (other.texts.count(s)) both.insert(s);
            }
            return both.empty() ? bottom() : text(std::move(both));
        }
        default: return top();
    }
}

std::string Range::print() const {
    switch (kind) {
        case Kind::Bottom: return "(nothing)";
        case Kind::Top:    return "(anything)";
        case Kind::Number:
            return "[" + formatNumber(number.lo) + ", " + formatNumber(number.hi) + "]";
        case Kind::Boolean:
            if (maybeTrue && maybeFalse) return "true|false";
            return maybeTrue ? "true" : "false";
        case Kind::Text: {
            std::string s = "{";
            bool first = true;
            for (const auto& t : texts) {
                if (!first) s += ", ";
                s += "\"" + t + "\"";
                first = false;
            }
            if (textMaybeOther) s += first ? "..." : ", ...";
            return s + "}";
        }
    }
    return "(anything)";
}

nlohmann::json Range::toJson() const {
    nlohmann::json j;
    switch (kind) {
        case Kind::Bottom:  j["kind"] = "bottom"; break;
        case Kind::Top:     j["kind"] = "top"; break;
        case Kind::Number:
            j["kind"] = "number";
            j["lo"] = number.lo;
            j["hi"] = number.hi;
            break;
        case Kind::Boolean:
            j["kind"] = "boolean";
            j["maybeTrue"] = maybeTrue;
            j["maybeFalse"] = maybeFalse;
            break;
        case Kind::Text:
            j["kind"] = "text";
            j["texts"] = nlohmann::json::array();
            for (const auto& t : texts) j["texts"].push_back(t);
            j["maybeOther"] = textMaybeOther;
            break;
    }
    j["print"] = print();
    return j;
}

// ---------------------------------------------------------------------------
// Path names — the sound over-approximation of "which registered property
// could this authored path touch".
// ---------------------------------------------------------------------------

void collectPathNames(const PropertyPath& path, std::unordered_set<std::string>& out) {
    const auto& seg = path.segments;
    const std::size_t n = seg.size();
    for (std::size_t i = 0; i < n; ++i) {
        if (!seg[i].empty() && seg[i][0] == '@') continue;   // a referent, not a property
        std::string joined;
        for (std::size_t j = i; j < n; ++j) {
            if (j > i) joined += ".";
            joined += seg[j];
            out.insert(joined);
        }
    }
}

// ---------------------------------------------------------------------------
// Pass 3's engine — the possibility space of an authored OntoMath model.
// ---------------------------------------------------------------------------

Range rangeOfPiecewise(const OntoMath::Piecewise& model,
                       const std::map<std::string, Interval>& bound) {
    if (model.pieces.empty()) return Range::top();

    Range acc = Range::bottom();
    for (const auto& piece : model.pieces) {
        // A piece whose value is a call into the function registry, or a fold
        // over the world's beings, is a value this walk does not follow. One
        // such piece makes the whole model's range unknown.
        if (piece.call || piece.fold || !piece.mathNode) return Range::top();

        std::map<std::string, OntoMath::MathNode::RangeValue> vars;
        for (const auto& [name, iv] : bound) {
            vars.emplace(name, OntoMath::MathNode::RangeValue::makeScalar(iv));
        }
        // The piece's own interval bounds constrain the input variable — this
        // is the "x in [0, 10]" of §7 Pass 3's worked example. A guard
        // constrains WHEN the piece applies, never WHAT it can produce, so a
        // guarded piece contributes its full range.
        if ((piece.hasLo || piece.hasHi) && !bound.count(model.inputVariable)) {
            const float lo = piece.hasLo ? static_cast<float>(piece.lo)
                                         : -std::numeric_limits<float>::infinity();
            const float hi = piece.hasHi ? static_cast<float>(piece.hi)
                                         : std::numeric_limits<float>::infinity();
            vars.emplace(model.inputVariable,
                         OntoMath::MathNode::RangeValue::makeScalar(Interval(lo, hi)));
        }

        auto r = piece.mathNode->evalRange(vars);
        if (!r || r->kind != OntoMath::ValueKind::Scalar) return Range::top();
        acc = acc.joined(Range::fromInterval(r->scalar));
        if (acc.isTop()) return acc;
    }
    return acc;
}

Range rangeOfCurve(const CurveModel& curve) {
    switch (curve.form) {
        case CurveModel::Form::Constant:
            return Range::fromInterval(
                Interval(static_cast<float>(curve.coeffs.empty() ? 0.0 : curve.coeffs[0])));
        case CurveModel::Form::Sinusoid: {
            const double amp = std::fabs(curve.amplitude);
            return Range::fromInterval(Interval(static_cast<float>(curve.bias - amp),
                                                static_cast<float>(curve.bias + amp)));
        }
        case CurveModel::Form::Polynomial: {
            // Degree 0 is a constant wearing a polynomial's clothes; anything
            // higher is unbounded over an unbounded input.
            if (curve.coeffs.size() <= 1) {
                return Range::fromInterval(
                    Interval(static_cast<float>(curve.coeffs.empty() ? 0.0 : curve.coeffs[0])));
            }
            return Range::top();
        }
    }
    return Range::top();
}

// ---------------------------------------------------------------------------
// Reading an action tree: what can this law write, and where?
// ---------------------------------------------------------------------------

void analyzeAction(const ActionNode& node, LawFacts& out) {
    const std::string branchId = actionBranchId(node);
    const auto structural = [&](const char* why) {
        out.opaqueWrites = true;
        out.notes.push_back(std::string("opaque write: ") + why);
    };
    const auto emit = [&](const PropertyPath& path, Range range, const char* via) {
        if (path.empty()) return;
        out.writes.push_back(
            WriteEffect{out.lawId, branchId, path.toString(), std::move(range), via});
    };

    switch (node.kind) {
        case ActionNode::Kind::Set:
            emit(node.path, Range::ofValue(node.operand), "Set");
            break;

        // Add/Scale/Lerp compose with the value ALREADY THERE, which this
        // analysis does not know. The composition is unbounded even when the
        // operand is a literal — repeated Adds walk anywhere — so the honest
        // answer is Top. (Bounding these is the natural next refinement: a
        // fixpoint over the write graph, widening until stable.)
        case ActionNode::Kind::Add:
        case ActionNode::Kind::Scale:
        case ActionNode::Kind::Lerp:
            emit(node.path, Range::top(), ActionNode::kindName(node.kind));
            break;

        case ActionNode::Kind::Drive:
            emit(node.path, rangeOfCurve(node.curve), "Drive");
            break;

        case ActionNode::Kind::Map:
            emit(node.path, rangeOfPiecewise(node.mapFunction), "Map");
            break;

        // Flow authors dp/dt and INTEGRATES it into the property, so a bounded
        // rate is not a bounded value. The accumulated position is unbounded
        // unless the author bounded the piece's domain, which bounds the
        // duration rather than the range.
        case ActionNode::Kind::Flow:
            emit(node.path, Range::top(), "Flow");
            break;

        case ActionNode::Kind::AddProperty:
            // The leaf is granted on whoever `path` names; the opening value
            // is what lands in it.
            out.writes.push_back(WriteEffect{out.lawId, branchId, node.propertyName,
                                             Range::ofValue(node.operand), "AddProperty"});
            break;

        case ActionNode::Kind::Sequence:
        case ActionNode::Kind::Parallel:
            break;   // pure composition; the children carry the writes

        // Creation, destruction, composition and publication change the FACT
        // BASE, not just a value in it: a new being arrives with a whole
        // vocabulary, a destroyed one takes its facts with it, a published
        // event mints a fact of a type this walk never sees. None of that is
        // enumerable from a path walk, and pretending otherwise is how a
        // filter derived from this index would start dropping real changes.
        case ActionNode::Kind::Create:      structural("Create mints a being"); break;
        case ActionNode::Kind::Spawn:       structural("Spawn instantiates a concept"); break;
        case ActionNode::Kind::Synthesize:  structural("Synthesize composes creation"); break;
        case ActionNode::Kind::Destroy:     structural("Destroy unmakes a being"); break;
        case ActionNode::Kind::AddElement:
        case ActionNode::Kind::RemoveElement:
            structural("element membership changes what a being is made of");
            break;
        case ActionNode::Kind::RemoveProperty:
            structural("RemoveProperty retires a name");
            break;
        case ActionNode::Kind::Publish:     structural("Publish mints an event"); break;
        case ActionNode::Kind::AuthorZone:  structural("AuthorZone mints a Zone"); break;
        case ActionNode::Kind::AddRelation: structural("AddRelation mints a Relation"); break;
        case ActionNode::Kind::PlayAudio:   break;   // reaches a channel, writes no property
        case ActionNode::Kind::WritePixel:  break;   // reaches Screen; elevated samples announce separately
        case ActionNode::Kind::ElevatePixels:
            structural("ElevatePixels grants a surface Property");
            break;
    }

    for (const auto& child : node.children) analyzeAction(child, out);
}

namespace {

// A Law's condition is an invariant pre-state for every firing: if the Law
// fires, its subject satisfied these demands immediately before the action.
// This is the sound seed for current-value-dependent writes. It is deliberately
// exact-path only: pathsMayAlias() is a MAY-alias relation and may never be
// used to narrow a value.
using AbstractState = std::map<std::string, Range>;

bool asNumericInterval(const Range& range, Interval& out) {
    if (range.kind == Range::Kind::Number) {
        out = range.number;
        return true;
    }
    if (range.kind == Range::Kind::Boolean) {
        out = boolAsInterval(range.maybeTrue, range.maybeFalse);
        return !out.empty();
    }
    return false;
}

Range stateRange(const AbstractState& state, const PropertyPath& path) {
    const auto it = state.find(path.toString());
    return it == state.end() ? Range::top() : it->second;
}

void writeState(AbstractState& state, const PropertyPath& path, const Range& range) {
    if (!path.empty()) state[path.toString()] = range;
}

std::map<std::string, Interval> bindingBounds(
    const MathBindings& bindings, const AbstractState& state) {
    std::map<std::string, Interval> out;
    for (const auto& [variable, path] : bindings) {
        Interval iv;
        if (asNumericInterval(stateRange(state, path), iv)) out.emplace(variable, iv);
    }
    return out;
}

Range affineCurrentWrite(const ActionNode& node, const Range& current) {
    Interval lhs;
    if (!asNumericInterval(current, lhs)) return Range::top();

    double rhsValue = 0.0;
    if (!propertyValueToNumber(node.operand, rhsValue)) return Range::top();
    const Interval rhs(static_cast<float>(rhsValue));

    switch (node.kind) {
        case ActionNode::Kind::Add:
            return Range::fromInterval(lhs + rhs);
        case ActionNode::Kind::Scale:
            return Range::fromInterval(lhs * rhs);
        case ActionNode::Kind::Lerp: {
            const float f = static_cast<float>(node.factor);
            return Range::fromInterval(lhs * Interval(1.0f - f) + rhs * Interval(f));
        }
        default:
            return Range::top();
    }
}

Range flowCurrentWrite(const ActionNode& node, const AbstractState& state) {
    Interval current;
    if (!asNumericInterval(stateRange(state, node.path), current)) return Range::top();

    const Range rateRange = rangeOfPiecewise(node.mapFunction,
                                             bindingBounds(node.bindings, state));
    Interval rate;
    if (!asNumericInterval(rateRange, rate)) return Range::top();

    // A zero rate is an identity regardless of the engine's frame delta.
    if (rate.lo == 0.0f && rate.hi == 0.0f) {
        return Range::fromInterval(current);
    }

    // dt is part of Flow's semantics but is not an implicit engine promise to
    // Prophetic Rete. Only authored text may bound it. If the Law did not
    // constrain time.delta, the honest result stays Top.
    const auto dtIt = state.find("time.delta");
    if (dtIt == state.end()) return Range::top();
    Interval dt;
    if (!asNumericInterval(dtIt->second, dt)) return Range::top();

    return Range::fromInterval(current + rate * dt);
}

// Context-sensitive action interpretation. analyzeAction() above remains the
// context-free public primitive (therefore unguarded Add/Scale/Lerp/Flow are
// still Top). A whole Law has more information: its condition is a pre-state,
// and an ordered Sequence may establish a value before a later action reads it.
//
// For the supported current-dependent transforms the transfer is monotone over
// interval inclusion. Applying it to the ENTIRE authored guard therefore
// already yields a sound post-fixpoint for repeated firings: any later firing
// must re-enter through that same guard, a subset of the pre-state already
// analyzed. This is the widening fixpoint without pretending the open world is
// closed. First Movers / foreign channels can still supply any unguarded
// starting value, so those cases correctly remain Top.
void analyzeActionWithState(const ActionNode& node, LawFacts& out, AbstractState& state) {
    const std::string branchId = actionBranchId(node);
    const auto emit = [&](const PropertyPath& path, Range range, const char* via) {
        if (path.empty()) return;
        out.writes.push_back(
            WriteEffect{out.lawId, branchId, path.toString(), range, via});
        writeState(state, path, range);
    };

    switch (node.kind) {
        case ActionNode::Kind::Set:
            emit(node.path, Range::ofValue(node.operand), "Set");
            return;

        case ActionNode::Kind::Add:
        case ActionNode::Kind::Scale:
        case ActionNode::Kind::Lerp:
            emit(node.path, affineCurrentWrite(node, stateRange(state, node.path)),
                 ActionNode::kindName(node.kind));
            return;

        case ActionNode::Kind::Drive:
            emit(node.path, rangeOfCurve(node.curve), "Drive");
            return;

        case ActionNode::Kind::Map:
            emit(node.path,
                 rangeOfPiecewise(node.mapFunction, bindingBounds(node.bindings, state)),
                 "Map");
            return;

        case ActionNode::Kind::Flow:
            emit(node.path, flowCurrentWrite(node, state), "Flow");
            return;

        case ActionNode::Kind::AddProperty: {
            const Range opening = Range::ofValue(node.operand);
            out.writes.push_back(
                WriteEffect{out.lawId, branchId, node.propertyName, opening, "AddProperty"});
            state[node.propertyName] = opening;
            return;
        }

        case ActionNode::Kind::Sequence:
            for (const auto& child : node.children) {
                analyzeActionWithState(child, out, state);
            }
            return;

        case ActionNode::Kind::Parallel: {
            // Siblings are conceptually simultaneous. No sibling may borrow
            // another sibling's write as its pre-state. For whatever follows
            // the Parallel block, join all possible sibling outputs per path.
            const AbstractState incoming = state;
            std::map<std::string, Range> parallelWrites;
            for (const auto& child : node.children) {
                AbstractState childState = incoming;
                const std::size_t before = out.writes.size();
                analyzeActionWithState(child, out, childState);
                for (std::size_t i = before; i < out.writes.size(); ++i) {
                    const WriteEffect& effect = out.writes[i];
                    auto it = parallelWrites.find(effect.path);
                    if (it == parallelWrites.end()) parallelWrites.emplace(effect.path, effect.range);
                    else it->second = it->second.joined(effect.range);
                }
            }
            for (const auto& [path, range] : parallelWrites) state[path] = range;
            return;
        }

        default:
            // Structural / modality actions retain the existing fail-open
            // analysis. Their children (Create/Synthesize) act in a different
            // subject scope, so none of those writes may seed this subject's
            // sequential abstract state.
            analyzeAction(node, out);
            return;
    }
}

AbstractState conditionPreState(const LawFacts& facts) {
    AbstractState state;
    for (const auto& read : facts.reads) {
        if (read.aboutInstances || read.satisfying.isTop()) continue;
        auto it = state.find(read.path);
        if (it == state.end()) state.emplace(read.path, read.satisfying);
        else it->second = it->second.met(read.satisfying);
    }
    return state;
}

} // namespace

// ---------------------------------------------------------------------------
// Reading a condition tree: what does this law read, and what would satisfy it?
//
// The demand map is returned rather than accumulated, because All and Any
// combine their children's demands DIFFERENTLY — All meets them (both must
// hold), Any joins them, and a path constrained by only one arm of an Any is
// unconstrained overall. Getting that backwards is the difference between a
// filter and a bug.
// ---------------------------------------------------------------------------

namespace {

using DemandMap = std::map<std::string, Range>;

// The satisfying set of one Compare, as a range over the left-hand path.
Range satisfyingRange(const ConditionNode& node) {
    // A right-hand side read live off another property is a value this
    // analysis has no bound for.
    if (!node.operandPath.empty()) return Range::top();

    switch (node.op) {
        case ConditionNode::Op::Eq:
            return Range::ofValue(node.operand);
        case ConditionNode::Op::Ne:
            // The complement of a point is a hole, and this lattice holds
            // intervals and sets, not holes.
            return Range::top();
        case ConditionNode::Op::Lt:
        case ConditionNode::Op::Le: {
            double n = 0.0;
            if (!propertyValueToNumber(node.operand, n)) return Range::top();
            return Range::fromInterval(Interval(-std::numeric_limits<float>::infinity(),
                                                static_cast<float>(n)));
        }
        case ConditionNode::Op::Gt:
        case ConditionNode::Op::Ge: {
            double n = 0.0;
            if (!propertyValueToNumber(node.operand, n)) return Range::top();
            return Range::fromInterval(Interval(static_cast<float>(n),
                                                std::numeric_limits<float>::infinity()));
        }
        case ConditionNode::Op::Near: {
            double n = 0.0;
            if (!propertyValueToNumber(node.operand, n)) return Range::top();
            const float tol = static_cast<float>(std::fabs(node.tolerance));
            return Range::fromInterval(Interval(static_cast<float>(n) - tol,
                                                static_cast<float>(n) + tol));
        }
        case ConditionNode::Op::InRange: {
            double lo = 0.0, hi = 0.0;
            const bool hasLo = propertyValueToNumber(node.lo, lo);
            const bool hasHi = propertyValueToNumber(node.hi, hi);
            if (!hasLo && !hasHi) return Range::top();
            return Range::fromInterval(
                Interval(hasLo ? static_cast<float>(lo)
                               : -std::numeric_limits<float>::infinity(),
                         hasHi ? static_cast<float>(hi)
                               : std::numeric_limits<float>::infinity()));
        }
    }
    return Range::top();
}

// The interval a Zone condition's satisfaction window carves, reusing the
// InRange lo/hi slots (either side may be absent = unbounded).
Interval zoneWindow(const ConditionNode& node) {
    double lo = 0.0, hi = 0.0;
    const bool hasLo = propertyValueToNumber(node.lo, lo);
    const bool hasHi = propertyValueToNumber(node.hi, hi);
    return Interval(hasLo ? static_cast<float>(lo) : -std::numeric_limits<float>::infinity(),
                    hasHi ? static_cast<float>(hi) : std::numeric_limits<float>::infinity());
}

DemandMap walkCondition(const ConditionNode& node, LawFacts& out, bool insideQuantifier);

// Record every demand in a map as a read on the law, then hand it back.
void fileDemands(const DemandMap& demands, LawFacts& out, bool insideQuantifier,
                 const std::string& branchId) {
    for (const auto& [path, range] : demands) {
        out.reads.push_back(ReadDemand{out.lawId, branchId, path, range, insideQuantifier});
    }
}

DemandMap walkCondition(const ConditionNode& node, LawFacts& out, bool insideQuantifier) {
    DemandMap demands;
    const auto note = [&](const std::string& why) { out.notes.push_back(why); };

    switch (node.kind) {
        case ConditionNode::Kind::Compare: {
            if (!node.path.empty()) {
                collectPathNames(node.path, out.readNames);
                demands[node.path.toString()] = satisfyingRange(node);
            }
            if (!node.operandPath.empty()) {
                collectPathNames(node.operandPath, out.readNames);
                demands[node.operandPath.toString()] = Range::top();
            }
            break;
        }

        case ConditionNode::Kind::InRegion: {
            const PropertyPath probe = node.probe.empty() ? PropertyPath::parse("position")
                                                          : node.probe;
            collectPathNames(probe, out.readNames);
            demands[probe.toString()] = Range::top();
            break;
        }

        case ConditionNode::Kind::Zone: {
            for (const auto& [var, path] : node.bindings) {
                (void)var;
                if (path.empty()) continue;
                collectPathNames(path, out.readNames);
                demands[path.toString()] = Range::top();
            }
            break;
        }

        // A quantifier's inner condition is about the INSTANCES it ranges
        // over, not about the law's subject. Its demands are filed as
        // instance reads and deliberately do NOT propagate to the parent:
        // meeting "every Object's height > 3" against a demand on the
        // subject's own height would be a claim about two different beings.
        // An empty map imposes nothing, which is the sound direction.
        case ConditionNode::Kind::ForAny:
        case ConditionNode::Kind::ForAll: {
            for (const auto& child : node.children) {
                DemandMap inner = walkCondition(child, out, true);
                fileDemands(inner, out, true, conditionBranchId(child));
            }
            return {};
        }

        case ConditionNode::Kind::All: {
            for (const auto& child : node.children) {
                for (const auto& entry : walkCondition(child, out, insideQuantifier)) {
                    auto it = demands.find(entry.first);
                    if (it == demands.end()) demands.emplace(entry.first, entry.second);
                    else it->second = it->second.met(entry.second);
                }
            }
            break;
        }

        case ConditionNode::Kind::Any: {
            bool first = true;
            DemandMap merged;
            for (const auto& child : node.children) {
                DemandMap inner = walkCondition(child, out, insideQuantifier);
                if (first) { merged = inner; first = false; continue; }
                DemandMap next;
                for (const auto& entry : inner) {
                    auto it = merged.find(entry.first);
                    // A path only ONE arm constrains is unconstrained under
                    // Any: the other arm can satisfy the law without it.
                    if (it != merged.end()) {
                        next.emplace(entry.first, it->second.joined(entry.second));
                    }
                }
                merged = std::move(next);
            }
            demands = std::move(merged);
            break;
        }

        // The complement of a satisfying set is not one this lattice holds.
        // The reads are still real, so walk for their names and discard the
        // ranges.
        case ConditionNode::Kind::Not: {
            for (const auto& child : node.children) {
                for (const auto& entry : walkCondition(child, out, insideQuantifier)) {
                    demands[entry.first] = Range::top();
                }
            }
            break;
        }

        // Geometric contact and graph membership are answered by the engine
        // against state this walk cannot name — a collision reads whatever
        // the collision test reads. Marking the law's reads opaque is what
        // keeps a filter derived from this index from starving it.
        case ConditionNode::Kind::Overlaps:
            out.opaqueReads = true;
            note("opaque read: Overlaps consults the collision test");
            break;
        // A TYPED Related is legible, and must be: marking it opaque made the
        // whole index incomplete, which turned LawManager::propheticHears off
        // for EVERY property write in the world. One category-scoped law
        // ("every instance-of category.target") then sent each unrelated write
        // (position.z) through markFactDirty -> evaluateDirty -> retractFact,
        // a linear walk of every fact. Measured 2026-09-14 at 400 beings: 6.5
        // ms/tick against 0.63 for the same law reading a property — and the
        // relation endpoint index (rung 4) could not touch it, because the
        // cost was never in walking relations.
        //
        // What it hears, exactly (FORMATION_RETE.md §8 rung 4):
        //   - graph changes arrive as relation-state facts, asserted by the
        //     relation-formed handler and the back-seed — never through the
        //     property-change callback this filter gates;
        //   - its Rete node (ConditionNode::compileToRete) wakes on a state
        //     fact only when the fact's attribute ROOT is the relation type,
        //     so a write to "<type>" or "<type>.<x>" is heard, by root;
        //   - the Relation's own fields the predicate reads, by name.
        // The subject's identifier is not listed, following IsKind/Identity
        // below: it names the being, and no write to it wakes this node.
        //
        // An UNTYPED Related has no attribute filter — its node re-runs on
        // every state fact — so it stays opaque. Guarded by
        // tests/law/related_prophetic_legibility_test.cpp.
        case ConditionNode::Kind::Related:
            if (node.relationType.empty()) {
                out.opaqueReads = true;
                note("opaque read: an untyped Related wakes on every state fact");
                break;
            }
            out.readRoots.insert(node.relationType);
            for (const char* field : {"type", "directed", "entityA", "entityB"}) {
                out.readNames.insert(field);
            }
            break;

        // A kind this build does not know. It never holds — but it also never
        // tells us what it would have read, so nothing may be pruned around it.
        case ConditionNode::Kind::Unsupported:
            out.opaqueReads = true;
            note("opaque read: condition kind unknown to this build");
            break;

        // These ask about the being itself, not about any property it carries.
        case ConditionNode::Kind::IsKind:
        case ConditionNode::Kind::Identity:
            break;
    }

    // All / Any / Not / the quantifiers already recursed above, each with its
    // own combination rule. Every other kind's children (a save from a build
    // that nested where this one does not) are walked for their reads alone.
    const bool alreadyRecursed =
        node.kind == ConditionNode::Kind::All || node.kind == ConditionNode::Kind::Any ||
        node.kind == ConditionNode::Kind::Not || node.kind == ConditionNode::Kind::ForAny ||
        node.kind == ConditionNode::Kind::ForAll;
    if (!alreadyRecursed) {
        for (const auto& child : node.children) {
            fileDemands(walkCondition(child, out, insideQuantifier), out, insideQuantifier,
                        conditionBranchId(child));
        }
    }

    return demands;
}

// Preserve leaf/local branch provenance separately from the effective demand
// algebra above. Any(A, B) may make a path unconstrained for the whole Any
// expression, while A itself is still exactly the frontier a write can make
// relevant. This walk never narrows execution; it only constructs a
// conservative relevance graph.
void collectBranchReads(const ConditionNode& node, LawFacts& out,
                        bool insideQuantifier, bool forceTop = false) {
    const std::string branchId = conditionBranchId(node);
    const auto emit = [&](const PropertyPath& path, Range range) {
        if (path.empty()) return;
        if (forceTop) range = Range::top();
        out.branchReads.push_back(
            ReadDemand{out.lawId, branchId, path.toString(), std::move(range), insideQuantifier});
    };

    switch (node.kind) {
        case ConditionNode::Kind::Compare:
            emit(node.path, satisfyingRange(node));
            if (!node.operandPath.empty()) emit(node.operandPath, Range::top());
            break;
        case ConditionNode::Kind::InRegion:
            emit(node.probe.empty() ? PropertyPath::parse("position") : node.probe, Range::top());
            break;
        case ConditionNode::Kind::Zone:
            for (const auto& [var, path] : node.bindings) {
                (void)var;
                emit(path, Range::top());
            }
            break;
        case ConditionNode::Kind::ForAny:
        case ConditionNode::Kind::ForAll:
            for (const auto& child : node.children) {
                collectBranchReads(child, out, true, forceTop);
            }
            return;
        case ConditionNode::Kind::Not:
            for (const auto& child : node.children) {
                collectBranchReads(child, out, insideQuantifier, true);
            }
            return;
        case ConditionNode::Kind::All:
        case ConditionNode::Kind::Any:
            for (const auto& child : node.children) {
                collectBranchReads(child, out, insideQuantifier, forceTop);
            }
            return;
        default:
            break;
    }

    for (const auto& child : node.children) {
        collectBranchReads(child, out, insideQuantifier, forceTop);
    }
}

} // namespace

void analyzeCondition(const ConditionNode& node, LawFacts& out, bool insideQuantifier) {
    DemandMap demands = walkCondition(node, out, insideQuantifier);
    fileDemands(demands, out, insideQuantifier, conditionBranchId(node));
    collectBranchReads(node, out, insideQuantifier);
}

// ---------------------------------------------------------------------------
// One law, read whole.
// ---------------------------------------------------------------------------

LawFacts analyzeLaw(const Law& law) {
    LawFacts facts;
    facts.lawId = law.getIdentifier();

    if (const ConditionModel* condition = law.conditionModel()) {
        analyzeCondition(*condition, facts);
    } else if (law.conditionPredicateCount() > 0) {
        // Compiled predicates with no model behind them are arbitrary
        // closures (Law::addCondition). A closure cannot be introspected, so
        // this law's reads are unknowable — the same honest answer
        // ReteNetwork::hasOpaqueBoundAlpha gives about the network.
        facts.opaqueReads = true;
        facts.notes.push_back("opaque read: condition predicates with no authored model");
    }

    if (const ActionModel* action = law.actionModel()) {
        AbstractState state = conditionPreState(facts);
        analyzeActionWithState(*action, facts, state);
    }

    // A First Mover actuates in C++ — that is what makes it a first mover.
    // Its writes are outside the law calculus by construction.
    if (law.isFirstMover()) {
        facts.opaqueWrites = true;
        facts.notes.push_back("opaque write: First Mover actuates in C++");
    }

    // Self-impossibility (a Zone function whose range misses its own window)
    // is proved from this same text, but it belongs to the Index rather than
    // here: it is a FINDING about the law, not a fact the law states.
    return facts;
}

// ---------------------------------------------------------------------------
// The index
// ---------------------------------------------------------------------------

namespace {

// Candidate normalized forms of an authored path, used ONLY to line reads and
// writes up with each other. A referent prefix (@event.subject, @being-id,
// @world) names WHOSE property, not which one, so the tail is what two laws
// have in common. `@being-id` may itself contain dots and cannot be stripped
// unambiguously, so every plausible tail is offered and the caller takes the
// union — over-offering on the write side and on the read side both push the
// conclusion toward "possible", which is the safe direction.
std::vector<std::string> normalizedPaths(const std::string& dotted) {
    PropertyPath p = PropertyPath::parse(dotted);
    std::vector<std::string> out;
    out.push_back(dotted);
    if (p.segments.empty() || p.segments[0].empty() || p.segments[0][0] != '@') return out;
    for (std::size_t drop = 1; drop < p.segments.size(); ++drop) {
        std::string tail;
        for (std::size_t i = drop; i < p.segments.size(); ++i) {
            if (i > drop) tail += ".";
            tail += p.segments[i];
        }
        out.push_back(tail);
    }
    return out;
}

bool pathsMayAlias(const std::string& a, const std::string& b) {
    const auto aa = normalizedPaths(a);
    const auto bb = normalizedPaths(b);
    for (const auto& left : aa) {
        for (const auto& right : bb) {
            if (left == right) return true;
        }
    }
    return false;
}

bool namesWorldReading(const std::string& dotted) {
    return dotted.rfind("@world", 0) == 0;
}

// Walk a condition tree for Zone pieces whose authored mathematics can never
// reach their own satisfaction window.
void collectSelfImpossible(const ConditionNode& node, const std::string& lawId,
                           std::vector<Index::Unreachable>& out) {
    if (node.kind == ConditionNode::Kind::Zone) {
        const Range produced = rangeOfPiecewise(node.zoneFunction);
        const Range window = Range::fromInterval(zoneWindow(node));
        if (!produced.isTop() && !window.isTop() && !produced.mayIntersect(window)) {
            out.push_back(Index::Unreachable{
                lawId, "(zone function)",
                "the authored function's range " + produced.print() +
                    " never meets its satisfaction window " + window.print(),
                true});
        }
    }
    for (const auto& child : node.children) collectSelfImpossible(child, lawId, out);
}

} // namespace

bool unknownSourceMayReach(const Index::UnknownWriteSource& source,
                           const std::string& path) {
    // This is the positive/negative knowledge asymmetry in executable form.
    // A partial domain may contain useful positive facts, but absence from it
    // proves nothing. Only a source whose domain is certified complete may
    // use disjointness to answer "no".
    if (!source.domainComplete) return true;
    for (const auto& candidate : source.knownMayWritePaths) {
        if (pathsMayAlias(candidate, path)) return true;
    }
    return false;
}

void Index::clear() {
    _facts.clear();
    _readNames.clear();
    _readRoots.clear();
    _writeRanges.clear();
    _unreachable.clear();
    _relevanceEdges.clear();
    _unknownWriteSources.clear();
    _complete = true;
    _relevanceComplete = true;
}

void Index::rebuild(const std::vector<std::shared_ptr<Law>>& laws) {
    clear();
    _facts.reserve(laws.size());

    bool anyOpaqueWrite = false;
    for (const auto& law : laws) {
        if (!law) continue;
        LawFacts facts = analyzeLaw(*law);
        if (facts.opaqueReads) {
            _complete = false;
            _relevanceComplete = false;
        }
        if (facts.opaqueWrites) {
            anyOpaqueWrite = true;
            _relevanceComplete = false;

            std::string why = "opaque write: transform not structurally enumerable";
            for (const auto& note : facts.notes) {
                if (note.rfind("opaque write:", 0) == 0) {
                    why = note;
                    break;
                }
            }
            _unknownWriteSources.push_back(
                UnknownWriteSource{facts.lawId, why, !facts.writes.empty()});
        }
        _readNames.insert(facts.readNames.begin(), facts.readNames.end());
        _readRoots.insert(facts.readRoots.begin(), facts.readRoots.end());

        // Pass 1: the union of everything any law can put at each path.
        for (const auto& write : facts.writes) {
            for (const auto& norm : normalizedPaths(write.path)) {
                auto it = _writeRanges.find(norm);
                if (it == _writeRanges.end()) _writeRanges.emplace(norm, write.range);
                else it->second = it->second.joined(write.range);
            }
        }

        if (const ConditionModel* condition = law->conditionModel()) {
            collectSelfImpossible(*condition, facts.lawId, _unreachable);
        }
        _facts.push_back(std::move(facts));
    }

    // Pairwise Prophetic relevance graph. This is the modern descendant of
    // the old "ActionNode -> Beta back-pointer" idea: prove which modeled
    // write branches can possibly feed which modeled read branches.
    //
    // IMPORTANT: opacity no longer ERases known edges. It creates an explicit
    // unknown frontier and leaves relevanceComplete() false. That distinction
    // is the §20/§21 unknown-variable model: "these edges are genuinely known"
    // and "there may also be other edges we cannot enumerate" can both be true.
    // Runtime consumers still must fall back unless the graph is complete.
    _relevanceComplete = _complete && !anyOpaqueWrite;
    std::set<std::tuple<std::string, std::string, std::string, std::string,
                        std::string, bool>> seen;
    for (const auto& writerFacts : _facts) {
        for (const auto& write : writerFacts.writes) {
            for (const auto& readerFacts : _facts) {
                for (const auto& read : readerFacts.branchReads) {
                    if (!pathsMayAlias(write.path, read.path)) continue;
                    if (!write.range.mayIntersect(read.satisfying)) continue;
                    const auto key = std::make_tuple(
                        write.lawId, write.branchId, read.lawId, read.branchId,
                        read.path, read.aboutInstances);
                    if (!seen.insert(key).second) continue;
                    _relevanceEdges.push_back(RelevanceEdge{
                        write.lawId, write.branchId,
                        read.lawId, read.branchId,
                        read.path, read.aboutInstances});
                }
            }
        }
    }

    // Pass 3, across laws: a demand every authored writer misses. Only asked
    // where the whole register was legible. The architecture document has
    // always promised BOTH halves of that guard; enforcing _complete here
    // closes the case where one opaque read used to coexist with a cross-law
    // "no lawful driver" finding.
    if (anyOpaqueWrite || !_complete) return;

    for (const auto& facts : _facts) {
        for (const auto& demand : facts.reads) {
            if (demand.satisfying.isTop()) continue;
            if (namesWorldReading(demand.path)) continue;   // an external first mover
            bool anyWriter = false;
            bool anySatisfiable = false;
            for (const auto& norm : normalizedPaths(demand.path)) {
                auto it = _writeRanges.find(norm);
                if (it == _writeRanges.end()) continue;
                anyWriter = true;
                if (demand.satisfying.mayIntersect(it->second)) anySatisfiable = true;
            }
            // "Nobody writes it" is NOT a finding: a property no law touches
            // is exactly what a First Mover or a tool moves, and most of them
            // are. Only a writer that provably misses is worth saying.
            if (anyWriter && !anySatisfiable) {
                _unreachable.push_back(Unreachable{
                    demand.lawId, demand.path,
                    "no authored law writes " + demand.path + " within " +
                        demand.satisfying.print() + "; every authored write lands in " +
                        writeRangeOf(demand.path).print(),
                    false});
            }
        }
    }
}

bool Index::anyConditionReads(const std::string& propertyName) const {
    if (!_complete) return true;
    if (_readNames.count(propertyName) != 0) return true;
    if (_readRoots.empty()) return false;
    const std::size_t dot = propertyName.find('.');
    return _readRoots.count(dot == std::string::npos ? propertyName
                                                     : propertyName.substr(0, dot)) != 0;
}

Range Index::writeRangeOf(const std::string& path) const {
    Range acc = Range::bottom();
    bool found = false;
    for (const auto& norm : normalizedPaths(path)) {
        auto it = _writeRanges.find(norm);
        if (it == _writeRanges.end()) continue;
        found = true;
        acc = acc.joined(it->second);
    }
    return found ? acc : Range::top();
}

bool Index::unknownWriteMayReach(const std::string& path) const {
    for (const auto& source : _unknownWriteSources) {
        if (unknownSourceMayReach(source, path)) return true;
    }
    return false;
}

bool Index::unknownWriteDomainCompleteFor(const std::string& path) const {
    // The argument is intentionally present even though today's frontier has
    // only whole-domain completeness. It makes the API property-granular now
    // and leaves room for future provenance to certify subdomains without
    // changing the cross-Law solver's question.
    (void)path;
    for (const auto& source : _unknownWriteSources) {
        if (!source.domainComplete) return false;
    }
    return true;
}

nlohmann::json Index::toJson() const {
    nlohmann::json j;
    j["complete"] = _complete;
    j["relevanceComplete"] = _relevanceComplete;
    j["lawCount"] = _facts.size();

    j["readNames"] = nlohmann::json::array();
    std::vector<std::string> names(_readNames.begin(), _readNames.end());
    std::sort(names.begin(), names.end());
    for (const auto& n : names) j["readNames"].push_back(n);

    j["writeRanges"] = nlohmann::json::object();
    for (const auto& [path, range] : _writeRanges) {
        j["writeRanges"][path] = range.print();
    }

    j["laws"] = nlohmann::json::array();
    for (const auto& facts : _facts) {
        nlohmann::json lj;
        lj["id"] = facts.lawId;
        lj["opaqueReads"] = facts.opaqueReads;
        lj["opaqueWrites"] = facts.opaqueWrites;
        lj["writes"] = nlohmann::json::array();
        for (const auto& w : facts.writes) {
            lj["writes"].push_back({{"branchId", w.branchId},
                                     {"path", w.path},
                                     {"via", w.via},
                                     {"range", w.range.print()}});
        }
        lj["reads"] = nlohmann::json::array();
        for (const auto& r : facts.reads) {
            lj["reads"].push_back({{"branchId", r.branchId},
                                   {"path", r.path},
                                   {"satisfying", r.satisfying.print()},
                                   {"aboutInstances", r.aboutInstances}});
        }
        lj["branchReads"] = nlohmann::json::array();
        for (const auto& r : facts.branchReads) {
            lj["branchReads"].push_back({{"branchId", r.branchId},
                                         {"path", r.path},
                                         {"satisfying", r.satisfying.print()},
                                         {"aboutInstances", r.aboutInstances}});
        }
        lj["notes"] = facts.notes;
        j["laws"].push_back(std::move(lj));
    }

    j["unknownWriteSources"] = nlohmann::json::array();
    for (const auto& source : _unknownWriteSources) {
        j["unknownWriteSources"].push_back({
            {"lawId", source.lawId},
            {"why", source.why},
            {"hasModeledWrites", source.hasModeledWrites},
            {"knownMayWritePaths", source.knownMayWritePaths},
            {"domainComplete", source.domainComplete}});
    }

    j["relevanceEdges"] = nlohmann::json::array();
    for (const auto& edge : _relevanceEdges) {
        j["relevanceEdges"].push_back({
            {"writerLawId", edge.writerLawId},
            {"writerBranchId", edge.writerBranchId},
            {"readerLawId", edge.readerLawId},
            {"readerBranchId", edge.readerBranchId},
            {"path", edge.path},
            {"aboutInstances", edge.aboutInstances}});
    }

    j["unreachable"] = nlohmann::json::array();
    for (const auto& u : _unreachable) {
        j["unreachable"].push_back({{"lawId", u.lawId},
                                    {"path", u.path},
                                    {"why", u.why},
                                    {"selfImpossible", u.selfImpossible}});
    }
    return j;
}

} // namespace Prophetic
