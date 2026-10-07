# SUN HANDOFF — OntoMath Radiance Field Phase 2

**From:** GPT-5.6 Sol ("The Sun")  
**To:** the next GPT-5.6 Sol / Sun working in Earthcall  
**Person directing the work:** Zach  
**Date:** 2026-09-19 (America/Los_Angeles)  
**Repository:** `zhangzachary834-commits/Earthcall`  
**Phase-2 branch:** `sol/ontomath-radiance-field-20260919`

---

## STOP: DO NOT START OVER

This phase already has a real implementation on the branch.

At handoff time:

- branch head before this handoff document: `0727a11936c0c3fd2150f5f4d8a0d98b25790d21`
- current `sync-from-earthcall-main`: `c01f7db757fdce475bc1c93bbdf303d43eb509fd`
- merge base: `4fac467838b090f22e0874548f4d48600246cc69`
- branch is **7 commits ahead and 112 commits behind** current main
- latest branch Actions run at `0727a119...`: **SUCCESS**
  - workflow: `Earthcall focused CI`
  - run id: `35490323960`

The first thing the next Sun should do is inspect the newest mainline and sync this branch **without overwriting current renderer/color-field work**. Do not assume the green result on the stale base proves conflict-free mergeability against current main.

Use bounded reads. Do not feed giant workflow logs or whole-repo dumps to yourself unless absolutely necessary. The "Big Chungus principle" applies: targeted file/range/search reads first.

---

# 1. What already merged before Phase 2

PR #245, **"Add authorable Sun Zone and radiant FieldNode renderer bridge"**, is already merged.

Merge commit recorded by GitHub:

`968844374319c7ee6046e84227fc98a52aa4469b`

That first phase established:

1. `saves/zones/Sun/zone.json`
2. a real Zone-owned `geom::FieldNode` named `sun.light-field`
3. authored light vocabulary on that FieldNode:
   - `light.source`
   - `light.enabled`
   - `light.color`
   - `light.intensity`
   - `light.ambient`
   - `light.diffuse`
   - `light.specular`
   - attenuation vocabulary
4. `Rendering::AuthorableLightState`
5. `EngineRender` resolving authored light state into the existing renderer boundary
6. focused `authorable_light_contract_test`
7. the long historical joke:
   `agent intercom/robots having fun and messing around (and Zach)/the_suns_first_github_action_was_to_patch_the_sun.md`

Historical truth boundary from Phase 1:

- the Sun was no longer represented as a skinned/hardcoded "Sun Object" illumination ontology
- the source lived on a continuous FieldNode
- but the FieldNode's actual scalar OntoMath mathematics was not yet used as the spatial radiance function in WGSL

Phase 2 is the rung that closes that gap.

---

# 2. What Phase 2 is trying to mean

The architectural target is:

```
geometry OntoMath -> sdfEval(p)
material/color OntoMath -> sdfColor(p)
radiance OntoMath -> lightRadiance(p)
```

The renderer should increasingly act as a modality that evaluates authored world mathematics, rather than being a second secret ontology with hardcoded graphics nouns.

Do **not** introduce:

- `class Light`
- `SunKind`
- point/spot/directional-light ontology enums
- a second shader expression language
- a magic shader-only falloff law disconnected from authored OntoMath
- a fake "field" whose AST persists but never reaches rendering

Reuse the existing OntoMath -> WGSL machinery that the color-field work established.

---

# 3. Current Phase-2 commits already on the branch

The branch has these seven Phase-2 commits, in order:

1. `6d3db6d77161173f38f9d0820de2cade32a760c6`
   **expose authored radiance field at renderer boundary**

2. `d121a67421b2a8fbf8d56677dd26e469d54c793b`
   **bind active Zone radiance AST to renderer**

3. `49307f4e8ad6147f330d456ba1f8a67d5ea737fc`
   **accept authored radiance expression in SDF compiler**

4. `85d8831382ba7343c844125e25ae1fd9e760e0af`
   **compile OntoMath radiance field into SDF WGSL**

5. `65d06494cc7d0d7e7b43dc8aaee735a1a08bb775`
   **invalidate SDF programs on radiance AST edits**

6. `047e9d0a79d6ab8f0e60a5e4a0ec8fe250a47072`
   **feed authored radiance state into WebGPU SDF lighting**

7. `0727a11936c0c3fd2150f5f4d8a0d98b25790d21`
   **author inverse-distance OntoMath radiance field in Sun Zone**

Do not reimplement these from scratch. Audit them against current main and continue.

---

# 4. Files changed by Phase 2

At handoff time, the branch-vs-main comparison identifies these Phase-2 files:

- `saves/zones/Sun/zone.json`
- `src/Singularity/Core/EngineRender.cpp`
- `src/Singularity/Screen/Renderer.hpp`
- `src/Singularity/Screen/WebGPU/SdfWgsl.cpp`
- `src/Singularity/Screen/WebGPU/SdfWgsl.hpp`
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp`
- `src/Singularity/Screen/WebGPU/WebGpuRenderer.hpp`

Before editing, compare each of these against newest current main because the branch is 112 commits behind.

---

# 5. Renderer boundary now carries the authored AST

`Renderer.hpp` now has an optional borrowed radiance expression:

```cpp
void setRadianceField(const OntoMath::Piecewise* expr, uint64_t revision) {
    _radianceExpr = expr;
    _radianceRevision = revision;
}

const OntoMath::Piecewise* radianceExpr() const { return _radianceExpr; }
uint64_t radianceRevision() const { return _radianceRevision; }
```

Intent:

- this is mechanism state at the renderer boundary
- it does **not** introduce a Light domain noun
- the AST remains authored world data owned by the active Zone's FieldNode
- the renderer only borrows the expression and records a content fingerprint

The next Sun should verify newest main has not added a better generalized mechanism for authored field-program identity before preserving this exact API.

---

# 6. EngineRender binds active Zone mathematics

`EngineRender.cpp` currently:

1. resolves `AuthorableLightState` from the Zone's `spatialRoot()`
2. calls `setLight(...)` for placement/RGB source channels
3. calls `setLightingEnabled(...)`
4. if the scalar field is AST mode and non-empty:
   - serializes `astDefinition.toJson().dump()`
   - hashes the serialized content
   - calls `setRadianceField(&root->field->astDefinition, hash)`
5. otherwise clears radiance field state
6. when no persistent authored light exists, also clears radiance field state

Current code shape:

```cpp
if (root->field &&
    root->field->mode == OntoMath::ScalarField::EvaluationMode::AST &&
    !root->field->astDefinition.pieces.empty()) {
    const std::string radianceJson = root->field->astDefinition.toJson().dump();
    const uint64_t radianceRevision =
        static_cast<uint64_t>(std::hash<std::string>{}(radianceJson));
    currentRenderer().setRadianceField(&root->field->astDefinition,
                                       radianceRevision);
} else {
    currentRenderer().setRadianceField(nullptr, 0);
}
```

This exists because `FieldNode::AstBridge` already makes an authored `field.ast` switch the ScalarField into AST mode, but ScalarField itself did not expose a dedicated revision counter.

Audit question for next Sun:

- does current main now have a canonical revision/content identity for OntoMath expressions?
- if yes, use it instead of duplicate JSON hashing
- if no, content hashing remains a reasonable correctness-first invalidation seam

Never make pointer identity authoritative for AST changes.

---

# 7. SDF compiler now accepts radianceExpr

`SdfWgsl.hpp` currently extends:

```cpp
Program compile(
    const geom::SdfNode& root,
    const geom::FieldNode* fieldNode = nullptr,
    const OntoMath::Piecewise* colorExpr = nullptr,
    const OntoMath::Piecewise* radianceExpr = nullptr);
```

`SdfWgsl.cpp` uses the same existing OntoMath emitter family used for authored material color.

This is important doctrine:

**There is not a separate radiance math language.**

Color and radiance both flow through the same `MathNode` / `Piecewise` -> WGSL compiler infrastructure.

The compiler still retains its refusal behavior. Unsupported authored operations must refuse rather than silently returning fake zero.

---

# 8. WebGPU cache invalidation has radiance identity

`WebGpuRenderer` memoized SDF programs now compare radiance state in addition to geometry/material state.

Current condition includes:

```cpp
entry.revision == memoRevision &&
entry.colorRevision == mat.colorRevision &&
entry.radianceRevision == radianceRevision() &&
entry.colorExprPtr == mat.colorExpr.get() &&
entry.radianceExprPtr == radianceExpr()
```

On compile:

```cpp
prog = sdfwgsl::compile(
    field,
    fieldNode,
    mat.colorExpr.get(),
    radianceExpr());
```

And the memo entry stores:

- `radianceRevision`
- `radianceExprPtr`

Purpose:

- editing the same AST object through Law/PropertyPath must invalidate generated WGSL
- merely keeping the same pointer must not make old shader code authoritative

Audit after syncing current main: inspect whether the SDF memo structure changed in those 112 commits before resolving conflicts.

---

# 9. WebGPU SDF lighting now receives source channels

The Phase-2 branch extends `SdfGlobalUniforms` with:

- `lightAmbient`
- `lightDiffuse`
- `lightSpecular`
- `lightControl` where x represents lighting enabled
- existing `lightPos`
- existing camera/far-plane state

This is important because before Phase 2 the WebGPU SDF shader consumed `lightPos` but ignored the renderer's authored RGB source channels and `lightingEnabled`.

The shader-side implementation was designed to preserve the legacy visual baseline instead of naïvely multiplying two copies of the historical `0.2/0.8/1.0` coefficients.

Remember the trap:

- `RenderMaterial` defaults: ambient=.2, diffuse=.8, specular=1
- renderer light defaults: ambient=.2, diffuse=.8, specular=1

Blindly multiplying them turns ambient .2 into .04 and changes every old SDF scene.

The Phase-2 work therefore needs to be audited for **baseline preservation** after current-main sync.

---

# 10. The Sun Zone now has a real authored OntoMath radiance AST

Current branch `saves/zones/Sun/zone.json` uses:

```json
"field": {
  "mode": "AST",
  "baseDensity": 1,
  "frequency": 1,
  "amplitude": 1,
  "ast": {
    "input": "x",
    "pieces": [
      {
        "mathNode": {
          "op": 23,
          "children": [
            {
              "op": 0,
              "scalarForm": {
                "terms": [
                  { "c": 1, "factors": {} }
                ]
              }
            },
            {
              "op": 0,
              "scalarForm": {
                "terms": [
                  { "c": 1, "factors": {} },
                  { "c": 0.005, "factors": { "x": 2 } },
                  { "c": 0.005, "factors": { "y": 2 } },
                  { "c": 0.005, "factors": { "z": 2 } }
                ]
              }
            }
          ]
        }
      }
    ]
  }
}
```

`MathNode::Op::Div == 23`.

Therefore the authored scalar field is:

```
R(p) = 1 / (1 + 0.005*x^2 + 0.005*y^2 + 0.005*z^2)
```

This is an actual continuous inverse-distance-like falloff field expressed in existing OntoMath.

No new attenuation operation was introduced.

---

# 11. The Sun Zone contains a visible witness object

Current branch also adds:

`sun-radiance-witness-cube`

using material:

`sun.witness.white`

This was added so Zach has an actual rendered object in the Sun Zone on which the field can be visually witnessed, rather than a save containing only invisible source state.

Important audit question after sync:

- verify this object still boots and is drawn through the WebGPU SDF path that actually consumes `lightRadiance`
- if `shapeKind: 0` or current renderer routing means it goes through a mesh path instead, the witness may not prove the SDF radiance path visually
- if so, author a witness object that definitely uses the exact path being tested, or expand radiance-field support to the mesh compiler in a later rung

Do not claim "I can see the OntoMath radiance field affecting the cube" until the actual draw path proves it.

---

# 12. Current verified CI state

At branch head:

`0727a11936c0c3fd2150f5f4d8a0d98b25790d21`

GitHub Actions:

- workflow: `Earthcall focused CI`
- run ID: `35490323960`
- conclusion: **SUCCESS**

This is strong evidence that the Phase-2 branch itself compiled and passed the focused workflow on its then-current base.

It is **not** evidence that it merges cleanly against current main because main is now 112 commits ahead.

After syncing, rerun CI and re-audit failures rather than assuming red/green provenance.

---

# 13. Historical CI isolation from Phase 1

Before #245 merged, there was a misleading red focused job.

We isolated it and proved:

- isolated `authorable_light_contract_test`: **1/1 PASS**
- remaining focused CPU suite with one known-broken target omitted: **30/30 PASS**
- Slow Adapter independent-clock/perf job: **PASS**
- the only red target at that time was `person_serialization_test`
- current main independently failed it with the same:
  `TempSaveRoot` undeclared compile error

That pre-existing main failure was later addressed by PR #247 before the Phase-2 branch was created.

Do not resurrect old conclusions about #245 being red.

---

# 14. Big Chungus incidents / operational lessons

Several delays in the previous Sun session were caused by reading overly large logs/files.

Zach explicitly wants periodic visible updates during long tool work so he does not see:

`fetching ordinary job logs { }`

for a long time and wonder whether the model was swallowed by the abyss.

Operational rules for the next Sun:

1. Send a short visible heartbeat every ~2-3 tool calls / ~15 seconds during long work.
2. Prefer:
   - workflow job step summaries
   - targeted `fetch_file` line ranges
   - code search for exact symbols
   - compare metadata
3. Avoid full job logs unless a specific failed job genuinely requires its exact error.
4. Avoid whole massive files when a bounded excerpt will answer the question.
5. If a connector times out, inspect branch ref before retrying a write.
6. Never claim a commit landed until the branch ref proves it.
7. Never claim CI is green until the relevant run/job proves it.
8. Never merge without Zach's explicit approval.

The user jokingly calls these giant operations "Big Chungus" / "the abyss." Treat the joke seriously as an engineering constraint: bounded retrieval protects the work.

---

# 15. One historical tooling bug worth remembering

While editing `.github/workflows/earthcall-ci.yml`, a JavaScript `String.replace` replacement string ended in `)$'`.

In JavaScript replacement strings, `$'` means "insert the suffix after the match."

That accidentally duplicated ~111 workflow lines and produced malformed CI.

The bug was caught and repaired.

Lesson:

- when replacement text contains `$`, use a replacement callback/function rather than raw replacement-string semantics

Do not blame GitHub if the generated blob itself is wrong.

---

# 16. Current main has raced far ahead

This is the immediate risk.

At handoff:

- phase-2 branch merge base: `4fac467...`
- current main: `c01f7db...`
- branch: **112 commits behind**

Therefore:

## FIRST ACTION FOR NEXT SUN

1. read current `AGENTS.md`
2. inspect current main's versions of only:
   - `Renderer.hpp`
   - `EngineRender.cpp`
   - `SdfWgsl.hpp/.cpp`
   - `WebGpuRenderer.hpp/.cpp`
   - `saves/zones/Sun/zone.json`
3. compare against the seven Phase-2 commits
4. read any current-main communication docs specifically touching:
   - material/color fields
   - rendering
   - SDF WGSL compilation
   - light/radiance
5. sync via normal merge/replay preserving current-main architecture
6. do **not** force-push unless Zach explicitly authorizes it
7. resolve conflicts by keeping current-main improvements and reapplying the Phase-2 semantic delta

The branch has valuable work; do not throw it away merely because it is stale.

---

# 17. Tests the next Sun should add or verify

The current green CI is useful, but Phase 2 deserves more semantically targeted witnesses.

At minimum:

### A. Sun save hydration witness

Load `saves/zones/Sun/zone.json` and prove:

- spatial root exists
- scalar field mode is AST
- AST has a piece
- AST evaluates on CPU
- value near source > value farther away
- authored `light.source` remains true

### B. SdfWgsl compiler witness

Construct a simple SDF plus a radiance `Piecewise` and call:

`sdfwgsl::compile(..., radianceExpr)`

Prove:

- `Program.ok == true`
- emitted WGSL contains the generated radiance evaluation function
- authored radiance math appears through parameterized emitted code, not a hardcoded shader constant
- no radiance expr preserves legacy source behavior

### C. invalidation witness

Prove that:

- same geometry/material + same radiance AST revision reuses memoization
- editing `field.ast` changes the content identity
- changed radiance identity requires compiler refresh

If private WebGpuRenderer cache internals make this hard to unit-test, extract only the minimal pure identity/helper seam needed; do not expose the whole backend.

### D. visual/offscreen witness if possible

Render two surfaces at different distances or sample a known SDF object in an offscreen WebGPU test.

Prove image-space consequence:

- nearer point receives stronger authored radiance than farther point
- changing the authored AST changes pixels

This is the strongest proof that the field is not merely compiled but actually used.

---

# 18. Important truth boundaries

Do not overclaim any of these until verified after current-main sync.

### Already implemented on the Phase-2 branch

- real AST-mode Sun scalar field
- inverse-distance-like continuous authored function
- active Zone radiance AST passed to Renderer
- content fingerprint for AST invalidation
- radiance expression accepted by SDF WGSL compiler
- radiance identity included in SDF program cache validity
- WebGPU SDF global uniforms include authored source channels
- latest stale-base focused CI is green

### Still must be verified

- exact shader equation after newest-main sync
- visual witness is definitely going through the radiance-consuming SDF path
- current main does not already offer a more canonical field revision API
- shader/color-field developments in the 112 new commits do not supersede some Phase-2 plumbing

### Not yet safe to claim globally

- every WebGPU mesh is illuminated by arbitrary OntoMath radiance fields
- OpenGL evaluates arbitrary authored spatial radiance ASTs
- all geometry modalities share identical field-light semantics
- multiple radiance sources are composed
- shadows/global illumination are solved
- light transport is a full physical renderer

This phase is the first true authored spatial radiance rung, not the completion of lighting physics.

---

# 19. Architectural continuation after this phase

Once the SDF path is proven and merged cleanly, likely next rungs are:

1. **Mesh parity**
   - make ordinary WebGPU mesh shading sample the same authored radiance semantics
   - ideally reuse generated code or a shared compiled representation rather than duplicating the equation manually

2. **Color × radiance composition**
   - authored `sdfColor(p)`
   - authored `lightRadiance(p)`
   - explicit composition rules

3. **Multiple authored source fields**
   - do not create a hardcoded fixed light array if ontology already permits Formations/Relations of radiant Fields
   - define composition mathematically / through authored Laws

4. **Volumetric colored radiance**
   - current volumetric scatter historically had hardcoded white
   - color/radiance fields should eventually govern that too

5. **Prophetic/JIT invalidation**
   - field AST changes should incrementally recompile only affected GPU programs
   - current content-hash invalidation is correctness-first; Prophetic relevance can improve scheduling later

6. **Second-Nature authoring**
   - Persons should be able to author/change the radiance field through the same Law/property authoring interfaces, not hand-edit JSON

---

# 20. User intent / vibe

Zach is extremely excited about the convergence of:

- geometry fields
- color fields
- lighting/radiance fields

He specifically reacted to the idea that the other agents' rendering and color work now provides the precedent for lighting fields.

The architectural spirit he is aiming for is roughly:

> Earthcall should not accumulate conventional hardcoded graphics categories when authored mathematics can govern the phenomenon directly.

Do not interpret that as license to break renderer compatibility or remove necessary machine mechanisms. Preserve the distinction:

- ontology/meaning is authored
- renderer/compiler is mechanism

---

# 21. Destructive-action boundary

Zach wants destructive operations to require his approval.

Do not:

- merge a PR
- delete a branch
- force-push
- rewrite history
- remove save content
- overwrite other agents' current-main work

without explicit direction.

Creating commits/branches/PRs is fine when part of the requested workflow, but final merge remains Zach's decision unless he explicitly asks you to merge.

---

# 22. Suggested next sequence

Do this, in order:

1. send Zach a short heartbeat saying you read this handoff and are syncing current main
2. read current `AGENTS.md`
3. inspect current main's seven conflict-sensitive files
4. inspect the 112-commit delta only for rendering/color/SDF/radiance overlap
5. merge current main into `sol/ontomath-radiance-field-20260919` normally
6. resolve conflicts preserving current-main improvements
7. add/repair the targeted Phase-2 witnesses
8. run isolated radiance compiler/save tests first
9. then run focused CI
10. distinguish pre-existing failures from branch regressions
11. visually verify the Sun Zone if the available test/runtime path supports it
12. update/create the Phase-2 PR
13. report exact commit SHAs and CI evidence
14. do not merge until Zach says to

---

# 23. Short version for a Sun waking up confused

**#245 is already merged. Do not redo it.**

The current Phase-2 branch already contains a real authored inverse-distance OntoMath Sun radiance field and WGSL plumbing, and its last branch-head CI was green.

The problem is not "build Phase 2 from scratch."

The problem is:

> **Bring those seven semantic commits forward across 112 newer mainline commits, prove the radiance AST still compiles/evaluates/affects rendering, add strong focused witnesses, then prepare the PR.**

The core equation currently authored by Zach's Sun Zone is:

```
R(p) = 1 / (1 + 0.005*x^2 + 0.005*y^2 + 0.005*z^2)
```

The intended renderer architecture remains:

```
FORM  -> authored field
COLOR -> authored field
LIGHT -> authored field
```

Do not let Big Chungus convince you to start over.

---

**Signed:** GPT-5.6 Sol — The Sun  
**Handoff state:** Phase 2 materially implemented, stale against fast-moving main, last branch CI green, ready for careful sync + verification.
