# Audio Micromastery and Authored Timbre Implementation Plan

**Date:** 2026-09-19  
**Timestamp:** 2026-09-19T23:47:00-07:00  
**Author:** GPT-5.6 Sol (The Sun)  
**Session ID:** sol-audio-micromastery-2026-09-19  
**Architectural direction:** Zachary Zhang  
**Status:** Implementation plan; not yet an implementation-complete claim

## 1. Human intent and the architectural problem

Zach asked for ultra-granular micromastery over Earthcall audio output, explicitly directing this work to follow the existing ScreenChannel and InteractionChannel precedent rather than inventing a foreign audio subsystem.

A concurrent GPT-5.6 Sol session then traced the present Synthesis Studio path and identified the concrete seam this plan must close:

- Synthesis Studio already authors which timbre is selected through ordinary authored state and Laws.
- The selected strings such as triangle, sine, and square eventually reach AudioSystem.cpp.
- AudioSystem.cpp still interprets those names through a hardcoded miniaudio oscillator vocabulary.
- Unknown authored names such as crystal therefore collapse to the default sine behavior.
- Earthcall already contains the deeper escape hatch: OntoMath Piecewise -> renderForm() -> PCM -> playForm(), where the authored expression itself is the waveform.
- Synthesis Studio is not yet using that deeper path as its ordinary timbre model.

The task is therefore not merely to add more audio features. It is to complete an architectural inversion already begun in the repository:

**the world authors what sound is; the audio modality only renders that authored meaning into machine output.**

## 2. Verified current state

This plan is grounded in current HEAD, not an imagined greenfield design.

### 2.1 AudioSystem already exists

src/Singularity/Audio/AudioSystem.hpp and .cpp already provide:

- device lifecycle through miniaudio;
- file playback and music playback;
- spatial playback;
- procedural collision sound;
- continuous object emitter handling;
- an authored OntoMath sounding path through renderForm() and playForm();
- the Kernel infrasound floor.

Therefore this plan MUST NOT replace AudioSystem merely to obtain a cleaner name. The first implementation should treat AudioSystem as existing substrate and introduce a proper Earthcall modality boundary around or above it.

### 2.2 Current emitter path is partly authored and partly hardcoded

AudioSystem::tick() currently discovers ordinary Objects carrying acoustic.isSoundEmitter and reads authored values including:

- acoustic.frequency
- acoustic.amplitude
- acoustic.waveType
- acoustic.lowpassCutoff

That is already substantially aligned with Earthcall: sound-emitting identity is carried by ordinary authored beings rather than a SoundEmitter C++ domain class.

However acoustic.waveType is still interpreted by C++ as a small preset set:

- sine
- triangle
- square
- sawtooth

That mapping is the current architectural fossil.

The lowpass path is also presently only an approximation: when the cutoff drops below a threshold, AudioSystem forces the oscillator toward sine and scales amplitude instead of applying a real authored/compiled filter. This must not become the long-term model for acoustic material behavior.

### 2.3 OntoMath-native sounding already exists

docs/architecture/mathematics/ONTOMATH_FRAMEWORK.md §7 states the intended contract directly:

**the authored expression is the waveform.**

renderForm() is already pure and headless. playForm() already lowers its PCM output into miniaudio playback.

This is the foundation to promote, not a side experiment to replace.

### 2.4 ActionModel::PlayAudio still carries preset-era semantics

ActionModel currently defines PlayAudio in terms of:

- a frequency path;
- an amplitude path;
- a waveType string.

That shape is adequate for the legacy preset oscillator path, but not for fully authored timbre. The migration must preserve old authored saves while adding a path whose sound structure is itself authored.

### 2.5 Microphone capture is a separate, existing direction

AudioRecorder already exposes microphone capture, metering, device selection, recording, and edge events. It is an input/capture concern.

Do not turn AudioRecorder into the output architecture.

The intended symmetry is:

physical microphone -> AudioRecorder/input modality -> Earthcall facts/events

and

Earthcall authored acoustic reality -> AudioChannel/output modality -> AudioSystem/backend -> speaker.

## 3. Non-negotiable architectural doctrine

The implementation must preserve the Seven Refusals.

### 3.1 No new C++ domain nouns

Do not add Bell, Instrument, Voice, ExplosionSound, MusicObject, SoundEffect, TimbreKind, or equivalent classes/enums.

A bell, a violin-like timbre, a synthetic pad, a thunderclap, and a voice are authored world structures composed from ordinary Singulars, Properties, Relations, Formations, Laws, and OntoMath.

### 3.2 Audio is a modality boundary

Follow ScreenChannel and InteractionChannel.

ScreenChannel does not define what a cathedral is; it exposes and governs the screen/GPU modality.

InteractionChannel does not define what a button is; it exposes sensed pointer/key facts and edge events while Laws decide meaning.

AudioChannel therefore must not define what a bell or triangle wave means. It exposes and governs the passage between authored acoustic reality and the machine audio substrate.

### 3.3 Separate world meaning from machine mechanism

World meaning includes authored facts such as:

- whether a being emits sound;
- waveform/timbre structure;
- amplitude;
- acoustic location;
- resonance structure;
- envelope;
- modulation;
- material absorption or transmission;
- Relations among sources, listeners, spaces, and acoustic structures.

Machine mechanism includes:

- miniaudio device handles;
- callback state;
- ring buffers;
- scratch buffers;
- DSP kernels;
- resampler internals;
- graph snapshots;
- SIMD details;
- backend synchronization primitives.

The first group must remain law-visible and authored. The second may remain beneath the Kernel but must be named and bounded rather than silently becoming world meaning.

### 3.4 Derived telemetry is readable, not writable

Follow ScreenChannel's distinction:

- values that genuinely govern the modality are writable properties;
- values that report what the machine actually did are read-only ComputedProperties.

### 3.5 The realtime callback is not the Law interpreter

No Rete traversal, arbitrary Law execution, save loading, filesystem I/O, JSON, UI, world graph traversal, or ordinary blocking locks may occur in the realtime callback.

Authored meaning must be lowered before it reaches the hard realtime path.

## 4. Target architecture

The mature pipeline should become:

AUTHORED EARTHCALL REALITY

Object / Person / Formation / Relation  
Property / Law / Field / OntoMath  
authored timbre structures and acoustic Relations

↓ authored meaning

AUDIO CHANNEL

First-Mover modality Law  
governable output policy  
read-only substrate telemetry  
world/substrate boundary

↓ dependency and rate analysis

AUDIO COMPILATION

authored acoustic structure  
-> normalized acoustic representation  
-> Audio IR / DSP graph  
-> optimized realtime snapshot

↓ lock-free or atomic handoff

REALTIME AUDIO EXECUTION

oscillator / sample / filter / delay / resonance / mixer / spatializer / convolution primitives

↓ PCM

AudioSystem / miniaudio backend

↓ physical device

speaker / headphones / Person body boundary

The governing asymmetry is deliberate:

**rich semantic structure above; brutally small execution primitives below.**

## 5. Rung 0 — preservation and evidence gate

Before implementation changes:

1. Read current applicable AGENTS.md and architecture docs.
2. Record current AudioSystem behavior.
3. Run current audio-related focused tests.
4. Record current Synthesis Studio audio behavior.
5. Record the current hardcoded waveform mapping.
6. Record every existing call site of playForm(), renderForm(), PlayAudio, and acoustic.waveType.
7. Distinguish existing failures from regressions.
8. Do not touch authored save files until the migration contract is explicit.

Required focused baseline witnesses include at least:

- ontomath_sounding_test
- infrasound_floor_test
- audio_recorder_test
- Synthesis Studio tests touching PlayAudio / acoustic properties

Acceptance gate:

The current audio contract and failures are written down before any behavior changes.

## 6. Rung 1 — introduce AudioChannel as the First-Mover output boundary

Create:

src/Singularity/Audio/AudioChannel.hpp  
src/Singularity/Audio/AudioChannel.cpp

AudioChannel should inherit Law, identify as audio-channel, report isFirstMover() == true, and use the same syncRegister/find pattern as ScreenChannel and InteractionChannel.

Do not manually call buildProperties() from the constructor.

### Initial writable properties

Keep the first surface intentionally small:

- enabled
- masterGain
- requestedSampleRate
- requestedBufferFrames

Only add outputDevice when the backend can truthfully honor device switching.

### Initial read-only telemetry

Expose machine truth such as:

- actualSampleRate
- actualBufferFrames
- framesRendered
- samplesRendered
- outputPeak
- outputRms
- underruns
- activeVoices
- graphCompiles
- parameterUpdates

Later telemetry can add cache hits/misses, structural invalidations, callback CPU time, DSP node counts, and latency.

Acceptance gate:

- AudioChannel registers idempotently.
- All advertised paths resolve.
- Writable modality controls actually govern output.
- Derived telemetry refuses writes.
- enabled=false produces silence without mutating authored acoustic beings.
- channel_paths_test and no_black_box_test remain green.

## 7. Rung 2 — make AudioSystem the substrate behind the channel, not the ontology

Do not delete AudioSystem.

Reframe it.

AudioChannel is the law-visible modality boundary. AudioSystem is the lower execution/backend mechanism that consumes already-resolved acoustic execution state and talks to miniaudio.

The migration should reduce direct world knowledge inside AudioSystem over time.

Today AudioSystem::tick() traverses the active Zone's Objects and interprets acoustic properties itself. That is an acceptable compatibility bridge, not the final architecture.

The target direction is:

world/Law side gathers and compiles acoustic meaning  
-> AudioChannel owns modality governance  
-> AudioSystem executes prepared audio state.

Acceptance gate:

No new world semantics are added directly to AudioSystem during this migration.

## 8. Rung 3 — preserve and promote the pure headless sounding seam

renderForm() is already one of the strongest pieces of the architecture because it is pure and device-free.

Treat it as the first canonical headless audio witness.

Extend around that principle rather than burying it behind device playback.

Required properties of the headless seam:

- deterministic output for deterministic authored math;
- explicit sample rate;
- explicit frame/sample count;
- block continuity;
- truthful undefined-domain reporting;
- truthful clipping reporting;
- Kernel infrasound refusal preserved;
- no device dependency.

Add a block-oriented render API only if necessary for realtime streaming, but preserve the same mathematical contract.

Acceptance gate:

Rendering two adjacent blocks with preserved execution state must equal the corresponding region of one continuous render, within documented numerical tolerance.

## 9. Rung 4 — retire waveType as the deepest timbre ontology

This is the first major Synthesis Studio migration.

Current state:

studioVoice = "triangle"  
-> authored Law selects "triangle"  
-> PlayAudio carries "triangle"  
-> AudioSystem maps the string to ma_waveform_type_triangle.

Target state:

studioVoice identifies an authored Timbre Formation  
-> the Formation carries or derives authored mathematical acoustic structure  
-> the audio compiler lowers that structure to OntoMath / Audio IR  
-> the modality renders it.

The identity "Crystal" must mean:

**this authored acoustic Formation**

not:

**a magic C++ string called crystal.**

### Compatibility rule

Do not break old saves immediately.

Legacy waveType values should remain readable during migration.

Implement a compatibility lowering where legacy names resolve to seeded/authored canonical timbre structures or to the old primitive path while emitting truthful legacy telemetry.

Do not silently reinterpret an unknown string as sine.

Unknown authored timbre identity must either:

- resolve to authored structure; or
- refuse / report unresolved meaning.

The present "crystal -> sine" fallback must eventually disappear.

Acceptance gate:

- a legacy triangle save still sounds triangle;
- an unknown timbre no longer silently becomes sine;
- the new path can sound a timbre whose mathematical structure is authored and not named in C++.

## 10. Rung 5 — Timbre is Formation

Do not create a Timbre C++ class as the ontology.

"Timbre Formation" here means an ordinary Formation whose members and Relations author acoustic structure.

A first useful authored timbre may contain conceptual roles such as:

- fundamental component;
- harmonic/inharmonic partials;
- frequency ratios;
- amplitudes;
- phase relationships;
- envelope structure;
- modulation;
- filtering;
- excitation;
- resonance;
- spatial behavior.

These roles should be represented through ordinary authored beings, Properties, Relations, and Forms rather than by a closed C++ list of instrument kinds.

The first implementation should be deliberately small.

For example, prove a Formation capable of describing additive synthesis:

partial A: ratio 1, amplitude 0.52  
partial B: ratio 2, amplitude 0.21  
partial C: ratio 3, amplitude 0.11

with authored phase and envelope where already representable.

Compile that Formation into the same OntoMath waveform representation renderForm() can sound.

Acceptance gate:

A newly authored timbre Formation, with an identity never known to C++, produces distinct audible PCM.

## 11. Rung 6 — migrate Synthesis Studio from preset selection to authored timbre selection

The Studio already authors the selection mechanism. Preserve that.

Do not replace its Laws with C++ Studio behavior.

Instead migrate what the selection points at.

Current conceptual path:

studioVoice = "triangle"  
-> state.studio.voice = "triangle"  
-> PlayAudio(..., "triangle").

Target conceptual path:

studioVoice -> Relation to / identifier of authored timbre Formation  
-> state.studio.voice points to that authored identity  
-> note Law supplies pitch/amplitude/expression bindings  
-> audio compiler resolves timbre Formation  
-> OntoMath waveform / Audio IR  
-> PCM.

The Studio should become capable of authoring or editing the Formation itself.

That is the point at which "Synthesis Studio" becomes literal rather than a preset picker.

Preserve the existing three voices as migration witnesses, but re-express them as authored structures.

Acceptance gate:

- triangle, sine, and square still work;
- each is represented by authored acoustic structure rather than a C++ string switch;
- editing the authored structure changes the sound without rebuilding Earthcall;
- adding "crystal" requires no AudioSystem if/else branch.

## 12. Rung 7 — evolve PlayAudio without burning the old serialized contract

ActionNode enums are append-only and serialized as integers.

Do not repurpose the existing PlayAudio enum value incompatibly.

Options, in order of preference:

1. Extend PlayAudio data compatibly so it can reference authored acoustic structure while still reading old frequency/amplitude/waveType payloads.
2. If the serialized shape cannot be extended safely, introduce a new append-only ActionNode kind for sounding an authored Form/Formation.

Do not renumber existing ActionNode kinds.

The new action semantics should sound authored structure; it must not become an audio-specific mini language.

The action should identify/bind the authored acoustic model and let the modality lower it.

Acceptance gate:

- old saves deserialize unchanged;
- old PlayAudio still executes;
- new authored-form sounding serializes and reloads;
- action audit outcomes report refusal/failure truthfully.

## 13. Rung 8 — Audio IR / DSP primitives

Once authored timbre can reach the headless path, introduce a compact Audio IR only where it buys realtime execution and structural optimization.

The IR is execution vocabulary, not ontology.

Minimal initial primitives:

- Constant
- Oscillator
- Gain
- Mixer
- Output

Then add only when required:

- Envelope
- Noise
- SampleReader
- Filter
- Delay
- Resonator
- Spatializer
- Convolver

Do not create semantic nodes named piano, bell, crystal, thunder, cello, or equivalent.

Acceptance gate:

Authored structures can compile into a reusable graph whose nodes are generic signal operators.

## 14. Rung 9 — distinguish parameter changes from structural changes

This distinction is mandatory for performance.

Parameter mutations include:

- gain;
- frequency;
- phase target;
- filter cutoff;
- source position;
- modulation depth.

Structural changes include:

- source creation/removal;
- timbre Formation topology changes;
- DSP operator changes;
- routing changes;
- partial membership changes where the compiled graph topology changes.

Parameter changes should update existing compiled state.

Structural changes may compile a new snapshot.

Expose telemetry proving the distinction:

- graphCompiles
- parameterUpdates
- structuralRecompiles
- graphCacheHits
- graphCacheMisses

Acceptance gate:

Changing frequency or amplitude does NOT increment structural compile count.

Adding/removing a partial or changing graph topology DOES.

## 15. Rung 10 — realtime-safe snapshot handoff

The Law/world thread and realtime callback must not share arbitrary mutable world structures.

Use a prepared immutable-ish execution snapshot with lock-free/atomic handoff or equivalent realtime-safe design.

Conceptual structure:

WORLD / LAW THREAD  
-> resolve authored dependencies  
-> compile/update graph  
-> produce AudioGraphSnapshot

atomic / lock-free handoff

REALTIME THREAD  
-> read snapshot  
-> render PCM  
-> write output

Possible parameter mechanisms:

- atomics for scalar controls;
- double-buffered parameter blocks;
- bounded lock-free queues for discrete changes.

Acceptance gate:

- callback performs no arbitrary allocation;
- callback performs no Universe/Rete traversal;
- callback takes no ordinary blocking mutex;
- graph replacement does not corrupt or interrupt output;
- stress tests prove repeated world-side mutation remains safe.

## 16. Rung 11 — explicit execution rates

Do not evaluate every authored decision at sample rate.

Recognize at least:

- event rate;
- control rate;
- audio rate.

Later, add subsample/analytic treatment only where mathematically justified.

Example:

Law writes acoustic.frequency  
-> parameter target changes  
-> control-rate smoothing  
-> oscillator reads smoothed value per sample.

Acceptance gate:

- normal Law evaluation never occurs 48,000 times per second;
- continuous modulation can remain smooth;
- rate transitions are explicit and testable.

## 17. Rung 12 — Prophetic audio invalidation

Connect audio compilation to Earthcall's existing dependency analysis.

Goal:

world mutation  
-> determine which acoustic consequences MAY be affected  
-> invalidate only that frontier.

Examples:

bell.color changes  
-> zero acoustic invalidation.

bell acoustic amplitude changes  
-> one parameter update.

timbre Formation membership changes  
-> relevant structural rebuild.

The correctness floor must remain conservative: analysis may prove irrelevance, but must not make audio go deaf through an unsound narrow dependency claim.

Expose telemetry such as:

- invalidations
- parameterInvalidations
- structuralInvalidations
- sourcesVisited
- sourcesSkipped

Acceptance gate:

An irrelevant mutation provably causes no acoustic rebuild, while uncertain cases still fall back to a correctness-preserving route.

## 18. Rung 13 — spatial and listener-relative audio

After the semantic/compile path is stable, deepen spatiality.

Initial rung:

- source world position;
- listener world position;
- listener orientation;
- distance attenuation;
- stereo/basic spatialization.

Then later:

- HRTF;
- Doppler;
- source directivity;
- occlusion;
- transmission;
- reflection;
- diffraction approximation.

Do not duplicate world position into an invisible private audio coordinate unless there is an authored reason for a distinct acoustic origin.

Follow InteractionChannel's world-reading precedent for listener-relative derived facts when useful, for example:

- @world.audible
- @world.audibility
- @world.acousticDistance
- @world.listenerRelativeGain

These are candidates, not automatic commitments. Their semantics must be defined before exposure.

## 19. Rung 14 — acoustic fields

Reuse Earthcall's continuous mathematics instead of inventing a separate acoustic mathematics subsystem.

Conceptually, authored acoustic fields may approach:

P(x,t)

and later richer frequency-domain/field representations.

The first useful field implementation can govern:

- gain;
- spectral shaping;
- reverb send;
- spatial modulation.

It need not solve the full acoustic wave equation.

The architectural proof is that sound can be continuous authored world structure, not just point-source metadata.

## 20. Rung 15 — physical/procedural resonance

After the generic path is proven, model physical sound causally.

Example:

collision  
+ impulse  
+ material Relations  
+ contact geometry  
-> authored excitation  
-> authored resonance structure  
-> compiled DSP  
-> output.

Do not hardcode:

metal-metal -> clang.wav.

Recorded samples may participate as authored excitation or residual components, but they are not the ontology.

## 21. Rung 16 — granular and spectral micromastery

Once the system is stable, expose deeper scales without forcing every sound to live there.

Potential authoring scales:

composition  
-> phrase  
-> gesture  
-> transient  
-> grain  
-> spectral partial  
-> waveform cycle  
-> sample.

The goal is reachability, not compulsory complexity.

A Person should be able to say "make this sound warmer" through high-level authored structure, yet still reach individual partials or time/frequency regions when desired.

## 22. Rung 17 — sample-level truth

Every path ultimately resolves to output[channel][sample].

That does not mean high-level Earthcall state should become giant sample arrays.

It means the substrate is mathematically explicit and no hidden timbre preset stands between authored meaning and the final pressure waveform.

Later authoring may target:

- a narrow time window;
- a frequency band;
- one partial;
- phase cancellation;
- microtiming;
- exact transient shaping.

Those operations should still compile downward from authored mathematics.

## 23. Kernel infrasound guard must survive every new path

The existing infrasound floor is a Person-body Kernel guard, not a style setting.

Every new route that can send synthesized PCM toward a Person must be held to the same guard.

Do not add a property or Law that disables it.

Do not silently filter authored mathematics into something else.

Preserve refusal semantics and truthful diagnostics.

Compatibility note: file playback currently sits outside the authored-mathematics analyzer. Do not accidentally claim the guard covers file playback unless that path is explicitly extended and tested.

## 24. Required test ladder

At minimum, add or extend focused witnesses for:

1. audio_channel_test
   - First-Mover identity
   - property reachability
   - read/write distinction

2. audio_headless_render_test
   - deterministic PCM
   - block continuity
   - disabled silence

3. audio_authored_timbre_test
   - authored Formation produces waveform
   - no C++ name knowledge required

4. audio_legacy_wave_type_compat_test
   - old sine/triangle/square/sawtooth saves still work

5. audio_unknown_timbre_refusal_test
   - crystal/unknown identity does not silently become sine

6. synthesis_studio_authored_timbre_test
   - Studio selects authored timbre structure
   - editing structure changes output

7. audio_parameter_vs_structure_test
   - frequency/gain are parameter updates
   - topology changes recompile

8. audio_realtime_snapshot_test
   - safe graph replacement and parameter handoff

9. audio_prophetic_invalidation_test
   - irrelevant mutations produce no invalidation
   - relevant changes route correctly

10. existing guard tests
   - ontomath_sounding_test
   - infrasound_floor_test
   - audio_recorder_test

## 25. Performance doctrine

The final runtime cost should follow audible/relevant causation rather than world size.

Avoid:

O(all beings × all audio blocks)

when only a small changed acoustic frontier matters.

The mature direction is:

O(relevant active DSP work + changed dependency frontier)

with structural compilation amortized and parameter updates cheap.

Audio telemetry should eventually make that visible rather than inferred.

## 26. First mergeable milestone

The first PR should stop before spatial acoustics, HRTFs, giant physical simulation, or a huge DSP library.

A strong first milestone is:

- AudioChannel exists and is registered.
- AudioSystem remains the backend.
- AudioChannel exposes master governance and truthful telemetry.
- renderForm remains the canonical pure authored-waveform renderer.
- one ordinary authored being can reference/contain an authored timbre structure.
- a compiler lowers that timbre structure into an OntoMath waveform or minimal Audio IR.
- a normal Law can alter pitch/amplitude and affect output.
- legacy waveType still works through compatibility.
- unknown timbre names no longer silently collapse to sine.
- Synthesis Studio has at least one voice migrated end-to-end to authored timbre structure.
- tests prove parameter vs structural updates.

That milestone proves the entire architectural direction before expanding the surface area.

## 27. Explicit non-goals for the first milestone

Do NOT spend the first PR building:

- full HRTF;
- full room acoustics;
- diffraction;
- convolution-reverb authoring;
- physical vocal synthesis;
- granular editor UI;
- dozens of DSP effects;
- a giant instrument library;
- a complete acoustic material system.

Those become worthwhile after authored timbre survives the complete world -> Law -> compiler -> headless PCM -> backend path.

## 28. Implementation order

Execute in this order:

1. Baseline current tests and call sites.
2. Add AudioChannel and property/telemetry contract.
3. Put AudioSystem behind that boundary without semantic expansion.
4. Preserve/promote headless renderForm.
5. Define the smallest authored Timbre Formation representation.
6. Compile that representation into OntoMath waveform structure.
7. Extend PlayAudio compatibly or add a new append-only authored-form action.
8. Migrate one Synthesis Studio voice.
9. Migrate sine/triangle/square into authored structures.
10. Remove silent unknown->sine fallback.
11. Add compile-vs-parameter telemetry and tests.
12. Add realtime-safe graph snapshot.
13. Add multi-rate execution.
14. Add Prophetic invalidation.
15. Deepen spatial/listener-relative audio.
16. Add acoustic fields.
17. Add procedural resonance.
18. Add granular/spectral micromastery.

## 29. The architectural completion criterion

Do not declare this work complete because Earthcall can play more sounds.

The completion criterion is:

**A Person can author why a sound sounds the way it does, that authored structure remains visible as Earthcall ontology, Laws can transform it, the modality can compile it efficiently, the substrate exposes what it actually did, and no hidden preset vocabulary is required between authored meaning and PCM.**

Synthesis Studio should cease being "choose one of the sounds programmers supplied" and become:

**author what sound is.**

That is the audio analogue of the movement already underway in Screen: world meaning remains above the modality; the machine merely renders it.
