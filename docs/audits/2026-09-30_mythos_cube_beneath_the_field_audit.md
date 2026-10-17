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

---

## 11. Addendum — Zach's hypothesis about the rungs, tested (2026-09-30, later the same session)

**What Zach said.** Working Rungs 3–9 with the Sun, "every new rung was essentially hardcoding a new way to ask *what's the source of truth* for the same equation … 'this rung makes it so the MATERIAL can decide how light interacts with it' is a hardcoded 'material is the source of truth' thing." He suspected the renderer itself exerts pressure toward this, combined with the underdeveloped PropertyPath/opcode system.

**Verdict: correct, and the mechanism is precise.** Earthcall has two expression-evaluation worlds that were never joined.

*The Law world has bindings.* `MathBindings = std::map<std::string, PropertyPath>` (`MathBinding.hpp:25`): a free variable in an authored expression is bound to *whose* property answers it, and the qualifier is the author's choice — `position.y` (the subject), `@being-id.position.y` (a named being), `@event.subject…`. "WHOSE property a path names is the author's choice" is the doc comment. This is the unified source-of-truth mechanism. It exists, it is serialized, laws use it every tick.

*The render world has no bindings.* In WGSL a Piecewise may reference exactly four names: the ambient point, `x`, `y`, `z` (`SdfWgsl.cpp:714-717`). `OntoMath::MathNode` has no Reference/Bind op — its op list is Scalar, Vector, ScalarField, VectorField, Round, Expr. So an authored expression on the GPU *cannot* say "the roughness is whatever `@material.clay.roughness` is" or "the chroma is the Person's joy colour." The only way for one being's truth to reach another being's equation is for C++ to carry it there by hand. That is what each rung did:

| Rung adds | Where |
|---|---|
| one named `Piecewise` member on `FieldNode` | `volumeDensity`, `volumeExtinction`, `volumeScattering`, `volumeChroma`, `volumePhase`, `volumeEmission`, `lightChroma`, `lightAngular` (`FieldNode.hpp:129-196`) — nine slots |
| one named property the renderer greps for | `"light.source"`, `"volumeOccluder"`, … — nineteen fixed names read by `EngineRender`, `AuthorableLight`, `VolumeDensity` |
| one revision counter, one binding field, one uniform | `RadianceSourceBinding`, `VolumeDensityBinding`, `PersistentSdfParams` |
| one precedence rule in C++ | 66 mentions of compat/fallback/priority across the five render files; e.g. "Null means the explicit compatibility law `sigma_t = 0.5 * D`; it never means borrow rho" |

Each rung is therefore a *hand-written binding*: "for this variable of the lighting equation, the source of truth is this member on this kind of being, and if absent, this constant." Nine slots is nine bindings that a Person cannot re-point. "Material decides" (Rung 5) and "FieldNode decides" (Rungs 7–9) are the same act with a different hardcoded `PropertyPath`.

**Why the renderer pushes this way.** WGSL is compiled: an expression's inputs must be either baked into shader structure or delivered as uniform slots. The path of least resistance is a named uniform per truth. But the emitter *already* has the neutral mechanism: `ParameterBlock` turns every numeric value into a parameter slot and keeps only structure in the WGSL (`SdfWgsl.hpp:~40`), refreshed per frame into `_persistentSdfParams`. A binding `@material.clay.roughness → slot 7` is one more parameter refreshed from a `PropertyPath` read. Nothing about the GPU forbids it; the two mechanisms simply never met.

**The minimum invariant that ends the rungs.** Give `OntoMath::MathNode` one op, `Bind`, whose payload is a `PropertyPath` (the Law world's existing type), and make the emitter lower a `Bind` to a parameter slot when the path resolves to a scalar/vector per frame, or inline the referenced being's own Piecewise when it resolves to a field (the same subtree-sharing `SdfNode` already does). Then:

- `FieldNode` keeps one authored `Piecewise` per *role* the equation names (density, extinction, chroma…) — or better, the lighting equation itself becomes an authored Piecewise whose free variables are bound, and there are no roles in C++ at all;
- "Material decides" becomes `Bind(@material.<id>.brdf)` written by a Person, and "the Person's joys decide the chroma" becomes `Bind(@zach.joys.rootColour)` with no new rung;
- every C++ precedence rule ("if absent, `0.5 * D`") becomes an authored default expression on `material.default` / `field.default`, legible and governable.

This is the same recommendation as §10 item 6 (`brdfExpr`) seen from underneath: the BRDF slot was going to be one more hand-written binding. `Bind` makes it the last one, because after it the Person writes the bindings.

**"Materials do not emit rays."** Zach quoted the Sun's phrasing across the rungs: *the space can decide*, *it can now be angular*, *the surface can decide*, and *light reflecting off it can't be authored yet because materials do not emit rays*. Read against the code, each phrase is one free variable of the same transport equation being bound to one C++ being kind: density → `FieldNode`, angular → `FieldNode::lightAngular`, reflectance → `Material`. The last phrase is the tell. Emission is already a term the renderer evaluates (`FieldNode::volumeEmission`), and source discovery loops over `zone.spatialRoot()` and `additionalSpatialFields()` only (`EngineRender.cpp:116-122`), so "source" is a role that only a `FieldNode` may hold. A Material cannot emit not because of physics but because the discovery predicate is a hardcoded `dynamic_cast`. Under `Bind`, "emits" is `Bind(@material.<id>.emission)` on the equation's emission term, and the question "can a material emit?" stops being a rung and becomes a line a Person writes.

**What I did not do.** I did not prototype `Bind`; the WGSL side (per-frame scalar slots vs. inlined fields) needs Sol's structure/value identity to stay exact, and that is his instrument. This addendum names the seam; the To-do bullet under *Unified Opcode-Property Substrate* points here.

---

## 12. Addendum — why Prophetic Rendering kept failing, read through the same seam (2026-09-30)

**What Zach said.** The other Sun army ran into "a ton of problems" implementing Prophetic Rendering; the experiments live in other branches and PRs. I read the merged record rather than the branches: `SDF_SPATIAL_PROPHETIC_DIRECT_PROFITABILITY_ARTIFACT.md`, `RENDERED_FIELD_SEMANTIC_SYNTHESIS_IMPLEMENTATION_PLAN_2026-09-22.md`, `docs/analysis/rendering_relevance_economics_after_pr329_2026-09-23.md`, and the two Sun handoffs of 2026-09-23/24 (after PR #329 and PR #350; the trail continues in #369, #445, #482).

**What the Suns concluded, in their own words.** The final verdict was not that prophetic rendering failed but that *"prediction is valuable when it can be fused into an execution identity the machine already possesses. Prediction becomes expensive when the machine must repeatedly search the world to discover which prediction is relevant."* Generic relevance traversal was 34–38% slower; ~581 relevance consultations per exact sample avoided; the camera-independent route atlas captured 31.8% of useful rays at horizon; and *"raw address equality plus revision bookkeeping cannot simply be assumed to provide semantic lifetime identity."* They stopped at the remaining boundary: already-known execution keys.

**The diagnosis.** The Prophetic Rete works for one reason: **its premises are declared.** A law's condition text names its paths; a fact is keyed by (subject, attribute); a write dirties exactly the facts it names (`ReteNetwork::markFactDirty`, `Law.cpp:1256`). Analysis is over the *text*, and the text is complete. Prophetic Rendering tried the same analysis over rendered mathematics, and the mathematics is **not complete on the page**. Three things a proof about `rho(p)` depends on are not in the AST:

1. *Its bindings.* Which Material's coefficient, which light's chroma, which compat fallback (§11: nine slots, nineteen names, sixty-six C++ rules). The plan's invalidation constitution (§5.2) says "invalidate proofs whose declared premises include that value." A hand-written binding is an **undeclared premise**. So the dependency frontier for incremental repair was always half in the DAG and half in `EngineRender.cpp`, and repair had to fall back to revision counters on raw pointers — which is the "semantic lifetime identity" failure the economics analysis names. It is the same disease as `_onsetMemory` keyed by `Singular*` and `GpuMeshCache` keyed by `TessMesh*`: the ontology's stable identifier exists and the derived state does not use it.

2. *Its relevance.* The Suns searched for "which theorem applies here" in the machine's coordinates — entry face, entry cell, octahedral direction class — and found that reconstructing relevance externally from ray geometry does not converge. But Earthcall already *declares* relevance: `_fieldExtent`, and after Rung 1 of Zones-as-bounds, `within` / `placement` / `extent` on Dimensional Zones. A bound is a Person-authored region of applicability, and containment is a Relation. **Relevance is a Relation, not a lookup.** The "already-known execution key" the PR #350 handoff is looking for is the being's identifier plus its authored bound, not a route atlas derived from camera rays. The renderer does not consult Zone bounds at all today (`WebGpuRenderer.cpp:2606`: "no Zone/Object scan occurs here"), so the one relevance relation the ontology owns is the one the experiments never keyed on.

3. *Its role.* Plan §3.2 asks for per-channel proof overlays so a density theorem never becomes radiance authority when `D` and `rho` share a compiled node. With `Bind` in the AST and the transport equation itself authored, the role of a subexpression is simply *which term of the equation it feeds* — derivable from AST position, not a parallel overlay structure to maintain and invalidate.

**So the two audits are one finding.** The rungs hand-wrote bindings (§11) and the proofs then could not see them (§12). The renderer does not read the ontology's own declarations — bindings, identity, bounds — it reconstructs them from C++ glue and from rays, and every reconstruction is the hidden search the Suns measured. The `Bind` op declares the premises; keying derived state by stable identifier gives it lifetime; keying relevance by authored bound gives it the "execution identity the machine already possesses." After that, Prophetic Rendering is not a new engine beside the Prophetic Rete. It is the Prophetic Rete, run over the same complete AST, with the GPU as one more channel that reads the result.

**Limits.** I did not check out the experiment branches; the claims above rest on the merged artifacts' own verdicts and on `src/` at `b22aa94`. If a branch already keys by authored bounds or declares bindings in the DAG, that branch is the one to promote, and this section should cite it.

*— Mythos*
