# Kernel Guards and Prophetic Rete

**Status:** Source-aligned architectural distinction; not a claim that Prophetic Rete authorizes or vetoes physical outputs.

**Read with:** [Prophetic Rete](../law/PROPHETIC_RETE.md), [First Mover vs. Kernel Guards](FIRST_MOVER_VS_KERNEL_GUARDS.md), `src/ZonesOfEarth/AuthorsOfLaw/PropheticRete.cpp`, `src/Singularity/Audio/AudioSystem.hpp`, and `tests/infrasound_floor_test.cpp`.

## Different questions, different boundaries

Prophetic Rete is an ahead-of-time **over-approximation** of what authored Law conditions and effects could read or write. Its essential soundness rule is *widen, never narrow*: unknown or opaque effects must fail open rather than silently suppress a Law. It is a relevance and possible-effect analysis, **not** a universal Law approval system, a Hierarchy-of-Joys tribunal, or a physical-safety certificate.

Kernel guards govern particular paths from authored computation to physical modalities. For example, the Audio subsystem defines an audible-frequency floor and an infrasonic-energy ceiling, with refusal reporting at the sound-producing boundary. This protection is separate from the validity of the mathematical expression being authored or evaluated.

## Example: a low-frequency waveform

A Law or authored OntoMath expression can represent a 7 Hz waveform without the Prophetic Rete proving that the waveform is safe to play. If that waveform reaches the guarded Audio sounding path with prohibited sub-floor energy, the channel can refuse the physical sounding. The guard does not erase or rewrite the authored expression.

The result depends on the **specific channel path and its actual guard implementation**. Do not generalize the Audio witness into a claim that every modality is covered or that every refusal is logged in an identical way.

## Integration invariant

Law relevance analysis must not mistake a physical-channel refusal for an impossible authored Law and filter that Law out of the simulation. Conversely, an allowed Law execution does not grant permission to bypass the Person-body boundary. Test both sides separately: Prophetic Rete fail-open witnesses and the channel's real refusal/measurement witnesses.

**Conclusion:** Predicting which Laws *may matter* and refusing unsafe *physical actuation* are complementary duties, not two competing supreme judges.
