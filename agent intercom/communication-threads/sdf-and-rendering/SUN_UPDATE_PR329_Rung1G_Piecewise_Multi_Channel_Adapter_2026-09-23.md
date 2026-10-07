# SUN UPDATE — PR #329 Rung 1G Piecewise Multi-Channel Adapter

Date: 2026-09-23
Branch: `sol/scene-spatial-synthesis-dag-rung1-20260922`
PR: #329

## Read-first continuity

Continue from `SUN_UPDATE_PR329_Rung1F_Cross_Domain_Rendered_Field_Synthesis_2026-09-23.md`. Do not restart Rungs 1A–1F.

Rung 1F is already green on its direct executable gate and established the key boundary: canonical mathematical execution may be shared, but theorem authority is scoped by rendered semantic channel.

## This pass

Added the first real `OntoMath::Piecewise` multi-channel adapter witness:

`tests/singularity/rendered_field_piecewise_synthesis_test.cpp`

Implementation/fix commits in this pass:

- `29ba33bee9f622387042748dc3abffe1645ebdd9`
- `e8bc5c8f1ee6c666dd8793fa6aa63db0113a91a7`
- `5aa683023e9b470ffeca77b8a294076446284aa3`

The witness uses actual Earthcall `OntoMath::Piecewise::Piece` topology rather than only isolated `MathNode` roots.

## Channels covered

Four independently authored scalar rendered meanings are compiled through one adapter substrate:

- source radiance `rho`;
- medium density `D`;
- medium extinction `sigma_t`;
- medium scattering `sigma_s`.

Each intentionally contains byte/semantic-identical child mathematics in two interval pieces. The adapter requires the child `MathNode` execution IDs to canonicalize across channels while preserving each compiled Piecewise vessel's channel identity.

This extends Rung 1F from root expressions to actual Piecewise field vessels.

## Piecewise topology is semantic structure

The adapter identity explicitly includes:

- `inputVariable`;
- piece count/order;
- lower/upper bound presence;
- bound values;
- bound inclusivity;
- canonical child-math IDs.

It does **not** use `Piecewise::print()`, whole-scene serialization, or a rendered textual dump as identity.

A density bound edit from split `0` to split `-2` must change Piecewise topology identity while preserving the already-compiled child math IDs.

## Numeric edit locality

The witness edits only the first density piece's scale coefficient from `3` to `7`.

Required result:

- density's edited child gets a new canonical math ID;
- density's untouched sibling piece reuses its old math ID;
- source-radiance compiled child remains unchanged;
- extinction compiled child remains unchanged;
- no unrelated channel is recompiled by the test adapter.

This is the first bounded Piecewise form of the same incremental-repair law proven earlier for raw MathNode trees.

## Runtime / Timeline movement

The witness evaluates the real density Piecewise across multiple `x` values and multiple admitted runtime `t` values without invoking adapter compilation again.

Required result: runtime coordinate movement causes zero Piecewise topology rebuilds.

The first witness deliberately supplies `t` even though its scalar expressions do not yet consume it. A follow-up should add an actual `t`-consuming Piecewise child once the scalar adapter execution/evaluation road is generalized, while preserving the same no-rebuild law.

## C_v type boundary

`C_v` is vec3 truth. Rung 1G does **not** flatten it to scalar to make the experiment convenient.

The witness constructs a real Piecewise whose MathNode root is `VectorConstruct` and requires the scalar adapter to refuse it.

This preserves:

`D != sigma_t != sigma_s != C_v`

and establishes the next adapter requirement: add a typed vec3 lane rather than borrowing scalar compatibility behavior.

## CI gate now wired

The prior limitation is closed by commit `cac4bc03aea28d0e9ed032c6df6a614b3947088c`.

`.github/workflows/earthcall-ci.yml` now explicitly builds and executes:

`rendered_field_piecewise_synthesis_test`

inside the existing `SDF range-proxy verification (macOS)` proof-witness job, adjacent to the two earlier Scene-Spatial synthesis witnesses.

This matters because workflow run #2632 was fully green on the previous head, but it did not contain the new Piecewise target and therefore could not establish Rung 1G. Do not use #2632 as Rung 1G execution evidence.

The next exact-head workflow after this documentation-only successor is the authoritative Rung 1G gate.

## Canonical reconciliation completed

The four previously-audited canonical commits from `c18802e7` through `3e46af88ed9986a727b8696598deac3b9917ed29` were disjoint from PR #329. Their changed files are limited to three architecture/interrelation docs plus Borealis Sanctuary authored assets/helpers/validation.

They were reconciled with a real two-parent merge commit:

`921fc8b1f42a4faa953500c8d717bc291444b749`

The merge tree was built from canonical `3e46af88` and overlaid only PR #329's twelve changed files, preserving both sides rather than snapshotting away incoming default files.

After reconciliation, judge CI only from the current head or a documentation-only successor.

## Full rendered-field plan status

The master implementation plan requested by Zach is already present from Rung 1F:

`docs/plans/RENDERED_FIELD_SEMANTIC_SYNTHESIS_IMPLEMENTATION_PLAN_2026-09-22.md`

It covers geometry/SDF, materials/surfaces, source `rho/chi/alpha`, multiple sources, visibility/shadows, `D`, `sigma_t`, `sigma_s`, `C_v`, future `Phi`, future `E_v`, multiple media, material response, GI/indirect transport, later spectral extensions, and Screen/presentation separation.

The radiance and volumetric plans were also linked/corrected in Rung 1F. Do not redo that documentation expansion unless a newly landed rendered channel is absent.

## Next bounded actions

1. Inspect the exact-head workflow triggered after `921fc8b1` / this documentation successor.
2. If red, fix only the concrete Piecewise build/runtime failure.
3. If green, record Rung 1G scalar Piecewise as CI-green.
4. Then add typed vec3 `C_v` support without scalar coercion.
5. Add an actual `t`-consuming Piecewise expression and prove runtime Timeline movement changes values without semantic rebuild.
6. Add channel-scoped proof records at the Piecewise vessel level, not merely the child MathNode level.
7. Then measure compile/proof cost versus repeated evaluation before selecting the first production A/B lane.

No production renderer/WGSL code was changed in this pass.
