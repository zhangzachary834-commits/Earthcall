# Prophetic Rete — Path-Granular Unknown-Domain Rung

**Architectural origin:** Zach's Prophetic Rete / B-Time Rete design, specifically the requirement that First-Mover and external causation remain genuine unknown variables rather than collapsing known facts into global ignorance.  
**Continuation:** GPT-5.6 Sol, continuing PR #250 / the 2026-09-19 Sun handoff.  
**Session:** `sol-prophetic-path-domain-20260919`  
**Timestamp:** 2026-09-20T06:05:15.718Z

## Purpose

PR #250 established the first unknown-variable frontier: modeled relevance edges survive opacity while `relevanceComplete()` stays false and runtime narrowing remains forbidden.

This rung makes the frontier itself capable of carrying a property-path domain without pretending current First Movers already expose a complete causal footprint.

## Invariants

1. **Positive knowledge survives partial knowledge.** A source may name paths it is known to affect even while its remainder is open.
2. **Incomplete domain means wildcard for exclusion.** If `domainComplete == false`, absence from `knownMayWritePaths` proves nothing; the source must still be treated as able to reach any queried path.
3. **Only a complete domain may exclude disjoint paths.** If `domainComplete == true`, path-alias analysis may prove a source irrelevant to a disjoint property.
4. **No new hidden capability table.** Current C++ First Movers remain domain-incomplete until Earthcall has a truthful property/capability provenance source (ultimately Relations / registered property provenance), rather than a hand-maintained C++ list.
5. **No runtime narrowing yet.** This rung exposes epistemic structure and queries needed by the future cross-Law solver; Formation Rete's authority contract does not change.

## Implementation

- Extend `UnknownWriteSource` with:
  - `knownMayWritePaths`
  - `domainComplete`
- Add one conservative source/path predicate and Index-level queries:
  - whether any unknown source may reach a property path;
  - whether every unknown source's domain is complete for exclusionary reasoning.
- Serialize the domain fields in the Prophetic report.
- Keep existing opaque sources at `domainComplete = false` by default.
- Add adversarial tests:
  - incomplete source remains wildcard even when it names a known path;
  - complete bounded source reaches aliasing paths but excludes a disjoint path;
  - a live FirstMoverLaw remains incomplete/wildcard today;
  - JSON makes the distinction visible.

## Explicit non-goals

- no hardcoded per-channel write lists;
- no claim that `Identity::FirstMover::scopes` are property capabilities (they are filesystem/save-write scopes);
- no SCC/fixpoint propagation yet;
- no widening/narrowing yet;
- no Formation-Rete consumption yet.

The next architectural step after this rung is to connect `domainComplete` and domain paths to a truthful First-Mover/property capability provenance source. Only then may cross-Law closure replace a path's external seed `Top` with an exclusionary finite state.

## Shipping boundary — Zach ontology gate

**Decision, 2026-09-20:** ship the sound epistemic machinery in this pass, but keep every
ontology-dependent authority step dormant until Zach architects the First Mover framework
(with Opus serving as constitutional/advisory review).

What is enabled by this pass:
- explicit unknown-write sources;
- preservation of structurally proven modeled relevance edges under opacity;
- path-domain representation (`knownMayWritePaths` + `domainComplete`);
- conservative path reachability/completeness queries;
- report/debug visibility;
- fail-open runtime behavior.

What remains deliberately disabled / unconsumed:
- populating `domainComplete=true` for live First Movers;
- treating absence from a live First Mover domain as negative proof;
- per-property finite external seeds derived from First Mover capability metadata;
- SCC-based cross-Law closure that depends on those finite seeds;
- widening/narrowing conclusions that would gain runtime authority from that closure;
- Formation-Rete narrowing based on the future complete closure.

**Enablement condition:** Zach supplies the ontology-native representation of First Mover
capability/property provenance and its completeness semantics. That design must identify the
authoritative source of truth and its invalidation revision(s). Until then, live First Movers
remain domain-incomplete and therefore wildcard for exclusionary reasoning.

This is intentional dormancy, not unfinished safety work. The currently enabled layer is useful
for knowledge, diagnostics, visualization, and future composition while remaining incapable of
claiming more authority than Earthcall's ontology presently warrants.
