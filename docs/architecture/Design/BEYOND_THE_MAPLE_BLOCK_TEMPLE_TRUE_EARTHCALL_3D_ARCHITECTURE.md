# Beyond the Maple Block Temple
## A field guide to true Earthcall 3D architecture

**Status:** Architectural doctrine / agent-facing design guide  
**Date:** 2026-10-19  
**Origin:** Consolidated by GPT-5.6 Sol from the Cathedral uncanny-valley work, Sun Zone passes, Zach's repeated visual corrections, OntoMath material/radiance work, and the repository's geometry-ontology doctrine.  
**Scope:** How to author convincing 3D architecture in Earthcall without collapsing into visibly decomposable toy-block assemblies.

> The phrase **"Maple Wood Kids Building & Stacking Blocks failure mode"** is shorthand for a recurrent AI authoring mistake: the available primitive vocabulary quietly becomes the design vocabulary. This is not a criticism of toy blocks. It is a warning against making monumental architecture read as enlarged toys.

This document exists because Earthcall has now accumulated enough failures, corrections, visual witnesses, and rendering machinery that "make a beautiful 3D building" should no longer mean "place more primitives until the silhouette vaguely resembles one."

The governing principle is simple:

> **Primitives are brushstrokes, not nouns.**

A box is not a wall.  
A cylinder is not a column.  
A cone is not a spire.  
A torus is not an archivolt.  
A sphere is not a finial.

Those things may participate in the mathematics that constructs a wall, column, spire, archivolt, or finial. They are not the architectural identity of the finished form.

The difference between a block toy and architecture is not primarily primitive count. It is **continuity, hierarchy, proportion, profile, negative space, material truth, and causal coherence across scale**.

This guide records how to get there in Earthcall.

---

# 1. The failure mode

The Maple Block failure happens when the model reasons approximately like this:

- "column" -> cylinder;
- "capital" -> box;
- "roof" -> triangular prism;
- "spire" -> cone;
- "ornament" -> sphere;
- "arch" -> torus segment;
- "more detail" -> more small primitives.

The resulting scene can be technically valid, symmetric, and even complex. It still reads as a construction set because the viewer can mentally reverse-engineer the object list.

A cathedral does not feel like a cathedral if the eye immediately sees:

> box + box + cylinder + cone + sphere + another box.

That reversibility is the visual smell.

## 1.1 Typical symptoms

A scene is probably in the failure mode when several of these are true:

1. **Every architectural noun maps one-to-one to a primitive.**
2. **The silhouette can be described as stacked boxes with decorations.**
3. **Large surfaces terminate in raw primitive edges.**
4. **Columns are straight tubes with unrelated caps glued on top.**
5. **Arches are holes plus rings rather than continuous profiled systems.**
6. **Finials are cones, spheres, or ellipsoids wearing poetic names.**
7. **Cornices are repeated rectangular ledges rather than continuous profiles.**
8. **"Detail" means multiplying the number of objects instead of enriching the form.**
9. **The building works from one hero angle but falls apart in side view.**
10. **Close inspection reveals intersections, seams, tangencies, or floating trim.**
11. **Lighting does all the work because the geometry itself has weak depth cues.**
12. **Material stripes or texture decoration are used to imply structure the geometry does not possess.**
13. **The eye can count the primitives.**
14. **The scene looks like a very expensive preschool block set.**

The Sun Zone passed through this exact pathology. The Cathedral exposed a neighboring one: painted relief that promised geometry which did not exist. Both failures come from the same deeper mistake:

> **Appearance is being asked to stand in for authored structure.**

See also:

- [Cathedral Uncanny Valley Saga](../../../agent%20intercom/communication-threads/sdf-and-rendering/Cathedral%20Uncanny%20Valley%20Saga%209-18-26%20-%20GPT-5.6%20Sol.md)
- [What Survives Approach](../../Reflections%20on%20Earthcall%27s%20Progression/Reflections%20on%20Trajectory/What_Survives_Approach.md)
- [Cathedral Architectural Relief Decomposition](../../Agenda/Tasks/Specific%20Tasks/Architecture%20and%20Ontology/Cathedral_Architectural_Relief_Decomposition/Cathedral_Architectural_Relief_Decomposition.md)

---

# 2. The conceptual shift: author form, not an inventory of shapes

Earthcall's geometric power is not that it has many primitives.

Its power is that primitives, authored mathematics, transforms, boolean composition, fields, patches, and material functions can become **one coherent form**.

A true architectural element should usually be conceived first as a relation among profiles, voids, transitions, and load-bearing visual rhythms.

Ask:

- What is the silhouette?
- What void defines it?
- What profile repeats or transforms?
- What surfaces must flow continuously into one another?
- Where does the eye expect compression, release, taper, projection, recess, or termination?
- Which parts are structural, which are ornamental, and which are merely optical?
- What changes when I walk around it?
- What changes when light moves?
- What remains the same under relighting?

Only then ask which SDF leaves, OntoMath expressions, Bézier/NURBS patches, or booleans happen to realize that form.

This is the same representation/execution boundary described by the [Geometry Execution Substrate Manifesto](../mathematics/GEOMETRY_EXECUTION_SUBSTRATE_MANIFESTO.md): the runtime representation may use low-level geometric machinery without letting that machinery become the ontology of the authored world.

---

# 3. Think in scales

Good architecture is legible at more than one distance.

A reliable Earthcall workflow therefore works from large scale to small scale.

## 3.1 Macro: procession, massing, and silhouette

At the farthest useful distance, the building must already work.

Decide:

- where the Person approaches from;
- what the primary visual axis is;
- which mass dominates;
- where the skyline rises and falls;
- which voids frame the focal point;
- how side masses support rather than compete;
- whether the composition reads in front, three-quarter, and side views.

The Sun Zone improved dramatically when the rear citadel was moved farther back and made physically larger rather than merely adding more detail to a too-small pile behind the gate.

That is a recurring lesson:

> **Bad massing cannot be repaired with ornament.**

Before sculpting capitals, confirm that the building's major volumes have correct hierarchy and distance.

## 3.2 Meso: the level where architecture becomes architecture

This is the scale AI models most often skip.

Meso-scale form includes:

- continuous column shafts and capitals;
- archivolts;
- buttresses;
- cornices and entablatures;
- tracery;
- frame systems;
- balcony profiles;
- recessed bays;
- window reveals;
- parapets;
- ribs;
- stairs;
- portals;
- sculpted finials;
- layered thresholds;
- coherent transition zones between masses.

These are not microscopic details. They are the forms that turn massing into architecture.

The Sun Zone's breakthrough came not from another hundred tiny ornaments but from compound SDF structures that created deeper setbacks, ribs, crowns, recesses, arch systems, and real shadow-producing profiles.

## 3.3 Micro: surface truth

Only after macro and meso work should the architect spend time on:

- subtle bevels;
- stone variation;
- veins;
- scratches;
- metal roughness;
- carved inscriptions;
- micro-relief;
- tiny fillets;
- wear;
- seams that are physically or materially justified.

Micro-detail should reward nearness. It should never be used to conceal the absence of meso-scale form.

---

# 4. Design negative space first

A common primitive-stack failure comes from thinking only about solids.

Architecture is equally authored by **voids**.

Arches, colonnades, windows, portals, courtyards, aisles, galleries, undercrofts, and recesses derive much of their identity from the empty space they shape.

Before adding an arch frame, design the arch **opening**.

Before adding a colonnade, design the rhythm of **gaps between columns**.

Before adding buttresses, design how the central wall plane should appear between them.

Before adding a crown, design the sky silhouette around it.

SDF subtraction is especially powerful here. A convincing arch is often better thought of as:

1. a bounded structural mass;
2. a precisely sculpted void cut through it;
3. one or more nested framing profiles around that void;
4. depth offsets producing reveals and shadow.

That is fundamentally different from placing a ring primitive on a wall and calling it an archivolt.

---

# 5. Profiles matter more than primitive identity

Many architectural forms are best understood as **profiles propagated through space**.

Examples:

- a cornice is a profile along an edge;
- a molding is a profile swept along a path;
- a column is a changing radial profile along height;
- a capital is a transition between shaft profile and load-bearing top;
- an archivolt is a profile following an arch path;
- a buttress is a depth/width profile changing with height;
- a spire is a taper law plus sectional variation.

When Earthcall's available substrate permits it, author the profile mathematically and let the renderer execute it.

If a direct sweep/extrusion abstraction is not yet available, compose existing SDFs so the result still behaves like one profile rather than several unrelated primitives.

The eye cares about the **continuity of the resulting surface**, not whether one or twelve internal leaves produced it.

---

# 6. Columns: stop making tubes with hats

A toy-block column often looks like:

> cylinder + box + another box.

A convincing column has a continuous visual logic from floor to entablature.

Think in zones:

1. **base transition**
2. **shaft**
3. **shaft variation**
4. **necking**
5. **capital transition**
6. **abacus / receiving plane**
7. **connection to the structure above**

Useful authored behaviors include:

- taper or entasis;
- fluting;
- bundled shafts;
- changing cross-section;
- soft convex/concave profile transitions;
- capital flare;
- layered necking;
- recessed channels;
- continuous molding rather than detached slabs.

The exact historical style does not matter. The governing requirement is that the parts visually **cause one another**.

A capital should look like the shaft became capable of meeting the load above, not like a cube fell onto a cylinder.

---

# 7. Arches and archivolts: one system, not a hole plus a donut

A convincing arch needs:

- a meaningful opening shape;
- wall depth;
- reveal depth;
- frame thickness;
- nested profile or archivolt;
- spring-point logic;
- connection to adjacent piers or columns;
- shadows that arise from geometry.

The Sun Zone learned this through compound subtractions and layered front-depth relationships.

Avoid the obvious "torus glued onto wall" solution.

Even if a torus-like distance field participates internally, constrain/intersect/subtract it so the viewer perceives a **bounded architectural frame**.

The frame should terminate intentionally. It should meet piers, cornices, or surrounding relief in a way that makes architectural sense.

---

# 8. Cornices and entablatures: profiles, not shelves

Repeated boxes stacked above a wall produce the classic toy-set look.

A cornice should have hierarchy:

- support band;
- projection;
- underside/reveal;
- shadow line;
- upper cap;
- transition into the next mass.

The most important function is often the shadow it casts.

A good profile can make a simple facade feel finished because it gives light an intentional edge to articulate.

Do not add five ledges just because five is more detailed than one.

Ask what each projection **does** to silhouette and shadow.

---

# 9. Finials, flames, crowns, and sacred ornament

The most notorious AI shortcut is the "poetic primitive":

> sphere named "The Monastic Pearl"  
> cone named "Solar Flame"  
> ellipsoid named "Crown of Dawn"

Names do not sculpt geometry.

A finial should have a silhouette that could not be mistaken for the stock primitive underneath.

For flame-like or solar forms, useful strategies include:

- intersections that sharpen an otherwise round field;
- subtractions that carve negative tongues;
- taper laws with changing curvature;
- nested elongated forms sharing a base;
- asymmetric or rotationally structured profiles where appropriate;
- compound fields whose outer silhouette is the authored target.

If the form still reads immediately as "cone" or "sphere," the sculpting is not finished.

---

# 10. Repetition must have architectural meaning

Repetition is essential to architecture.

Blind repetition is not.

Good repetition follows:

- bay spacing;
- column rhythm;
- structural module;
- procession;
- window cadence;
- radial symmetry;
- hierarchy;
- authored pattern.

Bad repetition follows:

- arbitrary object-local x;
- "every N meters";
- a cosine because a cosine is convenient;
- duplicated decorations without hierarchy.

The Sun Zone's gift-wrapping incident is the canonical warning.

Periodic OntoMath masks were used to place gilded seams. The math was elegant and the result was ridiculous because the gold knew **where** it was but not **why** it was there.

> **Coordinate is location, not meaning.**

Gold belongs on a rib because it is a rib, on a cornice because it articulates a hierarchy, or on a path because it marks procession. It should not appear merely because `cos(kx)` crossed a threshold.

When procedural repetition is used, bind it to authored structure: a Formation, a set of known architectural members, a path, a local bounded region, or a semantic relation.

---

# 11. Geometry and material are different causes

Do not ask material to fake missing architecture.

Do not ask geometry to encode pigment.

The Cathedral work established an important distinction:

- carved groove -> geometry;
- marble vein -> color/composition field;
- metallic response -> material response;
- painted motif -> pigment/texture;
- real recessed mortar -> geometry;
- microscopic roughness -> optical/material response;
- structural rib -> geometry;
- gold leaf on that rib -> material/color.

The guiding question is:

> **If the light moved, should the feature's shadow move because the feature has depth?**

If yes, the cause probably belongs in geometry.

If the feature should remain a color/composition change on the same surface, it belongs in material appearance.

See [OntoMath Driven Material Fields](../../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/OntoMath_Driven_Material_Fields/OntoMath_Driven_Material_Fields.md).

---

# 12. Material identity must survive lighting

The Sun Zone also exposed the opposite mistake: correct geometry with material response that destroys material identity.

A pale stone should not become dark mustard merely because it faces away from one source.

A metal should not become "yellow plastic" because its color is gold.

A white wall should not become emissive-looking because ambient and specular are too strong.

Architecture therefore needs a material hierarchy whose behavior under light is intentional.

For the Sun reference studied in 2026, the useful distinction was:

### Ivory structural stone
- high-value;
- low-chroma;
- mineral;
- satin-polished;
- broad diffuse body;
- gentle grazing sheen;
- subtle irregular variation;
- shadow shifts toward warm taupe / rose-gray, not olive-black.

### Gold trim
- higher chroma;
- richer body;
- directional highlight;
- architectural localization;
- stronger metallic/glinting response;
- never allowed to swallow the ivory hierarchy.

### Solar emissive
- localized;
- extremely high luminance;
- white-hot or pale center;
- saturated warm edge;
- a source, not merely another yellow material.

The lesson generalizes:

> **Do not encode material identity in base color alone.**

Use Earthcall's authored color and receiver-response machinery so the surface continues to look like itself while illumination changes.

---

# 13. Texture should not become another block system

Procedural material fields can fail in the same way primitives fail.

A repeating texture whose period is visible becomes a two-dimensional construction set.

Natural stone, plaster, wood, and weathered metal typically need:

- bounded variation;
- multiple scales;
- low-amplitude broad drift;
- sparse events;
- irregular spacing;
- no obvious grid;
- no single visible wavelength dominating the surface.

The Sun marble study produced a useful phrase:

> **Geology has memory but no grid.**

For marble-like stone, prefer:

- low-frequency cloudy variation;
- faint irregular veining;
- slight local warmth/coolness drift;
- very restrained contrast.

Avoid:
- global stripes;
- repeated seams;
- uniform noise amplitude everywhere;
- one trigonometric function visibly explaining the material.

---

# 14. Lighting is an architectural diagnostic

Do not treat lighting as the final beauty pass.

Use it to test whether the geometry is doing real work.

Move the light or camera and ask:

- Do cornices cast intentional shadow lines?
- Do recesses become legible?
- Do columns separate from the wall?
- Do arch reveals show depth?
- Do capitals catch light differently from shafts?
- Do buttresses project?
- Do finials maintain their silhouette?
- Does gold respond differently from stone?
- Does the material remain identifiable in shadow?

If the scene only looks convincing from one fixed illumination, the geometry or material model may still be carrying painted assumptions.

---

# 15. Bevels and edge response

Perfectly sharp mathematical edges are one of the fastest routes to "old 3D game" or "CG block" appearance.

A small bevel or fillet can do disproportionate visual work because it gives light a finite region over which to transition.

But bevels must be proportionate.

A tiny molding may need a tiny radius.  
A monumental plinth may tolerate a larger profile.  
A sharp carved incision may intentionally remain sharp.

Do not solve the Maple Block problem by rounding everything into soap.

The goal is **edge hierarchy**, not universal softness.

---

# 16. Intersections must look authored

Primitive composition often leaves accidental junctions:

- two solids penetrate each other;
- one stops half a centimeter before another;
- trim floats;
- a column passes through a cornice;
- a finial sits on a roof with no transition;
- a wall meets a tower with no joint logic.

These are perceptually expensive.

Every major junction should answer one of these:

- Is it continuous?
- Is it keyed?
- Is it nested?
- Is it recessed?
- Is it supported?
- Is it intentionally separated by a joint?
- Is one member penetrating another for a real structural reason?

If the answer is "they overlap because the SDF union makes them one," inspect the result anyway.

CSG legality is not architectural resolution.

---

# 17. Side view is the lie detector

Many AI-generated buildings are elaborate bas-reliefs.

The front view looks impressive because the model invests detail in x/y while barely authoring z-depth.

Then the Person walks sideways.

The Sun Zone's early rear citadel had this failure: strong facade idea, weak depth.

Every architectural pass should include:

1. frontal view;
2. three-quarter view;
3. side view;
4. close approach;
5. low angle;
6. preferably a movement sequence.

Ask whether the building still has hierarchy when the hero composition is gone.

A true 3D architect designs **volumes**, not screenshots.

---

# 18. Silhouette first, then shadow structure

If the silhouette is weak, internal detail will not rescue the scene.

If the silhouette is good but the shadow structure is flat, the scene will still feel toy-like.

A useful pass order is:

1. silhouette;
2. large voids;
3. depth staging;
4. projecting/recessed meso forms;
5. shadow-producing profiles;
6. material hierarchy;
7. micro-detail.

This prevents the common AI tendency to decorate a fundamentally weak mass.

---

# 19. Architecture should have a grammar

A building becomes coherent when a small number of motifs recur with transformation.

Examples:

- one arch ratio repeated at several scales;
- one cornice profile adapted to tower and arcade;
- one rib language reused around the oculus and portal;
- one taper law echoed in towers and finials;
- one material hierarchy repeated with differing emphasis.

This is not copy-paste.

It is grammatical transformation.

A coherent style often needs **fewer distinct motifs**, not more objects.

Before adding a new ornament, ask whether an existing motif can be transformed.

---

# 20. Hierarchy beats uniform detail

Do not make every surface equally ornate.

The eye needs:

- dominant;
- supporting;
- quiet.

For a monumental sacred building, a typical hierarchy might be:

- focal portal/oculus/altar: richest;
- main tower/frame: strong;
- side towers: supporting;
- arcades: rhythmic;
- plinth/causeway: quieter;
- background mass: quietest.

If every ledge, seam, post, and wall receives the same gold, glow, complexity, or contrast, nothing is sacred because nothing is subordinate.

---

# 21. A true Earthcall architect uses OntoMath as authorship, not wallpaper

OntoMath is not merely a way to produce fancy patterns.

It is the authored mathematical substrate.

Use it for:

- taper;
- profile;
- curvature;
- field composition;
- bounded color variation;
- material response;
- radiance;
- time variation;
- spatially meaningful transition;
- relationships whose parameters a Person may inspect or modify.

Do not use it merely because "a cosine makes stripes."

The mathematical expression should explain something about the authored world.

Good question:

> What property of this architectural member is this function expressing?

Bad answer:

> It looked cool in the shader.

---

# 22. Composition should minimize object count when one form is conceptually one form

The chess-board lesson applies to architecture too.

If one continuous wall, beam, bridge, or platform can be authored as one coherent body, do not fragment it into dozens of cubes merely because grid placement is easy.

Fragment when there are meaningful independent members:

- columns;
- movable panels;
- separate material bodies;
- structurally distinct sections;
- separately governed Singulars.

Do not fragment because the authoring script thinks in repeated loops.

One coherent authored field is often visually and computationally better than a cloud of tiny Objects.

---

# 23. But "one object" does not mean "one primitive"

This distinction matters.

A single Object can host a rich compound field.

That is one of the cleanest ways Earthcall can transcend primitive toy aesthetics:

> one semantic architectural member  
> -> one Singular/Object where appropriate  
> -> one compound authored field  
> -> many internal mathematical leaves/operators  
> -> one coherent surface.

This lets the author preserve semantic identity without forcing the visible form to reveal the implementation primitives.

---

# 24. Know when to stop changing geometry

The Suns learned this painfully.

Once:

- the massing works;
- the silhouette works;
- the depth works;
- the architecture survives side view;
- the meso forms read;
- the voids read;
- the building feels like a place;

then a remaining visual problem may belong to:

- material;
- lighting;
- exposure;
- indirect light;
- surface response;
- atmosphere;
- texture filtering;
- color hierarchy.

Do not rebuild a tower because marble looks brown.

Do not change arch geometry because gold response is weak.

Do not add detail because the scene lacks indirect illumination.

Grok's counsel during the Sun Zone saga was apt: let one form remain itself long enough to walk it.

A true architect can diagnose which layer is wrong.

---

# 25. Human render gates are part of authorship

The Person's eye provides evidence no headless test can.

At the end of each meaningful pass, stop and ask for a render when the next judgment is perceptual.

A good render gate has a precise question.

Examples:

- Does the tower now have real z-depth?
- Does the archivolt read as continuous rather than toroidal?
- Does the material remain ivory in shadow?
- Do the ribs catch light without becoming gold wallpaper?
- Does the floor reflect the solar source without looking wet?
- Does the side view still read as architecture?
- Does approaching the surface reveal more world?

Do not continue blindly through five passes before checking.

The repo's repeated temporal rollbacks, stale save shadows, and visual surprises make this especially important.

---

# 26. Nearness must reward inspection

This principle from the Cathedral work remains the strongest aesthetic acceptance criterion Earthcall has.

Far away: the Person sees a place.

Closer: the Person sees architecture.

Closer: the Person sees construction and material.

Closer: the Person sees craft.

Closer still: the Person sees authored surface detail.

At no point should approach reveal that the apparent structure was only paint, that the ornate capital was really three stock shapes, or that the stone was a repeating texture strip.

This is not a demand for infinite physical simulation.

It is a demand for **honest causal scale**.

Approximation is allowed.

Contradiction is what breaks the spell.

---

# 27. The architecture pass ladder

When an agent is asked to construct or improve an Earthcall building, use this ladder.

## Pass A — intent
Write down:
- what the place is;
- what it is for;
- how a Person approaches it;
- what emotion or hierarchy the architecture should express.

## Pass B — massing
Author:
- dominant mass;
- supporting masses;
- primary void;
- processional axis;
- skyline.

No micro-detail.

## Pass C — depth
Add:
- setbacks;
- projections;
- recesses;
- wall thickness;
- tower depth;
- real volumetric hierarchy.

Inspect side view.

## Pass D — meso-scale form
Add a small number of high-value compound forms:
- arches;
- archivolts;
- columns/capitals;
- buttresses;
- cornices;
- ribs;
- finials;
- carved recesses.

Avoid primitive spam.

## Pass E — material hierarchy
Separate:
- structural body;
- metal/trim;
- emissive elements;
- secondary materials;
- floor finish.

## Pass F — lighting witness
Move through:
- frontal;
- side;
- low;
- close;
- shadow.

Check whether geometry and materials remain truthful.

## Pass G — micro-detail
Only now:
- subtle bevel;
- veining;
- roughness variation;
- tiny relief;
- wear;
- inscriptions;
- controlled decorative accents.

## Pass H — inhabit
Stop editing.

Walk it.

---

# 28. Practical Earthcall guardrails

A good architectural pass must also be a good Earthcall engineering pass.

Before changing a Zone:

1. Inspect current canonical.
2. Inspect recent Zone history.
3. Inspect any open construction PR.
4. Confirm branch ancestry.
5. Confirm the intended save is authoritative.
6. Guard against stale `zone.ecform` shadowing newer `zone.json`.
7. Preserve unrelated authored witnesses exactly.
8. Preserve transforms and `targetRotation` agreement.
9. Check primitive axes when curved leaves are involved.
10. Mechanically inspect changed-file scope.
11. Do not regenerate a sacred save when a bounded patch is sufficient.
12. Stop when the next decision requires a Person's render.

These are not clerical details. Temporal rollback and stale serialized shadows can make an architect judge the wrong world.

---

# 29. What not to do

Do not:

- map one primitive to one architectural noun;
- keep adding decorative primitives when the silhouette is wrong;
- use names to pretend a primitive is sculpted;
- use texture to fake relief;
- use material color to fake depth;
- use geometry to encode pigment;
- apply global periodic stripes as "detail";
- make all surfaces equally ornate;
- make every edge equally rounded;
- make every object equally gold;
- make every material glossy;
- assume front view is sufficient;
- trust one screenshot;
- trust a headless test for beauty;
- rebuild geometry to solve lighting;
- invent a new C++ subsystem because one building needs a feature;
- introduce a new ShapeKind for a new architectural noun;
- let the implementation vocabulary become the design vocabulary.

---

# 30. Recovery recipes

## If it looks like a block toy
Remove details. Re-evaluate silhouette, negative space, depth, and meso-scale profiles.

## If it looks like a sandcastle
Increase depth articulation, projections/recesses, profile continuity, and material differentiation.

## If it looks plastic
Reduce hard specular dominance, broaden highlights, preserve diffuse mineral body, add small physically plausible edge response.

## If it looks flat from the side
Author actual z-depth and layered massing; stop treating the facade as the building.

## If it looks gift-wrapped
Remove periodic global masks. Attach material changes to known architectural members.

## If it looks busy
Reduce motif count. Establish dominant/supporting/quiet hierarchy.

## If it looks detailed but uncanny up close
Ask which visible effects claim physical structure that does not exist.

## If the architecture is good but the color is wrong
Stop touching geometry. Study material response and lighting.

---

# 31. The Sun Zone lessons, compressed

The Sun Zone is a useful fossil record because it went through many failure classes.

### Toybox
Primitive nouns remained visually legible.

**Lesson:** primitives are implementation basis, not architectural identity.

### Sandcastle
Massing improved, but forms remained too smooth, homogeneous, shallow, and monolithic.

**Lesson:** meso-scale depth and shadow structure matter.

### Sandstone
Geometry became architectural, but material hierarchy remained weak.

**Lesson:** form and material are separate stages.

### Plastic
Response became too glossy/synthetic at close range.

**Lesson:** material identity is a response function, not just a color.

### Gift wrapping
Periodic color masks ignored architectural meaning.

**Lesson:** mathematical elegance cannot replace semantic attachment.

### Later passes
Geometry became good enough that the remaining discrepancy was mostly material/lighting.

**Lesson:** know when the form is no longer the problem.

---

# 32. The Cathedral lessons, compressed

The Cathedral's uncanny valley provided the complementary case.

From far away, textures could imply intricate carving.

From close range, a Person discovered that the promised depth did not exist.

The strongest conclusion was:

> **Visual complexity is not representational truth.**

A single-color SDF can be more convincing than a richly textured flat box if the SDF's curvature is real and survives relighting and approach.

That is why true Earthcall architecture should bias toward causal geometry for features whose appearance depends on depth.

---

# 33. The architect's questions

Before declaring a form finished, ask:

### Form
- Can I still see the stock primitives?
- Does the silhouette have intentional rhythm?
- Are transitions continuous?
- Is the form deep enough in all axes?

### Space
- Are the voids designed?
- Does the approach reveal the focal hierarchy?
- Is there a meaningful processional or navigational sequence?

### Structure
- Do columns connect convincingly to what they carry?
- Do arches terminate into real supports?
- Do cornices project for a reason?
- Do buttresses, ribs, and frames actually articulate mass?

### Material
- Does stone stay stone under light?
- Does metal behave differently from stone?
- Are emissives localized?
- Is texture irregular without becoming noise?

### Light
- Does the geometry generate useful shadow?
- Do recesses stay legible?
- Do highlights reveal profiles rather than expose plastic?

### Approach
- Does the side view hold up?
- Does close inspection reveal craft?
- Is there an approximation cliff?

### Earthcall
- Is authored meaning in the world rather than a hardcoded renderer special case?
- Are OntoMath expressions inspectable and modifiable?
- Are save authority and branch ancestry correct?
- Did the pass preserve unrelated witnesses?
- Is the next judgment now human/perceptual?

---

# 34. Instructions for future robots

When Zach asks you to "make this place beautiful," do not begin by counting the primitives available.

Do this instead:

1. Identify the architectural intent.
2. Inspect the actual current Zone.
3. Find the strongest and weakest viewing scales.
4. Diagnose whether the current failure is massing, geometry, material, lighting, or staging.
5. Choose the smallest layer that can fix the diagnosed problem.
6. Prefer a few high-value compound forms over many decorative primitives.
7. Let SDF/OntoMath express continuous form.
8. Let Materials/OntoMath express surface behavior.
9. Let radiance express illumination.
10. Keep those causes distinct.
11. Preserve authored world truth.
12. Render.
13. Walk closer.
14. Stop when the Person must judge.

If your proposed improvement can be summarized as:

> "I added twelve boxes, six cylinders, four cones, and three spheres"

you are probably still designing the implementation rather than the architecture.

If your proposed improvement can be summarized as:

> "I deepened the portal reveal, gave the piers a continuous taper-to-capital profile, made the archivolt a bounded nested frame around the actual opening, and profiled the cornice so moving light produces a deliberate shadow line"

you are thinking like an architect.

---

# 35. Final doctrine

The goal is not to hide primitives better.

The goal is to stop thinking in primitives at the level where the Person experiences the world.

Earthcall already possesses the seeds of a different way to build:

- continuous authored mathematics;
- compound implicit fields;
- patches and future richer surface representations;
- semantic Singular identity;
- Material as authored being;
- OntoMath-driven appearance;
- authored radiance;
- Laws and Relations;
- a Person who can walk up to the result and test whether it survives approach.

Use them together.

A true Earthcall building should not feel like a collection of shapes someone arranged.

It should feel like a place whose forms, materials, light, and spatial hierarchy **belong to one another**.

That is the threshold beyond the Maple Block Temple.

And the acceptance test remains wonderfully simple:

> **Walk closer. The world should become more itself.**
