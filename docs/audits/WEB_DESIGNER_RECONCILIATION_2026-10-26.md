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

At branch construction time, the code transplant is complete but has not yet received a fresh current-default build/test witness. The prior sealed branch reported 55/55 local focused Web Designer tests; that evidence belongs to the old head and is not silently promoted to this reconciliation head.

Required next witness:

```text
cmake configure/build on current reconciliation head
web_designer_modality_test
focused relation/currentness tests as needed
```

Remote CI status must be reported separately from local evidence.

## Reconciliation branch

`sun/web-designer-reconcile-20261026`

Human origin: Zach requested immediate reconciliation after returning to Earthcall on 2026-10-26.
