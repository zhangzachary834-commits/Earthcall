# Sun Zone Pass #031 — Solar Reliquary (parallel sculptural study)

Status: **design-only checkpoint; no geometry has been authored or rendered**.
Base: `18456320d10f7bddfa0bc5074dd665f1092ec7fe` (`sync-from-earthcall-main`).
Target: `sol/sun-zone-parallel-solar-reliquary-031-20261015`.

## Intent
Build a spatially independent Solar Reliquary **inside the existing Sun Zone** while preserving the post-#019 temple composition and leaving the unresolved #030 gold/ivory/moonlit render witness isolated.

## Sculptural language
- **One continuous solar portal**: compound arched opening with nested archivolt grooves carved by SDF subtraction; the arch's profile and recesses must read as continuous stone rather than torus/box assembly.
- **Two integrated pilasters**: tapered, fluted shafts transitioning into carved capitals through continuous compound SDF profiles; no decorative stacks of cylinders.
- **Deep solar relief**: recessed radial sunburst and negative-space carving, with an intelligible silhouette under directional lighting.
- **Sculpted cornice**: a single profiled entablature that flows across the supports with undercut shadow lines, not several stacked slabs.
- **Solar-flame crown**: asymmetric, swept flame lobes with negative-space channels; not cones/spheres.
- **Material discipline**: existing authored ivory/gold material response only; no new emissive or radiance witness.

## Spatial separation and noninterference
1. Read the full **current live** Sun Zone state, not just the frozen repository snapshot, and establish an occupied-bounds inventory (including existing SDF support extents).
2. Select a demonstrably unoccupied location; record its coordinates and minimum separation from all existing architectural bounds before authoring.
3. Append uniquely identified architectural objects; never renumber, rewrite, or transform existing objects.
4. Keep the existing camera, initial spawn, temple, citadel, and oculus unchanged. Provide an optional, separately invoked inspection viewpoint.
5. Preserve all existing `spatialFields` and authored radiance witness bytes/semantics.

## Required mechanical checks before any geometry commit
- Verify latest canonical HEAD and branch merge-base; reconcile canonical forward without merging this branch into canonical.
- Verify absent or non-shadowing `saves/zones/Sun/zone.ecform` and inspect the application's current data-home save.
- Compare all original object IDs, transforms, `targetRotation`, `authoritativeAxis`, and radiance witnesses exactly before/after.
- Validate compound SDF operators, curved primitive axes, material bindings, serialization, and nonoverlapping bounding volumes with existing Earthcall tooling.
- Limit diff to the new Reliquary authoring and its dedicated proof; no unrelated renderer, sky, mist, clouds, aurora, or volumetric work.
- Run available focused validation and capture exact commit/PR before asking for human render.

## Human render gate
Inspect the Reliquary independently from a frontal and three-quarter view. Confirm that arch/capital/cornice/flame read as sculpted **compound masses**, carved relief casts legible shadows, and the original temple is visually and structurally unchanged.

## Current blocker
The GitHub connector returned Sun `zone.json` blob SHA `e2d3291af549f1388e165feb86d678f25332e2c3` but no content for a targeted file read. Consequently, no safe spatial placement or schema-correct geometry edit can yet be made. **Do not treat this document as a built installation.**
