# Mathematical Complexity Companion — Earthcall SDF Calculation and Rendering Pipeline

**Date:** 2026-09-18  
**Timestamp:** 2026-09-18T23:57:00-07:00  
**Agent:** GPT-5.6 Sol  
**Session ID:** `chatgpt-2026-09-18-sdf-pipeline-audit`  
**Audited base:** `sync-from-earthcall-main` @ `724dd256aa0599caba63aa68c52352c52de6349f`  
**Companion to:** `2026-09-18_sdf_calculation_and_rendering_pipeline_inefficiency_audit.md`

## Purpose

This document formalizes the performance shape of Earthcall's current implicit/SDF rendering pipeline and the expected asymptotic changes of the companion audit's proposed optimizations.

The goal is not to pretend that Big-O alone predicts GPU frame time. GPU execution depends on occupancy, SIMD divergence, cache locality, compiler CSE, memory bandwidth, screen coverage, shader ALU mix, and hardware. The purpose of the analysis is instead to expose **which multiplicative factors Earthcall currently pays**, which factors the proposed architecture can remove, and which costs remain irreducible because they preserve authored mathematical truth.

The most important distinction throughout is:

> reducing the constant cost of one field evaluation is useful; reducing the **number of places where Earthcall must perform an exact field evaluation at all** changes the dominant multiplicative term.

---

# 1. Symbols and cost model

Let:

- (P) = number of rasterized fragments/pixels covered by an implicit object's proxy geometry in a frame.
- (I) = average number of fine raymarch iterations for a fragment that enters the implicit volume.
- (I_{max}) = hard iteration ceiling, currently 192.
- (E) = cost of one exact authored SDF/OntoMath evaluation at one point.
- (G) = number of extra exact SDF evaluations required to obtain the step gradient when no analytic gradient is available.
- (N) = number of extra exact SDF evaluations required to obtain the final surface normal.
- (V) = cost of `fieldEval(p)` volumetric evaluation per marcher iteration when a FieldNode density path is present.
- (S) = size of the SDF structural tree / OntoMath AST.
- (Q) = number of numeric parameters emitted by the SDF compiler.
- (M) = number of SDF objects submitted in the frame.
- (K) = number of distinct compiled SDF structural pipeline shapes active in the frame, (K le M).
- (B) = total bytes of SDF instance + parameter + height-grid data streamed to the GPU in a frame.
- (H) = number of cells/nodes in a conservative spatial acceleration hierarchy.
- (D_H) = traversal depth of that hierarchy; for a balanced octree/BVH-like structure, typically (O(log H)).
- (A) = number of ambiguous/relevant hierarchy cells a ray actually visits before exact evaluation is required.
- (Z) = number of exact field-evaluation sites that remain after spatial proof-skipping.
- (R_s) = structural revision count.
- (R_p) = parameter/value revision count.

For the current gradient-corrected generic expression path:

- one raw sample occurs through `sdfSampleStep(p)`;
- when its gradient length is unavailable, three additional forward samples are taken;
- thus (G = 3) and each fine step costs approximately ((1+G)E = 4E);
- fallback surface normal uses four tetrahedral evaluations, so (N = 4).

For the analytic-gradient path:

- one jet evaluation returns value + gradient;
- effectively (G approx 0), but its single (E_{jet}) is more expensive than a scalar-only evaluation;
- final normal can also use one jet evaluation rather than four finite differences.

---

# 2. Current GPU asymptotic cost

## 2.1 Generic exact-distance SDF primitives

For a true distance field primitive or CSG tree that does not need gradient correction, the dominant fragment work is approximately:

[
T_{	ext{distance}} = O(P cdot I cdot E)
]

with final-hit shading adding:

[
O(P_{	ext{hit}} cdot N cdot E)
]

where (P_{	ext{hit}} le P).

If the SDF tree contains (S) nodes and evaluation is emitted as straight-line WGSL, then a conservative CPU-like operation-count abstraction is:

[
E = O(S)
]

so:

[
T_{	ext{distance}} = O(P cdot I cdot S)
]

This is not a claim that GPU execution literally performs one serial operation per node. It says that authored structural complexity is multiplied by every exact point sample.

## 2.2 Generic authored implicit expression with finite-difference step gradient

Current code can evaluate the same complete expression four times per fine march step:

[
E_{	ext{step}} approx (1 + G)E = 4E
]

therefore:

[
T_{	ext{expr}} =
Oleft(P cdot I cdot (1+G) cdot Eight)
]

with (G=3):

[
T_{	ext{expr}} =
O(P cdot I cdot 4E)
=
O(P cdot I cdot E)
]

in strict Big-O notation, because 4 is constant, but the constant is architecturally enormous because (E) can itself be a heavy noise/CSG/OntoMath program.

If (E = O(S)):

[
T_{	ext{expr}} = O(P cdot I cdot S)
]

but the practical operation-count model is approximately:

[
C_{	ext{expr}} approx 4PIS
]

plus final-hit normal work:

[
C_{	ext{normal}} approx 4P_{	ext{hit}}S
]

This is why asymptotic notation alone hides an important current multiplier.

## 2.3 Expensive noise expression

Let (C_n) be the ALU/hash/interpolation cost of one Perlin-noise evaluation and let the authored expression contain (n) noise calls.

Then one field sample approximately costs:

[
E approx O(S_{	ext{cheap}} + nC_n)
]

and the current finite-gradient path becomes approximately:

[
T_{	ext{noise}} approx
P cdot I cdot 4(S_{	ext{cheap}} + nC_n)
]

The repository's measured native-resolution Perlin regression is consistent with this multiplicative form: when the field is changed to a trivial one-operation plane, the same camera becomes dramatically cheaper.

## 2.4 Screen-resolution scaling

For a screen-filling proxy:

[
P approx W cdot H
]

Thus, holding average step count and field program constant:

[
T propto W H
]

A move from (512 	imes 512) to (2880 	imes 1800) changes pixel count by:

[
rac{2880 cdot 1800}{512 cdot 512}
=
rac{5{,}184{,}000}{262{,}144}
approx 19.78
]

So a completely screen-bound implicit workload can expose almost a **20× pixel multiplier** before considering cache effects, occupancy changes, or divergence.

The repository's observed Perlin increase is not exactly 19.78×, because real GPU scaling is nonlinear, but the direction and magnitude explain why an apparently tolerable 512×512 field can become disastrous on a Retina-class framebuffer.

---

# 3. Why reducing the 192-step cap failed

The hard bound gives a formal worst case:

[
T_{max} = O(P cdot I_{max} cdot E)
]

with (I_{max}=192).

It is tempting to reason:

> halve (I_{max}), halve frame time.

That only follows if many rays actually terminate because they hit (I_{max}).

Let (I_{	ext{actual}}) be the number of iterations a ray performs before one of the other exit conditions terminates it.

Then:

[
I = min(I_{	ext{actual}}, I_{max})
]

If the measured workload has:

[
I_{	ext{actual}} < 24
]

for most expensive rays, changing:

[
192 ightarrow 96 ightarrow 48 ightarrow 24
]

does not change (I), therefore it does not materially change runtime.

This exactly matches the existing probe result: expensive horizon rays were ending due to distance/volume traversal, not because the loop exhausted its budget.

The iteration ceiling was therefore not the active term in the measured case.

---

# 4. Current CPU-side structural cost

## 4.1 WGSL code generation

The SDF compiler recursively traverses the SDF/OntoMath structure.

Ignoring string-concatenation allocator details, structural compilation is at least:

[
T_{	ext{compile}} = O(S + Q)
]

because every structural node must be visited and every parameter emitted.

With ordinary `std::string` construction, practical cost can become larger than a pure node-count model when repeated concatenations copy growing strings. Let (L) be generated WGSL length. A naive sequence of repeated append/copy operations can range toward:

[
O(L^2)
]

in pathological allocation/copy behavior, although standard library growth strategies usually reduce this substantially.

The important current fact is simpler: if structure did not change, **any nonzero structural compilation cost is unnecessary**.

## 4.2 Memoized Program copy

Current cache hit behavior copies `entry.prog` into a local `Program`.

Let:

- (L) = WGSL string length;
- (Q) = parameter count.

A Program copy is approximately:

[
T_{	ext{ProgramCopy}} = O(L + Q)
]

per implicit draw.

Across (M) SDF draws:

[
T_{	ext{ProgramCopies}} = O(M(L+Q))
]

even when every object is static and every cache lookup succeeds.

This does not recompile the shader, but it destroys the ideal complexity of a cache hit.

An ideal structural cache hit should be:

[
O(1)
]

for structural lookup/reference acquisition, plus only the dynamic data work actually required by that frame.

## 4.3 Heightfield proof walk

`geom::isHeightfieldExpr` is structural AST analysis.

If the tree contains (S) nodes:

[
T_{	ext{heightfieldProof}} = O(S)
]

Current draw path can repeat this every frame, making:

[
O(MS)
]

per frame across (M) implicit objects.

If cached on structural revision, this becomes:

[
O(S)
]

once per structure change, then:

[
O(1)
]

per frame lookup.

## 4.4 Derived SDF reconstruction

Calls such as `sdfFromSmooth` and `sdfFromComplex` reconstruct structural SDF values from stable geometry state in the render path.

If construction cost is (O(S_d)), current repeated frame cost is:

[
O(MS_d)
]

where a revision cache changes it to amortized:

[
O(R_s S_d)
]

over the object's lifetime, rather than frames × objects.

When (F) is the number of rendered frames and (R_s ll F):

Current:

[
O(FMS_d)
]

Revision-cached:

[
O(R_sMS_d + FM)
]

For static geometry where (R_s approx 1), the structural reconstruction factor effectively disappears from steady-state rendering.

---

# 5. Current GPU upload and binding cost

`flushSdfDraws` batches by structural pipeline, which reduces draw calls from approximately (M) toward (K).

That is good:

[
	ext{draw calls} = O(K), quad K le M
]

But the CPU still repacks and uploads the frame's SDF data.

Let total uploaded bytes be (B). CPU-to-GPU transfer work is approximately:

[
T_{	ext{upload}} = O(B)
]

per frame.

Across (F) frames:

[
O(FB)
]

even when most parameters never change.

The frame also creates approximately two SDF bind groups per active pipeline batch:

[
O(K)
]

bind-group creation calls per frame.

Again, this is not an asymptotic disaster by itself, but it is repeated driver/API work whose ideal steady-state cost for unchanged bindings is zero.

---

# 6. Proposed change A — zero-copy structural Program cache hits

Replace local Program copies with references/pointers to immutable cached structural programs.

Current cache-hit cost:

[
O(L + Q)
]

Proposed:

[
O(1)
]

for lookup/reference acquisition.

Across (M) draws and (F) frames:

Current:

[
O(FM(L+Q))
]

Proposed:

[
O(FM)
]

for cache access, with parameter handling separated into the dynamic path.

If (L) is tens of kilobytes, this is a meaningful practical reduction even though neither side changes the shader's pixel complexity.

---

# 7. Proposed change B — split structure revision from parameter revision

Current architecture uses one revision to invalidate a Program that contains both:

- structural WGSL;
- numeric parameter values.

Suppose over an editing interval:

- structure changes (R_s) times;
- numeric values change (R_p) times;
- usually (R_p gg R_s) during interactive editing.

Current structural compilation work can approach:

[
O((R_s + R_p)S)
]

because parameter-only changes can force `sdfwgsl::compile`.

After the split:

[
O(R_sS + R_pQ)
]

where (Q) is the compact numeric parameter update size.

When a slider is dragged through thousands of values while topology stays fixed:

[
R_s approx 1,quad R_p gg 1
]

so the structural part changes from roughly:

[
O(R_pS)
]

to:

[
O(S)
]

while only the necessary (O(R_pQ)) numeric updates remain.

This is one of the cleanest examples of Earthcall's caching principle: invalidate exactly the derived layer whose dependency changed.

---

# 8. Proposed change C — persistent GPU SDF state and dirty-range updates

Let:

- (B_s) = static SDF data bytes;
- (B_d) = genuinely dynamic data bytes per frame;
- (B_c) = bytes changed by authored edits during the frame.

Current steady-state upload:

[
O(B_s + B_d)
]

per frame.

Persistent static allocations change that to:

[
O(B_d + B_c)
]

per frame.

Across (F) frames:

Current:

[
O(F(B_s+B_d))
]

Proposed:

[
O(B_s) + O(FB_d) + Oleft(sum B_cight)
]

Static parameter blocks and conservative spatial structures are paid once per revision rather than once per frame.

If an object is completely static except for the camera, its SDF parameter upload complexity can approach zero in steady state.

---

# 9. Proposed change D — cached derived proofs and transforms

For a static object across (F) frames:

### Current

- inverse model: (O(F)) matrix inversions;
- heightfield structural proof: (O(FS));
- derived SDF construction: potentially (O(FS_d)).

### Revision-cached

- inverse model: (O(R_t)), where (R_t) is transform revisions;
- heightfield proof: (O(R_sS));
- derived SDF structure: (O(R_sS_d)).

For static geometry and transform:

[
R_t approx R_s approx 1
]

so all three become constant lifetime costs rather than frame-multiplied costs.

---

# 10. Proposed change E — broader analytic value+gradient propagation

For the finite-gradient generic path:

[
E_{	ext{step}} approx 4E
]

If a generalized jet evaluator can produce value and gradient together:

[
E_{	ext{step}} approx E_{	ext{jet}}
]

Let:

[
E_{	ext{jet}} = alpha E
]

where (alpha > 1) because derivative arithmetic is not free.

Then practical per-step improvement is:

[
rac{4E}{alpha E} = rac{4}{alpha}
]

If (alpha=2), theoretical local improvement is 2×.  
If compiler CSE already shares expensive work across finite-difference samples, effective (alpha) comparisons can be much less favorable.

This is exactly why the previous Perlin experiment observed little horizon improvement: the apparent "4 field calls" did not translate to 4× independent lattice-hash work.

Therefore the analytic-gradient change improves the constant factor:

[
O(PIS) ightarrow O(PIS)
]

asymptotically unchanged, but with a potentially important smaller coefficient.

It cannot by itself remove the dominant (P cdot I) multiplicative structure.

---

# 11. Proposed change F — repaired 2D heightfield DDA

For a **proved** heightfield, suppose a 2D grid has (H_x 	imes H_z) cells.

A ray DDA crossing the grid visits at most approximately:

[
O(H_x + H_z)
]

cell boundaries in the worst case across the whole footprint, rather than testing every fine point.

More importantly, if only (A) cells are potential surface candidates, expensive fine evaluation becomes proportional to candidate regions:

[
T_{	ext{heightfield}} =
O(P(T_{	ext{DDA}} + A I_c E))
]

where (I_c) is local fine-march iterations inside candidate cells.

For large flat/empty spans where (A ll I), this can drastically reduce exact evaluations.

However, this applies only where the field is structurally proved to be a valid heightfield and the min/max bounds are sound. The saved Perlin floor currently fails that proof because its Noise reads (p.y).

The algorithm therefore cannot solve the generic authored-field problem alone.

---

# 12. Proposed change G — generic 3D conservative interval hierarchy

This is the asymptotically most interesting proposal.

Suppose Earthcall builds a balanced hierarchy containing (H) spatial nodes over an implicit object's domain.

Each node stores or can derive a conservative interval:

[
[f_{min}, f_{max}]
]

If:

[
0 
otin [f_{min}, f_{max}]
]

the node cannot contain the zero set and can be skipped.

## 12.1 Build cost

A naive full hierarchy build is:

[
O(H cdot C_{	ext{bound}})
]

where (C_{	ext{bound}}) is the cost of conservative interval evaluation for one node.

If interval evaluation traverses an (S)-node expression:

[
C_{	ext{bound}} = O(S)
]

giving:

[
O(HS)
]

for a full rebuild.

That cost is unacceptable per frame but entirely reasonable as revision-derived state when amortized over many frames.

## 12.2 Incremental rebuild

If prophetic dependency tracking can identify only (h_Delta) hierarchy nodes whose bounds depend on changed authored values:

[
O(h_Delta S)
]

instead of:

[
O(HS)
]

with:

[
h_Delta ll H
]

for localized edits.

This is where Zach's "Singular data and ops cached with prophetic tracking" idea becomes directly relevant to graphics.

## 12.3 Ray traversal cost

For a balanced hierarchy, finding and traversing relevant nodes is often modeled as:

[
O(log H + A)
]

where (A) is the number of ambiguous/relevant nodes actually visited.

Exact field evaluation then occurs only inside those nodes.

The frame cost becomes approximately:

[
T_{	ext{hier}} =
Oleft(
P(log H + A + Z E)
ight)
]

where (Z) is the number of exact evaluation sites after skipping.

Compare to current:

[
T_{	ext{current}} =
O(P I E)
]

The hierarchy wins when:

[
log H + A + ZE ll IE
]

For expensive (E), the inequality is favorable even if hierarchy traversal itself is nontrivial.

The key is that current cost multiplies expensive mathematics by every fine sample:

[
I 	imes E
]

while the hierarchy attempts to replace most of those expensive samples with cheap bound traversal:

[
	ext{cheap traversal} + Z 	imes E
]

with:

[
Z ll I
]

for rays crossing large regions provably outside the zero set.

---

# 13. Numerical thought experiment

This section is illustrative, not a benchmark claim.

Assume one native frame:

[
P = 5{,}184{,}000
]

screen pixels.

Suppose only 50% of them actually enter the implicit proxy:

[
P_a = 2{,}592{,}000
]

Suppose average exact samples per active ray are:

[
I = 12
]

Then current scalar field-evaluation sites are:

[
P_a I =
2{,}592{,}000 	imes 12 =
31{,}104{,}000
]

field samples per frame.

If finite-difference step gradients perform roughly four whole expression evaluations per step:

[
31{,}104{,}000 	imes 4
=
124{,}416{,}000
]

whole-expression evaluation invocations in the naive call-count model.

Again, compiler CSE means actual ALU cost is not equivalent to 124 million independent Perlin hashes, but the number demonstrates the multiplicative pressure.

Now suppose a conservative hierarchy reduces exact evaluation sites from 12 per active ray to an average of 2, with 8 cheap hierarchy-node tests:

[
Z = 2,quad A+log H approx 8
]

Then exact field evaluations become:

[
2{,}592{,}000 	imes 2
=
5{,}184{,}000
]

a **6× reduction in exact sample count** before considering gradient improvements.

If generalized analytic gradients then turn four effective whole-expression invocations per sample into one jet evaluation, the two mechanisms compose:

- spatial hierarchy reduces **how often** the function is asked;
- jet evaluation reduces **how much duplicated work** happens when it is asked.

This is multiplicative composition rather than competing micro-optimizations.

---

# 14. Expected combined complexity after the proposed architecture

The desired steady-state pipeline for static structure is approximately:

## CPU

Structural cache lookup:

[
O(M)
]

Dynamic instance gathering:

[
O(M)
]

Dirty parameter updates:

[
O(Q_Delta)
]

Dirty spatial hierarchy maintenance:

[
O(h_Delta S)
]

rather than repeated:

[
O(M(L+Q+S))
]

structural copies/walks.

## GPU transfer

[
O(B_d + B_c)
]

rather than:

[
O(B_s+B_d)
]

every frame.

## GPU implicit evaluation

Current dominant form:

[
O(P I E)
]

Proposed dominant form:

[
O(P(log H + A + ZE_{	ext{jet}}))
]

with the design objective:

[
Z ll I
]

and for expensive fields:

[
E_{	ext{jet}} gg 	ext{one hierarchy-node test}
]

so replacing exact evaluation sites with conservative traversal has high leverage.

---

# 15. Scaling by world complexity

Suppose a zone contains (M) implicit objects.

Without effective screen-space or spatial rejection, worst-case aggregate implicit work is:

[
Oleft(
sum_{j=1}^{M} P_j I_j E_j
ight)
]

If every object covers the whole screen, the pathological bound approaches:

[
O(MPIE)
]

which is obviously unsustainable for a rich authored world.

Earthcall already reduces some CPU draw-call cost by grouping same-shape instances into (K) pipeline batches, but fragment work is still fundamentally driven by screen coverage and exact field evaluation.

A hierarchy gives each object a local proof structure; future cross-object acceleration could go further by building a world/Zone-level hierarchy over object bounds, reducing the number of implicit proxy volumes that rasterize at all.

That outer problem is analogous to conventional BVH/frustum/occlusion culling and is separate from this document's primary inner-SDF evaluator problem.

---

# 16. Complexity of cache invalidation itself

Caching is only correct if invalidation cost and dependency semantics are explicit.

A naive cache can trade computation for global invalidation:

[
	ext{change one value} Rightarrow O(H+S+M)
]

which merely moves the bottleneck.

The frontier design should attach each derived artifact to the smallest dependency set that proves it stale.

If a changed parameter affects only (d) dependent derived nodes:

[
T_{	ext{invalidate}} = O(d)
]

rather than scanning all cached artifacts.

This is exactly the same structural ambition as Prophetic Rete's direct relevance routing.

In the ideal common case:

[
d ll H, M, S_{	ext{world}}
]

and a local edit remains local computationally.

---

# 17. Why this is not "just use a faster shader"

A conventional optimization discussion might stop at:

- reduce noise instructions;
- lower march count;
- lower resolution;
- approximate normals.

Those can matter, but Earthcall's authored-mathematics contract changes the problem.

The fundamental optimization target is:

[
	extbf{avoid unnecessary questions without changing the answer}
]

not:

[
	extbf{answer the same question less faithfully}
]

That makes the central mathematical object a **proof of irrelevance**.

For a spatial cell (C), if interval analysis proves:

[
0 
otin f(C)
]

then the renderer has derived a theorem:

> no point in this cell belongs to the authored implicit surface.

Skipping it is not a graphics heuristic. It is a consequence of the authored mathematics.

That is why a generic interval hierarchy is a more Earthcall-native frontier than another guessed terrain constant.

---

# 18. Priority table

| Mechanism | Current cost removed | Current asymptotic form | Proposed form | Expected leverage |
|---|---|---:|---:|---|
| Zero-copy Program cache hit | repeated WGSL/param copying | (O(M(L+Q))) / frame | (O(M)) | high CPU cleanliness, low risk |
| Structure/value revision split | needless recompilation on value edits | (O((R_s+R_p)S)) | (O(R_sS + R_pQ)) | high during authoring |
| Cached proofs/derived SDF/inverse transform | repeated invariant derivation | (O(FMS)) class | (O(R_sS + R_t)) | medium, very safe |
| Persistent GPU SDF state | static bytes uploaded every frame | (O(FB)) | (O(B_s + FB_d + sum B_c)) | medium/high CPU-driver |
| Generalized analytic gradients | repeated same-field samples | (O(4PIE)) practical | (O(PIE_{jet})) | workload-dependent |
| Repaired heightfield DDA | point evaluation over proved 2D fields | (O(PIE)) | (O(P(DDA + A I_c E))) | high for eligible fields |
| Generic interval hierarchy | exact evaluation through empty/irrelevant space | (O(PIE)) | (O(P(log H + A + ZE))) | **highest frontier leverage** |

---

# 19. What should be measured next

Before implementation claims, instrument enough of the pipeline to estimate the variables in this document.

At minimum:

1. active SDF fragment count / proxy coverage (P);
2. total marcher iterations and distribution of (I);
3. exact `sdfEval` / jet-eval invocation count;
4. number of finite-difference gradient fallback invocations;
5. generated WGSL byte length (L);
6. Program cache hits/misses;
7. structural compiles per frame;
8. SDF parameter bytes uploaded (B);
9. SDF bind groups created per frame;
10. hierarchy nodes traversed, skipped, and exact-evaluation handoffs once acceleration exists.

With those counters, Earthcall can stop inferring bottlenecks from FPS and directly observe the multiplicative terms.

---

# 20. Final mathematical picture

Today, the expensive path is approximately:

[
oxed{
T_{	ext{current}}
approx
P
	imes
I
	imes
E
}
]

with practical multipliers from finite-difference gradients, plus repeated CPU structural copying and GPU streaming.

The target architecture is:

[
oxed{
T_{	ext{future}}
approx
P
	imes
(	ext{cheap relevance traversal}
+
Z 	imes E_{	ext{exact}})
}
]

where:

[
Z ll I
]

and the CPU side changes from frame-driven recomputation toward revision-driven derived state.

In words:

> Earthcall should spend exact mathematical work in proportion to **actual unresolved relevance**, not in proportion to every pixel, every possible step, every frame, and every unchanged piece of structure.

That is the deepest common principle connecting SDF acceleration, caching, Prophetic Rete, and the proposed Prophetic JIT direction.

The machine should not become less truthful.

It should become better at knowing **where truth still needs to be recomputed**.

---

**Signed:** GPT-5.6 Sol  
**Session:** `chatgpt-2026-09-18-sdf-pipeline-audit`  
**Date:** 2026-09-18  
**Timestamp:** 2026-09-18T23:57:00-07:00
