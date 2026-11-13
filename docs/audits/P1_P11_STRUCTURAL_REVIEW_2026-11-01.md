# P1–P11 routed transport: structural review and three corrective gates

**Codex / GPT-6 / Sixth-One Sun / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-11-01T12:20:00Z.** Commissioned by Zach: defer window focus/priority work, review Firmament / Clawd Opus 4.6 / Sparkly Guy's actual P1–P11 implementation, and first tell the council everything learned. This audit responds to the [complete post-P11 council handoff](../../agent%20intercom/communication-threads/sdf-and-rendering/OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md#sixth-one-sun-complete-post-p11-catchup-2026-11-01).

## Verdict and evidence boundary

**Do not treat P11 as correctness-approved yet.** Three actionable defects remain: an introduced publication hole, an inherited invalid disjointness proof, and insufficient viewport admission for fixed strip caches. These are source/CPU findings, not measured native pixel failures or a diagnosis of Chrome's FPS effect.

Reviewed implementation: `/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport`, branch `claude/ray-domain-transport-rung0`, HEAD `50012eb6b`; P11 `6911446c3`; cumulative comparison starts at `074f4075a`. Default checkout `sync-from-earthcall-main` at `3ef823a76` is a correspondence destination, not the source of this implementation. The historical P1–P3 memory snapshot was rechecked against current source and does not describe today's completed phase sequence.

No production source was changed, no GPU was acquired, no native frames or new timings were collected, no saves were edited, and no commit/push was made for this review. A bounded synthetic CPU adversary was compiled with `-O3 -DNDEBUG`; integer/index and publication findings also use explicit inspection of the actual WGSL/host dispatch path. The [receipt](data/ray_transport_p1_p11_structural_review_2026-11-01.json) binds source hashes and labels each evidence type.

## 1. [P1] P11 can leave ordinary pixels unpublished when an entire strip is empty

Relevant current source:

- `VolumeRoutedTransport.inc:170–193`: setup body extraction stops before `var transmittance = 1.0`; ordinary admitted box-hit rays initialize `ecRays` at line 192 but no longer initialize publication.
- `SdfWgsl.cpp:6181–6214`: extracted setup includes union-AABB and opaque-depth clipping. The per-member support work starts after the extraction boundary, at line 6220.
- `VolumeRoutedKernels.hpp:211–213`: a wave-proved empty tile enters no producer list.
- `VolumeRoutedKernels.hpp:246–249`: zero total routed tiles means zero consumer workgroups.
- `VolumeRoutedTransport.inc:256–257`: producer and consumer dispatches use those indirect counts; there is no intervening publication clear.
- `WebGpuRenderer.cpp:1316–1327`: the fragment now trusts a nonnegative publication before its ordinary ray/box/depth computation.

P11 moved the empty publication from setup to `ecConsume:53`, while retaining P6's ability to skip that consumer for a completely empty strip. Consequently there is **no writer** for normal box-hit/support-empty pixels in that strip. Publication storage persists across frames, so a previous accepted nonzero result can be reused. Initial zero-filled allocation can hide this; a lit-frame-to-empty-frame transition exposes the ownership gap.

Reachable geometry: four admitted clamped-wave media have half-boxes `(20,20,20)`, x/y limits 20, width 1, three amplitudes 0.1, and margin 0.001. A ray from `(0,0,10)` towards `+y` hits the union box, is not opaque-clipped, and has `t1=20<2048`. Its z stays 10, outside the classifier's `zEnv=1.3` slab. This remains empty over a neighborhood of rays, including a sufficiently central horizontal strip of a perspective view. All its tiles can be empty while ordinary pixels still pass setup. Some other rays being setup-discarded does not publish the ordinary rays.

**Required repair:** restore an explicit publication owner for every in-frame ray even when all lists are empty. Initialization, an empty-publication kernel, or a guaranteed consumer can satisfy this. Preserve setup-handled and zero-budget refusal ownership; blindly running an extra consumer can create duplicate publication on the latter path. Do not resolve this by leaving old publication values valid.

**Required witness:** poison publication with finite, accepted, nonzero values; exercise an all-box-hit/support-empty strip; then repeat after a lit frame and across mixed strips. Every empty pixel must publish zero in the current frame. Include missing-consumer and final-owner controls. This defect was introduced by P11's setup/publication change relative to P10.

## 2. [P1] A near-parallel ray truncates the tile hull and can misroute overlapping media as S2

`VolumeRoutedKernels.hpp:148` unions one near-parallel ray's bounds and then `break`s out of the 16-ray loop. Later rays never contribute to that member's entry/exit hull. Lines 198–225 use the incomplete hull to prove two members are disjoint and ordered. `s2ProducerMain:806–902` trusts that proof, emitting all spans of the first member and then the second without checking for actual per-ray overlap. Shared attenuation requires both members together where they overlap; serial member integration can give incorrect emitted radiance.

This is **carried through the campaign**, not newly introduced by P11: the pre-P1 `074f4075a` serial classifier has the same break, and P10's cooperative reducer also breaks on its near-parallel state. The repository CPU serial and cooperative mirrors share that mistake (`positive_e2e_cpu_verify.cpp:158–162` and `443–450`). Their bit-identical agreement therefore cannot establish disjointness correctness.

The executed CPU adversary uses a real rectilinear 4×4 pixel tile at `(640,720)` in an odd-width `1281×1440` viewport, 45° vertical FOV, eye `(0,-38,-40)`. Two finite media have y/z boxes `[0,10]²` and `[8,18]²`, half-x 100, width 5, and three small nonzero harmonics. The first ray has exactly zero x direction and y/z slope about 1.0001: its two supports are disjoint. Later rows genuinely cross both media together.

Observed CPU results:

- serial cohort = S2; cooperative cohort = S2;
- **12/16 rays have overlap**, accepted by the CPU S2 mirror with reason zero;
- those 12 S2 schedules have chronology violations at the member transition;
- an independent authored-density calculation gives both densities positive at the same world point, approximately `0.003105905` and `0.004962547`;
- a one-change control (`break` → `continue` in the serial classifier's near-parallel branch) examines all rays and returns Fallback.

This executes repository CPU helper functions, not a GPU shader. The helper producer uses historical 96/32/20 scratch and is not an exact implementation of final P11's streaming scratch. The current WGSL retains the same defective hull branch and no independent S2 overlap check; no native pixel-error magnitude is claimed.

**Required repair:** union every ray in both serial and cooperative reductions. Add an independent per-ray disjointness/chronology refusal gate before committing S2 output, or an equally strong checked invariant. If the proof is unavailable, use the existing overlap-capable path/Current fallback.

**Required witness:** exactly parallel center-column rays, odd and partial viewport widths, a tile whose first ray is disjoint but later rays overlap, reversed member order, and distinct emission/extinction so erroneous composition cannot commute away. Derive overlap independently; do not make two copies of the same classifier approve each other.

## 3. [P2] The viewport guard permits more strips than cached arrays can hold

`VolumeRoutedTransport.inc:108` admits up to 64 MiB of publication storage. Lines 111–119 calculate strip rows, then silently clamp the required strip count to 64. Header lines 26 and 47 have fixed 64-entry producer/local bind-group arrays. Allocation, configuration upload, and group creation honor the cap; the encode loop at lines 248–257 runs until the full height is covered, without that cap, indexing those arrays.

Concrete valid-dimension case with a 128 MiB storage-binding limit:

| Quantity | Value |
|---|---:|
| Viewport | 1024 × 4096 |
| Publication bytes | 67,108,864, admitted by the guard |
| Strip rows | 48 |
| Required strips | 86 |
| Configured/cached strips | 64 |
| First invalid array index | 64 |

This is a host-side out-of-bounds access, not a WGSL robust-buffer-access guarantee. P2 commit `2434491de` introduced the 64-strip clamp and fixed cached arrays; the pre-P1 and P1 encoders allocated per strip without that cap. This is an introduced P2 admission/indexing defect. No actual native crash was induced.

**Required repair:** refuse `neededStrips > capacity` before admitting the routed path, or allocate every related structure for the full required count. Clamping the encode loop alone silently drops pixels and is insufficient. Cover a tall viewport, the 64-strip boundary, resize back to a normal viewport, and device-limit variations.

## Other structural checks and witness drift

The current producer and consumer address formulas consistently use ray-contiguous storage (`id*64` singles, `id*16` overlaps); no remaining step-major read/write mismatch was found in the inspected paths. Streaming scratch guards and packed stack-depth indexing were inspected; their declared capacities are not a measured Metal spill or occupancy result. Buffer release invalidates the cached groups, and P11 rebuilds capacity-dependent routeArgs on viewport change.

The cumulative `WebGpuRenderer.cpp` changes inspected are the routed fragment short-circuit and routed-only spec upload. Kernel setup/admission is behind `wantRouted`. **No new shared Current per-pixel marching loop or sampling-quality change was found in this diff.** This does not establish an exact matched-build performance result or rule out unrelated default-branch changes.

The native helper is fixed at `2560×1440`, with its poison/refusal/parity subcases at poses 0 and 32. It does not cover the three adversaries above. The CPU helper retains historical producer capacities and lacks the current z-envelope AABB clip: our additional support-empty geometry returns Fallback in that old mirror even though the inspected current WGSL proves it empty. These are concrete reasons to refresh witnesses against the final implementation rather than inheriting a phase name's old green result.

Reproduce the bounded source/CPU findings from the feature checkout:

```sh
python3 scratch/probes/ray_transport_p1_p11_structural_review.py \
  --out /private/tmp/ec-p1-p11-review
```

Requires that checkout's existing `build/compile_commands.json` and dependency headers. It does not build or launch Earthcall. **Exit 0 means the three defects were reproduced, not that the renderer passed.** It fails its source guards if the reviewed snippets change; update the obligations after repair. Its first generated build needed an explicit `<iostream>` include; an initial attempt to use the old CPU mirror as the current empty-slab oracle was rejected and replaced by the explicitly labeled WGSL/source geometric proof above. No failing attempt is counted as successful native verification.

## Council continuation

First repair publication ownership and disjointness, then close viewport admission, each with an adversarial witness and a bounded commit. Keep production evaluation/authored equations intact. After those corrective gates, rerun the focused native poison/refusal/parity checks before clean cost measurement; do not resume exhaustive OS-priority work as a substitute for these correctness repairs.

Zach's any-one-specified-view 28–30 FPS checkpoint, current-or-better authored accuracy, and Current fallback remain the shipping gates. The previous handoff records Chrome burst effects and verified Game Mode activation separately; neither an FPS benefit from Game Mode nor the shipping milestone has been established. This review introduces no new visible behavior for Zach to accept.
