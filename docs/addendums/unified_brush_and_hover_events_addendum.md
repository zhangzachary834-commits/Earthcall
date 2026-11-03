# Integrating the Unified Brush System and Object Hover Events

*Initial synthesis: Jules. Reviewed against the Earthcall implementation.*

## Two existing paths, not one automatic integration

The [Unified Brush System](../tools/UNIFIED_BRUSH_SYSTEM.md) describes layered 2D/3D authoring. In code, [`BrushSystem`](../../src/Singularity/Screen/BrushSystem.cpp) owns brush presets, stroke sampling, per-layer RGBA pixel data, and CPU compositing. That is an authoring/painting pathway.

The [Object Hover Events System](../architecture/events/OBJECT_HOVER_EVENTS_SYSTEM.md) describes spatial observation. [`InteractionChannel::observe`](../../src/Singularity/Input/Interaction/InteractionChannel.cpp) resolves the currently hovered Object, updates its hover state and events, and exposes the read-only `@world.pointerOver` reading for Laws. It does **not** currently feed hovered-object metadata into `BrushSystem` or automatically change brush settings.

## A possible Law-mediated bridge

The two systems could be composed deliberately: a Law or tool contract could read the pointer-over observation, validate that the selected target permits painting, then choose a brush preset or preview before a stroke is committed. This would require an explicit input-to-authoring bridge, clear target selection and permission rules, and evidence that the same hit/coordinates are used by the preview and stroke.

This is a **proposed interaction**, not an assertion of implemented pressure adaptation, automatic OntoMath material rewriting, or hover-driven stroke prediction. The hover path tells the engine *which Object* is being indicated; the brush path determines *how a stroke is applied*. Keeping that boundary visible supports the No Black Box principle.

## Verification needed for integration

An end-to-end witness should move the pointer across two distinct Objects, verify enter/exit and `@world.pointerOver` observations, confirm that a brush preview changes only when the relevant Law authorizes it, and verify that a committed stroke affects only the selected target. Neither the current hover documentation nor the existing brush code alone proves that bridge.

**Source anchors:** [BrushSystem implementation](../../src/Singularity/Screen/BrushSystem.cpp), [InteractionChannel implementation](../../src/Singularity/Input/Interaction/InteractionChannel.cpp), [hover event design](../architecture/events/OBJECT_HOVER_EVENTS_SYSTEM.md).
