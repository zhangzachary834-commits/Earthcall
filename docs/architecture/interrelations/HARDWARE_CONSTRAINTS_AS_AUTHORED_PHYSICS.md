# Hardware Constraints as Authored Physics

**How exposing hardware limits to the No Black Box introspection fundamentally reframes level-of-detail and performance constraints as governable laws of physics.**

**Status:** Conceptual interrelation.
**Connected Documents:**
*   `../ontology/NO_BLACK_BOX.md` (The refusal of unregistered state; everything must be governable)
*   `../mathematics/FAR_LANDS_FRAMEWORK.md` & `../ontology/CPU_GPU_MICRO_MASTERY.md` (The `ScreenChannel` and how rendering bounds are handled)

---

## The Interrelation

In conventional game engine architecture, hardware constraints—like VRAM exhaustion or a high triangle count—are managed by hidden, hardcoded engine subsystems like Level of Detail (LOD) managers or culling algorithms. These systems operate as black boxes beneath the simulated world, abruptly popping low-resolution models into existence when the system is strained, severing the connection between the ontology of the world and its visual representation.

Earthcall breaks this dichotomy by utilizing the "No Black Box" principle. The GPU telemetry data tracked by the `ScreenChannel`—such as `@screen-channel.trianglesDrawn` or `@screen-channel.vramAllocatedBytes`—is explicitly registered as a `PropertyPath` in the global ontology.

Because of this, hardware telemetry is no longer a hidden technical artifact; it becomes a **physical property of the environment**.

When a Person uses the `FAR_LANDS_FRAMEWORK.md` to author infinitely layered mathematical terrain, the recursion depth is not cut off by an invisible, hardcoded engine fail-safe. Instead, a Person authors a Law:

```
when: @screen-channel.vramAllocatedBytes > 2_000_000_000
do:   set @far-field.recursionDepth = @far-field.recursionDepth - 1
```

This interrelation ensures that the engine never lies. The terrain simplifies because the *Law of the world* dictated that it should, based on the *physical strain* on the reality vessel (the GPU).

**Conclusion:** By treating the hardware state (like VRAM and triangle counts) as governable `PropertyPath`s under the "No Black Box" doctrine, Earthcall transforms performance engineering into ontological physics. Level-of-Detail is no longer an engine trick; it is an authored response to environmental limits.
