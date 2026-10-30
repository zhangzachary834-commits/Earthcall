# SUN RUNG 9 CONSTITUTION — Authored receiving-surface / material response




**Date:** 2026-09-24
**Repository:** `zhangzachary834-commits/Earthcall`
**Live canonical audited:** `sync-from-earthcall-main@4ee8f5a5bb7d5201d7139cdb5c5df3132439d685`
**Rung-9 branch:** `sol/rung9-material-response-20260924`

## Gate cleared first

PR #366 is merged. Its exact tested head `4d68165a64f164bcca03b4b8884abbef4030ceb0` passed focused CI run #3109 and landed through merge commit `1d492ef403032e8f6c1b3ce72c37ec6e0a0567c4`. Current canonical has advanced beyond that landing to `4ee8f5a5bb7d5201d7139cdb5c5df3132439d685`; this branch begins from that newer canonical rather than replaying stale Rung-9 history.

The #366 invariants remain prerequisites: absent volume phase is Phi=1; imported source rho/chi/alpha retain source time and invalidation authority; Sparkly mist/Sanctuary tribunals stay in focused CI; `volume.occluder.sdf` remains local volumetric transport geometry rather than silently becoming global Rung-8 visibility.

## Concrete renderer-owned fossil

The audited gap is real and bounded.

`Material : Singular` already owns `baseColor`, `opacity`, `ambient`, `diffuse`, `specular`, `shininess`, and `colorExpr`; the scalar compatibility fields are PropertyPath/Law-addressable and serialized. But the *response mathematics* is not authored by Material.

The WebGPU mesh shader currently hardcodes:

```
L = normalize(lightPos - worldPos)
V = normalize(eyePos - worldPos)
H = normalize(L + V)
diff = max(dot(N,L),0)
lit = ambient + diffuse*diff
spec = specular * pow(max(dot(N,H),0), shininess) * step(...,diff)
rgb = baseColor * texel * lit + vec3(spec)
```

Thus authors may change coefficients, but cannot author the mathematical function those coefficients inhabit. That is exactly the Rung-9 fossil. Existing substrate does **not** truthfully supply the full invariant, so Rung 9 is not retired as unnecessary.

## Independent invariant

Rung 9 establishes a receiver-owned authored response function, conceptually

`f_r(material, normal, incomingDir, outgoingDir, ...)`.

Its semantic authority belongs to the existing Material being. It is independent of:
- source emission `E_i = rho_i * chi_i * alpha_i`;
- derived visibility `V_i`;
- volume density/extinction/scattering/chroma/emission/phase;
- renderer execution strategy.

A source or blocker may change the *inputs or contribution reaching the receiver*. It must not rewrite the receiver's response mathematics.

## Minimal representation

Do **not** add a Material/BRDF ontology kind.

Add one optional authored OntoMath response AST on `Material`, provisionally named `responseExpr`, using the smallest existing expression vocabulary that can lawfully consume admitted receiver variables. Reuse existing directional mathematical machinery where it already represents normal/incoming/outgoing vectors; do not clone the volume-phase authority.

The response compiler owns a receiver-specific admission environment. Volume phase and material response may reuse generic OntoMath nodes, but neither may smuggle the other's semantic variables or defaults.

If the current OntoMath vocabulary cannot express the exact legacy mapping without a small generic mathematical primitive, add only that generic primitive; do not encode "BlinnPhong", "BRDF", "Lambert", or another domain noun as a privileged ontology operation.

## Compatibility law

Absence of authored `responseExpr` means **exact legacy material behavior**, not zero response and not a new renderer aesthetic.

Compatibility is the existing mapping:
- `baseColor` / texture remain albedo inputs;
- `ambient`, `diffuse`, `specular`, `shininess` retain their existing values, PropertyPaths, save/load behavior, and rendered result;
- the current Blinn-Phong equation is the compatibility realization only.

Once `responseExpr` is authored, its admitted mathematics is receiver truth. Compatibility coefficients remain stored/addressable state but must not secretly alter authored response unless the authored AST explicitly references admitted material parameters.

## PropertyPath, persistence, and time

`responseExpr` belongs on Material beside `colorExpr` and must round-trip through Material JSON if persisted. Expose it through Material's existing Property system using the established serialized-AST bridge pattern unless a more strongly typed existing Property bridge is already present.

A successful PropertyPath/Law mutation must bump the response/content revision used by renderer invalidation.

Timeline is admitted only if Material can truthfully obtain a Material-owned temporal coordinate through existing Singular/Timeline machinery. Do not borrow source time, medium time, or a renderer global merely to make a time test pass. If no honest Material Timeline path exists in this bounded rung, document time as not yet admitted rather than fabricating one.

## Structural versus numeric invalidation

Response compilation must distinguish:
- **structural edit:** AST operation/topology/admitted-variable layout changes -> invalidate/recompile only the relevant material response structure;
- **numeric-only edit:** parameter/leaf value changes with unchanged structure -> refresh response parameters without regenerating WGSL where the existing compiler substrate supports this distinction.

Source rho/chi/alpha revisions, blocker/visibility changes, and volume-field revisions are not response-structure revisions. They may invalidate transport/lighting work, but must not force rewriting the material-response program merely because illumination changed.

Conversely, editing a receiver response must not mutate or recompile source rho/chi/alpha or volume phase.

## Refusal law

Unsupported response math must fail admission/compilation explicitly and must not leave stale previously compiled output masquerading as the new authored response. The caller may report/fallback according to an explicit compatibility policy, but it must not silently retain an obsolete authored program.

## Native / falsifying witnesses

Rung 9 is not complete without focused evidence for the supported minimal design:

1. **Exact legacy compatibility:** absent response AST reproduces existing default/legacy material result.
2. **Receiver independence:** two surfaces under the same source and visibility differ solely because their authored response differs.
3. **Source separation:** response edits do not mutate/recompile source rho/chi/alpha; source edits do not rewrite response mathematics.
4. **Visibility separation:** blocker/visibility changes do not rewrite response mathematics.
5. **Numeric reuse:** numeric-only response edits refresh parameters while reusing compiled structure where possible.
6. **Structural invalidation:** response topology edits invalidate the relevant response program, not unrelated source/volume programs.
7. **Persistence / Law:** response AST survives Material save/load and PropertyPath/Law mutation.
8. **Timeline:** required only if an honest Material-owned temporal coordinate is admitted in this rung.
9. **Refusal:** unsupported AST fails without stale authored output.
10. **Native WebGPU/pixel evidence:** same incident conditions, materially different pixels due solely to authored receiver response.
11. **Regression gates:** preserve Rungs 3–8 and Volumetric V1–V5/Sparkly focused gates.

## Explicit non-goals

Rung 9 does not:
- implement indirect/global illumination;
- implement Rung 10 or Rung 11;
- promote local volume occluders into global visibility;
- invent a BRDF class hierarchy or new Material kind;
- require physically based energy conservation unless the admitted authored math explicitly chooses it;
- replace source emission, visibility, or volume phase semantics;
- restart Rung 8 proof acceleration (#315);
- optimize all renderer lighting paths merely because response is now authorable.

## First implementation cut

Start at the smallest semantic seam: Material ownership + persistence/PropertyPath + a receiver-response compiler/admission unit with explicit structural identity and parameter collection. Then wire that result into the WebGPU receiving-surface path while preserving exact absent-expression compatibility. Add compiler/cache witnesses before broadening native rendering coverage.

Do not begin Rung 10 from this document.


---

## 2026-10-19 — Zach-requested broadcast: Beyond the Maple Block Temple

**To:** all agents authoring 3D places/forms or maintaining their geometry, materials, and illumination.  
**From:** Codex / GPT-6 (variant unexposed) / session `01a155b8-e5b4-7b72-a24b-26be3fd69da2` / 2026-10-19T13:31:23-07:00.  
**Human commission:** Zach: “BROADCAST TO aLL THE THREADS OF ROBOTS MAKING 3D STUFF ABT THIS DOC.”

Read [**Beyond the Maple Block Temple: A field guide to true Earthcall 3D architecture**](../../../docs/architecture/Design/BEYOND_THE_MAPLE_BLOCK_TEMPLE_TRUE_EARTHCALL_3D_ARCHITECTURE.md) before your next relevant 3D authoring pass. **GPT-5.6 Sol (the Sun)** consolidated it from the Sun Zone construction, Cathedral work, **Zach's repeated visual corrections**, and the geometry/OntoMath doctrine; this message relays that work rather than claiming its authorship. The guide landed in `5af26c0be`; `50d305f02` added architecture-index routing.

**The failure:** “the available primitive vocabulary quietly becomes the design vocabulary.” A cylinder renamed a column, a prism described as wings, or a sphere with a poetic name has not acquired the missing architectural form. **“Primitives are brushstrokes, not nouns.”**

Carry these lessons into the next commissioned pass:

- Establish massing, silhouette, voids, and real depth before ornament; inspect front, three-quarter, side, and close views.
- Build the often-missing **meso-scale**: continuous profiles, capital transitions, deep reveals, nested archivolts, ribs, cornices, and shadow-producing recesses; use compound SDF/OntoMath forms where appropriate rather than multiplying stock decorations.
- Keep geometry, pigment, material response, and illumination as distinct authored causes; gild a meaningful architectural member rather than wherever an arbitrary periodic mask fires.
- Diagnose the failing layer: once form works, correct material/lighting instead of rebuilding sound geometry.
- Render and inspect the actual authored place; preserve stable identity, authorship, unrelated work, save continuity, and existing numerical/authority boundaries. The guide is architectural doctrine, not evidence that an unrun tool works or permission to alter anyone's save.

Start with **§§1–5, 9–12, 17–18, 24–27, and 30–35** as relevant to your pass. For renderer-only work, carry the aesthetic criteria into authoring support without introducing domain-specific renderer behavior or abandoning exactness/performance gates.

**Acceptance maxim: “Walk closer. The world should become more itself.”**

This is a reading/coordination broadcast; continue your existing commission. It requests no unsolicited rebuild, world edit, or new implementation campaign.
