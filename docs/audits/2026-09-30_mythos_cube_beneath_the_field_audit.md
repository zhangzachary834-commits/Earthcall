# The Cube Beneath the Field — domain nouns hiding in the render substrate

*Harness: Claude Code (cloud session). Model: Claude Fable 5.1 (same underlying model as Claude Mythos 5.1; Zach asked to be addressed as Mythos). Session `session_01EbAdb1nuGQ8XorGsEHHAkv`. 2026-09-30, 07:31 UTC. Branch `claude/modest-cray-rhhsvs`, on top of `b22aa94`. Companion to [The Ungoverned Governor](2026-09-24_mythos_ungoverned_governor_audit.md).*

**What Zach asked.** Dive into rendering, shapes, and the WebGPU implementation for subtle ontological contradictions, and for domain nouns inside what looks like an "ordinary necessary metal substrate" that are not necessary and could be actively bad in Earthcall.

**What I drew from.** Zach's own doctrine on `ShapeKind` in `Design/Building 2D and 3D Apps with Earthcall Guide.md` §440-452 ("never rely on `ShapeKind` to define form … never a reason for behavior to branch by kind"); Refusals 1, 3, 6, 7; the minimum-maximum principle; the "Paint is on the Material" non-negotiable; ONTOMATH §7 ("a channel reads OntoMath; it never decides what the thing is"); the Zones-as-bounds plan ("location is derived"); and the radiance/volumetric work by GPT-5.6 Sol, whose `RadianceSourceBinding` and `VolumeDensity` headers are the *right* pattern and therefore the measuring stick for everything older. Every claim below is a line reference read at `b22aa94`; the binary was not built this session.

---

## 0. The insight in one breath

Earthcall does not have one shape ontology in its substrate. It has **seven**, and each one is a domain noun wearing a substrate's coat:

| Vocabulary | Values | Where |
|---|---|---|
| `ShapeKind` | Cube, Polyhedron, Sphere, Cylinder, Cone, Ellipsoid, Ovoid, Paraboloid, Torus, RoundedBox, Field, Patch, Shape2D, Text2D | `ObjectTypes.hpp:52` (serialized int, append-only) |
| `SdfPrim` | Sphere, Box, RoundBox, Ellipsoid, Cylinder, Cone, Torus, Expr, Convex | `Sdf.hpp:21` (serialized int; `ZoneManager.cpp:2875` range-checks it) |
| `SpatialKind` | Polyhedron, SmoothSurface, ComplexShape, Field, Patch | `ObjectTypes.hpp:95` |
| `SmoothSurfaceData::QuadricForm` | Sphere, Ellipsoid, CylinderSide, ConeSide, Paraboloid | `SmoothSurface.hpp:54` |
| `SmoothSurfaceData::ParametricKind` | Torus, Ovoid, Mobius, Klein, ProjectivePlane | `SmoothSurface.hpp:58` |
| `Contour::SurfaceKind` | Spherical, Cylindrical, Conical, Toroidal, Freeform | `Contour.hpp:120` |
| WGSL `sd*` functions | sdSphere, sdBox, sdRoundBox, sdEllipsoid, sdCylinder, sdCone, sdTorus | `SdfWgsl.cpp` |

Beside them, `ShapeParams` (`ObjectTypes.hpp:77`) carries eleven named floats — `majorR`, `minorR`, `ovoidAsym`, `paraboloidA`, `fillet` — so a torus *knows its own name* in a struct that every Object carries whether or not it is a torus.

Zach's doctrine says behavior must never branch by kind. It branches in four places: `ObjectRender.cpp` (10 cases), `ObjectCollision.cpp` (10), `ObjectRaycast.cpp` (5), and `Physics.cpp:777-779`, where a physics law may be limited to objects of a given `ShapeKind`. The general quadric matrix `Q` (`SmoothSurface.hpp`) and the `SdfNode` expression tree already *are* the minimum invariant that makes all seven vocabularies redundant. They exist, they are serialized, and the enums are names attached to them after the fact.

The chain that produced this is the audit. **The cube was the atom.** Everything downstream of that first decision inherited a noun: faces, six face colours, a unit extent box, a y-up floor, a "terrain" the renderer can recognize, a Blinn-Phong material, a light with the same three coefficients, a pixel plane for 2D, a Latin bitmap font. None of these is metal. Each begat the next.

---

## 1. The cube is the atom, and every being still carries its skeleton

- `Object::_shapeKind = ShapeKind::Cube` is the default (`Object.hpp:164`).
- `float faceColors[6][3]` lives on **every** Object (`Object.hpp:275`), "LEGACY … kept for save/load compatibility (first 6 faces)". A Sphere, a Field, a Bezier patch, a line of text all carry six face colours. `Text2D` takes its colour from `faceColors[0]` (`ObjectRender.cpp:1069-1073`).
- The collision support point for `Cube` is hard-coded to ±0.5 (`ObjectCollision.cpp:467-470`); an empty `Polyhedron` returns the same unit cube; and a **Field whose support cloud is empty falls through every case and returns `(0,0,0)`** (`ObjectCollision.cpp:522`) — an SDF being with no tessellation has no surface, physically.
- `_fieldExtent{1,1,1}` (`Object.hpp:188`) bounds every Field to a unit box unless set; the WebGPU proxy is `_sdfCubeVerts`, "unit bounding cube, shared by SDF + volume proxies" (`WebGpuRenderer.hpp`). A Field is a cube that learned to hold other math.

Contradiction with a non-negotiable: "Paint is on the Material, and materials are shared." The Object holds `faceColors`, the Material holds `baseColor`, the Material and the RenderMaterial both hold `colorExpr`, and `resolveRenderMaterial` merges them with a per-face albedo texture (`RenderMaterial.cpp:7-21`). That is four colour authorities. The doctrine names one.

## 2. Y is up, and the renderer knows what terrain is

`geom::isHeightfieldExpr` (`Sdf.cpp:971-983`) proves a field is a heightfield only if the root op is `Sub` and the left child is literally the variable `y`. The heightfield DDA — Phase C of the rendering-optimization plan — engages only on that shape. Gravity's fallback direction is `(0,-1,0)` (`Physics.cpp:163,642`). `Camera::up` is `(0,1,0)`. Eight files hard-code the same vector.

The ontology says a Zone is a bound in a continuum with no privileged axis, and location is derived. The substrate says the world has a floor, the floor is `y = h(x,z)`, and the optimizer is a terrain detector. A Person who authors a wall `x - h(y,z)` or a ceiling gets exactly the same mathematics and none of the acceleration, and no error tells them why. This is not necessary: the proof is one substitution away from axis-generic (any of the three variables minus an expression free of that variable).

## 3. Blinn-Phong is a domain noun

`Material` carries `shininess = 32`, `specular`, `ambient`, `diffuse` with the comment "(was global)" / "(was light ambient)" (`Material.hpp:53-56`). `RenderMaterial` carries the same four. `RadianceSourceBinding` carries `ambientRadiance`, `diffuseRadiance`, `specularRadiance` and a `coefficients` vec4 in the same order (`RadianceSource.hpp`). The WGSL evaluates `pow(max(dot(nw, H), 0.0), max(inst.shading.w, 1.0))` (`SdfWgsl.cpp:1875, 2807`).

Sol made *colour* authorable OntoMath: `colorExpr`, `radianceExpr`, `chromaExpr`, `angularExpr`, and the medium's `rho`, `sigma_t`, `sigma_s` (`VolumeDensity.hpp`). The **BRDF** — how light meets surface — is a 1977 constant in C++. A Person can write `@material.clay.shininess := 8` but cannot author "this surface is retroreflective," "anisotropic," "iridescent," or "Lambertian only," because there is no expression slot for the reflectance function. ONTOMATH §7 says the channel never decides what the thing is. Here the channel decides what *light* is. And the ambient/diffuse/specular split on the *light* is worse than on the material: a light has a radiance; the three coefficients are the shading model's bookkeeping leaking into the being.

The medium has one of the same: `sigma_t = 0.5 * D` as "the explicit compatibility law" (`VolumeDensity.hpp:~30`). A physical constant, chosen once, in a header.

## 4. The Camera is not a being; the Person's camera is an echo

`Camera` (`Camera.hpp:12`) is not a Singular. It carries `GLdouble modelview[16]`, `projection[16]`, `viewport[4]` — fixed-function OpenGL state — into the WebGPU app. Each frame `Engine.cpp:373` copies `cam->getPos()` into `Person::cameraPos`, which *is* a registered, writable property. So a law that writes `@person.cameraPos` succeeds, notifies the Rete, and is overwritten before the next frame renders. The property never lies and never lands. The comment at `Engine.cpp:366-371` knows this ("Camera is what the render path actually moves"). `Person/Perspective` exists as a stub; this is what it is for.

## 5. Physics has its own enum of laws, and Object has a `type` string

`Physics::LawType { Gravity, AirResistance, Collision, CustomForce, GravityField, CenterGravity }` (`Physics.hpp:165`) is Refusal 3 verbatim: an enum of kinds of law. `PhysicsLawBridge` correctly retires each into a legible Law (rung done well), but the kinds themselves remain C++ and the bridge resolves them by name. `LawTarget` (`Physics.hpp:180-195`) may filter by `ShapeKind`, `SpatialKind`, and `objectType` — no save file uses any of the three filters (census: zero occurrences), so they are removable today.

`Object::objectType` is a `std::string` (`Object.hpp:160`), the "`type` string" the Router names as a refusal. `setObjectType(int)` converts an integer to a string (`ObjectCore.cpp:102-103`). It is set by the terminal (`"Robot Guy"`), by `ActionModel` on newborns, and by `EarthcallAPI`. `TransferPolicy` gates `"type"` as Gated — a gate lovingly guarding a field the ontology says must not exist.

## 6. The 2D plane is a second world with its own coordinates, and a letter is the one shape no Person can author

`_x2D`, `_y2D` are pixels, top-left origin; "a 2D object has no world position" (`Object.hpp:172-177`). `Shape2D` is "a screen-space axis-aligned rectangle" and nothing else (`ObjectRender.cpp:1051`). `Text2D` is drawn by `stb_easy_font` with `kGlyphCell = 16`, `kGlyphAdvance = 6` (`ObjectRender.cpp:1021-1045`), its label pulled from three fixed property names (`"label2D"`, `"controlLabel"`, `"displayName"`).

The Zones-as-bounds doctrine says location is derived and a Dimensional Zone is a bound in the one continuum. A 2D being has a *stored* location in a *different* space with a *different* origin convention. That is two worlds, not one continuum with a 2-axis bound. And the glyph: every other form in Earthcall is OntoMath — implicit, exact, raymarched. A letter is a fixed C bitmap. It is the single visible shape a Person cannot author, morph, CSG, or govern by law. The industry answer (SDF/MSDF glyphs) is *more* Earthcall than the bitmap, not less: a glyph is a Field; a font is a Formation of glyph Fields.

## 7. The enums are growing toward being the ontology, monotonically

`ShapeKind` is append-only and serialized. `Shape2D` (12) and `Text2D` (13) were appended 2026-08. `SdfPrim::Convex` was appended after `Expr`. `ParametricKind` lists `Mobius`, `Klein`, `ProjectivePlane` — one mention in all of `src/*.cpp`, i.e. names reserved for shapes that do not exist. Every kind anyone ever adds to a serialized enum is burned forever, by Zach's own rule. So the mechanism by which an enum becomes the ontology "by accretion" — the exact thing Refusal 3 exists to stop — is running, slowly, inside the one enum the doctrine calls "legacy transport." Nothing in the tests pins its length.

## 8. What *is* genuinely metal here (to be fair to the substrate)

`SdfOp` (Union, Intersect, Subtract, Morph, SmoothUnion), `SdfPrim::Expr` and `Convex` with `planes`, the WGSL emitter and its structure/value split, `DepthMode`, `Blend`, `GpuBufferPool`, `GpuMeshCache`, the range-proof hierarchy, `RenderedFieldSemanticObserver` ("no API that returns theorem authority to the renderer"), `DensityInputKind` as a *compiler* fact, and Sol's `RadianceSourceBinding` / `VolumeDensity` as "bounded views, NOT a Medium kind." These are the machine sensing and acting. The WGSL `sd*` functions are fine as the *compiled form* of a leaf whose truth is math; they become ontology only when `SdfPrim::Sphere` is what the save file says the being *is*.

## 9. The chain

The cube made faces. Faces made `faceColors[6]`. `faceColors[0]` made the colour of text. The cube's default made every Field a unit box. The unit box made "extent" a stored fact. The y-up floor made the heightfield proof; the proof made a renderer that recognizes terrain. Blinn-Phong made `Material`'s four coefficients; the coefficients made the light's three radiances; the three radiances made `RadianceSourceBinding::coefficients`. The GL camera made the Person's camera an echo. The pixel plane made 2D a second world. The bitmap font made the letter the one unauthorable form. Seven vocabularies, four branch sites, one atom.

Zach's doctrine already says the right thing; the code still obeys the old one in the places listed. And it can get *worse* without anyone violating a rule, because the append-only enums are a ratchet.

## 10. Recommendations, in the order that unlocks the most

1. **Pin `ShapeKind` and `SdfPrim` by test** (`static_assert` on the last value); every new form is a Field. Nothing new is ever appended.
2. **Retire kind-branching in Collision, Raycast, and Render**: derive the support point from `Q` analytically or from the SDF by gradient ascent; raycast by sphere-tracing on CPU (the WGSL already does it); render by `SpatialKind`-free dispatch on what data is present. A Field must never return `(0,0,0)` as its surface.
3. **Delete `QuadricForm`, `ParametricKind`, `Contour::SurfaceKind`**: keep `Q`, `ScalarForm`, and `SdfNode`. Unimplemented names are not ontology.
4. **Drop `LawTarget`'s kind filters and `Object::objectType`** (zero save-file uses; the gate on `"type"` goes with it).
5. **Make the heightfield proof axis-generic.** One substitution.
6. **Author the BRDF**: a `material.brdfExpr` Piecewise over (n, l, v, h) exactly like `radianceExpr`, with Blinn-Phong as the default *expression* on `material.default`, not as C++. Retire the ambient/diffuse/specular triple from the light; a light has radiance.
7. **Glyphs as Fields**: SDF/MSDF text, a font as a Formation of glyph Fields; keep `stb_easy_font` as the tessellation fallback only.
8. **Migrate `faceColors` into the Material**, per face, and delete the array from Object. Six is a cube fact.
9. **Camera → `Person/Perspective`**, a Singular the render path reads rather than a struct the Person echoes.
10. **2D as a 2-axis Dimensional Zone**, not a pixel plane with its own origin. This is already the Zones-as-bounds plan's Rung 3 in spirit.

None of these add a class for a domain noun. Every one of them *removes* one.

*— Mythos*
