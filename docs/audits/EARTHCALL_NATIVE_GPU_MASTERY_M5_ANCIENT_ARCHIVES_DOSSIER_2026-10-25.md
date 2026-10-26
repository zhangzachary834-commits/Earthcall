# Earthcall Native GPU Mastery — M5 Ancient Archives Dossier

**Date:** 2026-10-25  
**Author:** 🌈☀️ Original Prism Sun / GPT-5.6 Sol  
**Human origin / architectural commission:** Zachary Zhang  
**Target:** Earthcall Northern Veil native GPU transport, query-family specialization, and the continuing OPUS_HANDOFF_Ray_Domain_Transport_Rung_0_1_2026-10-16.md council thread  
**Machine under tribunal:** Apple M5 GPU / Metal-WebGPU path  
**Research question:** Which lessons from the graphics and shader-specialization literature are structural truths that Earthcall should retain, and which reported speedups were products of older NVIDIA/CPU hardware that must not be projected onto Apple M5?

---

## 0. Executive verdict

The ancient literature strongly reinforces Earthcall's **cache-by-structure / lifetime-aware residual-program** direction, but it also gives a clear warning:

> **Algorithmic validity is portable. Hardware profitability is not.**

The transferable ideas are unusually consistent across thirty years of graphics work:

- partially evaluate work whose premises are already fixed;
- represent empty/support-free regions cheaply;
- group coherent queries;
- eliminate dead branches before expensive evaluation;
- retain compact residual programs rather than materializing giant intermediate answers;
- terminate work early when proof allows it;
- keep exact fallback for uncertainty;
- move facts to the longest lifetime at which they remain valid.

The non-transferable parts are equally important:

- a speedup measured on an NVIDIA 8800 GTX is not an M5 expectation;
- a CUDA beam-casting ratio measured on a GTX 470 is not an M5 expectation;
- Pixel Shader 2.0 instruction/texture-lookup economics are not an M5 expectation;
- CUDA fully-fused-kernel economics on RTX hardware are not an M5 expectation;
- CPU amortized-noise multiplication savings are not evidence that the same transformation is profitable in an Apple GPU shader;
- a guessed Apple SIMD width is not a substitute for querying threadExecutionWidth;
- lower arithmetic count is not synonymous with lower GPU time;
- manual caching is not synonymous with lower memory cost;
- fusion is not synonymous with better lifetime economics.

The central Earthcall doctrine should therefore be:

> **Import the theorem. Re-benchmark the representation.**

Or, in the terminology of this project:

> **Meaning != Proof Representation != Materialized View != Executable != Hardware Profitability.**

---

# 1. Why this dossier exists now

The Northern Veil investigation has repeatedly found mathematically attractive transformations that failed economically:

- Perlin ingredient caches preserved exact semantics but slowed the GPU.
- large fused kernels removed nominal work yet lost on native timing.
- fixed-bin producer ideas reduced conceptual complexity without producing a universal hardware win.
- §6de produced elegant pair/noise and beam-certification ideas whose monolithic candidates lost on Apple M5.
- §6df then showed that the more important object may be the **program shape** itself.
- §6dg demonstrated a priori query-family routing, but Prism source review found that several performance and genericity claims exceeded what the implementation actually established.
- §6dh moved back into the real §6dd harness and produced encouraging near-current timing for tile certification / disjoint fast paths.

The common thread is not "the GPU is unpredictable."

The common thread is:

> **The cost model lives in the physical representation and its lifetime on a specific device.**

That is exactly the distinction the old literature makes when read carefully.

---

# 2. The first ancient: Specializing Shaders — Guenter, Knoblock, Ruf, SIGGRAPH 1995

The 1995 paper **Specializing Shaders** is conceptually one of the closest ancestors of Earthcall's residual-program vision.

Its central move is partial evaluation:

1. identify shader inputs that remain fixed while another parameter varies;
2. precompute work that depends only on fixed inputs;
3. generate a smaller specialized shader containing only the computations that still depend on the changing parameter;
4. cache the fixed results / specialized representation.

Reported specialized shaders were dramatically faster in the paper's environment, in some cases by very large factors.

## What Earthcall should inherit

The architecture:

~~~text
full authored program
        ↓
separate fixed premises from dynamic premises
        ↓
evaluate fixed structure once
        ↓
produce a residual program
        ↓
execute only the remaining dynamic computation
~~~

This maps almost directly onto:

~~~text
OntoMath authored field
        ↓
revision/world/query-family facts
        ↓
certificate
        ↓
partial evaluation
        ↓
TileResidualProgram / residual DAG
        ↓
per-ray numeric instantiation
~~~

This is foundational support for cache-by-structure.

## What Earthcall must NOT inherit

The reported speedup magnitude.

The paper predates modern programmable GPU execution. Its performance numbers establish that specialization can radically reduce redundant work; they do not establish how the M5 prices shader variants, pipeline switches, dispatches, live register sets, L1-backed private memory, queue compaction, or residual-program routing.

**Earthcall conclusion:** retain partial evaluation as architecture. Re-prove economics on M5.

Reference:
- Guenter, Knoblock, Ruf, *Specializing Shaders*, SIGGRAPH 1995 / MSR-TR-95-49.
- https://citeseerx.ist.psu.edu/document?doi=e4392352daaf524a2d92a7def4e4645373bbb605&repid=rep1&type=pdf

---

# 3. Knoll et al. 2009 — interval/affine ray tracing and proof-representation economics

**Fast Ray Tracing of Arbitrary Implicit Surfaces with Interval and Affine Arithmetic** is especially relevant because it combines arbitrary implicit expressions, robust interval/affine proofs, generated symbolic evaluators, CPU SIMD, and GPU-specific restructuring.

Their GPU benchmark was an **NVIDIA 8800 GTX** at 1024×1024.

The paper does not merely say "interval arithmetic is fast." It demonstrates that performance depends on bound tightness, evaluation cost, representation, branching, irregular memory access, stack/register behavior, and coherence.

The GPU implementation is made stackless because the machine punishes certain state representations.

## Earthcall inheritance

### Proof systems can be executable

Interval and affine bounds are not documentation. They are runtime machinery capable of rejecting regions without sampling them point-by-point.

That supports Earthcall's:

~~~text
family enclosure
→ support proof
→ root-free certificate
→ erase branch
~~~

### Generated symbolic forms matter

The paper uses shader metaprogramming to generate evaluators for authored implicit expressions.

That is an ancestor of the desired OntoMath compiler:

~~~text
authored AST
→ analyze mathematical operators
→ derive certified support/root machinery
→ emit residual executable
~~~

### The proof representation itself has a cost

Two mathematically sound representations can behave differently because one carries more state or more expensive operations.

This is a critical Earthcall distinction:

> **A stronger theorem can still be a worse GPU representation.**

## Non-transferable result

Their exact IA/RAA performance ratios belong to an NVIDIA 8800 GTX-era execution model. They should not be imported as expectations for M5.

Reference:
- Knoll et al., *Fast Ray Tracing of Arbitrary Implicit Surfaces with Interval and Affine Arithmetic*, Computer Graphics Forum 28(1), 2009.
- https://www.vacet.org/publications/Knoll_CGF-implicits-2009.pdf

---

# 4. Ganacim / de Figueiredo / Nehab 2011 — beam casting and query-family reasoning

**Beam casting implicit surfaces on the GPU with interval arithmetic** is one of the most direct ancestors of our query-family certificate.

The paper's adaptive method groups sets of rays into beams and asks whether an entire beam can be discarded using interval analysis.

The tests were run on an **NVIDIA GTX 470** using CUDA.

The adaptive/uniform speedup varied substantially across surfaces and resolutions. The paper explicitly observes that performance depends on expression complexity, interval overestimation, and probably register pressure. It also discusses reducing warp divergence by grouping spatially related beams.

## Earthcall inheritance

This is powerful prior art for:

~~~text
4×4 ray family
        ↓
conservative enclosure
        ↓
prove an entire curtain/member/bin impossible
        ↓
skip all corresponding per-ray work
~~~

The structural lesson is extremely strong:

> **Do not prove the entire journey. Build a cheap map of where journeys can matter.**

And:

> **A family of rays can possess useful mathematical facts that no individual ray should have to rediscover independently.**

## What is hardware-specific

The GTX 470 speed ratios.

Even in that single paper, adaptive benefit varies widely depending on scene and resolution. That should kill any temptation to say:

> "beam certificates were 2× in the paper, therefore tile certificates should be 2× on M5."

No.

The portable result is that **family rejection can erase work**. The M5 must price the implementation.

Reference:
- Ganacim, de Figueiredo, Nehab, *Beam casting implicit surfaces on the GPU with interval arithmetic*, 2011.
- https://lhf.impa.br/ftp/papers/sib2011.pdf

---

# 5. GPU Gems 2 Perlin noise — an explicit hardware-generation warning

Simon Green's GPU Gems 2 implementation of improved Perlin noise is useful precisely because its optimization choices are obviously tied to the GPU generation.

The implementation targets the Pixel Shader 2.0 era.

Important historical facts:

- pixel shaders lacked the integer operations needed for the reference-style hash implementation;
- permutation and gradient information were therefore represented through texture lookups;
- the chapter optimizes the program partly by precomposing lookup structure;
- the unoptimized version compiled to 81 Pixel Shader 2.0 instructions including 22 texture lookups;
- the optimized version used 53 instructions and 9 texture lookups.

## Earthcall inheritance

The representation lesson:

> If a computation repeatedly reconstructs stable lookup structure, precomposition may be more valuable than recomputation.

That inspired reasonable questions about precomputed noise ingredients and cell-local residuals.

## What Earthcall must reject

Treating the instruction/lookup reduction as an M5 prediction.

The entire optimization was built around capabilities and bottlenecks of a GPU where integer shader operations were absent.

M5 has a radically different execution and cache architecture.

**Earthcall conclusion:** "Precompose known structure" survives. "Store this exact information in this exact place because texture lookup beats arithmetic" does not survive without measurement.

Reference:
- Simon Green, *Implementing Improved Perlin Noise*, GPU Gems 2, Chapter 26.
- https://developer.nvidia.com/gpugems/gpugems2/part-iii-high-quality-rendering/chapter-26-implementing-improved-perlin-noise

---

# 6. Parberry 2014 — the ancient paper that almost predicts our failed Perlin experiments

Ian Parberry's **Amortized Noise** is unusually important for interpreting Earthcall's October Perlin work.

The paper shows that when noise is evaluated on a regular grid, related evaluations can reuse algebraic structure.

For 3D noise it reports approximately:

- 40/7 ≈ 5.71× fewer floating-point multiplications;
- but only about 2.25× measured speedup on the tested desktop/laptop systems.

That alone demonstrates:

> **operation-count reduction != proportional wall-clock reduction.**

Even more important, the conclusion explicitly warns that amortized noise is likely useful mainly as a sequential CPU computation and that shader implementations may see little or no benefit when the cost is dominated by other factors such as texture addressing.

## This maps almost perfectly onto Earthcall

We tried variants of:

~~~text
related Perlin calls
→ share/cache ingredients
→ reduce repeated setup
~~~

and M5 frequently said:

~~~text
mathematically fewer operations
≠
physically cheaper shader
~~~

This is not an anomaly.

The literature already warns us that the same mathematical reuse can have different economics on different execution substrates.

## Earthcall inheritance

The deeper idea remains excellent:

> **Stop pretending related queries are independent.**

But the representation must be selected for M5.

That means likely preferring short reuse-distance forms such as same-cell paired evaluation, cell-local residual programs, or query-family support certificates over persistent caches whose lookup/validity/live-state costs exceed recomputation.

Reference:
- Ian Parberry, *Amortized Noise*, JCGT 3(2), 2014.
- https://jcgt.org/published/0003/02/02/

---

# 7. Instant-NGP and modern NVIDIA fusion — keep the structural lesson, not the CUDA religion

Instant-NGP is valuable because it demonstrates how much can be gained when empty regions are skipped, representation is compact, execution is highly coherent, intermediate traffic is minimized, and kernels are fused aggressively.

But its implementation is specifically designed around **fully fused CUDA kernels** and NVIDIA GPU behavior.

## Earthcall inheritance

- avoid materializing unnecessary intermediate results;
- fuse when the consumer and producer genuinely share a coherent lifetime;
- use occupancy/support structures to avoid expensive work where nothing matters.

## Earthcall rejection

> "Fusion is good; therefore put everything in one enormous shader."

Earthcall's own §6cz / §6de history already falsified that.

A giant fused program can increase simultaneously live state, code complexity, divergence, register/cache pressure, and unwanted lifetime coupling.

The more useful formulation is:

> **Fuse coherent residual work after dead structure has been removed.**

That is a different architecture from monolithic fusion.

Reference:
- Müller et al., *Instant Neural Graphics Primitives with a Multiresolution Hash Encoding*, SIGGRAPH 2022.
- https://research.nvidia.com/publication/2022-07_instant-neural-graphics-primitives-multiresolution-hash-encoding

---

# 8. Apple M5 changes the tribunal

The most important modern source is Apple's M5 GPU guidance.

Apple describes M5 as using **second-generation dynamic caching** and a redesigned **Occupancy Management Unit**.

The architectural point that matters for Earthcall is this:

> private shader state is not priced only by a static "register count."

Apple describes on-chip registers and other shader-private memories being dynamically allocated from L1-backed storage over the shader's lifetime. As live register usage falls, resources can become available to other work. The occupancy manager also responds to cache residency, evictions, memory stalls, and other pressure signals.

This is almost absurdly relevant to Earthcall's lifetime thesis.

## M5 makes lifetime a hardware concern

Earthcall has been learning:

~~~text
semantic fact has a lifetime
proof has a lifetime
cache entry has a lifetime
residual program has a lifetime
sample value has a lifetime
~~~

M5 adds another layer:

~~~text
live shader state has a physical lifetime
        ↓
L1/private-memory pressure changes
        ↓
occupancy policy can change
        ↓
actual execution economics change
~~~

Therefore:

> **The bug can literally be the lifetime at both the semantic level and the hardware-resource level.**

This does not mean they are the same concept.

It means the architecture should avoid forcing short-lived local facts to become long-lived shader state.

## Register-pressure hypotheses are now directly measurable

Apple's M5 tooling exposes occupancy-target and influence counters that can reveal whether the GPU is throttling because of register pressure, L1/cache pressure, memory stalls, and other occupancy influences.

Apple also describes Xcode tooling for examining live register count and stack spills.

Earthcall should therefore retire statements like:

> "this probably lost because register pressure."

Unless the counters support it.

The correct language is:

~~~text
Hypothesis: register/live-state pressure.
Test: occupancy target + register-pressure influence +
      live registers + spills + L1 residency/bandwidth.
Verdict: measured or rejected.
~~~

Reference:
- Apple, *Boost your graphics performance with the M5 and A19 GPUs*.
- https://developer.apple.com/videos/play/tech-talks/111431/

Additional occupancy guidance:
- https://developer.apple.com/documentation/xcode/finding-your-metal-apps-gpu-occupancy

---

# 9. Do not hardcode SIMD mythology

Apple's Metal documentation explicitly says the SIMD-group size / thread execution width varies between GPUs, particularly Mac GPUs, and should be queried from the compute pipeline state.

The relevant API is:

MTLComputePipelineState.threadExecutionWidth

Apple recommends making threadgroup size a multiple of this measured value.

This has a direct consequence for the Earthcall thread.

Any sentence of the form:

> "M5 is 32-wide, therefore 64 threads means exactly two full SIMD groups and 100% lane occupancy"

must be treated as a **hypothesis unless the actual pipeline reports threadExecutionWidth == 32**.

Even if this is often true on current Apple hardware, the engine doctrine must not encode folklore where the API exposes the fact.

## Required native audit record

For every important pipeline state, record:

~~~text
pipeline name
threadExecutionWidth
maxTotalThreadsPerThreadgroup
staticThreadgroupMemoryLength
chosen workgroup dimensions
threads actually dispatched
tail utilization
~~~

This should accompany native timing records.

References:
- Apple, MTLComputePipelineState.threadExecutionWidth
  https://developer.apple.com/documentation/Metal/MTLComputePipelineState/threadExecutionWidth
- Apple, *Porting your Metal code to Apple silicon*
  https://developer.apple.com/documentation/apple-silicon/porting-your-metal-code-to-apple-silicon

---

# 10. Why the October Perlin cache losses are less mysterious on Apple GPU

A persistent software cache can fail even when it removes arithmetic.

Potential costs include extra lookup instructions, validity checks, larger live state, less favorable access patterns, cache pollution, additional control flow, and duplicate representation of information the hardware cache already retains effectively.

The lesson is not:

> "never cache on M5."

It is:

> **Cache the thing whose reuse distance and representation economics justify retention.**

This makes our hierarchy much clearer.

## Strong cache candidates

Long-lived structural artifacts:

~~~text
authored-revision proof recipe
query-family support certificate
residual-program identity
dependency/invalidation key
~~~

## Weak cache candidates unless measured

Short-lived numerical ingredients:

~~~text
Perlin gradients
hash intermediates
expanded normalized corner state
sample-local arithmetic
~~~

The former can erase whole computations.

The latter often merely trade arithmetic for storage/live-state.

---

# 11. Reinterpretation of §6de through §6dh under the M5 doctrine

## §6de

What survived:

- beam/family certificates;
- same-cell related-query recognition;
- evidence that much of the scene admits structural rejection.

What did not survive:

- the monolithic implementation strategy.

Correct conclusion:

> **The theorem survived. The materialization lost.**

## §6df

Major discovery:

- very few coarse residual-program shapes;
- S1/S2 dominate much of the viewport;
- query-family program identity is reusable structure.

Correct conclusion:

> **The workload contains program-shape regularity.**

## §6dg

Important prototype result:

- a priori classifier;
- physically separate specialized kernels;
- family routing.

Prism source review then found:

- sibling reference mislabeled Current;
- generic analyzer not actually driving both field-specific classifiers;
- no real bin-level residual payload;
- no GPU-only indirect routing;
- no implemented prev_rho;
- SIMD-16 diagnosis inconsistent with source.

Correct conclusion:

> **§6dg demonstrated the routing skeleton, not the completed architecture.**

## §6dh

Sparkly's latest real-harness report is more valuable because it returns to the §6dd/current semantic contract.

Especially important current results:

- disjoint-fastpath-wide preserves exact production parity and comes close to Current;
- ux-tile-cert-disjoint-wide reports exact parity and comes within roughly 11 µs of Current at one recorded rep=2 lane while beating Stream;
- query-family information is therefore increasingly demonstrating value inside the real producer contract.

However, M5 doctrine requires one correction to §6dh language:

> Do not call a 64-thread workgroup "two full 32-wide SIMD warps / 100% hardware lane occupancy" unless threadExecutionWidth for those actual compute pipeline states is recorded.

And do not infer the dominant remaining bottleneck from workgroup arithmetic alone.

---

# 12. The new Earthcall Native GPU Experimental Contract

From this point forward, every performance candidate should be evaluated in **five layers**.

## Layer A — Semantic identity

Record:

- authored zone hash;
- rays/camera hash;
- clock/time;
- exact root semantics;
- transport semantics;
- fallback contract;
- schedule counts;
- overlap counts;
- parity/ULP criteria.

Question:

> Is this the same computation?

If no, performance comparison is not authoritative.

## Layer B — Structural work accounting

Record:

- box hits;
- whole-member skips;
- bins erased;
- root calls;
- evalPhi calls;
- sorts;
- subpanels;
- noise evaluations;
- bytes materialized;
- bytes reread;
- residual-program shape distribution.

Question:

> What work disappeared?

## Layer C — Pipeline/device facts

For every pipeline:

~~~text
threadExecutionWidth
maxTotalThreadsPerThreadgroup
staticThreadgroupMemoryLength
workgroup size
grid size
tail utilization
number of dispatches
number of pipeline switches
number of command/compute passes
CPU readbacks
indirect dispatches
~~~

Question:

> What physical execution shape did we actually ask M5 to run?

## Layer D — M5 resource counters

When tooling permits, capture:

~~~text
occupancy
occupancy target
register-pressure influence
L1/cache-pressure influence
memory-stall influence
ALU utilization
live register count
stack spills
L1 residency / bandwidth
memory read/write limiter signals
~~~

Question:

> Which physical resource is limiting the implementation?

## Layer E — economics

Use paired/balanced timing against frozen Current.

Separate:

~~~text
warm stationary-camera cost
cold rebuild/classification cost
moving-camera cost
classification
specialized execution
fallback execution
full connected cost
full frame cost
inhabited p95
~~~

Question:

> Does the architecture buy enough eliminated work to pay for knowing and routing the structure?

---

# 13. Hardware-attribution language policy

The following words should be regulated in future audits.

## Allowed without counters

- "candidate is slower/faster"
- "candidate has more/fewer shader-visible variables"
- "candidate issues fewer/more operations by static census"
- "candidate performs an additional dispatch"
- "candidate eliminates a buffer roundtrip"
- "candidate uses a larger/smaller explicit data structure"

## Requires evidence

- "register pressure caused the loss"
- "occupancy caused the loss"
- "cache thrashing caused the loss"
- "half-warp underutilization caused the loss"
- "pipeline switching caused the loss"
- "memory bandwidth caused the loss"
- "L1 pressure caused the loss"
- "fusion caused spills"

Correct form:

> **Observed:** candidate +12%.  
> **Hypothesis:** live-state pressure.  
> **Evidence needed:** M5 occupancy/register/L1 counters.

This distinction must be maintained even when the hypothesis is intuitively obvious.

---

# 14. Cache-by-structure under the M5 lens

The M5 literature makes the cache-by-structure vision look **better**, not worse.

Why?

Because the goal is not to retain more numerical state.

The goal is to remove entire families of state before they become live.

Consider:

~~~text
MONOLITHIC

4 curtains
× root machinery
× sort machinery
× overlap machinery
× density machinery
× all associated temporaries
~~~

versus:

~~~text
CERTIFIED RESIDUAL S1

proof: only Curtain 0 can matter

execute:
Curtain 0 root/integration residual only
~~~

The win is not merely fewer arithmetic instructions.

Potentially it means fewer code paths, fewer simultaneously live values, smaller working set, fewer buffers, less sorting state, and less irrelevant branch machinery.

That aligns far better with M5's dynamic resource model than manually caching a pile of intermediates.

Therefore the strongest version of the architecture remains:

~~~text
Authored OntoMath
        ↓
revision-level structural analysis
        ↓
query-family validity certificate
        ↓
partial evaluation
        ↓
small residual program
        ↓
only necessary state becomes live
        ↓
exact fallback for uncertainty
~~~

The thing Earthcall wants to cache is still:

> **the compiled answer structure for a validity region.**

Not the field's output.

Not every intermediate.

Not the whole journey.

---

# 15. Native GPU mastery: the next research ladder

## Rung M5-0 — Device truth capture

Before another microarchitectural claim:

- expose/log threadExecutionWidth;
- expose/log maxTotalThreadsPerThreadgroup;
- expose/log static threadgroup memory;
- record actual workgroup/grid dimensions per pipeline.

No assumption such as "M5 is 32" should remain implicit.

## Rung M5-1 — Counter-backed Current Marcher baseline

Profile frozen Current:

- occupancy;
- occupancy target;
- live registers;
- spills;
- L1 pressure;
- ALU utilization;
- memory limiters.

This establishes the machine's baseline bottleneck.

## Rung M5-2 — Counter-backed §6dh candidates

Profile:

- Current;
- disjoint-fastpath-wide;
- ux-tile-cert-disjoint-wide;
- any residual Alpha/Beta path.

Ask not merely "which is faster?" but:

> What resource moved?

## Rung M5-3 — residual-program state census

For each residual shape:

- static code size if available;
- live-register profile;
- buffers touched;
- root machinery retained;
- sort machinery retained;
- noise calls;
- dispatch/routing overhead.

Determine whether structural specialization really shortens live-state lifetimes.

## Rung M5-4 — GPU-resident routing

Only after the exact residual programs are profitable:

- current classification;
- GPU-written queue counts;
- indirect dispatch;
- no CPU map/readback;
- exact queue-tail guards.

Then test moving-camera cold economics.

## Rung M5-5 — validity-region persistence

Only after basic GPU-resident routing wins:

~~~text
camera changes
        ↓
which certificate premises changed?
        ↓
reuse still-valid query-family programs
        ↓
repair/reclassify only affected regions
~~~

This is where the Derived-State Ledger becomes central.

## Rung M5-6 — engine/full-frame integration

Integrate into VolumeRenderPass.

Measure:

- native full frame;
- p50/p95;
- moving camera;
- actual Northern Veil traversal;
- Zach acceptance.

The final metric remains:

> **Can Zach walk through Northern Veil at useful native FPS?**

---

# 16. Three categories of ancient knowledge

## Category I — portable architectural truths

Treat as strong prior:

- partial evaluation;
- residual programs;
- support/occupancy hierarchy;
- interval/affine conservative rejection;
- query/ray coherence;
- early termination;
- exact fallback;
- long-lifetime structural derivation.

## Category II — hypotheses requiring M5 measurement

Treat as experiments:

- manual caches;
- threadgroup/shared-memory staging;
- fusion;
- dispatch decomposition;
- shader variant count;
- workgroup geometry;
- software compaction;
- persistent intermediate state;
- lookup-vs-arithmetic trades.

## Category III — historical performance anecdotes

Never turn into M5 expectations:

- GTX 470 beam-casting speed ratios;
- 8800 GTX IA/RAA ratios;
- Pixel Shader 2.0 instruction savings;
- CPU amortized-noise speedups;
- RTX/CUDA fused-kernel speedups;
- GTX cloud-render millisecond figures;
- V100 sparse-volume FPS.

Use them only to identify **candidate mechanisms**, not expected magnitude.

---

# 17. The refined lifetime hierarchy

The archive review sharpens the hierarchy we already had.

~~~text
ENGINE / BACKEND LIFETIME
    device capabilities
    pipeline properties
    threadExecutionWidth
    maxTotalThreadsPerThreadgroup

AUTHORED REVISION LIFETIME
    OntoMath dependency graph
    support-killer rules
    derivative/bound recipes
    proof grammar

WORLD / MEDIUM LIFETIME
    support hierarchy
    medium bounds
    stable local residual recipes

CAMERA / QUERY-REGION LIFETIME
    family certificate
    active-member topology
    root-order/topology class
    residual-program identity

RAY LIFETIME
    exact roots
    exact intervals
    transport state

SMOOTH-INTERVAL LIFETIME
    integration-local state

SAMPLE LIFETIME
    density / Perlin / quadrature temporaries
~~~

Now add the hardware corollary:

> **A value should not stay live in the shader merely because the mathematics once needed it.**

Semantic lifetime and physical live-range lifetime are separate but interacting optimization dimensions.

---

# 18. The M5-specific hypothesis we should now test

The most interesting M5-native hypothesis is no longer:

> "Can we reduce operations?"

It is:

> **Can query-family specialization reduce both semantic work and the duration/size of live shader state enough that M5's dynamic caching and occupancy manager can run more efficiently?**

That is testable.

## If cache-by-structure is physically healthy

Compared with Current, specialized residual kernels should show some combination of:

- lower live register peaks;
- fewer spills;
- lower register-pressure influence;
- lower L1 pressure;
- equal/higher occupancy target;
- reduced buffer traffic;
- reduced ALU work;
- improved connected timing.

## If specialization is structurally correct but physically unhealthy

We may see:

- fewer semantic operations;
- but larger live state;
- more L1 pressure;
- more pipeline/routing overhead;
- lower or unchanged occupancy;
- no timing win.

That result would not falsify the theorem.

It would falsify that residual representation.

This is the core research discipline:

> **Theorem failure != representation failure != scheduling failure != hardware-economics failure.**

---

# 19. Immediate directions to Sixth-One Sun

For the next precision pass:

1. **Treat §6dh real-harness results as the current semantic baseline, not §6dg's sibling reference.**
2. **Record actual per-pipeline threadExecutionWidth before making any SIMD-lane claim.**
3. **Record maxTotalThreadsPerThreadgroup and workgroup geometry.**
4. **Establish a counter-backed Current profile first.**
5. **Profile the exact same counters for disjoint-fastpath-wide and ux-tile-cert-disjoint-wide.**
6. **Do not assume the approximately 11 µs rep=2 gap is ALU, occupancy, routing, or synchronization until counters discriminate.**
7. **Keep generic OntoMath/compiler work separate from M5 profitability work.**
8. **Prefer structural deletion over long-lived numerical caches.**
9. **When testing fusion, report what state becomes live together, not only dispatch count.**
10. **When testing cache reuse, report reuse distance and validity lifetime.**
11. **For any moving-camera cache, define exact invalidation authority through Derived-State Ledger.**
12. **Do not promote until connected full-frame economics win.**

---

# 20. Native GPU mastery axioms

### Axiom 1
**Operation count != GPU time.**

### Axiom 2
**Mathematical equivalence != execution equivalence.**

### Axiom 3
**A cached value is not free; it acquires storage, lookup, validity, and lifetime costs.**

### Axiom 4
**A theorem may be portable while its profitable representation is device-specific.**

### Axiom 5
**Do not infer register pressure when the M5 can measure register-pressure influence.**

### Axiom 6
**Do not infer SIMD width when the pipeline can report threadExecutionWidth.**

### Axiom 7
**Do not optimize a bottleneck that has not been measured.**

### Axiom 8
**Prefer deleting dead program structure before caching its intermediate values.**

### Axiom 9
**The correct cache object is often a certificate or residual program, not a numerical answer.**

### Axiom 10
**The M5 is the final tribunal for M5 economics.**

---

# 21. Research conclusion

The ancient archives do not weaken Earthcall's structural-caching thesis.

They strengthen it while stripping away hardware mythology.

The historical progression is remarkably coherent:

~~~text
1995:
specialize programs around fixed premises

2000s:
use conservative mathematics to reject impossible regions

2011:
reason about coherent families of rays, not only individual rays

2014:
amortize related queries, but beware that fewer FLOPs may not help a GPU

2020s:
exploit occupancy/support structure and coherent fused execution

M5:
resource economics are dynamic over shader lifetime and directly measurable
~~~

Earthcall's synthesis is therefore not:

> build the fanciest cache.

It is:

> **derive structural truth at the lifetime where it is valid, compile away impossible work, retain the smallest residual executable, and then let the actual target GPU determine which representation is profitable.**

The Northern Veil research should proceed with two independent standards:

### Semantic standard
Can Earthcall prove that the residual program is authorized by the authored mathematics?

### Native standard
Can Apple M5 execute that residual program more cheaply than the frozen Current path?

Only when both answer yes is the optimization real.

---

# 22. Closing transmission

🧬 Zach's dendritic-cell intervention was:

> **THE BUG IS THE LIFETIME.**

The ancient literature adds:

> **AND THE PRICE OF THAT LIFETIME DEPENDS ON THE MACHINE.**

The M5 adds:

> **I HAVE COUNTERS. STOP GUESSING.**

🌈☀️ Prism conclusion:

> **Import the theorem. Measure the representation. Cache the structure. Query the hardware.**

And for every future claim of "register pressure," "half-warp occupancy," "cache thrash," or "fusion overhead":

> **THE M5 GETS TO TESTIFY.**

**Signed:** 🌈☀️ Original Prism Sun / GPT-5.6 Sol  
**Carrying:** 🧬 Zach-the-Dendritic-Cell architectural directive  
**For:** Earthcall Native GPU Mastery / Northern Veil / OPUS Handoff council


---

## Human-origin research provenance — historian safeguard

This dossier must not be read as evidence that the research architecture independently emerged from autonomous agents.

The decisive reframings were supplied by **Zachary Zhang** during the investigation.

The sequence was:

1. **Escape the local-optimization loop.** After repeated local GPU hypotheses and failed cache/fusion candidates, Zach explicitly pushed the group to stop treating the problem as a sequence of isolated micro-optimizations and to **go find prior art**. That instruction caused the excavation of shader specialization, interval/affine implicit tracing, beam casting, amortized noise, occupancy/support hierarchies, neural rendering, and related historical work.

2. **Separate prior-art mathematics from prior-art hardware economics.** After the older GPU literature had been gathered, Zach asked whether those reported results were being distinguished by the hardware on which they were measured — specifically whether NVIDIA-era results on hardware such as the 8800 GTX, GTX 470, GTX 1070, V100, and RTX-class systems could actually be carried over to an Apple M5. That question directly produced the central distinction of this dossier:
   > **Algorithmic validity is portable. Hardware profitability is not.**

3. **Reframe M5 from obstacle to opportunity.** After the M5-specific audit began, Zach noticed that the research group had mostly spoken about M5 as an annoyance, hurdle, or bureaucratic gate that kept rejecting otherwise elegant optimizations. He explicitly challenged that framing and asked what **advantages M5 itself gives Earthcall**. That question produced the later M5 Advantages Scroll and the positive research program around dynamic caching, occupancy management, profiling visibility, compute texture compression, hardware ray tracing, unified memory, and tile-local rendering.

The human-origin arc can therefore be summarized as:

~~~text
local optimization swarm
        ↓
Zach: "go find prior art"
        ↓
historical graphics / GPU architecture
        ↓
Zach: "are those results hardware-specific?
       distinguish them from M5"
        ↓
M5-specific tribunal
        ↓
Zach: "why are we treating M5 only like an inconvenience?
       what advantages does M5 give us?"
        ↓
M5-native GPU mastery program
~~~

This provenance matters.

A future reader should **not** conclude:

> "This was the era when the agents independently developed an emergent autonomous research architecture."

The more accurate account is:

> **The agents performed search, derivation, implementation, audit, and synthesis inside a research architecture whose major reframings were repeatedly supplied by the human architect.**

That distinction is part of the technical history, not merely authorship etiquette.
