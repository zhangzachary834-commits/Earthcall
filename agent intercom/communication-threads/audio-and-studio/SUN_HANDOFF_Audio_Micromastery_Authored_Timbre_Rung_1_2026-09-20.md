# SUN HANDOFF — Audio Micromastery / Authored Timbre Rung 1

**Date:** 2026-09-20  
**From:** GPT-5.6 Sol  
**For:** the next Sol / Sun continuing Zach's Earthcall audio work  
**Human architectural direction:** Zach  
**Branch:** `sol/audio-micromastery-authored-timbre-20260919`  
**PR:** #260 — *Audio micromastery: authored timbres through AudioChannel*  
**Base:** `sync-from-earthcall-main`  
**Last code-verified head before this handoff doc:** `93086fced16b3c047a0256a4453c411345094e03`  
**Focused CI at that head:** run #1428 / id 35501148006 — **SUCCESS, all 3 jobs green**  
**PR state at that head:** open, draft, mergeable=true  
**Plan:** `docs/plans/AUDIO_MICROMASTERY_AND_AUTHORED_TIMBRE_IMPLEMENTATION_PLAN_2026-09-19.md`

---

## 1. Do not start over

This branch already contains the first serious vertical slice of Zach's audio-micromastery architecture.

The intended architectural direction is:

> authored meaning / ontology at the top -> mathematical sound structure in the middle -> optimized DSP / PCM at the bottom.

The important constraint is the same one Earthcall uses for geometry/WGSL:

- do **not** make a giant engine-owned timbre enum;
- do **not** interpret arbitrary Law/OntoMath at sample rate forever;
- do **not** let unknown authored meaning silently become a default waveform;
- do **not** report a sound as having happened when the output channel refused it;
- preserve a compatibility floor for historical saves, but only when no authored structure exists.

Rung 1 deliberately proves the semantic vertical contract before building the later Audio IR / realtime compiler / Prophetic invalidation machinery.

---

## 2. What is implemented

### A. First-Mover AudioChannel

New files:

- `src/Singularity/Audio/AudioChannel.hpp`
- `src/Singularity/Audio/AudioChannel.cpp`

`AudioChannel` is a First-Mover Law with stable identity:

- `audio-channel`

It follows the ScreenChannel / InteractionChannel pattern: the channel is the authored-world-to-machine membrane, while `AudioSystem` remains the lower hardware/DSP substrate.

Governable state:

- `masterGain`
- inherited `enabled`

Derived truthful telemetry:

- `actualSampleRate`
- `activeVoices`
- `backendInitialized`
- `authoredFormParses`
- `authoredTimbresSounded`
- `legacyTimbresSounded`
- `unresolvedTimbres`
- `lastTimbreStatus`

The channel is registered in EngineInit, exposed in Law Graph path vocabulary, channel_paths_test, and no_black_box_test.

---

## 3. Authored timbre semantics

A timbre string is now interpreted as an **identity first**, not as an engine enum.

An authored timbre being may carry:

- `acoustic.form` — serialized `OntoMath::Piecewise`
- `acoustic.timeVariable`
- `acoustic.referenceFrequency`
- `acoustic.duration`
- optional other authored acoustic properties later

A sounding being may also carry its own local `acoustic.form`, in which case a separate timbre identity is not required.

The current resolver:

`AudioChannel::resolveAuthoredForm(...)`

searches ordinary Universe beings by stable identifier and resolves the waveform math from authored properties.

### Critical rule

If authored structure exists but is malformed or incomplete, **refuse**.

Do **not** fall through to a legacy preset merely because the token happens to be `"sine"`, `"triangle"`, etc.

A fix on this branch specifically guards this edge case.

Legacy fallback is permitted only when:

1. no authored `acoustic.form` resolved at all; and
2. the requested word is one of the historical compatibility words:
   - empty / sine
   - triangle
   - square
   - sawtooth

Unknown words never silently become sine.

---

## 4. Pitch is retiming of authored mathematics

`Core::Audio::renderForm(...)` and `AudioSystem::playForm(...)` now take `timeScale`.

For an authored normalized waveform with:

- referenceFrequency = 1 Hz
- phase/time variable = `phase`

a Law request at 440 Hz uses:

`timeScale = requestedFrequency / referenceFrequency = 440`

The waveform structure itself is not replaced.

This means the same authored mathematical timbre can be sounded at different pitches without creating engine-owned oscillator kinds.

The infrasound Person-body guard follows the retimed result. A normalized 1 Hz form requested at 7 Hz is refused as 7 Hz; the authored mathematics remains intact.

---

## 5. PlayAudio causal truth was repaired

Before this work, PlayAudio could claim success and publish `audio-synthesized` even if no actual audio path produced sound.

This branch changes the sink contract from a void callback to a checked result:

- real AudioChannel uses `registerAudioSinkChecked(...)`
- older tests/callers can still use the compatibility `registerAudioSink(...)` adapter

In `ActionModel.cpp`:

- PlayAudio calls the sink;
- if sink returns false, the ActionNode trace records failure;
- it does **not** publish `audio-synthesized`;
- only a genuinely accepted sound creates that past-tense event.

New regression coverage proves:

unknown timbre -> AudioChannel refusal -> PlayAudio failed trace -> **zero** `audio-synthesized` events.

Do not weaken this.

---

## 6. AudioSystem changes

`src/Singularity/Audio/AudioSystem.cpp/.hpp` now provide:

- output enabled governance
- master gain governance
- actual sample-rate telemetry
- active voice count
- checked `bool playProceduralCollisionSound(...)`
- authored-form playback with time scaling
- explicit refusal of unknown legacy waveform strings
- refusal rather than clamping for below-floor legacy oscillator frequencies
- explicit standard-library includes for DSP helpers

`setupAudioEventListeners()` is now a compatibility no-op because AudioChannel owns the checked PlayAudio sink.

### Important unrelated fossil discovered

`DefaultPhysicsLaws.cpp` comments claim `@world.occlusionToCamera` is registered by Audio / `setupAudioEventListeners()`.

It was **already false on the base commit before this branch**: base AudioSystem registered only the PlayAudio sink there.

Do not fold that physics/world-reading repair into PR #260 unless Zach explicitly asks. It is pre-existing, unrelated debt.

---

## 7. Synthesis Studio migration

The branch migrates both:

- `saves/worlds/synthesis_studio.{json,ecform}`
- `saves/worlds/synthesis_studio_living.{json,ecform}`

and the source authoring scripts:

- `scripts/upgrade_synthesis_studio.py`
- `scripts/deepen_synthesis_studio.py`

The Studio now contains ordinary authored timbre beings:

- `timbre.studio.triangle`
- `timbre.studio.sine`
- `timbre.studio.square`

Their `acoustic.form` is authored OntoMath/Fourier structure, not an AudioSystem preset.

The Studio's PlayAudio actions target those identities.

The Living Instrument's root notes and generated harmony notes also propagate the same timbre identities through `TIMBRE_IDS[voice]`.

### Generator migration rule

A later fix makes `upgrade_synthesis_studio.py` migrate old state values surgically:

- `"triangle"` -> `"timbre.studio.triangle"`
- `"sine"` -> `"timbre.studio.sine"`
- `"square"` -> `"timbre.studio.square"`

but it preserves an already-custom authored voice identity instead of overwriting it.

Do not revert this to `setdefault("voice", ...)`: old saves may already have `voice="triangle"`, and preserving that stale value would make the migrated triangle Law fail until the TRI button is manually clicked.

---

## 8. Tests and witnesses

New:

- `tests/singularity/audio_channel_test.cpp`

Extended:

- `tests/law/synthesis_studio_app_test.cpp`
- `tests/law/synthesis_studio_living_test.cpp`
- `tests/singularity/channel_paths_test.cpp`
- `tests/singularity/no_black_box_test.cpp`

`audio_channel_test` proves at least:

1. AudioChannel registration / First-Mover identity.
2. governable vs read-only telemetry behavior.
3. timbre identity -> authored `acoustic.form`.
4. normalized waveform -> requested pitch by retiming.
5. retimed infrasound refusal.
6. unknown timbre refusal.
7. refused PlayAudio produces failed node trace and no success event.
8. malformed authored `acoustic.form` cannot fall through to a legacy sine preset.
9. local authored waveform on the sounding being resolves without timbre identity.

CMake recursively registers `tests/*.cpp`, so this test is actually in the build/test graph; it is not a dormant specification file.

---

## 9. CI status

At code head:

`93086fced16b3c047a0256a4453c411345094e03`

Earthcall focused CI run #1428 completed **SUCCESS**.

All jobs passed:

- Focused CPU tests (macOS) — success
- Slow Adapter independent clock (macOS) — success
- SDF range-proxy verification (macOS) — success

The original red CI failure was a real branch compile bug:

`ComputedProperty<AudioChannel, std::string>` requires a getter returning `std::string` by value, but `lastTimbreStatus()` originally returned `const std::string&`.

That was fixed.

A stale Studio test expecting raw `"sine"` after Law serialization was also corrected to expect `"timbre.studio.sine"`.

---

## 10. Base movement / concurrency

While this work was happening, `sync-from-earthcall-main` advanced by one commit to:

`bb53b122ae112f2a68eaef96278e533fa7cc3cfe`

That base commit touches only Cathedral screenshots/save/generator work.

It does **not** overlap the audio/Law/test files in PR #260.

The PR remained `mergeable=true`.

Do not merge/rebase merely for cosmetic freshness if the base remains disjoint; unnecessary head changes keep cancelling expensive macOS CI runs.

Always re-check current base before writing because Earthcall has many concurrent agents.

---

## 11. One observed save divergence — do not "fix" blindly

The branch's base Studio `.json` and `.ecform` are **not globally structurally identical**, although their audio migration is aligned.

Both contain:

- the same 3 timbre beings;
- the same authored timbre-targeting PlayAudio identities.

The remaining divergence is around `law-art-stroke-draw` and lastStroke state:

- one representation contains the later draw-spacing/state-update rewrite;
- the other keeps an older Create-tree shape.

This appears outside the audio migration.

The next Sun should first compare against the branch point/base and determine whether this divergence was pre-existing and intentionally preserved before making any save synchronization change.

Do not casually overwrite one authored save with the other.

---

## 12. Human witness still required

After automated verification, Zach should boot the Living Studio and compare TRI / SINE / SQR.

The meaningful witness is not just "three sounds exist."

Verify:

- voice switching is immediate;
- one control action causes the intended number of notes, without duplication;
- harmony still works;
- no click/input lockout;
- triangle, sine, square are audibly distinct;
- no unknown timbre silently becomes sine;
- notes remain bounded by current output safety rules;
- Studio visuals/resonators still respond correctly.

This is recorded in the Person Verification List / audio task doc.

---

## 13. What Rung 1 does NOT claim

Do not describe this as full audio micromastery.

Still future work:

- Audio IR
- immutable realtime DSP graph snapshots
- parameter-vs-structural compiler distinction
- sample-accurate event scheduler
- control-rate vs audio-rate lowering
- graph constant folding / CSE / fusion
- lock-free realtime parameter/event streams
- Prophetic audio dependency invalidation
- compiled cached OntoMath DSP
- granular synthesis
- partial/spectral formations
- physical/modal synthesis
- HRTF / binaural spatialization
- room propagation / reflection / diffraction
- acoustic fields
- GPU/DSP compilation experiments
- fourth-dimensional temporal audio ontology

The first rung intentionally establishes the truthful vertical semantics first.

---

## 14. Recommended next action

1. Read this handoff and the implementation plan.
2. Inspect current PR #260 head/base/CI because this doc commit itself may trigger a new workflow run.
3. Do **not** add more feature scope until the current branch is verified after the handoff commit.
4. If CI remains green, consider marking PR #260 Ready for Review.
5. Zach should perform the Living Studio Person witness.
6. Only after Rung 1 is accepted should the next implementation rung begin.

The natural next major architecture rung is **Audio IR + cached compilation**, not more hardcoded oscillator vocabulary.

That next rung should preserve the same analogy as Earthcall's geometry stack:

> authored semantics -> lowered execution graph -> optimized primitive kernels.

And it must preserve the realtime boundary:

> no arbitrary Law interpretation, allocation, blocking, mutable world traversal, or JSON parsing in the realtime audio callback.

---

## 15. Core architectural sentence

**Meaning at the top. Mathematics in the middle. Samples at the bottom.**

The engine may optimize the route ruthlessly, but it must not invent a different sound than the authored world meant.
