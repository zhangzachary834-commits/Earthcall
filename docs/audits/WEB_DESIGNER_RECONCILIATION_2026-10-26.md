# Web Designer reconciliation — 2026-10-26

## Why this pass exists

The sealed Web Designer branch stopped at `e5793c71279dc22975291486618e77c869fa22f3` on 2026-10-21 while `sync-from-earthcall-main` continued through `7f0e351ed2ec39a8684f007d54a828bd953015d2` by 2026-10-26.

Direct PR #786 proved the old branch is not mechanically mergeable: it diverged from common base `2814a16aaa961eb6da3b9baf2a617f3ea2e1e957`, with current default carrying substantial later engine work.

This reconciliation therefore starts from current default and transplants the sealed Web Designer artifact forward rather than merging the fossil branch backward over newer work.

## Preserved current-default authority

Current default remains authoritative for all post-seal engine evolution, especially RelationManager lifetime retention, endpoint indexing, graph generation tracking, later Law/Terminal work, rendering research, and living documentation.

The reconciliation does **not** replace current `RelationManager` or `Universe` with their 2026-10-21 copies. It composes only the sealed Web additions:

- `RelationManager::isSuperseded` / `isCurrent`;
- `Universe::isRelationSuperseded` / `isRelationCurrent`;
- the query contract that raw Relation enumeration preserves historical truth while semantic currentness follows first-class `superseded-by` Relations.

## Carried-forward Web Designer artifact

The reconciliation carries forward the sealed Web Designer sources, fixtures, focused test, prototype, architectural contract, task records, and internet/multi-Person plan.

The governing boundaries remain:

- Person-exclusive authorship/stewardship; efficient causation is not authorship.
- Stable Person identity, not display spelling.
- Lexeme/Relation/Law meaning remains ordinary Earthcall state; no Web semantic gift shop.
- Browser HTML/CSS/DOM is a foreign realization target.
- `TerminalLine` is Terminal-local ingress and is not generalized into Web.
- Historical denotation is preserved through first-class supersession rather than deletion or adapter-private flags.
- Raw CSS fails closed unless an exact `web.rawCss` capability Relation grants the governing Person authority over the exact page/root.
- Persistent theme definitions require owning Zone custody; Formation membership is non-owning and cannot substitute for custody.
- Detached no-owner theme application is draft-only and creates no persistent theme-definition being.
- One semantic act must not multiply because Formation, Universe, and local graph views can all see the same Relation.

## Historical thread handling

The old feature branch contains a much longer append-only Sun/Sparkly review chronology than current default, but current default changed the older prefix of that same thread. Therefore this pass does not overwrite the living thread with fossil bytes.

The sealed branch and diagnostic PR #786 remain the historical archive. The living thread receives only a reconciliation addendum.

## Verification status

At branch construction time, the code transplant was complete but had not yet received a fresh current-default build/test witness. The prior sealed branch reported 55/55 local focused Web Designer tests; that evidence belonged to the old head and was not silently promoted to this reconciliation head.

### Verification witness (2026-10-26)

Executed on Apple Silicon (macOS 26.6, Apple M5):

1. **Compilation repair:** Initial build identified missing `#include "Relation/Relation.hpp"` in `src/ZonesOfEarth/AuthorsOfLaw/Universe.cpp`, where `isRelationSuperseded()` accessed members of incomplete `const Relation`. Including `Relation/Relation.hpp` completed compilation cleanly.
2. **Web Designer modality suite:** `web_designer_modality_test` executed from repo root and achieved **55/55 checks passed (0.44s)**:
   - Sibling sequence strictly derived from `precedes` Relation topology (numeric `web.order` rejected as override).
   - Provenance relation `authored-by` confirmed as sole source of truth (shadow properties eliminated).
   - Multi-Person provenance isolation (Alice & Bob) verified.
   - Stable DID (`did:earthcall:...`) enforced; duplicate display name collision strictly quarantined.
   - Theme definition requires owning Zone custody; dangling/unowned definitions strictly refused.
   - Raw CSS fails closed; requires explicit, page-scoped `grants-capability` Relation.
   - World-level supersession and currentness query contract verified across first-class `superseded-by` sub-relations without historical erasure.
3. **Focused relation suite:**
   - `web_designer_modality_test`: PASSED
   - `relation_manager_test`: PASSED
   - `relation_state_index_test`: PASSED
   - `relation_endpoint_index_test`: PASSED
   - `relation_serialization_topology_test`: PASSED
   - `zone_relation_roundtrip_test`: PASSED (10/10 checks)
   - Result: 6/6 test targets (100%) passed in 7.57s.

**Signed:** Antigravity / Gemini / session `50cf28df-09e9-4c2b-a4d5-1f796abbc73a` / 2026-10-26T13:51:30-07:00

## Reconciliation branch

`sun/web-designer-reconcile-20261026`

Human origin: Zach requested immediate reconciliation after returning to Earthcall on 2026-10-26.
