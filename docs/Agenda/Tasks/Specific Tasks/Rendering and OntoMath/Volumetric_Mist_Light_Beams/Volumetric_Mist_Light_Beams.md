# Volumetric Mist Light Beams

**Opened by:** Antigravity / Gemini · session `46a7b4aa-6373-429b-af0e-3377758af9ff` · 2026-09-24 11:45 PDT.

## Ontological Grounding & Minimum-Maximum Formulation

Volumetric light beams ("god rays" or crepuscular shafts) are not an image-space post-processing trick or ad-hoc engine visual hack. In Earthcall's physics and ontology, light beams shining through mist are the physical, mathematical consequence of four foundational truths uniting in continuous transport:

$$\text{LIGHT} + \text{MEDIUM} + \text{SPACE} + \text{VIEW} \longrightarrow \text{visible volumetric radiance}$$

1. **LIGHT ($\mathcal{L}$)**: A celestial radiance source ($\rho(p), \chi(p), \alpha(p, \omega)$) radiating energy into the zone from a distinct spatial origin.
2. **MEDIUM ($\mathcal{M}$)**: A participating medium governed by OntoMath scalar and vector fields:
   - Density $D(p, t)$ and Extinction $\sigma_t(p, t)$ attenuating light exponentially ($e^{-\int \sigma_t ds}$) along view and incident light paths.
   - Scattering $\sigma_s(p, t)$ determining the fraction of incident light redirected toward the eye.
   - Chroma $C_v(p, t)$ defining the intrinsic particulate tint.
   - Phase $\Phi(p, \omega_i, \omega_o)$ governing angular directional scattering when explicitly authored. The Sanctuary currently uses V3's exact isotropic compatibility phase $\Phi=1$; a forward-scattering phase must be authored rather than hidden in renderer fallback state.
3. **SPACE ($\mathcal{S}$)**: Signed Distance Field (SDF) geometry defining physical architecture, apertures, and occluders. Where solid matter blocks the path to the light source, the visibility function $V(p) = 0$ (shadow). Where open apertures allow rays to pass unobstructed, $V(p) = 1$ (illuminated volumetric beam).
4. **VIEW ($\mathcal{V}$)**: The Person's eye ray integrating in-scattered radiance along the ray span through the volume.

## Architectural Implementation (The Seven Refusals Compliant)

1. **Refusal #1 (No new domain C++ classes)**: No `GodRayEffect`, `SunbeamVolume`, or `FogShaft` classes were introduced. The phenomenon is entirely composed of existing universal ontological primitives: `FieldNode` (continuous mathematical field) and `SdfNode` (geometric distance field).
2. **Refusal #6 (No black box)**: The occluder geometry on `FieldNode` is exposed directly through the property path `"volume.occluder.sdf"`, readable and writable by Law.
3. **Refusal #7 (No hardcoded variable behavior)**: The shapes of the beams, the falloff, the aperture cutouts, and the scattering properties are authored in data, not hardcoded into engine methods.
4. **Bounded Sphere-Tracing for Visibility**: Rather than an expensive nested raymarch ($O(N^2)$), the visibility function $V(p)$ employs bounded sphere-tracing over the authored occluder SDF ($O(1)$ empty-space leaps, max 24 steps) with soft penumbra estimation:
   $$V(p) = \min\left(1.0, \frac{k_{\text{penumbra}} \cdot d(t)}{t}\right)$$
   This produced smooth penumbra transitions in the focused image witness. The original 60+ FPS assertion has no saved-scene frame-time witness; see the dated correction below.

## Performance correction — 2026-09-25

Zach reported that Sanctuary of Sunlit Mist becomes unresponsive sooner during use. Codex / GPT-6 measured the unchanged authored mist in a native, volume-only saved-scene probe: current 1280×720 synchronized wall time was approximately 85–87 ms/frame in one run, with substantial run-to-run load variation. The original `1d84821f` implementation was also costly. A no-occluder diagnostic was far faster but changed the image and is not an acceptable replacement. The initial 60+ FPS statement above is withdrawn as a performance claim; the focused 32×16 contrast test verifies appearance, not interactive frame rate. The saved-scene A/B, caveats, and next gate are in [the audit](../../../../../audits/rendering_optimization/2026-09-25_sunlit_mist_saved_scene_ab.md).

**Correction signed:** Codex / GPT-6 · session `01a0cfbf-c751-7af0-b160-df07da055bc0` · 2026-09-25 12:56 PDT. Antigravity's original authorship and its correctness witness remain attributed above.

## Constitutional correction after cross-rung review

The initial renderer patch temporarily supplied a hardcoded Henyey-Greenstein fallback ($g=0.55$) whenever no authored phase existed. That crossed the established Volumetric V3 contract, where absent `volumePhase` means exact isotropic identity $\Phi=1$. The reconciliation removes that renderer-owned fallback. The light-shaft phenomenon remains the composition of authored source illumination, participating-medium scattering, and local occluder visibility; forward-scattering is available when the medium explicitly authors `volumePhase`.

`volume.occluder.sdf` is also scoped as local volumetric transport geometry, not a replacement for Rung 8's eventual scene-wide visibility authority.

## Verification & Tribunal Tests

1. **`tests/singularity/webgpu_volumetric_mist_test.cpp`**:
   - Native WebGPU offscreen test validating mist illuminated by an incident light source behind an occluding aperture.
   - **Result**: **Passed**. Measured spatial contrast ratio between beam core and shadowed region = **765.00x** (Shadow Lum = 0, Beam Lum = 765, sharp penumbra transition at the aperture boundary).
2. **`tests/zones/sanctuary_of_sunlit_mist_test.cpp`**:
   - Validates end-to-end loading, deserialization, and projection of `saves/worlds/sanctuary_of_sunlit_mist.json` and `saves/zones/Sanctuary of Sunlit Mist/zone.json`.
   - Confirms spatial root light properties, volume mist ASTs, active CSG occluder SDF, and all 12 architectural sanctuary objects.
   - **Result**: **Passed** (21/21 checks green).

## Authored Demonstration Scene

- **Zone**: `saves/zones/Sanctuary of Sunlit Mist/zone.json`
- **World**: `saves/worlds/sanctuary_of_sunlit_mist.json`
- **Authors**: Zachary Zhang & Antigravity (First Mover)
- **Features**:
  - Golden celestial sun (`[1.0, 0.88, 0.65]`, diffuse 3.8, intensity 3.0) positioned at high elevation.
  - Suspended participating mist volume ($\sigma_t = 0.28$, $\sigma_s = 0.25$, albedo $\approx 0.89$, chroma `[0.95, 0.97, 1.0]`).
  - Authored SDF clerestory aperture wall (CSG Subtraction) cutting three distinct window apertures through the lintel wall, casting three luminous shafts across the hall.
  - Colonnade of stone pillars, altar dais where the central light shaft strikes, polished flagstone floor, and physics stele explaining the participating medium transport.

**Signed:** Antigravity / Gemini · session `46a7b4aa-6373-429b-af0e-3377758af9ff` · 2026-09-24 11:45 PDT
