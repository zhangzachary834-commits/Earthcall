#pragma once

#include "ECA.hpp"
#include "Form/Object/Geometry/Sdf.hpp"
#include "Form/Singular/Property/PropertyPath.hpp"
#include "MathBinding.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "json.hpp"

#include <memory>
#include <string>
#include <vector>

// The law's condition as data (LAW_AND_CREATION_SYSTEM.md §2a): an expression
// tree over PropertyPaths, serializable and Person-authorable, compiled once
// into the ECA::ConditionPredicate slot. The SdfNode move applied to
// predicates — the tree is the law's text; the closure is derived.
struct ConditionNode {
    // Serialized as ints — all enums here are APPEND-ONLY.
    //
    // The condition calculus, mapped to what C++ itself gives:
    //   All / Any / Not  =  && / || / !   — and tree NESTING is
    //   parenthesization: All(Any(a,b), Not(c)) is (a || b) && !c.
    //   IsKind           =  runtime instanceof (dynamic_cast)
    //   Identity         =  this one specific being
    //   ForAny / ForAll  =  first-order quantifiers over the Universe of
    //                       beings (with exceptions) — the inner condition is
    //                       evaluated with each INSTANCE as its subject.
    enum class Kind { Compare = 0, InRegion = 1, Related = 2, All = 3, Any = 4, Not = 5,
                      Zone = 6, IsKind = 7, Identity = 8, ForAny = 9, ForAll = 10,
                      // Overlaps — geometric contact between the subject and a
                      // named other, answered by the engine's collision test
                      // (a first mover shrunk to a pure PREDICATE): perception
                      // as an ordinary condition.
                      Overlaps = 11,
                      // 12 and 13 were the pair quantifiers (ForAnyPair /
                      // ForAllPair), retired in favour of modelling pairs as
                      // Relations in the graph. APPEND-ONLY means those two
                      // stay BURNED: a future kind must not reuse them, or a
                      // saved world would load as something else entirely.
                      //
                      // Not a kind an author can pick — the landing place for
                      // a kind THIS BUILD does not know: a save from another
                      // version, or one of the retired pair quantifiers. It
                      // never holds (compile() answers false and says why in
                      // the audit log), and the original JSON rides along in
                      // `unsupported` so a load/save round trip does not
                      // destroy law text we merely cannot evaluate.
                      Unsupported = 255 };
    enum class Op { Eq = 0, Ne = 1, Lt = 2, Le = 3, Gt = 4, Ge = 5, Near = 6, InRange = 7 };

    // The ontology's kinds, checked by dynamic_cast — honest C++ instanceof.
    // (Note: a Law IS an Object in this ontology — extra-spatial — so
    // BeingKind::Object matches laws too; use BeingKind::Law for precision.)
    // (Zone is likewise an Object — extra-spatial, per the manifesto — so
    // BeingKind::Object matches zones too; use BeingKind::Zone for precision.)
    enum class BeingKind { AnyBeing = 0, Object = 1, Person = 2, Relation = 3,
                           Formation = 4, Law = 5, World = 6, Zone = 7, Lexeme = 8 };

    Kind kind = Kind::Compare;

    // Compare payload.
    PropertyPath path;               // lhs
    Op op = Op::Eq;
    PropertyValue operand;           // rhs literal…
    PropertyPath operandPath;        // …or rhs read live from another property
    double tolerance = 0.0;          // Near
    PropertyValue lo, hi;            // InRange

    // InRegion payload — a shape IS the condition. Authored with the ordinary
    // shape tools in projection mode; evalSdf(region, probe) < 0 is the test.
    geom::SdfNode region;
    PropertyPath probe;              // point tested; defaults to "position"

    // Related payload — graph-shaped conditions ("x touching y"). Recorded in
    // the model now; resolution against the relation graph lands with the
    // event/Rete wiring (commit 4).
    std::string relationType;
    std::string otherId;

    // Zone payload — the authored satisfaction zone of a mathematical
    // function: satisfied when f(bindings) lies within [lo, hi] (either side
    // may be absent = unbounded, reusing the InRange lo/hi slots; a monostate
    // bound is an open side). The function itself is Person-authored OntoMath
    // — piecewise, multivariate, exact — and the bindings name where each
    // variable lives on the subject. Undefined f (outside every piece, or an
    // unbound variable) is NOT satisfied: laws never fire on undefined math.
    OntoMath::Piecewise zoneFunction;
    MathBindings bindings;

    // IsKind payload + the quantifiers' domain filter.
    BeingKind beingKind = BeingKind::AnyBeing;
    // Quantifier exceptions: "every instance ... with possible exceptions".
    std::vector<std::string> exceptIds;

    std::vector<ConditionNode> children;   // All/Any/Not members; quantifier inner test

    // Unsupported payload: the node's original JSON, kept verbatim so this
    // build can hand back law text it cannot read. shared_ptr because the
    // common case is null and ConditionNode is copied freely.
    std::shared_ptr<nlohmann::json> unsupported;

    nlohmann::json toJson() const;
    static ConditionNode fromJson(const nlohmann::json& j);

    // Tree → closure, once. The tree remains the law's text.
    ECA::ConditionPredicate compile() const;

    // Compiles this condition tree into the given ReteNetwork.
    // Returns a list of terminal node IDs (Alpha or Beta) that represent the satisfied conditions.
    std::vector<std::size_t> compileToRete(class ReteNetwork& rete,
                                           const std::string& lawId,
                                           std::size_t leftId = 0,
                                           bool leftIsBeta = false) const;

    // One-line human summary for ApplicationRecord logs.
    std::string describe() const;

    // Every property this condition addresses ON ITS OWN SUBJECT — the
    // vocabulary a being must have for the condition to be about it at all.
    // Quantifiers are deliberately NOT descended into: their inner condition
    // is evaluated against each INSTANCE it ranges over, not against the
    // law's subject, so folding those paths in would filter out exactly the
    // beings a quantified law is meant to sweep.
    void collectPaths(std::vector<PropertyPath>& out) const;

    // Factories (mirror SdfNode::leaf/binary).
    static ConditionNode compare(const std::string& dottedPath, Op op, PropertyValue rhs);
    static ConditionNode comparePaths(const std::string& dottedPath, Op op, const std::string& rhsPath);
    static ConditionNode inRegion(geom::SdfNode region, const std::string& probePath = "position");
    static ConditionNode zone(OntoMath::Piecewise function, MathBindings bindings,
                              PropertyValue zoneLo = PropertyValue{},
                              PropertyValue zoneHi = PropertyValue{});
    static ConditionNode isKind(BeingKind kind);
    static ConditionNode identity(const std::string& beingId);
    // Empty type = any relation kind; empty otherId = related to anyone.
    static ConditionNode related(const std::string& type = "",
                                 const std::string& otherId = "");
    // otherToken: a being id, or "@event.subject" / "@event.object".
    static ConditionNode overlaps(const std::string& otherToken);

    // Honest C++ instanceof, shared with everything that ranges over the
    // Universe by kind (quantifiers, folds).
    static bool matchesKind(const Singular& being, BeingKind kind);

    // (Pair quantifiers removed: model pairs as Relations in the graph.)
    static ConditionNode forAny(BeingKind kind, ConditionNode inner,
                                std::vector<std::string> exceptions = {});
    static ConditionNode forAll(BeingKind kind, ConditionNode inner,
                                std::vector<std::string> exceptions = {});
    static ConditionNode all(std::vector<ConditionNode> children);
    static ConditionNode any(std::vector<ConditionNode> children);
    static ConditionNode negate(ConditionNode child);
};

// A law's condition model is the root of one such tree.
using ConditionModel = ConditionNode;
