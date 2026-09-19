#include "PropheticRete.hpp"

#include "Law.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

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
    const auto structural = [&](const char* why) {
        out.opaqueWrites = true;
        out.notes.push_back(std::string("opaque write: ") + why);
    };
    const auto emit = [&](const PropertyPath& path, Range range, const char* via) {
        if (path.empty()) return;
        out.writes.push_back(WriteEffect{out.lawId, path.toString(), std::move(range), via});
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
            out.writes.push_back(WriteEffect{out.lawId, node.propertyName,
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
    }

    for (const auto& child : node.children) analyzeAction(child, out);
}

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
void fileDemands(const DemandMap& demands, LawFacts& out, bool insideQuantifier) {
    for (const auto& [path, range] : demands) {
        out.reads.push_back(ReadDemand{out.lawId, path, range, insideQuantifier});
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
                fileDemands(inner, out, true);
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
        case ConditionNode::Kind::Related:
            out.opaqueReads = true;
            note("opaque read: Related consults the relation graph");
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
            fileDemands(walkCondition(child, out, insideQuantifier), out, insideQuantifier);
        }
    }

    return demands;
}

} // namespace

void analyzeCondition(const ConditionNode& node, LawFacts& out, bool insideQuantifier) {
    DemandMap demands = walkCondition(node, out, insideQuantifier);
    fileDemands(demands, out, insideQuantifier);
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
        analyzeAction(*action, facts);
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

void Index::clear() {
    _facts.clear();
    _readNames.clear();
    _writeRanges.clear();
    _unreachable.clear();
    _complete = true;
}

void Index::rebuild(const std::vector<std::shared_ptr<Law>>& laws) {
    clear();
    _facts.reserve(laws.size());

    bool anyOpaqueWrite = false;
    for (const auto& law : laws) {
        if (!law) continue;
        LawFacts facts = analyzeLaw(*law);
        if (facts.opaqueReads) _complete = false;
        if (facts.opaqueWrites) anyOpaqueWrite = true;
        _readNames.insert(facts.readNames.begin(), facts.readNames.end());

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

    // Pass 3, across laws: a demand every authored writer misses. Only asked
    // where the whole register was legible — one opaque write anywhere and
    // the union above is not the union of everything laws can do.
    if (anyOpaqueWrite) return;

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
    return _readNames.count(propertyName) != 0;
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

nlohmann::json Index::toJson() const {
    nlohmann::json j;
    j["complete"] = _complete;
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
            lj["writes"].push_back({{"path", w.path}, {"via", w.via}, {"range", w.range.print()}});
        }
        lj["reads"] = nlohmann::json::array();
        for (const auto& r : facts.reads) {
            lj["reads"].push_back({{"path", r.path},
                                   {"satisfying", r.satisfying.print()},
                                   {"aboutInstances", r.aboutInstances}});
        }
        lj["notes"] = facts.notes;
        j["laws"].push_back(std::move(lj));
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
