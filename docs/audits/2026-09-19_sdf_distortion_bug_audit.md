# SDF Distortion and Shading Motion Bug Audit — 2026-09-19

**Author:** Gemini Spark  
**Date:** 2026-09-19  
**Requested by:** Zach  
**Context:** Investigation into the "SDF distortion bug" (Perlin noise floor and shader distortion when moving, and its relationship to the Far Lands Zone)  
**Status:** Audit and architectural specification; ready for implementation

---

## 1. Executive Summary

During locomotion in Earthcall—most prominently across the Perlin Noise Floor Zone (`saves/zones/NoiseFloorWorld/zone.json`) and scenes utilizing implicit field raymarching—a distinct visual distortion occurs:
1. **Terrain Geometry Distortion:** The terrain does not remain a rigid, static Euclidean ground. Instead, as the camera translates and rotates, the perceived hill shapes "breathe," bend, and stretch non-linearly across the screen.
2. **Shading / Specular Distortion:** Shader lighting does not change naturally from the observer's perspective; instead, diffuse gradients and specular highlights slide, rotate, and distort across surfaces during locomotion.
3. **The Far Lands Phenomenon:** As Zach noted in `docs/Agenda/Tasks/To-do list.md`, this non-Euclidean spatial bending creates a striking visual aesthetic reminiscent of cosmic reality breakdown when moving through the Far Lands Zone, prompting the directive to elevate this effect from an accidental defect into a deliberate, authorable Law/property.

Our investigation traced this behavior through the WebGPU rendering pipeline, camera projection math, shader WGSL compilation, and the lighting subsystem. We identified **four interconnected root causes**:
- **Proxy Geometry Clipping & Attribute Interpolation Warping:** The SDF raymarcher derives primary ray directions from vertex attributes interpolated across a rasterized bounding-cube proxy (`rdWorld = normalize(in.worldPos - roWorld)`). When the camera is inside the bounding box or spans the near/far planes, homogeneous clip-space clamping (`o.clip.z = min(o.clip.z, o.clip.w * 0.999999)`) severely distorts attribute interpolation, bending ray directions non-linearly across pixels as the camera moves.
- **Bounding Box (1000m) vs. Far-Plane (100m) Disparity:** The noise floor's extent is `[1000, 30, 1000]`, while `EngineRender.cpp` sets `farZ = 100.0f`. Clamping vertices 10× past the far plane forces non-affine barycentric warping across rasterized proxy triangles.
- **Camera-Relative "Headlamp" Lighting:** `ShadingSystem::update()` and `EngineRender.cpp` default to positioning the primary scene light relative to the camera (`_camera->pos + offset`). As the camera moves, the world-space light vector rotates relative to static surface normals, causing shading to "swim" across the world.
- **Heightfield Invalidation & Root Leaping:** The authored noise floor expression `y - 40 * noise(0.008 * (p + vec3(100, 0, 100)))` depends on `p.y`, violating 2.5D heightfield assumptions (`df/dy = 1`). Planar leaping and DDA skips based on this false proof can bypass roots, introducing perspective-dependent surface popping.

---

## 2. Lived Experience and Authorial Context

In `docs/Agenda/Tasks/To-do list.md` (lines 52–54), Zach documented the dual nature of this behavior:

> *"There is a rendering issue--the perlin noise floor when I move distort rather than appearing as the same shape. Same thing with shaders, moving around appears to distort shading rather than making them change naturally from the perspective. Patch this - Zach.*  
> *WAIT **Part 2** BROOOOOO THE DISTORTION EFFECT LOOKS SO COOL WHEN I MOVE THROUGH THE FAR LANDS ZONE. NEED TO HAVE THIS AT THE RIGHT MOMENT. PLZ MAKE THIS AN AUTHORABLE PROEPRTY INSTAEDDDDDDDDDD - Zach"*

Zach's observation captures two critical requirements:
1. **The Substrate Must Be Truthful by Default:** Ground should remain physically rigid, stable, and Euclidean under ordinary locomotion.
2. **The Aesthetic Must Be Preserved as Authorable OntoMath:** The non-Euclidean bending effect should not be erased from existence, but rather liberated from being a buggy byproduct of hardware clip planes and transformed into an authored property under `@screen-channel` or Zone Law.

---

## 3. Detailed Technical Root Cause Analysis

### Root Cause 1: Proxy-Derived Ray Generation in `SdfWgsl.cpp`

In standard screen-space raymarching (e.g., full-screen quad or camera unprojection), primary ray directions are derived directly from Normalized Device Coordinates (NDC) via the inverse view-projection matrix:
$$\mathbf{P}_{\text{world}} = (\mathbf{V} \cdot \mathbf{P})^{-1} \cdot \begin{pmatrix} x_{\text{ndc}} \\ y_{\text{ndc}} \\ 1 \\ 1 \end{pmatrix}$$
$$\mathbf{r}_d = \text{normalize}\left(\frac{\mathbf{P}_{\text{world}}}{P_{\text{world}, w}} - \mathbf{E}\right)$$
This formula is strictly linear in screen space and guarantees perspective coherence regardless of camera placement.

However, Earthcall's WebGPU implementation rasterizes an analytic bounding box proxy (`_sdfCubeVerts` in `WebGpuRenderer.cpp:1065-1081`). The vertex shader in `SdfWgsl.cpp` executes:
```wgsl
@vertex
fn vs(@location(0) pos: vec3<f32>, @builtin(instance_index) instIdx: u32) -> VSOut {
    var o: VSOut;
    let inst = instances[instIdx];
    let world = inst.model * vec4<f32>(pos * inst.extents.xyz, 1.0);
    o.clip = u.viewProj * world;
    // Clamp to far plane so proxy geometry is never lost to far-plane clipping.
    o.clip.z = min(o.clip.z, o.clip.w * 0.999999);
    o.worldPos = world.xyz;
    o.instIdx = instIdx;
    return o;
}
```
And in the fragment shader (`SdfWgsl.cpp:1065-1069`):
```wgsl
let roWorld = u.eyePos.xyz;
let rdWorld = normalize(in.worldPos - roWorld);
let ro      = (inst.invModel * vec4<f32>(roWorld, 1.0)).xyz;
let rdField = (inst.invModel * vec4<f32>(rdWorld, 0.0)).xyz;
let rd      = normalize(rdField);
```

#### Why This Distorts During Camera Motion:
1. **Camera Inside the Volume ($w < 0$ Inversion):**
   The Perlin noise floor bounding box has extents `[1000, 30, 1000]`. The camera walks *inside* this box (typically around $y \in [0, 15]$). Vertices behind the camera plane have negative eye-space depth ($z_{\text{eye}} > 0$), producing negative homogeneous clip coordinates ($w < 0$).
   In line 970:
   ```wgsl
   o.clip.z = min(o.clip.z, o.clip.w * 0.999999);
   ```
   When $w < 0$, multiplying by $0.999999$ flips the sign relationship. The minimum operator forces $o.\text{clip}.z$ to become deeply negative.
2. **Fixed-Function Triangle Clipping:**
   When proxy cube triangles cross the camera plane ($w = 0$), GPU rasterizer hardware clips the polygon against clip-space planes. Because $o.\text{clip}.z$ has been modified independently of $o.\text{clip}.w$ and $o.\text{worldPos}$, the perspective division ($attr / w$) across the clipped polygon faces becomes warped and non-linear.
3. **Motion Coupling:**
   As the camera moves forward by $\Delta \mathbf{x}$, the distance and angles to the proxy cube vertices change. The clipped polygon edges shift across the screen, causing the interpolated `in.worldPos` at any fixed screen pixel $(x, y)$ to drift and bend. Consequently, `rdWorld` swings dynamically, making the terrain appear to stretch, compress, and warp under motion.

---

### Root Cause 2: 1000m Extent vs 100m Far Plane

In `src/Singularity/Core/EngineRender.cpp` (lines 51–59):
```cpp
float farZ  = 100.0f;
float top   = tanf(fov * M_PI / 360.0f) * nearZ;
...
glm::mat4 proj = currentRenderer().zeroToOneDepth()
    ? glm::frustumZO(left, right, bottom, top, nearZ, farZ)
    : glm::frustumNO(left, right, bottom, top, nearZ, farZ);
```
The view frustum terminates at $Z = 100.0\text{ m}$.
Yet the terrain bounding box extends to $Z = \pm 1000.0\text{ m}$.
- All 8 vertices of the bounding box sit between $10\times$ and $14\times$ beyond the far plane.
- The clamp `o.clip.z = min(o.clip.z, o.clip.w * 0.999999)` artificially drags the depth of these distant vertices onto the far plane ($z_{\text{ndc}} \approx 1.0$), while leaving `worldPos` at $1000$.
- This breaks projective geometric consistency: the rasterized polygon is no longer a Euclidean planar face, and linear interpolation in screen space produces severe non-linear distortion.

---

### Root Cause 3: Camera-Relative "Headlamp" Lighting

Zach noted: *"Same thing with shaders, moving around appears to distort shading rather than making them change naturally from the perspective."*

In `src/Singularity/Screen/ShadingSystem.cpp` (lines 23–29):
```cpp
void ShadingSystem::update(const glm::vec3& cameraPos) {
    if (!s_enabled) return;

    // Keep light a bit above and behind the camera for consistent illumination
    currentRenderer().setLight(cameraPos + glm::vec3(2.0f, 5.0f, 2.0f),
                               kAmbient, kDiffuse, kSpecular);
}
```
And in `src/Singularity/Core/EngineRender.cpp` (lines 122–126):
```cpp
const glm::vec3 lightWorldPos = screenChannel->lightCameraRelative
    ? _camera->pos + screenChannel->lightCameraOffset
    : screenChannel->lightPosition;
```
In the SDF fragment shader (`SdfWgsl.cpp:1252-1258`):
```wgsl
let L = normalize(u.lightPos.xyz - pw);
let V = normalize(u.eyePos.xyz - pw);
let H = normalize(L + V);

let diff = max(dot(nw, L), 0.0);
let lit  = inst.shading.x + inst.shading.y * diff;
let spec = inst.shading.z * pow(max(dot(nw, H), 0.0), max(inst.shading.w, 1.0)) * step(0.0001, diff);
```
#### Why This Distorts:
When `lightCameraRelative` is true, the light source is physically clamped to the camera's position. In the real world, as an observer walks past an illuminated mountain or floor, the sun remains fixed, and diffuse shading remains stationary while only specular reflections shift with viewing angle.
With camera-relative lighting:
- The incident light vector $\mathbf{L} = \text{normalize}(\mathbf{L}_{\text{pos}} - \mathbf{P}_w)$ rotates continuously with respect to the world-space surface normal $\mathbf{N}_w$ whenever the camera moves.
- Diffuse illumination ($\mathbf{N} \cdot \mathbf{L}$) changes value dynamically at every static point on the terrain.
- The entire surface appears to "swim" and distort its lighting, creating an illusion that the geometry itself is twisting.

---

### Root Cause 4: Heightfield Invalidation & Vertical Leaping

In the Perlin floor save (`saves/zones/NoiseFloorWorld/zone.json`), the field is authored as:
$$f(\mathbf{p}) = y - 40 \cdot \text{noise}(0.008 \cdot (\mathbf{p} + (100, 0, 100)))$$
Because $\mathbf{p}$ contains $y$, $f$ has a non-trivial vertical gradient:
$$\frac{\partial f}{\partial y} = 1 - 40 \cdot \frac{\partial \text{noise}}{\partial y} \neq 1$$
However, `geom::isHeightfieldExpr()` previously performed a purely syntactic check for `Sub(y, h)` without proving that $h$ was independent of $y$.
When the heightfield fast path was taken:
1. **Planar Leap (`SdfWgsl.cpp:1123-1126`):**
   ```wgsl
   if ((isHeightfield && damping < 0.5) && rd.y < -1e-4 && (ro.y + rd.y * t) > inst.extents.y) {
       let planeT = (inst.extents.y - ro.y) / rd.y;
       t = max(t, planeT);
   }
   ```
   For general 3D noise fields, the surface can penetrate or exceed the bounding box top depending on amplitude and rotation, and the step may skip the true boundary root.
2. **DDA Height-Grid Skip:**
   Because the noise reads $y$, 2D grid cells derived over $(x, z)$ cannot conservatively bound the 3D zero-crossings. Rays crossing cell boundaries at grazing angles skip valid roots, causing terrain features to abruptly shift or pop as the view angle tilts.

---

## 4. Architectural Resolution Plan

### Phase 1: Clean Screen-Space Ray Emission (Fix Default Euclidean Behavior)

To make the terrain stable, rigid, and distortion-free:
1. **Decouple Ray Generation from Proxy Box Interpolation:**
   Instead of using `rdWorld = normalize(in.worldPos - roWorld)`, emit primary rays from pixel coordinates or inverse view-projection:
   - In the vertex shader for the proxy cube, if the camera is inside the box, rasterize using a full-screen near-plane quad or derive the ray in the fragment shader using `@builtin(position) fragCoord: vec4<f32>`:
     ```wgsl
     let ndc = vec4<f32>(
         (fragCoord.x / u.screenSize.x) * 2.0 - 1.0,
         (1.0 - (fragCoord.y / u.screenSize.y)) * 2.0 - 1.0,
         0.0,
         1.0
     );
     let worldPt = u.invViewProj * ndc;
     let rdWorld = normalize(worldPt.xyz / worldPt.w - u.eyePos.xyz);
     ```
   - This completely isolates ray direction from proxy box clipping, vertex clamping, and near-plane polygon splitting.
2. **Align Far Plane or Unclamp Infinite Terrain:**
   Either expand `farZ` in `EngineRender.cpp` to encompass the active zone's extents (e.g., matching the zone horizon), or perform analytic ray/AABB clipping strictly in fragment shader code starting from the eye.
3. **World-Anchored Illumination by Default:**
   Utilize `Rendering::readAuthorableLight()` with persistent `AuthorableLight` entities (such as the Sun in Cathedral or Sanctum) rather than camera-relative headlamps. Ensure `screenChannel->lightCameraRelative` defaults to `false` for outdoor world zones.

---

### Phase 2: Fulfilling Zach's "Part 2" Vision — The Far Lands Authorable Property

Zach's insight in `To-do list.md` is profound: the spatial distortion that was an artifact on normal terrain is aesthetically magnificent when inhabiting the Far Lands.

In alignment with `NO_BLACK_BOX.md` and `ONTOMATH_FRAMEWORK.md`:
> *"Rendering is a channel reading authored reality, never an authority that invents behavior."*

We should not discard this visual effect. Instead, we elevate it into a first-class, authorable Law/property.

#### Proposed Architecture for Authorable Space Curvature:
1. **Property Registration on ScreenChannel:**
   Register new authorable properties on `@screen-channel`:
   - `@screen-channel.spaceDistortion` (Float: $0.0 = \text{Euclidean}$, $1.0 = \text{Full Far Lands Warp}$)
   - `@screen-channel.distortionWavelength` (Float: spatial frequency of ray curvature)
   - `@screen-channel.distortionOrigin` (Vec3: center of the reality breakdown event)
2. **WGSL Shader Implementation:**
   In `SdfWgsl.cpp`, evaluate the distortion factor during ray generation:
   ```wgsl
   var rd = normalize(rdField);
   let distortion = inst.misc.distortion; // Passed from @screen-channel / Law
   if (distortion > 1e-4) {
       // Controlled, continuous OntoMath ray bending
       let warpOffset = sin(ro.xyz * 0.05 + vec3<f32>(0.0, u.time * 0.5, 0.0)) * distortion;
       rd = normalize(rd + warpOffset);
   }
   ```
3. **Authoring via Law in the Far Lands Zone:**
   In `saves/zones/FarLands/zone.json`, a Person or metalaw can author an ECA law:
   ```text
   When Person moves through Far Lands:
       @screen-channel.spaceDistortion = clamp((distance(Person.pos, FarLands.origin) - 1000.0) / 5000.0, 0.0, 1.0)
   ```
   As the Person travels beyond the civilized boundary and approaches the cosmic threshold, space itself begins to bend and ripple with mathematical purity, authored by Law rather than produced by an unguided clipping bug.

---

## 5. File & Symbol Audit Index

| File Path | Impacted Subsystem | Role in the Distortion Bug |
|:---|:---|:---|
| `src/Singularity/Screen/WebGPU/SdfWgsl.cpp:963-974` | Proxy Vertex Shader | `o.clip.z = min(o.clip.z, o.clip.w * 0.999999)` causes clip-space warping when $w < 0$. |
| `src/Singularity/Screen/WebGPU/SdfWgsl.cpp:1065-1069` | SDF Fragment Shader | Derives `rdWorld` from interpolated `in.worldPos - roWorld`, propagating proxy distortion to rays. |
| `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp:1065-1081` | SDF Pipeline Setup | Draws 36-vertex cube proxy (`_sdfCubeVerts`) with back-face culling on camera-interior zones. |
| `src/Singularity/Core/EngineRender.cpp:51-59` | Camera Setup | Sets `farZ = 100.0f`, creating a $10\times$ mismatch against the 1000m noise floor extent. |
| `src/Singularity/Screen/ShadingSystem.cpp:23-29` | Lighting Subsystem | Moves light position with `cameraPos + offset`, distorting diffuse/specular shading during motion. |
| `src/Singularity/Core/EngineRender.cpp:122-126` | Light Dispatch | Fallback preserves camera-relative light position when no `AuthorableLight` is authored. |
| `docs/Agenda/Tasks/To-do list.md:52-54` | Requirements & Telos | Zach's bug report and Part 2 authorable property vision for the Far Lands. |

---

## 6. Conclusion & Recommendation

The "SDF distortion bug" is not an issue with the signed distance math or the Perlin noise algorithm itself; it is an architectural seam between **fixed-function proxy polygon clipping** and **volumetric ray direction reconstruction**, compounded by camera-relative lighting.

By moving primary ray generation to pure inverse view-projection screen rays and anchoring light to world space, normal zones will become rock-solid, rigid, and realistic. Simultaneously, by introducing `@screen-channel.spaceDistortion` into OntoMath and the Screen Channel, Zach's vision for the Far Lands can be realized with complete authorial control.
