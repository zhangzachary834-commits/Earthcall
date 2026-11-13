# Physics Collision Events and Relation Recording

Physics emits observations; an explicitly installed listener translates eligible observations into a **physics-owned relation registry**. This is not automatic authorship of a Person's Formation relations.

## Source-checked path

- [`EngineInit.cpp`](../../../src/Singularity/Core/EngineInit.cpp) invokes `Physics::setupPhysicsEventListeners()` during engine initialization.
- [`Physics.cpp`](../../../src/ZonesOfEarth/Physics/Physics.cpp) subscribes to `PhysicsCollisionEvent` on the `Core::EventBus`. When both object pointers are present, its handler calls `recordCollision`, which adds a `Relation("collision", ...)` to the physics registry and publishes a `Core::Event::Custom` carrying that relation.
- Physics collision observations may be published on **each frame of sustained overlap**; the distinct `contact-began` and `contact-ended` ECA events are edge signals. Recording a collision is not the same as inferring an enduring Person-authored relation.
- [`BootedEngineHarness`](../../../tests/support/test_harness.hpp) does not itself install the default physics listener. A test that needs the production path must establish listener registration and lifecycle deliberately; adding a global subscription in a shared harness without teardown can duplicate listeners across tests.

## Engineering consequence

Verify separately that the collision event was published, the listener was installed, the physics registry recorded the expected endpoints, and no duplicate global subscriptions contaminated the witness. Do not infer Formation Rete relevance or authorial authority solely from the physics registry entry.