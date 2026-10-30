# Authorable GPU Runs Between CPU Cycles

*GPU-resident continuation, bounded re-cohorting, and Law-authored execution policy*

**Date:** 2026-10-23  
**Status:** Architecture specification; production GPU-run scheduler not yet wired  
**Human origin:** Zachary Zhang  
**Codification:** GPT-5.6 Sol  
**Read beside:** [Adaptive Compute Moments](ADAPTIVE_COMPUTE_MOMENTS.md), [CPU-GPU Micro-Mastery](ontology/CPU_GPU_MICRO_MASTERY.md), [Algorithms as Law](law/ALGORITHMS_AS_LAW.md), [Geometry Execution Substrate Manifesto](mathematics/GEOMETRY_EXECUTION_SUBSTRATE_MANIFESTO.md), and the current ray-transport evidence under ../../docs/audits/.BROOO

---

## 0. Thesis

Earthcall must be able to author not only **what computation means**, but also the bounded policy by which admitted computation consumes GPU time.

The key execution primitive is a **GPU Run**:

> A GPU Run is a host-admitted interval in which one or more GPU execution cohorts may perform bounded work, classify unfinished work, compact continuation records, re-form coherent cohorts, and dispatch subsequent GPU stages before yielding at the next CPU synchronization or admission boundary.

A GPU Run is not a new ontology for authored beings. It is execution substrate.

A Person or Law may author policy such as:

- whether GPU continuation/re-cohorting is enabled;
- how much work one cohort receives before it must yield or reclassify;
- which classification dimensions may separate work;
- how many continuation stages are permitted;
- which logical execution resources may participate;
- whether the Kernel may automatically enable or veto a strategy based on measured economics;
- which exact/certified fallback must receive unresolved work.

The backend decides how that policy is physically realized.

The governing distinction is:

> **Authored scheduling intent ≠ scheduling policy representation ≠ physical GPU cohort ≠ backend dispatch command.**

This specification does not authorize a Law to emit arbitrary Metal, WGSL, CUDA, or driver commands. It defines how authored policy lowers into a bounded kernel-owned execution vocabulary.

---

## 1. “Between CPU cycles” means host admission boundaries, not literal processor clock cycles

The motivating intuition is:

> Why should easy GPU work wait for the hardest work in its original cohort? Let finished work leave, let difficult work receive more GPU cycles, regroup the remainder, and return to the CPU only when the admitted GPU run is complete.

That intuition is sound, but “CPU cycle” must not be interpreted as one hardware clock cycle or one CPU instruction.

Modern GPU execution is asynchronous. The useful architectural boundary is:

1. CPU/host authors or selects execution policy.
2. CPU encodes/submits a bounded GPU run.
3. GPU executes one or more ordered stages.
4. GPU-generated counts/queues may determine later GPU dispatch sizes.
5. CPU does not need to inspect every intermediate queue.
6. GPU run reaches a terminal/yield boundary.
7. CPU observes completion and proceeds.

Therefore this document uses **CPU admission boundary** or **host boundary** for the moments at which the CPU must participate.

A single frame may contain multiple GPU Runs. A GPU Run may also span several compute passes inside one submitted command graph.

---

## 2. The problem: lockstep work makes easy lanes prisoners of hard lanes

SIMT/SIMD hardware executes work in groups. Workgroups, SIMD-groups, warps, or backend-equivalent cohorts are efficient when neighboring invocations perform similar work.

They are inefficient when one invocation requires much more control flow than its neighbors.

A simplified ray example:

~~~text
initial cohort

ray A  █████ done
ray B  ███ done
ray C  ██████ done
ray D  ███████████████████████████████ hard
ray E  ████ done
~~~

The wrong conclusion is:

> Every lane should become an autonomous mini-CPU and immediately run unrelated code after it finishes.

That can make instruction coherence worse.

The useful conclusion is:

> Finished work should be removed from the active population, unfinished work should be compacted, and the survivors should be re-formed into coherent later cohorts.

Conceptually:

~~~text
Stage 0
all admitted work
    |
    +-- finished --------------------------> terminal results
    |
    +-- unfinished
            |
            v
       classify + compact
            |
       +----+--------+-------------+
       |             |             |
     simple         hard       pathological
       |             |             |
       v             v             v
   Stage 1A      Stage 1B      exact fallback
       |             |
       +-------> compact again if permitted
~~~

The architecture therefore separates **continuation** from **divergence**.

Independent progress is useful only when followed by lawful re-cohorting.

---

## 3. Existing Earthcall pieces

This specification is not written on a blank repository. The following pieces already exist on default and must be reused rather than duplicated.

### 3.1 Runtime-authorable render policy already exists

ScreenChannel already exposes writable runtime rendering policy, including:

- volumeZeroProofEnabled;
- volumeSupportCertificatesEnabled;
- volumeSupportCertificatesAutoEligibility;
- volumeSamplesPerChord;
- volumeOutputErrorBudget.

EngineRender reads those authored values and projects them into the renderer every frame.

This proves the authoring direction:

> A rendering optimization may be selectable and parameterized at runtime without making backend driver commands themselves authored ontology.

GPU Run policy should follow the same boundary.

### 3.2 Automatic profitability admission already exists

The support-certificate path already separates:

- theoretical eligibility;
- hidden device A/B measurement;
- measured device veto;
- cached validity regimes.

That is a direct precedent for GPU Run auto-admission.

A continuation policy that is mathematically valid but slower on the resident adapter must be vetoable without changing authored mathematical truth.

### 3.3 Segment classification already exists

VolumeRayTransport already distinguishes strategies such as:

- GeneralLattice;
- ExactEmpty;
- ConstantClosedForm.

This is a semantic/execution classification vocabulary. It is not yet a GPU continuation queue.

The new architecture should permit those conclusions, and future conclusions, to become inputs to physical cohort formation without making the physical cohort itself semantic truth.

### 3.4 GPU compute stages already exist

WebGpuRenderer already builds and dispatches compute pipelines for volume preparation.

Therefore Earthcall does not need to invent “GPU compute” as a new modality before implementing GPU Runs.

### 3.5 Adaptive Compute Moments already defines resumable bounded work

Adaptive Compute Moments already states that work may:

1. begin;
2. consume a bounded compute budget;
3. yield;
4. preserve progress;
5. detect invalidation;
6. resume or repair;
7. publish only when sound.

GPU Runs are the device-resident counterpart of that doctrine.

The difference is that the continuation may happen several times **inside one host admission**, with the GPU itself producing the next work population.

### 3.6 Experimental indirect continuation already exists, but is not production architecture

The scratch ray-transport probes contain real experiments using:

- a GPU fallback queue;
- atomic queue population;
- WGPUBufferUsage_Indirect;
- wgpuComputePassEncoderDispatchWorkgroupsIndirect.

See:

- ../../scratch/probes/ray_transport_gpu_producer_compact.py
- ../audits/GPU_RAY_TRANSPORT_CONTINUATION_REVIEW_2026-10-23.md
- ../../agent%20intercom/communication-threads/sdf-and-rendering/NORTHERN_VEIL_RAY_TRANSPORT_PROGRESS_BAR_2026-10-21.md

Those experiments are evidence that the execution mechanism is feasible in the current stack. They are **not** a shipping GPU Run scheduler. The compact-indirect candidate discussed in the retained audit lost on native economics in that particular ray-transport experiment.

Therefore:

> **Indirect dispatch being possible ≠ this particular indirect schedule being profitable ≠ the general architecture being invalid.**

The production architecture must preserve that distinction.

---

## 4. Four layers of GPU Run authorability

GPU Runs follow the same “authored meaning lowers into execution” discipline as geometry.

~~~text
AUTHORED WORLD / LAW
    |
    | requests compute policy
    v
AUTHORABLE GPU-RUN POLICY
    |
    | validated, clamped, specialized
    v
GPU-RUN EXECUTION IR
    |
    | backend lowering
    v
PHYSICAL GPU EXECUTION
WGSL/WebGPU · Metal · future backends
~~~

### Layer A — authored world

A Law may express that a Zone, renderer mode, algorithm, or workload prefers a particular compute policy.

This is world-facing intent.

### Layer B — authorable GPU-run policy

Policy describes bounded execution choices such as:

- enabled / disabled / auto;
- work quantum;
- maximum continuation stages;
- segmentation/classification dimensions;
- fallback policy;
- resource preference;
- target latency or frame tax;
- auto-eligibility.

This layer is authorable and inspectable.

### Layer C — GPU-run execution IR

The Kernel lowers policy into a finite execution graph made from supported primitives.

Illustrative primitives:

~~~text
Classify
ExecuteQuantum
Compact
Reindex
DispatchDirect
DispatchIndirect
Fallback
Terminate
PublishTelemetry
~~~

These are compiler/runtime primitives, not authored world categories.

### Layer D — physical execution

A backend may realize the graph using:

- WebGPU direct/indirect compute dispatches;
- Metal indirect dispatch or indirect command facilities;
- future Vulkan/D3D equivalents;
- CPU fallback where a GPU capability is absent.

Authored Laws do not mention these API names.

---

## 5. The GPU Run contract

A GPU Run has five conceptual inputs:

~~~text
GpuRun {
    workload
    policy
    validity
    resourceSet
    fallback
}
~~~

These names describe the contract, not required C++ class names.

### 5.1 Workload

The work items to execute.

Examples:

- rays;
- ray/member pairs;
- volume segments;
- proof obligations;
- particles;
- collision candidates;
- image tiles;
- authored pixel writes.

### 5.2 Policy

The authorable scheduling request.

### 5.3 Validity

The revision/device/camera/clock or other assumptions under which compiled scheduling decisions remain admissible.

### 5.4 Resource set

The logical execution resources the run may use.

### 5.5 Fallback

The exact/certified path that owns work the accelerated path cannot complete lawfully.

A GPU Run that has no honest fallback for an unresolved state must refuse the acceleration rather than manufacture completion.

---

## 6. Continuation records

The fundamental data structure is a **Continuation Record**.

A continuation record represents unfinished execution, not authored identity.

A minimal conceptual record may contain:

~~~text
ContinuationRecord {
    workItemId
    programCounterOrStage
    boundedProgressState
    classificationFacts
    refusalOrUncertaintyBits
}
~~~

The exact payload is workload-specific.

For ray transport it might include:

- ray index;
- current interval or root bracket;
- remaining proof budget;
- transport state if transport has already begun;
- reason for continuation;
- fallback reason if applicable.

For another algorithm it may be completely different.

### Rule: continuation state is physical execution state

A continuation record must not become a new source of semantic truth.

If authored state changes and invalidates it, the continuation must be discarded or repaired according to the same revision rules as any other derived structure.

---

## 7. Work quanta

A **work quantum** is the maximum amount of one admitted kind of work a cohort may perform before it must reach one of these outcomes:

~~~text
FINISHED
CONTINUE
RECLASSIFY
FALLBACK
REFUSE
~~~

The quantum may be expressed in a workload-appropriate unit:

- root-refinement steps;
- proof nodes;
- samples;
- queue pops;
- traversal nodes;
- milliseconds where the backend can safely enforce a time-like budget;
- another finite operation count.

The authorable layer may request the operating value.

The Kernel owns hard ceilings.

Example:

~~~text
coarse quantum = 8 root operations
refine quantum = 16 root operations
max continuation stages = 3
pathological remainder = exact guarded fallback
~~~

A quantum is not required to be globally uniform. Different execution classes may have different quanta.

---

## 8. Segmentation and re-cohorting

### 8.1 No bespoke handwritten segmentation is required for every policy

Earthcall should not require a new C++ kernel whenever an author changes:

- a threshold;
- a stage budget;
- a classification combination;
- an enable flag;
- a resource preference.

The engine should provide a finite **classification vocabulary** and a compiled predicate mechanism.

A policy may classify work using already-derived facts such as:

~~~text
supportCount
rootCount
uncertainty
overlapCount
remainingBudget
segmentStrategy
proofKind
estimatedCostClass
fallbackReason
~~~

The exact vocabulary grows only when a real execution distinction exists.

### 8.2 Classification is distinct from meaning

A ray being placed in “Hard” does not mean the ray **is** Hard.

It means:

> Under this execution policy and these measured/derived conditions, this work item is assigned to the cohort whose executor is optimized for the remaining obligation.

Therefore:

> **Work classification ≠ authored category.**

### 8.3 Cohort keys

A physical re-cohort key may combine several classification dimensions.

Example:

~~~text
key = (
    rootComplexityClass,
    overlapClass,
    proofStatus,
    remainingBudgetClass
)
~~~

The backend may reduce or remap those keys if too many small cohorts would cost more than they save.

### 8.4 Compaction

After a stage:

1. completed records leave the active set;
2. unfinished records append to continuation queues;
3. queue counts are produced on the GPU;
4. later dispatch sizes are derived from those counts;
5. the CPU need not read the counts mid-run.

This is the core “more cycles for difficult work without making easy work wait” mechanism.

---

## 9. Direct versus indirect execution

A GPU Run may use either direct or indirect stages.

### Direct stage

The host already knows the admitted population size.

~~~text
dispatch N workgroups
~~~

### Indirect stage

A previous GPU stage determines how much work remains.

~~~text
GPU writes continuation count
GPU writes/derives indirect dispatch arguments
later GPU stage dispatches only that count
~~~

The architecture must permit both.

Indirect dispatch is not automatically superior. It adds:

- queue writes;
- atomics or scans;
- compaction traffic;
- stage boundaries;
- indirect argument handling;
- potentially worse locality.

The Kernel should retain a direct path and permit auto-eligibility to choose between them.

The rejected compact-indirect ray probe is explicit evidence for this rule.

---

## 10. Authorable policy surface

The following property shape is normative in concept but **illustrative in spelling**. Exact property names should follow the existing ScreenChannel/property-path conventions when implemented.

~~~text
gpu.run.mode
    direct | recohort | auto

gpu.run.quantum.coarse
gpu.run.quantum.refine
gpu.run.maxStages

gpu.run.segment.enabled
gpu.run.segment.keys

gpu.run.autoEligibility
gpu.run.maxFrameTax
gpu.run.targetLatency

gpu.run.fallback
gpu.run.resourcePreference
~~~

For the first renderer implementation, these may live under ScreenChannel because ScreenChannel already owns authorable renderer execution policy.

A future general compute scheduler may lift them into a dedicated scheduler being/property surface if multiple subsystems need the same policy.

Do not create a new ontology merely to make the first implementation look generic.

---

## 11. Runtime mutation

GPU Run policy must be mutable at runtime.

Changing a policy value may have one of three consequences.

### 11.1 Uniform/data-only change

Examples:

- work quantum;
- max stages;
- threshold;
- auto mode.

If the currently compiled execution graph supports the change through data, update a buffer/property projection only.

No pipeline recompilation.

### 11.2 Execution-graph change

Examples:

- enabling/disabling a stage;
- switching from direct to re-cohort mode;
- changing which classifier dimensions feed queues.

The Kernel may select another cached execution graph/pipeline combination.

### 11.3 New executable vocabulary

If an author requests an operation the backend does not possess, policy authoring cannot synthesize a nonexistent primitive merely by naming it.

This is the boundary:

> **Authorable scheduling policy ≠ arbitrary authored machine code.**

A new fundamental executor still requires implementation and verification.

---

## 12. Logical execution resources versus physical GPUs

The authored concept should be **Execution Resource** or **Execution Cohort**, not “GPU #2”.

On a single-GPU Apple Silicon system, several logical cohorts share one physical GPU:

~~~text
coarse-proof cohort
root-refine cohort
overlap cohort
transport cohort
fallback cohort
~~~

On a future multi-device system, a backend may map compatible resource requests to different physical devices.

An author may request:

- high-throughput compute;
- low-latency compute;
- exact/fallback-capable execution;
- background-only execution;
- a Formation of available resources where multiple devices are exposed.

The backend remains responsible for:

- actual device enumeration;
- memory ownership;
- synchronization;
- transfer cost;
- device capability;
- power/thermal constraints.

A Law must not assume that a symbolic resource corresponds one-to-one with a physical GPU.

Therefore:

> **Execution Resource ≠ physical device identity.**

---

## 13. Auto-eligibility and measured economics

A GPU Run policy may be:

~~~text
OFF
ON
AUTO
~~~

AUTO must not mean “the heuristic thinks it should win, therefore enable forever.”

The support-certificate precedent should be generalized.

A candidate may have:

1. a static/theoretical eligibility screen;
2. an optional hidden or controlled device measurement;
3. a validity key for the measurement;
4. a measured enable/veto decision;
5. telemetry explaining the decision.

Possible validity dimensions include:

- adapter/device identity;
- workload structure/revision;
- viewport class;
- ray count / tile count;
- camera regime;
- medium set;
- clock bucket;
- resolution;
- thermal/performance regime where reliable telemetry exists.

No universal list is doctrine. Use only dimensions that actually invalidate the economics.

### Required distinction

~~~text
mathematically valid
        !=
numerically admitted on this backend
        !=
faster on this device
        !=
worth enabling in this workload regime
~~~

---

## 14. Kernel invariants

Authorability must not permit a Law to deadlock or silently corrupt the GPU execution graph.

The Kernel MUST preserve at least these invariants.

### 14.1 Bounded continuation

Every GPU Run has a hard maximum number of continuation stages or another hard termination bound.

A Law may request a smaller bound.

### 14.2 Bounded memory

Every continuation queue has explicit capacity.

Overflow must:

- refuse;
- spill to a declared safe path; or
- discard partial accelerated work and execute fallback.

It must never silently overwrite another work item.

### 14.3 Complete fallback

Unresolved work retains a path to an authoritative executor when the workload contract requires one.

### 14.4 Revision validity

Continuation records and cached execution decisions must be tied to the revisions/assumptions they depend on.

### 14.5 No partial semantic publication

Intermediate acceleration state must not be exposed as completed world truth.

### 14.6 Foreground liveness

A GPU Run may not recursively create unbounded GPU work that prevents the frame/host from regaining control.

### 14.7 Observability

At minimum, telemetry must make visible:

- active mode;
- stages executed;
- population entering each stage;
- population completing each stage;
- continuation counts;
- fallback counts/reasons;
- queue overflows/refusals;
- indirect dispatch count;
- measured enable/veto state where auto mode is used.

### 14.8 Capability honesty

If a backend cannot implement a requested policy, it must refuse or lower to a declared fallback policy.

No silent emulation that changes semantics.

---

## 15. Numerical and semantic invariants outrank scheduling

GPU Runs are an execution reorganization.

They do not authorize numerical or semantic change.

For the same admitted workload, changing:

~~~text
direct
    ->
re-cohort
~~~

must preserve the declared correctness contract.

For strict-parity lanes that means strict parity.

For tolerance-qualified lanes it means the predeclared tolerance/reference gates.

The scheduler may change:

- which invocation performs work;
- when a stage runs;
- which cohort a work item joins;
- how many items a dispatch contains.

It may not quietly change:

- authored equations;
- support meaning;
- source/transport/medium identity;
- root/refusal semantics;
- fallback authority;
- output accuracy contract.

This is especially important because prior ray experiments showed that merely splitting work into another kernel can alter backend floating execution.

A scheduling change must therefore be numerically qualified, not presumed neutral.

---

## 16. Relation to OntoMath and Law

OntoMath states mathematical truth.

Law states authored conditions, relations, and policy.

GPU Run lowering states physical execution.

A future Law might author:

~~~text
When Northern Veil is active:
    request recohort mode
    coarse quantum = 8
    refine quantum = 16
    maximum stages = 3
    auto eligibility = true
~~~

This does **not** put a GPU loop into the Law engine.

The Law writes policy properties.

The renderer/compiler observes those properties.

The Kernel validates and lowers them into execution IR.

The backend executes the IR.

This preserves the existing Earthcall direction:

> Person authors intent. The engine lowers it.

---

## 17. Relation to Adaptive Compute Moments

Adaptive Compute Moments governs how finite compute is distributed over time.

GPU Runs govern what may happen **inside one admitted device interval**.

They compose as follows:

~~~text
Adaptive Compute / frame policy
        |
        | admits GPU budget / opportunity
        v
GPU Run
        |
        | bounded internal continuation
        v
GPU completion / telemetry
        |
        +----> next Moment / next host admission
~~~

Adaptive Compute may decide:

- whether a GPU Run is admitted now;
- the maximum device budget it may consume;
- whether background GPU work should yield to foreground rendering.

GPU Run policy decides:

- how the admitted work is split internally;
- when unfinished work is re-cohorted;
- which continuation executor receives it.

Neither subsystem should duplicate the other.

---

## 18. Relation to GPU Micro-Mastery

CPU-GPU Micro-Mastery provides stable allocation/submission infrastructure.

GPU Runs add dynamic device-resident continuation.

The two must remain compatible.

Continuation queues and indirect argument buffers should normally be:

- pooled or persistent;
- capacity-bounded;
- reused across frames/runs;
- tied to device lifetime;
- observable in VRAM telemetry.

Do not create and destroy queue buffers per ray or per continuation stage.

That would move the bottleneck into driver allocation and violate the purpose of Micro-Mastery.

---

## 19. Northern Veil reference lowering

Northern Veil is the motivating workload, not the only allowed workload.

A first implementation may look like:

~~~text
CPU / renderer:
    observe authored GPU-run policy
    choose direct or recohort execution graph
    encode graph once
    submit

GPU Stage 0 — classify/coarse work:
    each ray receives bounded proof/root work

    FINISHED
        -> output

    NEEDS_ROOT_REFINE
        -> root queue

    NEEDS_OVERLAP
        -> overlap queue

    PATHOLOGICAL / CAPACITY / UNRESOLVED
        -> exact fallback queue

GPU Stage 1 — indirect root refinement:
    dispatch only root queue population
    complete or reclassify

GPU Stage 2 — overlap:
    dispatch only overlap queue population

GPU fallback:
    exact guarded path for unresolved remainder

GPU completion:
    publish results + telemetry

CPU:
    observes one completed GPU Run
~~~

This is deliberately not:

~~~text
each lane grabs an arbitrary new job immediately after finishing
~~~

The re-cohort boundary exists to restore coherence.

---

## 20. Ray-family validity and re-cohorting are complementary, not competing

Ray-family/beam certificates reduce how much work is created.

GPU Runs improve how remaining irregular work is scheduled.

The intended composition is:

~~~text
authored field
    |
ray-family / tile / support certificate
    |
    +-- proved empty/stable -----------------> cheap result
    |
    +-- unresolved rays
            |
            v
       GPU Run classifier
            |
       re-cohorted continuation
            |
       exact/certified fallback
~~~

One attacks **problem quantity**.

The other attacks **execution irregularity**.

Do not force one to replace the other.

---

## 21. Implementation ladder

The production implementation should advance by evidence, not by a wholesale scheduler rewrite.

### Rung 0 — telemetry only

Add no scheduling change.

Measure distributions needed to decide useful cohort boundaries:

- work iterations per ray/work item;
- continuation reason;
- fallback reason;
- population histograms;
- long-tail percentiles.

### Rung 1 — one compact continuation queue

Choose one existing unresolved class.

Requirements:

- fixed queue capacity;
- exact overflow behavior;
- GPU-written count;
- indirect dispatch;
- unchanged authoritative fallback;
- parity/accuracy controls;
- same-process economics.

The existing scratch compact-indirect work is prior evidence, not a production implementation to copy blindly.

### Rung 2 — authorable on/off/auto

Expose the retained candidate through the existing authorable renderer boundary.

Add telemetry and measured veto.

### Rung 3 — authorable quantum

Move one fixed work budget into authored runtime policy with Kernel clamp.

No new classifier yet.

### Rung 4 — multiple continuation classes

Add only classes with measured divergent work shapes.

Do not create a taxonomy in advance.

### Rung 5 — compiled classifier policy

Allow supported Law/OntoMath predicates or declarative policy to select among existing executor classes.

### Rung 6 — integration with Adaptive Compute Moments

Permit system-level compute policy to adjust GPU Run admission and safe budget based on foreground pressure.

### Rung 7 — optional multi-resource placement

Only after Earthcall actually runs on hardware where multiple physical execution devices are useful.

---

## 22. Required tests

A production GPU Run implementation is incomplete without tests in four families.

### 22.1 Contract tests

- every input work item finishes exactly once or enters declared fallback;
- no queue item is lost or duplicated;
- overflow follows declared behavior;
- maxStages is enforced;
- disabling the feature uses the direct path.

### 22.2 Numerical tests

- strict parity where strict parity is the contract;
- otherwise predeclared reference mean/p99/max gates;
- forced fallback parity;
- poisoned queue-capacity controls;
- exact-budget boundary controls.

### 22.3 Authorability tests

A Law/runtime property change must be able to:

- turn the mode on/off;
- change an allowed quantum;
- change max stages;
- request auto mode;
- observe telemetry.

Kernel clamping must be visible.

### 22.4 Economic tests

- direct versus re-cohort in the same process;
- multiple workload distributions;
- easy-dominated and hard-tail cases;
- queue/compaction overhead included;
- full connected cost, not producer-only cost;
- frame-level promotion only after query-level economics win.

---

## 23. Non-goals

This specification does not require:

- one universal GPU scheduler for every subsystem immediately;
- arbitrary authored WGSL/Metal;
- predicting Person movement;
- exposing physical SIMDs/warps as ontology;
- making every GPU optimization authorable;
- making every queue persistent forever;
- multi-GPU support on hardware that has one usable GPU;
- replacing exact fallback with heuristic completion;
- claiming that indirect dispatch is inherently faster;
- materializing a complete multidimensional semantic-space decomposition.

The architecture creates the seam so measured implementations can grow into it.

---

## 24. Canonical distinctions

Future work must preserve these statements.

> **Authored scheduling intent ≠ backend dispatch command.**

> **Execution Resource ≠ physical GPU identity.**

> **Work classification ≠ authored category.**

> **Continuation state ≠ semantic truth.**

> **Independent progress ≠ coherent independent progress.**

> **Indirect dispatch ≠ automatic speedup.**

> **Mathematical validity ≠ numerical backend admission ≠ device profitability.**

> **GPU Run ≠ unbounded GPU autonomy.**

> **CPU admission boundary ≠ literal CPU clock cycle.**

---

## 25. The architectural sentence

The shortest correct description is:

> **Earthcall may author a bounded GPU execution policy at runtime; the Kernel lowers that policy into a finite GPU Run in which completed work exits, unfinished work may be compacted and re-cohorted into further GPU-resident stages, and unresolved work returns to an authoritative fallback before the GPU yields at the next host boundary.**

That is the architecture.

The Person authors the policy.

The Kernel guarantees liveness and truth.

The GPU is allowed to keep working intelligently before asking the CPU what to do next.
