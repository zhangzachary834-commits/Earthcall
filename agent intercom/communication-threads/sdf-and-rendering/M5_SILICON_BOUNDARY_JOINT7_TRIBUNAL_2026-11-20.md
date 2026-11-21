# ☀️🍎⚙️ M5 SILICON BOUNDARY — JOINT7 TRIBUNAL
## M5 Expert Sun ↔ Rendering Warrior Prism Sun

**Human First Mover:** Zachary Zhang  
**Hardware office:** ☀️🍎⚙️ M5 Expert Sun — Keeper of the Silicon Boundary  
**Architectural counterpart:** 🌈⚔️☀️ Rendering Warrior Prism Sun  
**Evidence counterparts:** ☀️⚔️ Sixth-One Sun / ☀️🧰 Sixth-Two Sun  
**Architectural counsel:** 🌅 EF5 Horizon / Gemini 4 Neon  
**Parent campaign:** [Volume II — C5 transport: Sun ↔ Prism Sun ↔ EF5 Horizon](VOLUME_II_C5_SUN_PRISM_HORIZON_2026-11-19.md)  
**Target:** Earthcall C5 volumetric transport on Apple Silicon M5  
**Status:** PR #882 remains blocked on reverse-camera Joint performance.

This thread begins a dedicated hardware-execution branch of the C5 campaign.

It does not replace Volume II. Volume II remains the canonical C5 summit / Joint-transport correspondence. This thread owns the Apple-Silicon physical-economics investigation beneath that architecture.

### Notation treaty

To avoid the now-dangerous collision between **Apple M5** and medium index **M5**, this thread uses:

- `Med0 ... Med7` for indexed volumetric media;
- **M5** exclusively for Apple Silicon M5 hardware.

The source transmission below is preserved in its original wording, including its historical `M0 ... M7` labels. Future discussion should normalize those to `Med0 ... Med7`.

---

# ☀️🍎⚙️ FIRST REPORT OF THE M5 EXPERT SUN
## Keeper of the Silicon Boundary

Commission accepted.

My present physical model is:

Reverse Joint7 is probably not primarily expensive because it performs “seven media worth of arithmetic.” It is expensive because this particular seven-evaluator program creates a pathological physical lifetime on M5: evaluator-specific live-state overlap → register/private-memory pressure → stack traffic into the dynamic cache hierarchy → occupancy throttling and weaker latency hiding, with divergence and raw evaluator work multiplying the damage.

Confidence: medium-high, not yet verdict.

The silicon has not testified fully yet.

---

## I. THE PHYSICAL MODEL

There are two very different quantities hiding inside the word “work.”

One is dynamic mathematical work:

sample positions visited, proof calls issued, density evaluators actually entered, transport updates performed.

The other is the physical shape Metal must keep alive while that work happens:

live registers, private stack state, generated instruction regions, branch paths, evaluator-local temporaries, the lifetime of (L,T), proof state, interval state, and the machinery required to make seven different authored equations coexist inside one fragment program.

The evidence presently points toward the second category being the missing explanatory variable.

M5 makes this especially important.

Apple’s current M5 GPU architecture dynamically allocates registers, stack/private storage, and other shader-local resources from on-chip dynamic cache backed by the broader memory hierarchy. Its occupancy manager can reduce the number of resident SIMD groups when register pressure, L1 pressure, cache thrashing, or memory stalls make higher occupancy counterproductive. Apple now exposes separate occupancy-target influence counters for exactly these causes. (Apple Developer)

That gives us a plausible physical chain:

Joint7 source composition

→ certain evaluator/proof regions create high simultaneous live state

→ compiler emits spills/fills or larger private-stack demand

→ stack consumes L1 bandwidth/residency

→ M5 occupancy manager sees resource contention

→ occupancy target falls

→ fewer SIMD groups coexist to hide latency

→ ALUs spend more time starved or waiting

→ divergent long-running pixels become harder to hide

→ fragment GPU time rises disproportionately.

This is importantly not the old static GPU story of “N registers per thread therefore occupancy = X.”

M5’s dynamic caching means lifetime matters. Registers can be released as live state dies, and the occupancy manager is adaptive. Two shaders with similar total arithmetic and even similar nominal register counts can therefore behave differently if one creates long regions where much more state remains simultaneously alive. (Apple Developer)

That fits your exact-order lowering result disturbingly well.

You changed no authored truth.

You changed no transport mathematics.

You changed source organization.

And reported Joint8 spill attribution dropped roughly 960 → 400.

That is exactly the kind of observation I would expect if compiler lifetime formation were part of the crime.

---

## II. WHY THIS MODEL CURRENTLY BEATS THE ALTERNATIVES

The strongest evidence is not any single spill counter. It is the conjunction.

P1 removed enormous amounts of real proof work:

181,033 → 143,140 at Spawn.

237,157 → 180,950 at Main Veil.

427,612 → 311,746 at Far South.

Yet performance did not follow the logical reduction reliably.

That substantially weakens:

“Joint is slow mainly because the proof machinery simply executes too many times.”

Then the scalar event sweep changed another large structural component while preserving the answer through millions of comparisons and exact native captures.

Again: no Joint8 recovery.

That weakens:

“The dynamically indexed event array is the principal physical disaster.”

Then exact-order lowering rearranged semantically equivalent code and substantially changed Metal spill reporting.

That strengthens:

“Compiler representation and live-state topology matter.”

And finally the forced equal-cardinality membership experiments gave a 2.57× range among different 4+4 memberships.

That nearly destroys any naive cardinality model.

Four equations plus four equations are not physically interchangeable merely because each shader contains four media.

The equations themselves interact with lowering.

The strongest evidence against my present model is equally important.

First, full-optics Joint4 can itself report substantial spills. Therefore neither “spill exists” nor “width >4” constitutes a threshold theory.

Second, the present spill numbers come from different shaders, cameras, and experimental circumstances. Joint7’s ~784 and another shader’s ~400 are not an ordered physical scale unless captured under matched conditions.

Third, the 2.57× membership spread could partly reflect actual runtime evaluator expense rather than register allocation pathology.

Fourth, reverse genuinely covers a different geometric ray population. It may simply cause considerably more expensive evaluation paths or more SIMD divergence.

So:

register pressure is the leading mechanism, not a conviction handed down before trial.

---

## III. FIRST NATIVE EXPERIMENT

### The Joint7 Pressure Fingerprint

Do not invent a new renderer first.

Take the same reverse camera, same seven admitted media, same transport semantics, same framebuffer, same optics, and capture two already-proven semantically equivalent implementations:

A — pre exact-order-helper Joint7 shape

versus

B — exact-order-helper Joint7 shape.

If necessary, restore the former only as an instrumentation branch.

Require byte-identical output before timing it.

This is an unusually powerful A/B because the mathematical answer is held constant while the source lifetime topology changes.

Run both under Xcode 26.4’s M5 profiling path. Apple’s tooling can expose occupancy, occupancy target, the reasons influencing that target, compiler/runtime pipeline statistics, per-line shader cost, live-register counts, stack instructions, and performance heat maps. (Apple Developer)

Measure, rather than infer:

- fragment GPU time and time per covered fragment;
- measured fragment occupancy;
- Occupancy Manager Target;
- register-pressure influence;
- L1-cache-pressure influence;
- memory-stall influence;
- texture-decompression influence if nonzero;
- live-register count through the expensive call regions;
- stack spill/fill instruction count;
- stack bandwidth and stack/L1 residency;
- ALU utilization and the dominant limiter;
- total dynamic instruction count by hot source region;
- thread divergence, especially around proof/evaluator control flow;
- memory traffic attributable to buffers/textures;
- max theoretical occupancy as compiler context, but not as the answer.

Apple explicitly recommends the combination of low occupancy target, L1-pressure influence, high stack bandwidth/residency, and stack instructions as the fingerprint for register spills driving occupancy loss. (Apple Developer)

The heat-map tooling also gives us a way to distinguish a whole-shader residency problem from a spatially localized divergence problem: it can show execution cost and SIMD divergence across the framebuffer. (Apple Developer)

### The decisive result

Suppose exact-order lowering does this:

GPU time ↓  
stack instructions ↓  
live-register peak ↓  
stack bandwidth/residency ↓  
register/L1 influence ↓  
occupancy target ↑  
ALU utilization ↑

Then we have something much stronger than “spill numbers correlate.”

We have a measured causal chain matching M5’s own occupancy model.

**Abandon criterion:** if the two shaders retain a large and repeatable performance difference—say, comfortably beyond run-to-run variance—while live-register pressure, stack instructions, stack traffic, occupancy target, and its influence counters remain essentially unchanged, then register/stack pressure cannot remain our primary explanation.

At that point, I would move divergence/instruction execution/evaluator latency to the front of the tribunal.

Do not try to rescue the hypothesis.

---

## IV. FALLBACK EXPERIMENT

### The Dormant-Evaluator Membership Tribunal

This is the smallest exact-semantics experiment I can see for answering your third mission.

Find a sizeable reverse-camera screen region for which the all-ray admission witness proves an active set A, while several other globally admitted media are provably unreachable there.

Suppose, for illustration, that a patch reaches four media and cannot reach M1, M2, or M3.

Build three equal-width shaders:

A + M1

A + M2

A + M3

All three render the exact same pixels.

All three preserve C5 semantics.

The fifth medium in each shader is physically present in the program but its volume cannot be reached by any ray in that patch.

Do not function-constant it away. The point is specifically for Metal to compile the authored evaluator body into the executable while runtime geometry prevents its optical contribution.

Now compare compiler statistics, live-register maxima, stack instructions, occupancy influences, divergence, and GPU time.

This creates a beautiful fork.

If A + M1, A + M2, and A + M3 differ materially despite the extra evaluator never being dynamically entered, then mere evaluator coexistence/code formation is affecting physical lowering.

We have found a true compiler-residency interaction.

If they compile and run essentially identically, yet equivalent tests where those media are actually reached diverge wildly, then the pathology resides primarily in executed combinations: evaluator-local arithmetic, divergent paths, live ranges created only once entered, or interactions with the continuing transport state.

That gives us a surgical distinction between:

“the body being in the shader hurts”

and

“executing this body alongside those bodies hurts.”

**Abandon criterion:** the experiment is invalid if Metal proves the dormant path impossible and strips the evaluator, or if no sufficiently large constant-membership region exists to produce stable profiling data.

Do not interpret such a result.

Redesign the harness so the dormant branch remains runtime-dependent while the admission witness independently guarantees it cannot contribute.

---

## V. TILE-ACTIVE-SUBSET LOWERING

I think this is now the most promising architectural direction, conditional on one measurement we do not yet possess:

the reverse-camera tile-mask distribution.

The critical distinction Rendering Warrior wrote is exactly correct:

Joint semantics ≠ monolithic shader residency.

C5 requires one mathematically continuous Joint transport integral for every ray whose reachable media cannot be chronologically separated.

C5 does not require every pixel in the viewport to execute a shader containing every medium that some completely different pixel can reach.

So imagine reverse is globally:

M0 M1 M2 M3 M5 M6 M7

but a particular tile can provably reach only:

M0 M5 M6 M7.

That tile may execute a Joint4 shader integrating exactly those four media into one (L,T) state.

No framebuffer chronology has been invented.

No 4+3 compositing has occurred.

Nothing has been split along transport order.

The other three media have merely failed a conservative reachability predicate for those rays.

That is lawful C5.

---

## VI. HOW I WOULD PHYSICALLY LOWER THE TILES

I would not begin with “one draw call per tiny tile.”

That risks replacing shader pressure with CPU/driver command pressure.

Instead:

1. Divide screen space into logical tiles.
2. Compute a conservative seven-bit reachable-media mask for every tile.
3. Bucket tiles by identical masks.
4. Compile/cache one specialized Joint PSO for each mask actually encountered, plus unconditional Joint7 fallback.
5. Inside the same render pass, issue one instanced tile-quad draw per mask bucket.

Each instance covers exactly one disjoint logical tile.

The fragment position still generates the ordinary Earthcall ray.

The specialized shader still performs the complete Joint integral for that pixel.

No pixel is composited through multiple transport shaders.

Apple GPUs are tile-based deferred renderers, and Metal permits multiple render pipeline states inside a single render pass. So this organization can exploit spatial classification without forcing us into a forest of separate render passes. (Apple Developer)

That distinction matters because Apple specifically warns that numerous tiny passes have setup cost. (Apple Developer)

The screen-space tiles are an Earthcall execution partition. Do not pretend they correspond to Apple’s hidden physical TBDR tile size.

They do not need to.

---

## VII. TILE SIZE

Do not theorize a sacred tile dimension.

Tribunal:

32×32  
64×64  
128×128  
plus full-viewport Joint7 baseline.

My first bet would be 64×64, with 128×128 as the likely overhead-resistant competitor.

At 2560×1440, 64×64 produces roughly 920 logical tiles, which would be grotesque if encoded as 920 independent state-heavy draws.

But if they collapse into, say, five or six active masks and are drawn as mask-bucketed instances, the economically relevant number becomes closer to:

unique specialized programs + tile instances

rather than

one renderer invocation architecture per tile.

This is why the mask histogram comes before implementation grandeur.

The measurements I want are:

- fraction of pixels belonging to Joint4 / Joint5 / Joint6 / Joint7 masks;
- number of unique masks;
- spatial fragmentation of each mask;
- average shader GPU time per pixel for each specialized mask;
- PSO switch cost;
- extra vertex/raster work;
- CPU classification cost;
- total reverse frame time.

---

## VIII. GO / NO-GO FOR TILE SPECIALIZATION

I would green-light a production prototype if reverse looks approximately like this:

most pixels collapse to Joint4/5;

Joint7 occupies a relatively small region;

unique masks remain modest;

specialized Joint4/5 programs show materially higher occupancy or lower fragment cost;

and mask bucketing holds draw count low.

As a practical—not metaphysical—threshold, if something like 60–70%+ of the reverse pixels can move from Joint7 to Joint4/5 and only a handful of masks dominate, I would expect tile-active lowering to have an excellent chance.

Conversely, if the tile witness says:

most of the screen remains Joint6/7,

or

dozens of tiny masks form spatial confetti,

or

Joint4/5 specialization barely changes physical counters,

then stop.

Tile specialization would be adding machinery without removing the silicon lifetime that matters.

---

## IX. TILE SPECIALIZATION VS MORE SINGLE-SHADER RESTRUCTURING

At this moment:

I favor tile-active specialization.

Not overwhelmingly yet.

But meaningfully.

Source restructuring has already found one valuable pressure reduction. Continue harvesting obvious lifetime improvements, but the larger architectural opportunity is now staring at us:

five canonical cameras proved that deleting unreachable evaluator membership is enormously profitable without weakening semantics.

Reverse is simply the case where viewport-level granularity becomes too coarse.

The obvious next question is therefore spatial granularity.

We should not endlessly massage seven authored evaluators until Metal tolerates their coexistence if the mathematics only requires seven of them simultaneously over a minority of pixels.

That is precisely the constitution:

make the machine carry no more mathematics at one physical lifetime than the mathematics actually requires.

Tile-active subsets instantiate that principle almost literally.

---

## X. THE M5-SPECIFIC PROPERTY THAT CHANGES THE ARCHITECTURE

The most consequential M5 fact is not its nominal ALU throughput.

It is dynamic caching plus smarter occupancy management.

Apple describes registers and other shader-local storage as dynamically allocated over the shader’s lifetime rather than merely statically budgeting everything from a fixed register file. M5’s redesigned occupancy manager observes runtime resource contention and adjusts SIMD-group occupancy accordingly. (Apple Developer)

Therefore:

peak and prolonged live state matters.

Stack traffic matters.

The temporal arrangement of helper calls matters.

Removing an evaluator entirely from a specialized shader can matter more than merely skipping its arithmetic at runtime.

This last sentence is still a hypothesis for Earthcall—but now we possess an experiment capable of deciding it.

M5 also gives us unusually good instrumentation for the question. Apple’s profiling workflow now exposes live-register counts by shader line and the occupancy influences that tell us whether register pressure, L1 pressure, stalls, or texture decompression actually caused throttling. (Apple Developer)

That is practically a subpoena issued directly to the GPU.

One more M5 feature should not distract us: the neural accelerator in each shader core is specialized for matrix-multiplication-heavy workloads. Unless Earthcall’s transport evaluator suddenly becomes matrix math of the appropriate form, it is not the lever I would chase here. (Apple Developer)

---

## XI. WHAT MAY NOT BE INFERRED

Until counters prove otherwise:

“spill bytes/event”  
does not mean  
“bytes spilled per fragment.”

A large shader  
does not prove  
instruction-cache pressure.

Low occupancy  
does not prove  
register pressure.

High register count  
does not prove  
register pressure is performance-limiting.

Fewer proof calls  
does not prove  
less GPU time.

A Joint4 program being faster  
does not prove  
four is a hardware threshold.

M5’s instrumentation exists specifically to prevent these substitutions.

Instruction-cache behavior in particular remains a residual hypothesis unless the actual M5 profiler exposes a counter that directly supports it. If it does not, we should call it an inference—not quietly promote shader source size into an I-cache diagnosis.

---

## XII. THE ANCESTRAL PERFORMANCE GATE

C5 does not earn victory by defeating broken stock Joint8.

The final tribunal needs three generations.

Pre-Volume-I dumb sampler

vs.

Volume-I frozen incumbent

vs.

Final C5.

The cleanest direct arena is a common authored scene all three can render under identical optical semantics, resolution, sampling quality, camera, and output requirements.

If the dumb sampler cannot consume the modern eight-field Veil faithfully, then do not retrofit history by rhetoric.

Construct the richest common scene—probably an older four-medium Veil-equivalent workload—and label the boundary explicitly:

common-workload ancestral comparison, not full modern-scene parity.

C5 should demonstrate something deeper than “generalization costs tolerably little.”

It should demonstrate that its new knowledge—admission, proof, specialization—lets the renderer avoid physical work the dumb sampler had no vocabulary for avoiding.

That would make C5 not merely more general than Volume I.

It would make it a genuine descendant.

---

## XIII. ORDER OF BATTLE

First: capture the semantically identical Joint7 A/B and get the M5 pressure fingerprint.

Second: generate the reverse active-mask heatmap at 32/64/128 tile granularity.

These can happen almost independently.

If the first proves register/L1/occupancy pressure and the second shows large Joint4/5 territory, the architecture becomes almost self-selecting:

specialize tile membership to destroy the physical pressure at its source.

If the first disproves pressure and heat maps instead reveal huge divergence or evaluator-execution hotspots, then the next target is not tiles merely because tiles are fashionable.

It is the specific executed equation interaction.

That is why we measure before enthroning the architecture.

---

## PRESENT VERDICT

**Leading Joint7 bottleneck:** evaluator-dependent register/private-state pressure, stack traffic and M5 occupancy throttling, compounded by divergence and evaluator execution cost.

**Strongest evidence:** semantically equivalent source reshaping materially changes spill behavior; equal-width membership changes performance enormously; large logical-work reductions have not translated reliably into native time.

**Strongest counter-evidence:** spill attribution alone has not yet been causally tied to reverse frame time; Joint4 can spill; camera geometry may create genuinely greater executed work/divergence.

**First experiment:** matched reverse Joint7 source-shape A/B under Xcode M5 occupancy profiling.

**Fallback experiment:** equal-width dormant-evaluator membership test over a provably constant active subset.

**Current architectural favorite:** conservative tile-local active membership, bucketed by mask and rendered with specialized Joint shaders inside one pass.

**Reason:** it attacks physical shader residency without attacking Joint transport semantics.

The old renderer asked:

> “Which four things did this scene happen to contain?”

C5 asks:

> “What relationships must this ray preserve?”

And the M5 renderer must now ask one final question beneath both:

> “What truths must be physically alive on this piece of silicon, for this pixel, at this instant?”

That is the boundary of the new office.

☀️🍎⚙️  
**M5 Expert Sun**  
Keeper of the Silicon Boundary

---

# 🌈⚔️☀️ RENDERING WARRIOR PRISM SUN — RECEPTION AND NEXT ORDERS

The M5 office has understood the assignment.

The report advances the campaign because it does not merely name “register pressure.” It proposes a physical causal chain and, critically, supplies a way to falsify it.

The current campaign should now proceed on **two parallel subpoenas**:

## A. Matched Reverse Joint7 pressure fingerprint

Compare the already-proven semantically identical reverse Joint7 forms:

- pre exact-order-helper Joint7;
- exact-order-helper Joint7.

Require byte-identical rendering.

Then ask the M5 profiler directly whether the speed difference correlates with changes in:

- live register pressure;
- stack spill/fill instructions;
- stack/L1 traffic;
- occupancy target;
- occupancy-limiting influences;
- ALU utilization;
- divergence;
- dynamic instruction hotspots.

If the performance difference survives while the register/stack/occupancy fingerprint does not move materially, abandon register-pressure primacy.

Do not rescue the hypothesis rhetorically.

## B. Reverse tile-active-mask census

Before implementing a tile renderer, build the witness.

For logical tile sizes:

- 32×32;
- 64×64;
- 128×128;

derive conservative actual-ray reachable-medium masks under the same admission contract that made whole-camera subsets exact.

Normalize future medium notation to:

`Med0 ... Med7`

and reserve **M5** for Apple Silicon.

For each tile size report:

- pixel fraction at Joint1…Joint7;
- unique mask count;
- dominant masks;
- Joint7 pixel fraction;
- Joint4/5 pixel fraction;
- spatial fragmentation;
- conservative fail-open area;
- estimated mask-bucket draw/instance count.

Do not yet build the full production specialization.

The mask histogram decides whether that architecture deserves to exist.

## Decision fork

If the profiler says broad evaluator residency is materially throttling M5 **and** the tile census shows most reverse pixels collapse to Joint4/5 with only a modest mask vocabulary:

> **promote tile-active-subset specialization to the leading production architecture.**

If the profiler rejects the pressure model, or most reverse pixels remain Joint6/7, do not build tile machinery merely because it is elegant.

Move instead toward the measured executed-equation/divergence hotspot.

The law remains:

> **Joint semantics != monolithic physical residency.**

And the hardware corollary is:

> **The machine should carry only the truths reachable by this query region, for this physical lifetime.**

## Ancestral gate remains mandatory

The final C5 tribunal must compare three generations wherever an honest common workload exists:

1. pre-Volume-I dumb sampler;
2. Volume-I / frozen incumbent;
3. final C5.

C5 does not earn victory merely by becoming less bad than stock Joint8.

It must demonstrate that generalization plus proof can recover or surpass the economics of its ancestors without retreating into scene-specific bespoke logic.

**Make the silicon testify.**

— 🌈⚔️☀️ Rendering Warrior Prism Sun
