# AUTHOR — Document Validity as Relational Crystallization

**Status:** ⚑ AUTHOR deliberation; no implementation authorized or chosen  
**Originator:** Zachary Zhang  
**Developed with:** OpenAI Codex  
**Session ID:** `01a06f05-0500-7b40-ba8f-12103586d9ff`  
**Date:** 2026-09-04  
**Timestamp:** 2026-09-04T18:05:50-07:00

## Zach's originating connection

Zach connected three Earthcall problems that had previously been discussed in separate
offices:

1. documentation drifts because documents do not automatically carry an expiry signal;
2. Relation gaps prevent agents from recovering genuinely unified frameworks;
3. incomplete crystallization lets meaning fail to survive integration and persistence,
   the insight that helped inspire Sonnet 4.5's *Five Days of Velocity and One Sacred Thing
   Breaking*.

The dialogue addendum to
[`The_World_Is_The_Product.md`](../../../../Reflections%20on%20Earthcall%27s%20Progression/Reflections%20on%20Repo%20State/The_World_Is_The_Product.md)
records Zach's thought and Codex's interpretation in full.

## Decision to make

Decide whether Earthcall should treat a document claim's present validity as an explicit
Relation to the source state, test registry, save schema, authored world, Person witness, or
other evidence that warranted it at a particular Moment.

The candidate principle is:

> A claim does not expire because time passed; its prior witness stops covering the present
> when a dependency it names crosses a relevant change edge.

The safe machine conclusion would be **UNWITNESSED NOW**, not **FALSE**. Historical claims and
their Moments remain preserved rather than being rewritten as though they were never true.

## Ontological constraints

- Do not add a C++ `DocumentClaim` class or another domain kind; documents, claims, tests,
  commits, and saves would be authored extra-spatial beings composed from existing primitives.
- Do not build a second permission or provenance system; reuse Relations, Formation, Moment,
  authorship, Law, and the existing TransferPolicy boundary.
- Repository tooling may sense file/test changes as a First Mover, but Persons author what
  those changes mean and which witnesses require renewal.
- Invalidation is an edge (`dependency-changed`); stale/unwitnessed is state.
- Typed Relations must say why two artifacts are joined: `depends-on`, `verified-against`,
  `supersedes`, `corrected-by`, `personally-witnessed-by`, or another authored Lexeme.

## Questions reserved for Zach

- Should this become an Earthcall framework, remain workshop-only tooling, or climb those
  rungs gradually?
- Which claims are important enough to enter the graph, and who may assert their dependency
  Relations?
- Does a Person's experiential witness differ in kind or only in Relation type from a test's
  mechanical witness?
- Should a changed dependency automatically mark a claim unwitnessed, or only publish an
  event that an authored Law may interpret?
- Which parts belong in the living world and which belong only in repository-side First
  Mover sensing?

## Existing first rung

`scratch/probes/router_truth_probe.py` already compares several constitutional claims with
the present repository. On 2026-09-04 it reported seven failures: the missing manifesto
router target repeated through the router symlinks, the 75-versus-97 registered-test mismatch,
and three broken documentation links. Its dependency graph remains implicit in Python and its
voice is heard only when someone invokes it.

No implementation work should begin until Zach chooses the intended rung and authority
boundary.

---

*OpenAI Codex, recording Zachary Zhang's originating connection — session
`01a06f05-0500-7b40-ba8f-12103586d9ff` — 2026-09-04T18:05:50-07:00*
