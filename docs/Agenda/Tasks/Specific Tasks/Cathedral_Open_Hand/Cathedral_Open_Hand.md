# The Court of the Open Hand

Codex / GPT-6 Astra · session `01a07eb3-8ee7-7aa3-8b34-65fea2f4cd44` · 2026-09-19T12:21:43-07:00.

## Commission and authorship

Zach asked Astra to make an original place in the Cathedral Zone, explicitly without overlapping or editing existing work. His earlier description of the hand internalizing an art, and of the channel from intention to actual becoming whole and human, supplied the governing intention. The Court's composition, palette, architectural arrangement, and opening sculpture are Astra's interpretation of that commission. It is an addition to the same Zone, not a replacement Cathedral.

The First Mover is **Codex / GPT-6 Astra**. The authorizing Person recorded in the world is **Zach**. The three Laws name Zach as their actual Person author under this commission, at authority zero. Every Object carries persistent agent, session, timestamp, and authorizing-Person properties. Objects, Materials, and Law provenance use `commissioned-under` Relations to the stable Lexeme `lexeme.astra.openhand.commission`, whose text names Zach's request. That Lexeme represents the commission, not a Person. No AI Person was created.

## Encounter

The court is centered at **(44, 0, 10)**, on the positive-X side of the Cathedral. Its reserved volume is X=31.5–56.5, Y=−0.3–13, Z=0–24. Existing authored geometry is checked against that volume before installation. The pond lies toward positive Z and the color-field cloister toward negative X; neither was moved or repainted.

Approach from **(44, 2, 26)** facing negative Z, or admire it from **(65, 15, 38)** while flying. A dark circular foundation carries ivory terraces, fine bronze inlays, a compass of raised lines, and seven open lancets. Stone seats flank the approach. Twelve carved leaves gather around a suspended blue seed, enclosed by three thin golden meridians; a separate open crown marks the place from a distance.

Click the **pearl touchstone at (44, 1.47, 19.35)**. The leaves spread from radius 2.45 to 4.8 meters and incline outward through 34 degrees, revealing the seed. Click again to gather them. The same gesture can reverse the movement midway. There are no new screen overlays and no added audio.

This is an authored kinetic sculpture, not a new interface class. `intention` and `aperture` are ordinary readable/writable properties on the touchstone. The gesture toggles intention; a Map computes an exponential approach using `time.delta`; the leaves read that aperture through another Map. A long frame cannot make the interpolation overshoot. The actual transforms change. Each Object owns its Material from birth, using eight palette choices, four with bounded OntoMath color expressions. Openings, stone edges, and leaf cavities are geometry, not painted shadows. Slender forms use spheres or circular CSG in nonuniformly transformed local coordinates, keeping local distance steps conservative instead of relying on an approximate elongated-ellipsoid distance.

## Files and stewardship

- Native identity: `saves/zones/Cathedral of the Living Logos/zone.json`.
- Session fallback: `saves/worlds/cathedral_of_the_living_logos.json`; its existing camera and other session state are retained.
- Three new shared roots: `saves/laws/law-astra-openhand-{gesture,approach,unfold}/law.json`.
- Additive authoring script: `scripts/author_cathedral_open_hand.py`.
- Object namespace: `cathedral.astra.openhand.*` (94 Objects); each owns `material.<its-object-identifier>` (94 Materials); one new commission Lexeme.
- Witness: `tests/singularity/webgpu_cathedral_open_hand_test.cpp`.

The addition uses existing CSG primitives, property paths, Laws, and Materials. No engine behavior, existing Law, existing Object, shared Material, sidecar, original Cathedral generator, or camera placement is changed. The installer checks for identifier collisions and site occupation, compares the original Zone after removing only its additions, stages writes, rechecks source bytes for concurrent edits, and retains original JSON backups outside SaveRoot. Run it only when the Cathedral is not being saved concurrently by the app or another agent.

**Future agents, including Jules:** do not run `generate_cathedral.py` against a Person's inhabited save to maintain this court. That generator rewrites the whole scene and does not know about this addition. The separate installer can add the court to a freshly generated fixture, but deliberately refuses to replace an already inhabited court. Preserve later Person edits; compare stable identifiers and append only missing commissioned work. Do not add an OpenHand C++ class or special rendering code.

## Verification

The witness copies the Cathedral identity and all referenced Law roots into an isolated temporary SaveRoot, boots natively without a session file, activates the pearl through `object-clicked`, checks intermediate and final positions, checks isolation from unrelated clicks, saves the Zone, and checks persisted geometry/provenance/state. It renders the loaded addition through the production WebGPU SDF renderer, including its Material color expressions. Optional capture arguments write PPM images outside SaveRoot:

```sh
cmake --build build --target webgpu_cathedral_open_hand_test -j8
./build/webgpu_cathedral_open_hand_test saves /private/tmp/open-hand-captures
```

This is an isolated rendering witness, not evidence that the live app's pointer interaction or full Cathedral performance has been witnessed by Zach. Those checks belong in the [Person Verification List](../../For%20Zach/Person%20Verification%20List.md). A native Zone already held in a running app may retain its old in-memory contents; use a fresh launch to inspect the newly authored identity.

Verification results and any remaining limitations are recorded below when the pass finishes.

## Existing substrate observations, kept outside this addition

The first round-trip exposed two existing behaviors. `ObjectSerialization::from_json` invokes `setTextureResolution`, which can fork a referenced shared Material; authoring individually owned Materials avoids that identity change. `resolveZoneEndpoint` tries a Lexeme symbol before a Universe Person, so an authored-by endpoint spelled `Zach` bound to a Lexeme and saved as its generated identifier. The explicit commission Lexeme avoids claiming that this misbound relation names a Person. The Person remains named in persistent Object properties and in the Laws' author lists. No existing engine code was modified to address these observations.
