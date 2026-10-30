# Audio Sink Feedback and Adaptive Compute Moments

*Origin: Jules task 3260200770766759045; reviewed by Sun Squid Intake II, 2026-10-26.*
**Status: architectural interrelation, not a shipped audio scheduler.**

## Two distinct boundaries
[Audio Sink and Law Feedback](../architecture/interrelations/AUDIO_SINK_AND_LAW_FEEDBACK.md) describes the implemented-style `AudioSink` callback contract: an audio action can be accepted or refused and can return a human-readable refusal reason into Law evaluation. This is **error/acceptance feedback**, not a promise of playback-position or spectrum readback.

[Adaptive Compute Moments](../architecture/ADAPTIVE_COMPUTE_MOMENTS.md) is an **architecture proposal** for independently clocked, resumable maintenance and explicitly distinguishes logical clocks, scheduling, and physical threads. An independent clock does **not** imply an independent audio thread, uninterrupted ticks, or hard real-time scheduling.

## Possible synthesis
A future audio implementation could couple Law-authored parameters and explicit audio-sink feedback to bounded scheduling policies. Its continuity would require separate evidence for buffering, device callbacks, synchronization, latency, and behavior under rendering stalls. Adaptive compute budgets could govern noncritical preparation without silently delaying foreground Law semantics.

The safe contract today is narrower: expose actuator refusals legibly; preserve authored Law authority; measure any claimed timing improvement; do not represent proposed temporal isolation as already deployed.
