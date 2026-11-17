# Ambient Zone Spatial Audio

**Status:** ✅ Completed and Verified (2026-10-07)  
**Stakeholder:** Zach (Verified native execution)

## Objective
Author a Zone with a soft, pleasant, non-buzzing environmental sound that spatially pans left to right, using *only* Earthcall's existing procedural audio and Law architectures—no new C++ domain classes for sounds.

## Implementation Details

### 1. The Sound Emitter
Three spherical Objects (Root, Fifth, Ninth) were seeded into `saves/zones/Ambient Zone/zone.json`.
Instead of hardcoding a sound file or creating an `AudioEmitter` class (adhering to Refusal 1: No new C++ class for a domain noun), the sound is generated purely via authored `acoustic.*` properties:
- `acoustic.isSoundEmitter = true` (Triggers `AudioSystem` instance creation)
- `acoustic.waveType = "sine"` (Smooth tone, unlike harsh saw/square waves)
- Frequencies for a lush major ninth chord (A2: 110Hz, E3: 164.81Hz, B3: 246.94Hz)
- `acoustic.lowpassCutoff = 300.0` (Heavily muffles the sound, simulating a soft environmental wind/drone rather than a raw synthesizer)
- `acoustic.amplitude = 0.6` (Pleasant volume level)

### 2. The Spatial Panning Law
Earthcall's `AudioSystem` natively uses `miniaudio` spatialization linked to macOS CoreAudio. This means stereo panning naturally occurs when an Object moves relative to the camera in 3D space.

Rather than building a fake "panning" property, three mathematical Laws were authored (`law-ambient-pan-root`, `-fifth`, `-ninth`):
- **Condition**: Targets any object where `acoustic.isSoundEmitter == true`.
- **Action**: Binds the object's `position.x` property to a Sine wave expression evaluated over time (`5.0 * sin(0.5 * time)`).

### 3. Result
When the Zone is loaded, the Law infinitely oscillating the orb's physical X-position causes CoreAudio to spatially pan the 140Hz lowpass-filtered drone from the left speaker/headphone to the right, and back again, continuously. Zach verified this directly on native Apple Headphones.

2026-11-16 revisit: Zach reported **Move to Zone** refused entry. The three shared Law roots omitted the activation field, decoding as `OnEvent` with no trigger. Their saved activation is now explicitly `WhileTrue`; the existing formulas and recorded `ambient.creator` authorship were preserved in the checkout and the live data home. Each voice moved in the isolated Zone run, and the saved round trip reopened. The headphone check for the rebuilt app remains with Zach. [Repair record](../../Zones%20and%20Ourverse/Zone_Switch_Repair/Zone_Switch_Repair.md). Codex / GPT-6 / session `01a1e641-c9be-7892-8938-ce93617a2341` / 2026-11-16.
