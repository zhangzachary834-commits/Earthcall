#pragma once

// ============================================================================
// The evaluation report an authored expression carries (2026-10-19, addendum B
// of the ray-domain plan).
//
// Human origin: Zachary Zhang commissioned Codex's ownership correction and its
// contract addendum ("legible outcomes"). Codex placed the report (2026-10-19
// 21:30): on the expression's EXISTING bearer, as a read-only derived
// PropertyDict at an authored route, with no new kind, no fixed field on every
// Singular, and no renderer service. The implementation is mine.
//
// Any Singular that carries, at an authored prefix,
//   <prefix>.expression   a ScalarField whose AST is the authored mathematics
//   <prefix>.bindings     (optional) a dictionary variable → property path,
//                         read through the Law binding reader (lawGetValue)
// answers <prefix>.evaluation, a derived reading (PropertyPath.hpp):
//   status              "admitted" | "refused"
//   value               the scalar, admitted only (stays composable)
//   reason              refused only, legible
//   admission           "exact" (the only class admitted today; no tolerance)
//   evaluation          how the number was produced
//   requestedTolerance  "none" (no error contract is authored yet: addendum C)
//   mathematicalBound, executionBound, totalBound
//                       numbers, in the result's units, when the expression
//                       IS one Integral (addendum C; see IntegralBounds and
//                       executionPremise). Otherwise "unavailable": an
//                       unavailable bound is unavailable, never zero (Codex).
//   propagatedInputBound, boundCategories (2026-10-20, Codex): the first two
//                       bounds are this expression's own; propagatedInputBound
//                       carries its inputs' and samples' upstream error (their
//                       mathematical and execution parts together, conservatively)
//   route, executionPremise   (with the bounds)
//   COMPOSITION (2026-10-20, plan §6am): a scalar expression whose bindings
//   read other reports composes their bounds. A binding path ending ".value"
//   whose parent is a report-like record (value + totalBound) is read once as
//   that record: value, bound and qualification from one evaluation. The
//   expression is evaluated over the box of input uncertainties (monotone
//   exp(−τ) gives [exp(−(τ+δ)), exp(−(τ−δ))]) plus its own execution under
//   cpuExecutionModel, with τ's signed range so exp(−τ) is never charged as
//   e^(+τ). Then: route "composition", inputBounds {var: bound | "exact"},
//   qualification "model-qualified" — or, when any input carries no bound,
//   the bounds stay "unavailable" with boundsUnavailable naming it. Nested
//   Integrals and vectors are not composed yet. A binding that reads its own
//   report is a cycle and refuses, naming it.
//   expressionDigest    FNV-1a 64 of the expression's JSON: evidence identity
//   bindings            the bound values this evaluation read (a copy)
//   backend             what executed it
// Each read evaluates afresh, so a refusal replaces a success and the report
// never outlives its inputs. Writes into the route refuse (ReadOnly).
// Claude Code · Claude Opus 5.5 · session f3af84cf-bfe3-429d-b417-2d533119f169 · 2026-10-19.
// ============================================================================

#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"

#include <memory>
#include <string>

class Singular;

namespace OntoMath {

// nullptr when the bearer carries no <prefix>.expression (then the route does
// not exist). Otherwise a fresh report, admitted or refused.
std::shared_ptr<PropertyDict> evaluationReport(Singular& bearer, const std::string& prefix);

// Registers "evaluation" as a derived reading. Idempotent; called at engine
// init alongside the other modality channels, and by tests.
void registerEvaluationReading();

} // namespace OntoMath
