# SUN UPDATE — V5 incremental runtime green; membership/refusal gate

<!-- NAV_BLOCK_START -->
> [!NOTE]
> **Thread Navigation: Volumetric V5 Updates (PR #343)**
> [View Full Thread Index](../00_THREAD_INDEX.md)
>
> **Related in this thread:**
> - [SUN_UPDATE_V5_CANONICAL_RECONCILED_FINAL_CI_GATE_2026-09-23.md](SUN_UPDATE_V5_CANONICAL_RECONCILED_FINAL_CI_GATE_2026-09-23.md)
> - [SUN_UPDATE_V5_EXACT_HEAD_GREEN_AND_CANONICAL_DIVERGENCE_2026-09-23.md](SUN_UPDATE_V5_EXACT_HEAD_GREEN_AND_CANONICAL_DIVERGENCE_2026-09-23.md)
> - [SUN_UPDATE_V5_EXACT_HEAD_GREEN_SEGMENTATION_GATE_2026-09-23.md](SUN_UPDATE_V5_EXACT_HEAD_GREEN_SEGMENTATION_GATE_2026-09-23.md)
> - [SUN_UPDATE_V5_FUSED_INCREMENTAL_RUNTIME_WITNESSES_2026-09-23.md](SUN_UPDATE_V5_FUSED_INCREMENTAL_RUNTIME_WITNESSES_2026-09-23.md)
> - [SUN_UPDATE_V5_LOCAL_SAMPLING_AND_CI_RECHECK_2026-09-23.md](SUN_UPDATE_V5_LOCAL_SAMPLING_AND_CI_RECHECK_2026-09-23.md)
> - [SUN_UPDATE_V5_MEDIUM_SET_COMPOSITION_CI_REVIEW_2026-09-23.md](SUN_UPDATE_V5_MEDIUM_SET_COMPOSITION_CI_REVIEW_2026-09-23.md)
> - [SUN_UPDATE_V5_MEDIUM_SET_IDENTITY_2026-09-23.md](SUN_UPDATE_V5_MEDIUM_SET_IDENTITY_2026-09-23.md)
> - [SUN_UPDATE_V5_MEMBERSHIP_GREEN_OVERLAP_NUMERICAL_GATE_2026-09-23.md](SUN_UPDATE_V5_MEMBERSHIP_GREEN_OVERLAP_NUMERICAL_GATE_2026-09-23.md)
> - [SUN_UPDATE_V5_MEMBERSHIP_REFUSAL_NATIVE_WITNESS_2026-09-23.md](SUN_UPDATE_V5_MEMBERSHIP_REFUSAL_NATIVE_WITNESS_2026-09-23.md)
> - [SUN_UPDATE_V5_NATIVE_GREEN_CANONICAL_RECONCILIATION_AUDIT_2026-09-23.md](SUN_UPDATE_V5_NATIVE_GREEN_CANONICAL_RECONCILIATION_AUDIT_2026-09-23.md)
> - [SUN_UPDATE_V5_OCCUPIED_SEGMENT_SAMPLING_2026-09-23.md](SUN_UPDATE_V5_OCCUPIED_SEGMENT_SAMPLING_2026-09-23.md)
> - [SUN_UPDATE_V5_OVERLAP_NATIVE_GREEN_RECONCILIATION_GATE_2026-09-23.md](SUN_UPDATE_V5_OVERLAP_NATIVE_GREEN_RECONCILIATION_GATE_2026-09-23.md)
> - [SUN_UPDATE_V5_OVERLAP_PHYSICS_CI_WIRED_2026-09-23.md](SUN_UPDATE_V5_OVERLAP_PHYSICS_CI_WIRED_2026-09-23.md)
> - [SUN_UPDATE_V5_OVERLAP_PHYSICS_TRIBUNAL_2026-09-23.md](SUN_UPDATE_V5_OVERLAP_PHYSICS_TRIBUNAL_2026-09-23.md)
> - [SUN_UPDATE_V5_RECONCILED_TO_789F4CB_CI_GATE_2026-09-24.md](SUN_UPDATE_V5_RECONCILED_TO_789F4CB_CI_GATE_2026-09-24.md)
> - [SUN_UPDATE_V5_SEGMENTATION_GREEN_NEXT_WITNESS_GATE_2026-09-23.md](SUN_UPDATE_V5_SEGMENTATION_GREEN_NEXT_WITNESS_GATE_2026-09-23.md)
<!-- NAV_BLOCK_END -->



Date: 2026-09-23
Repository: `zhangzachary834-commits/Earthcall`
PR: #343 — Volumetric V5: medium-set composition foundation
Branch: `sol/volumetric-v5-medium-set-composition-current-20260923`

## Fresh live-state inspection

Canonical was re-inspected first and remains:
`423cfd69dceb2959ccfe07d16fa2dd1ae86698f6`

That is the merge of PR #329 (scene-spatial synthesis DAG rung 1).

PR #343 was re-inspected at head:
`5339cd0da0a20f2363469f462cdd3a515a82c409`

It remains open, Draft, and mergeable.

V3/V4 remain landed and are not reopened by this pass.

## Incremental-runtime gate is now exact-head GREEN

Earthcall focused CI run `35932892219` completed SUCCESS on exact head `5339cd0d...`.

All four jobs passed:

- SDF range-proxy verification (macOS)
- Focused CPU tests (macOS)
- Slow Adapter independent clock (macOS)
- SDF authored-Perlin A/B (macOS Release)

The WebGPU object/radiance parity step inside the SDF verification job passed, so the newly landed native V5 witnesses are covered by the successful run.

This closes two explicit V5 plan obligations at renderer level:

1. numeric-only fused member edit => visible pixel change with no structural compile;
2. time-only fused member change => visible pixel change with no structure regeneration and fused memo reuse.

These are no longer merely compiler-level claims.

## Targeted membership/refusal architecture audit

The next unfinished rung is membership structural invalidation plus refused-member/no-stale-output.

I audited only the fused cache/refusal seam in `WebGpuRenderer::flushVolumeComposite()` and the existing V5 compiler witness.

### Membership semantics already encoded by the cache key

`VolumeSetProgramKey` is a vector of per-member six-channel expression identities.

Therefore adding/removing a member changes the set key itself and selects a different `VolumeSetProgramMemo`; numeric values and per-medium time do not belong to that structural key.

Expected bounded behavior for the native witness:

- establish valid `{A,B}` fused set;
- add structurally distinct admitted `C` => exactly one new fused set program compile;
- remove `C`, returning to the already-established `{A,B}` key => zero new structural compile and cache reuse;
- framebuffer after removal returns to the prior `{A,B}` answer within the established native tolerance.

This is the correct meaning of bounded invalidation: membership changes invalidate/select only the affected set structure; they do not flush unrelated one-medium or other set programs.

Do NOT assert that every removal must compile. Returning to an already memoized membership set should reuse its old program.

### Refusal semantics are fail-closed in the production seam

The fused path first inspects every admitted member. If any member layout refuses:

- `setMemo.ok = false`;
- `setMemo.error` receives the named member/channel refusal;
- `setMemo.pipeline = nullptr`.

The later batch stage records `volumeProgramRefusals` / `volumeLastProgramRefusal` and does not append that refused set to `_activeVolumePipelines`.

This is the correct architecture for no-stale-output: the previously valid fused pipeline may remain cached as an artifact, but a refused current answer is never scheduled for drawing.

The compiler-level V5 witness already proves an unsupported member is named as `volume set member 1` and `volume emission`. What is still missing is the native framebuffer witness that the prior valid integrated radiance is not replayed after the member becomes unsupported.

## Exact next native witness

Add this immediately beside the existing V5 fused native block in `tests/singularity/webgpu_object_test.cpp`:

1. Preserve a valid visible `{A,B}` fused baseline.
2. Add a third admitted medium `C` with a structurally distinct expression identity and measurable contribution.
3. Assert the membership-add frame reports exactly one fused structural compile and changes the pixel as expected.
4. Remove `C`, restore `{A,B}`, and assert zero structural compiles plus fused cache hits; pixel returns to the prior `{A,B}` answer.
5. Then mutate only B's `E_v` structure to unsupported `Raycast`, advance B's emission revision + set revision, render the fused set, and assert:
   - `volumeProgramRefusals >= 1`;
   - `volumeLastProgramRefusal` names both the set member and emission/Raycast;
   - center pixel is black/clear within the existing refusal tolerance, proving the previously valid fused radiance was not replayed.
6. Restore B to valid structure and verify the lawful set can render again; refusal must not poison future lawful reuse.

Keep this on the two-or-more-medium production fused path throughout. Do not satisfy the requirement with the existing one-medium V4 refusal tests.

## Why no production renderer edit in this pass

The production membership/refusal logic already encodes the intended law. The newly completed incremental-runtime CI is green. Changing production code before a native witness exposes a contradiction would be speculative churn.

The next move is therefore witness-first. If the native membership/refusal witness fails, repair only the narrow fused set-key/refusal/batch scheduling seam it exposes.

## Remaining V5 work after membership/refusal

After that native gate is green:

- audit whether the plan's combined-extinction/sequential-alpha counterexample and independent overlap-contribution requirements have a sufficiently direct native witness or only compiler structure evidence;
- reconcile PR #343 with current canonical;
- run final exact-head focused CI;
- only then decide whether V5 is ready to leave Draft.

## Guardrails

- No sequential whole-medium blending regression.
- No time or numeric values in structural identity.
- No global volume-cache flush for membership changes.
- Refusal is fail-closed: cached artifact may exist, stale current answer may not draw.
- No V6 scope, multiple scattering, GI, spectral transport, or new Medium nouns.
- Targeted reads only; no big chungus.

## Exact continuation point

Implement the native membership add/remove + refused-member/no-stale-output witness described above. Run exact-head focused CI. If green, audit the remaining V5 witness matrix and reconcile canonical.

— GPT-5.6 Sol
