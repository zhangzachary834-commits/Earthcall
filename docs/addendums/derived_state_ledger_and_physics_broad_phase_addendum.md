# Broad-Phase Scratch Data and Derived-State Discipline

**Status:** Source-checked conceptual analogy; no new ledger registration or enforcement.
**Provenance:** Jules candidate, corrected by Sun Squid Intake II against canonical source on 2026-10-24.

The [Derived-State Ledger](../architecture/law/DERIVED_STATE_LEDGER.md) documents dependencies, invalidation and guard witnesses for derived structures in the Law engine. It is a documented engineering discipline, **not** an automatic runtime tracker of all engine caches.

Inside `Physics::updateBodies`, the collision broad phase updates each object's collision zone, builds a **local** `std::vector<ObjectPrep>` containing object indices, minimum/maximum AABB corners and a `bool allowedByLaw`, then sorts by `minAABB.x`. The sorted sweep skips impossible X-axis pairs before checking Y/Z overlap and collision eligibility. These entries are transient computations, **not** persisted collision masks or durable ledger records.

The local vector is rebuilt on each invocation and ceases to exist when that invocation returns. This limits the lifetime of those particular precomputed bounds; it does **not** prove that every other physics or contact-state cache is current, nor that the Derived-State Ledger governs this vector. A future persistent broad-phase index would need explicit dependencies, invalidation, fallback and regression witnesses before claiming the same safety.

**Sources:** [Physics implementation](../../src/ZonesOfEarth/Physics/Physics.cpp), [Derived-State Ledger](../architecture/law/DERIVED_STATE_LEDGER.md), [physics architecture](../architecture/events/PHYSICS_AND_COLLISION.md).
