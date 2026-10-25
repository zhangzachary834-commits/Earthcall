# Fixed-Step Physics and First-Mover Law Boundaries

**Status:** Source-checked architectural interrelation; synchronization beyond the described paths is unproven.
**Provenance:** Jules candidate, corrected by Sun Squid Intake II against canonical source on 2026-10-24.

`Zone::update` runs bounded fixed-duration physical substeps (60 Hz nominal, at most three per update) and may drop excess accumulated time. It calls legacy `Physics::updateBodies` only when legacy physics is enabled and active. This is a local integration contract, **not** a global rule that all Laws execute on those same substeps.

`Physics::createDefaultPhysicsLaws()` constructs engine-seeded `FirstMoverLaw` instances for gravity, collision and kinematics. Its own source explicitly distinguishes these bootstrap Laws from **Person-authored text**. Their representation as Laws makes mechanics legible and subject to Law semantics; it does not make every default mechanic Person-authored or establish identical scheduling.

`Engine::update` calls the active Zone's `update(dt)` and then advances the world Timeline by the frame's `dt`. That ordering alone does not establish exact synchronization between the fixed-step accumulator, world Timeline, EventBus dispatch and authored Law evaluation. Contact events may connect physics to Law reactions, but claims of deterministic, frame-rate-independent Law execution require a specific scheduler trace and runnable test, especially under capped/dropped substeps.

**Sources:** [Zone update](../../src/ZonesOfEarth/Zone/Zone.cpp), [default physics First Movers](../../src/ZonesOfEarth/Physics/DefaultPhysicsLaws.cpp), [engine update](../../src/Singularity/Core/EngineUpdate.cpp), [Prophetic Rete](../architecture/law/PROPHETIC_RETE.md).
