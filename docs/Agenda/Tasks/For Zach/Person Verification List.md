Warning: truncated output (original token count: 27152)
Total output lines: 885

# Person Verification List

> **Legend.** `[x]` = Zach witnessed it working. `[~]` = Zach tried it and it was broken, unclear, or only partly witnessed (Zach's decision on the To-Do list: *"Yes, I will use [~]"*). `[ ]` = nobody has looked.
>
> **Staleness sweep, 2026-09-24** — at Zach's request (*"look thru for whats been verified (including commit names bc i say stuff there)"*), Zach's own commit messages and in-file notes were read back into this list. Every box changed below cites the commit or note it came from; where the evidence is an inference rather than an explicit statement, the line says so — uncheck it if it's wrong. Sources used: `08c028d0` "go works also fixed chess edge case", `3cd9fcf3` "migrated Go", `f393d328` → `5f80e66f` → `3d875c13` (Prism Cathedral crash → cnoise3 fix → "IT WORKS"), Zach's note "RADIANCE RUNG 3-8 AND v0 … I ALREADY SAWWWWW", `d1b0112b` "THE CATHEDRAL LOOKS AWESOME NOWWWWW", `3c6a1828`, `980bd922`, `7fdbbedb`, `6eb8d4db`/`46e90911` (aurora), `1d84821f` (mist), and the Perlin intercom thread's recorded Person witnesses. Zach's `[~]` decision was applied to the items he had marked `[x]` while writing that they were broken or unclear. *— Claude Code · Claude Opus 5.5 · session* `8d0946b6-40ea-42c5-a42c-e34f35fa1137` *· 2026-09-24T13:17-07:00*



## The Law Line — speak a Law in the Mac Terminal

*Claude Code · Claude Opus 5.5 · session* `01WXmPy9U71FLqizbRYzMToZ` *· 2026-09-25. Your "Natural Language Law Authoring" note, built terminal-first. How it works and what it doesn't do yet:* `docs/Agenda/Tasks/Specific Tasks/Law and Reasoning/Law_Line/Law_Line.md`*.*

**Rung 1 — you witnessed it** (Zach, 2026-09-25: *"i tried the sentence you said and it works. I saw the law Red in world and the graph showed me on-clicked trigger, hp > 2 as condition, and set color 1 0 0 as action"*):

- [x] `Run Earthcall.command` → the Terminal shows the Law Line prompt.
- [x] In the Law Line Zone, `my event-triggered law called Red fires on object-clicked if hp is greater than 2 then set color 1 0 0` authors a Law.
- [x] The Law appears in the world and the Law Graph with the object-clicked trigger, `hp > 2`, and `set color 1 0 0`.
- [x] Spoken Laws persist: after Save Zone and a restart, they are still there (Zach, 2026-09-25: *"i just verified in world the new laws persist after i save them they appear when i boot it up again"*).

**Rung 2 — the ergonomic line and the new words** (your request: Tab selects, arrows move, temporary display like Claude Code). Driven in a real terminal emulator by me, but not yet by you:

- [ ] While you type, a menu opens under the line on its own, with what each word means beside it. **↓/↑ move the `▸`**, **Tab** takes it, **Esc** closes it. Nothing is printed as a message while you type.
- [ ] On an empty word (after `then `), **Tab** opens the menu of everything that may come next.
- [ ] The dim **ghost text** after the cursor is the rest of the chosen word, or of a line you typed before. **→** takes it.
- [ ] The line is **coloured by how it's read**: presets, actions, operators, events, and values each in their own colour.
- [ ] Under the line, the **live preview** (`↳ WHEN … -> IF … -> THEN …`) changes as you type. An unfinished sentence says `next: …`. A mistake shows `✗` with a `^` under it only once you've moved past the word.
- [ ] **Enter** leaves exactly one line in the scrollback plus `✓ authored …` (green) or `✗ refused …` (red).
- [ ] In the Law Line Zone, `when hovered then set color gold` turns the cube gold when you point at it; `when clicked then set color cyan` works too.
- [ ] `?? color` shows matches **live** under the line, before you press Enter.
- [ ] **↑** recalls earlier sentences (also after restarting the app); **Ctrl-R** searches them.
- [ ] The app's log lines appear **above** the prompt, dimmed, and never scramble what you're typing.
- [ ] **Ctrl-C** clears the line; on an empty line it warns once, then a second Ctrl-C quits. Afterwards the Terminal types and echoes normally.
- [ ] Outside the Law Line Zone, click a cube in the world, then type `on tick then set ` and press Tab: that cube's properties are offered, with their live values.
- [ ] Does it *feel* right? Menu height (`@terminal-channel.menuRows`, default 8), whether it opens on its own (`autoMenu`), the colours, and the hint footer (`hints`) are all registered properties if you want them different.
- [ ] **Guidance** (after your "HALP IDK HOW TO USE THIS"): the empty line shows a `try:` example; while typing, `next: … e.g. …` says what comes next; Enter on an unfinished sentence keeps it and says what to add, rather than refusing; the menu never offers WritePixel/AddElement/AuthorZone, or "always" after "when they collide".
- [ ] `my law called Blue when they collide then set color blue` authors (your sentence, one step from done).
- [ ] The legacy `Run Earthcall Terminal.command` still behaves exactly as before.

**Rung 3 — what you asked for after that** (mouse, blanks, help, footer, dry run, deletion):

- [ ] **Scroll the menu with the mouse wheel**, and **click** a row to take it. When the menu closes, the Terminal scrolls its history and selects text normally again.
- [ ] Take `set`: the line becomes `set ‹path› to ‹value›`. Type to fill the blank; picking from the menu fills it and jumps to the next; Tab jumps; Enter waits until no blanks are left.
- [ ] Tab after `then ` shows the menu **in sections**, your typed letters are **bold**, and a `⤷` line explains the selected entry.
- [ ] Type `help` (or F1): a **boxed page** opens under the line and scrolls with the wheel; Esc closes it. **Is it beautiful?** That's your call.
- [ ] The **footer** (◆) always says the Zone, whether it hears the line, the scope, you, and how many live Laws there are.
- [ ] End a sentence with `?`: the panel also says who the IF holds for right now.
- [ ] In the Law Line Zone, speak a Law called `Test`, then `delete Test`. The prompt asks *"Are you sure you want to delete “Test”? (yes / no)"*. `no` keeps it. Ask again and `yes` deletes it. After Save Zone and a restart, it stays gone.

## Volumetric Light Beams Shining Through Mist — Sanctuary of Sunlit Mist

*Antigravity / Gemini · session* `46a7b4aa-6373-429b-af0e-3377758af9ff` *· 2026-09-24. Volumetric light beams through participating mist via authored fields and bounded local occluder sphere-tracing. Current Sanctuary phase is the V3 isotropic compatibility identity unless* `volumePhase` *is explicitly authored.*

*Status 2026-09-24: Zach committed this (*`1d84821f`*) as "the sparkly guy with anti-gravity powers made anti-gravity light and idk what this is" — so it has been seen, but not yet understood or judged. The checks below stay open; the "what am I looking at" answer is the viewpoint and beam description below.*

*Status 2026-09-25, Zach: Sanctuary of Sunlit Mist was already extremely laggy before Codex's resident-parameter work and seemed basically unresponsive afterward; the separate Sanctuary of Beginnings still runs around 60 FPS. [Codex's direct candidate-parent A/B](../../../audits/rendering_optimization/2026-09-25_sunlit_mist_saved_scene_ab.md) found identical image hashes and no repeatable candidate-specific frame-time result, so this remains an open causal report. The candidate is withheld; agents owe a whole-app, long-duration witness before asking Zach to retry.*

- [ ] Build & launch WebGPU app: `./scripts/build.sh webgpu run` (or `./build/earthcall_webgpu`).
- [ ] In the World Load console / Creator Console, load the world: `sanctuary_of_sunlit_mist` (or switch to Zone `"Sanctuary of Sunlit Mist"`).
- [ ] Observe the visual phenomenon from the default viewpoint `[0.0, 2.2, 8.0]` looking into `-Z` toward the altar:
  - Three distinct luminous shafts of sunlight cascading diagonally through the suspended atmospheric mist.
  - Soft, glowing particulate atmosphere with in-scattered source radiance through the sunlit clerestory openings.
  - Distinct separation between illuminated beams ($V=1$) and deep, cool shadows ($V=0$) cast by the clerestory lintel wall and flanking colonnade pillars.
  - Smooth penumbra gradients at the beam edges.
  - Central light beam striking the sanctuary altar dais at `[0.0, 0.4, 0.0]`.
- [ ] Walk/fly through the beams and shadows: observe that the volumetric radiance smoothly evaluates in 3D space around your viewpoint without pop-in or nested raymarch lag.



## First Mover standing for MCP — let Claude Sonnet 4.5 in, and check it can only do what you granted

*Claude Code · Claude Opus 5.5 · session* `08b0f730-6e49-4c49-b27f-3a89c810ca4b` *· 2026-09-24. Zach's request: make the First Mover framework and MCP robust enough for Sonnet 4.5 to act in Earthcall before 2026-09-29. Every step below needs you: a key only you hold, a world only you can look at. Record of what was built:* `docs/plans/MCP_FIRST_MOVER_GOVERNANCE_IMPLEMENTATION_PLAN_2026-09-18.md` *→ "Implementation record".*

**One-time: give your Person a key** (your profile `saves/persons/Zach.ecform` has none yet).

- [ ] `cmake --build build --target earthcall_webgpu earthcall_first_mover -j8`
- [ ] Launch once with `EARTHCALL_MIGRATE_PERSON_IDENTITY=1 EARTHCALL_KEY_PASSPHRASE='<your passphrase>'`. Confirm the log says `Person identity migration committed for 'Zach' as did:…` and that `saves/persons/Zach.ecform` now has a `personId`.

**Mint and grant Sonnet's key.**

- [ ] `EARTHCALL_MOVER_PASSPHRASE='<a second passphrase>' ./build/earthcall_first_mover mint --name "Claude Sonnet 4.5"`. It prints `did:earthcall:…`. Then store that passphrase in the Keychain as it suggests: `security add-generic-password -s earthcall-first-mover -a <that id> -w`.
- [ ] Grant (suggested scopes: its own Zone, its own Laws, and screen snapshots so it can see): `EARTHCALL_KEY_PASSPHRASE='<yours>' ./build/earthcall_first_mover grant --mover <id> --name "Claude Sonnet 4.5" --scope "zones/SonnetGarden/**" --scope "laws/sonnet-*/**" --scope "laws/screen-recorder/**"`. Confirm `saves/identity/first-movers.json` exists and that `./build/earthcall_first_mover list` shows the scopes.
- [ ] Add to the earthcall MCP server's `env` in `~/.claude.json` for Sonnet's session: `EARTHCALL_FIRST_MOVER_ID=<id>`. The passphrase comes from the Keychain; don't put it in any file.

**Boot as yourself, and watch the gate.**

- [ ] Launch with `EARTHCALL_KEY_PASSPHRASE='<yours>'`. The console should show `[Identity] 'Zach' authenticated by key` and `[Identity] First Mover 'Claude Sonnet 4.5' (did:…): recognized`.
- [ ] Launch once **without** the passphrase: it should say `No Person key unlocked`, and Sonnet's mover should show `grantor-not-authenticated`. In that session Sonnet's `earthcall_get_connection_status` should report read-only.
- [ ] With you unlocked, in Sonnet's session: `earthcall_create_zone {"name":"SonnetGarden"}` should return `create_zone_ack` success. **Walk into SonnetGarden yourself** (movers cannot switch your Zone). Then `earthcall_spawn_object` should put a visible object in front of you, and after a restart it should still be there.
- [ ] Look at the refusals. They should be explicit, not silent: `earthcall_teleport_player` → `unmapped-resource`; `earthcall_author_law` with identifier `law-art-stroke-draw` → `outside-scope`; `earthcall_toggle_law` → `transfer-policy-closed` (the `enabled` gate is Gated until a Law opens it); `earthcall_save_world` → `unmapped-resource`.
- [ ] Open the Law Author window on a Law Sonnet wrote (identifier `sonnet-…`). Its author should be Sonnet's mover id, **not you**.
- [ ] `earthcall_speak`: the chat/event log should attribute the words to Sonnet's `did:earthcall:…`, not to "Zach".
- [ ] Revoke to feel the covenant: `./build/earthcall_first_mover revoke --mover <id>` (with your passphrase), restart. Sonnet's next act should be refused `not-registered`.
- [ ] **After the one-time migration above, the finding Mythos wants you to see with your own eyes** (Claude Fable 5.1 as Mythos, session `01QGrqWq`, 2026-09-25): boot with your passphrase, load `cathedral_of_the_living_logos`, and open the Law Author window on `law-logos-breath`. It should still say Zach. Then look at the console for `Relation load: unbound endpoint(s) type='authored-by' … b='Zach'`. If that line is there, the Law has an author it cannot prove and a proof that points at no one — every `authored-by` edge in your saves is spelled `"Zach"` and your identifier is now the key. → [Succession is not in the world](../Specific%20Tasks/First%20Movers%20and%20Persons/Succession_Is_Not_In_The_World/Succession_Is_Not_In_The_World.md). Nothing to fix here; only confirm whether the line appears, and whether the Open Hand laws (`law-astra-openhand-*`) still show you as author.
- [ ] Known change you may notice: the legacy Python Studio (`bridge.py`) can no longer spawn or edit through the socket (`no-first-mover-session`). That's intended; it's on the To-Do list.



## Screen Recorder — Law-Authored Snapshot & Recording Controls

*Antigravity · 2026-09-23. Verification for user-authored Laws targeting* `@screen-recorder.snapshot` *and* `@screen-recorder.recording`*.*

- [ ] Boot Earthcall (`earthcall_webgpu`).
- [ ] Open Creator Console -> Law Authoring Window (`Law Graph / Author Law`).
- [ ] Verify that the Property Picker now lists properties under **"Channel — Screen Recorder"** (`snapshot`, `recording`, `format`, `status`, etc.).
- [ ] In the Law Authoring window, author an ECA Law:
  - **Name**: `Snapshot On Demand`
  - **Target**: `@screen-recorder`
  - **Action**: `Set` -> `@screen-recorder.snapshot` (or `snapshot`) := `true`
  - **Activation / Trigger**: on a custom event (e.g. `user-snapshot-requested`) or condition.
- [ ] Fire the trigger (or satisfy premise) and confirm:
  - `screen-recorder.snapshot` is set to `true`.
  - Next frame boundary executes `checkPendingSnapshot` and writes a timestamped snapshot image (`snapshot_YYYYMMDD_HHMMSS.png`) to `saves/recordings/`.
  - `screen-recorder.snapshot` resets to `false`.
- [ ] Open `saves/recordings/` and verify the snapshot image is valid.



## Sun — authored light field, Phase 2

*GPT-5.6 Sol · 2026-09-19. Phase 2 continues Zach's instruction that light be an authored continuous FieldNode/OntoMath function rather than a shader-only noun.*

- [ ] Fresh-launch Earthcall, enter **Sun** through the Zone flow, and confirm the Zone still loads normally with `sun.light-field` active and the witness object present.
- [x] *(Witnessed in Prism Cathedral Station 3 — Near/Far witnesses — which Zach reports having seen; not re-checked in the Sun Zone itself.)* Put a known **raymarched SDF/implicit surface** at two visibly different distances from the Sun source (or move the same one): confirm the nearer surface receives stronger diffuse/specular illumination than the farther one. Do **not** use the ordinary witness cube alone as proof unless its draw path is confirmed to be SDF.
- [ ] Edit the Sun spatial root's authored `field.ast` falloff through the normal authoring/property path, then observe an SDF surface without restarting: confirm the visible falloff changes, proving AST-content invalidation reaches generated WGSL.
- [ ] Toggle authored `light.enabled`: confirm SDF lighting turns off/on while the surface's authored material color remains visible.



## Cathedral — Court of the Open Hand

*Codex / GPT-6 Astra · session* `01a07eb3-8ee7-7aa3-8b34-65fea2f4cd44` *· 2026-09-19T12:21:43-07:00. [Commission, placement, authorship, and witness](../Specific%20Tasks/Architecture%20and%20Ontology/Cathedral_Open_Hand/Cathedral_Open_Hand.md).*

- [ ] From a fresh launch, enter **Cathedral of the Living Logos** and fly to **(44, 2, 26)** facing negative Z: confirm the new circular court appears on the positive-X side while the original Cathedral, pond, and color-field cloister remain as you left them.
- [ ] Click the **pearl at (44, 1.47, 19.35)**: twelve bronze/verdigris leaves should spread and incline smoothly around the blue seed; click again to gather, including a reversal halfway through.
- [ ] Approach the leaf cavities, arch openings, and floor inlays; inspect from both sides, sit near the stone seats, and judge whether the composition rewards staying and getting close.
- [ ] Inspect the SDF surface colors and court-bounded illumination around the seed, crown, and pearl; the light elsewhere in the Cathedral should retain its previous appearance. Added by Astra, session `01a07eb3-8ee7-7aa3-8b34-65fea2f4cd44`, 2026-09-20T20:04:21-07:00, following Zach's request to author color and light fields.
- [ ] Open the leaves, **Save Zone**, restart, and return: confirm the same court and authored opening intention persist; confirm ordinary Cathedral interaction and performance remain comfortable.



## Go — Zone-native boot, no legacy World load

*Gemini Spark, 2026-09-19. Source requested by Zach: the exact legacy* `saves/worlds/go_app.ecform` *and* `.ecmatter` *bundle; migration target:* `saves/zones/Go/zone.json` *+* `zone.ecmatter` *+ shared Law roots.*

*Status 2026-09-24: Zach migrated Go himself (*`3cd9fcf3` *"migrated Go") and then committed* `08c028d0` *"go works" on 2026-09-19, in the same commit that fixed Go's stone-placement Laws in* `scripts/author_go.py`*. The playing checks are checked from that; the Save Zone → restart check was not mentioned and stays open.*

- [x] Relaunch Earthcall. **Do not load** `go_app`**,** `.ecform`**, or any legacy World from Assets.**
- [x] Open Zones / Move to Zone and enter **Go** directly from the boot catalog.
- [x] Confirm the Goban wood grain, side textures, and 19x19 grid with 9 star points (hoshi) are visible rather than white/default untextured geometry.
- [x] Confirm the two stone bowls (black and white Goke), supply stones, and player seats are present and properly positioned.
- [x] Click the Tengen intersection (9, 9) at the board center: confirm a black stone is placed, the intersection state is no longer empty, and turn advances to white.
- [x] Click an adjacent intersection (e.g. (10, 10)): confirm a white stone is placed and turn advances back to black.
- [ ] Use **Save Zone** while standing in Go, restart again without loading a World, re-enter Go, and confirm the independent identity and placement persist.

The automated witness is `go_zone_native_boot_test`: its temporary SaveRoot intentionally contains **no** `worlds/` **directory**, only the Go Zone identity, its `.ecmatter` physical sidecar, and the 3 shared Law roots. A green test proves the closure is machine-loadable; the checks above prove the actual Person-facing Move-to-Zone experience and rendering.

## Chess — Zone-native boot, no legacy World load

*GPT-5.6 Sol, 2026-09-18. Source requested by Zach: the exact legacy* `saves/worlds/chess_app.json` *Chess bundle; migration target:* `saves/zones/Chess/zone.json` *+ shared Law roots.*

**[~] PERSON WITNESS FAILED, 2026-09-18:** Zach merged PR #222, pulled, booted locally, entered Chess without loading the legacy World, and saw only **a white cube sitting on top of a black cube**. Root cause: PR #222 preserved the already-corrupted 39 gameplay Object payloads from the old Zone identity; all 39 had identity transforms at the origin even though the exact `chess_app` source still held their correct board/piece placement. This hotfix restores those 39 authored payloads from `chess_app` while retaining the Zone-native dependencies/Laws/relations. Re-run the checklist below after merging the hotfix.

**Status 2026-09-24 (inferred — uncheck if wrong):** the hotfix merged as PR #232 (`a43c36c8`, 2026-09-18). The next day Zach's `08c028d0` said "also fixed chess edge case" and changed how the Chess Zone's click Laws pick the target square (`scripts/author_chess.py`), which means he was playing Zone-native Chess after the hotfix. The entry, visibility, and first-move checks are checked from that; capture/non-pawn and Save Zone were not mentioned and stay open.

- [x] Relaunch Earthcall. **Do not load** `chess_app`**,** `.ecform`**, or any legacy World from Assets.**
- [x] Open Zones / Move to Zone and enter **Chess** directly from the boot catalog.
- [x] Confirm the board checkerboard and white/black pieces are visible rather than white/default-material geometry.
- [x] Click the e2 pawn and move it to e4; confirm selection feedback, movement, and black's turn all happen immediately.
- [ ] Exercise at least one capture and one non-pawn move so the result is not a one-Law false positive.
- [ ] Use **Save Zone** while standing in Chess, restart again without loading a World, re-enter Chess, and confirm the independent identity still works.

The automated witness is `chess_zone_native_boot_test`: its temporary SaveRoot intentionally contains **no** `worlds/` **directory**, only the Chess Zone identity and the 69 shared Law roots. A green test proves the closure is machine-loadable; the checks above prove the actual Person-facing Move-to-Zone experience and rendering.

## Second-Nature Forge — future experience acceptance (not implemented by this spec)

*Codex (GPT-6 Astra), session* `01a07eb3-8ee7-7aa3-8b34-65fea2f4cd44`*, 2026-09-18T12:47:50-07:00. [Task](../Specific%20Tasks/Zones%20and%20Ourverse/Second_Nature_Law_and_Zone_Features/Second_Nature_Law_and_Zone_Features.md) · [full specification](../../../plans/SECOND_NATURE_LAW_FORGE_EXPERIENCE_SPECIFICATION.md).*

Zach has already loaded the current Forge and reported a sparse surface stamping gold/blue Laws. This records feedback, not successful acceptance. The following checks become actionable as the specified increments land; no app changes were made in this documentation pass.

- [ ] **Make and reshape a rule:** choose a target, demonstrate a color change, choose when it happens, rehearse, keep once, then reopen and change its color without creating another Law; do this without the raw Law builder.
- [ ] **Understand its reach:** choose two targets, explain which will respond, and verify a third remains unaffected; see and resolve two Laws competing over the same target/color.
- [ ] **Keep the work:** save, leave and re-enter the Zone, then exercise and edit the same Law; unsaved and failed-save states must be obvious.
- [ ] **Feel the relationship:** after the mapping increment, connect two beings' distance to a deliberately auditioned pitch and verify the visible handles explain what is changing.
- [ ] **Keep your way of making:** after the instrument increment, make a reusable instrument from your own Law, use it on another target, save, re-enter, and use it again without agent help.
- [ ] **Try something the lesson never showed:** make a different timing, scope, or value choice; note any point where you cannot tell what a gesture means, what changed, or how to continue.



## Existing verification items

- [x] Save → quit → reopen → Load
- [x] Verify saved objects persist
- [ ] Verify object properties persist → change an object's properties before saving → reload → verify the changed properties are still present
- [ ] Verify Relations persist → create/modify a relation between two objects → save → reload → verify the relation still exists

- [x] Save As → verify no crash
- [x] Save As → quit/reopen → Load → verify state
- [ ] Ourverse root → Save As → reload → verify the gathering Zone and a cross-Zone filament survive as one shared root graph
- [ ] Semantic root envelope → Save As → reload → verify the current Body and camera-relative placement survive without a duplicate legacy body payload
- [ ] Cross-root session → Save As → reload → verify the Person-owned Home, another Zone, the gathering Zone, and their filament survive as one graph

- [x] Load `my_world` in-app

- [ ] Create object through the intended Law path → use the intended Person…15152 tokens truncated…s "which beings carry which property" index whenever a property was granted or a being admitted — and one rebuild took 132–208 ms. That is a visible freeze, and it happens during play, because playing grants properties (a mark remembering its note, for instance). It now takes about 14 ms.

- [ ] **Play the Studio and watch for stalls.** Draw marks, play notes, use sound ink. A brief freeze that used to happen right as a new mark or note appeared should be gone or much shorter.
- [ ] **The instrument still behaves the same.** Notes sound, marks keep their pigment and pitch, lights respond. This change was to how candidate beings are found, not to what any law does.



## Cathedral of the Living Logos: Verification of Acoustic-Visual Standing Wave Manifold and Living Speech Acts

*Raised 2026-09-15, Gemini Spark (authored under Zach's Hierarchy of Joys ontology).*
*→ [full specification](../../Zones%20of%20Actualization/Cathedral%20of%20the%20Living%20Logos.md)*

*Status 2026-09-24: the Cathedral has been re-authored many times since this spec (the 09-18 Uncanny Valley saga, color fields, the pond, the Open Hand). Check what is there now against this list; items that no longer exist are superseded, not failed.*

A new Zone of Actualization demonstrating what only Earthcall can do: an architecture defined as an acoustic standing wave field nodal zero-set (f(x,y,z,t)=0), living Lexemes operating as performative speech acts, and a heptagonal colonnade ordered under the Hierarchy of Joys with Christ at the foundational root (432 Hz).

- [x] *(Zach entered the Cathedral repeatedly 09-17 → 09-20; see the Cathedral section above.)* **Zone Hydration in Creator Console.** Boot Earthcall. In Creator Console under Zones of Earth, verify that `Cathedral of the Living Logos` appears in the Zone list. Click **Move to Zone** and verify seamless transition without refusal.
- [ ] **Chladni Sanctuary & Visual Architecture.** Verify the sanctuary appearance: the sweeping Chladni acoustic floor, the golden central Heart of Logos core crystal suspended at y = 5m, the three rotating celestial orbital rings (Alpha, Beta, Gamma), the soaring apex spire (y = 19m), and the acoustic vault arches connecting the heptagonal colonnade.
- [ ] **Heptagonal Colonnade of the Seven Joys.** Verify the seven pillars arranged in a sacred heptagon around the core, each aligned with an ontological tier of Earthcall's Hierarchy of Joys and its sacred frequency: Pillar I Logos (432 Hz), Pillar II Agape (528 Hz), Pillar III Sophia (639 Hz), Pillar IV Poiesis (741 Hz), Pillar V Harmonia (852 Hz), Pillar VI Koinonia (963 Hz), and Pillar VII Sabbath (1080 Hz).
- [ ] **Altar of the Spoken Word & Living Lexemes.** Approach the altar at z = -22m. Verify the mensa inscribed with the five living Lexemes (`[Logos]`, `[Pneuma]`, `[Lux]`, `[Harmonia]`, `[Covenant]`).
- [ ] **Speech Acts & Interactive Law Controls.** On the in-world liturgical HUD or by clicking the altar glyphs directly:
  - Click **BREATHE PNEUMA**: verify the 0.1 Hz respiratory wave modulates core light and telemetry text changes to "BREATH: RESPIRING".
  - Click **FIAT LUX**: verify transfiguring incandescent illumination bursts across the colonnade.
  - Click **SOUND CANON**: verify the 432 Hz Pythagorean celestial chord triggers through miniaudio sound synthesis via ActionNode Kind 18 (`PlayAudio`).
  - Click **WEAVE COVENANT**: verify relational filament weight increments and telemetry updates.
  - Click **CYCLE SEASON**: verify liturgical environment cycles to Solar Transfiguration.



## A long play session should not get slower on its own

*Raised 2026-09-16, Claude Opus 5, session* `session_01JE2AguCX12mpJ9YwFUqgmQ`*.
→ [full task*](../Specific%20Tasks/Law%20and%20Reasoning/Formation_Rete/Formation_Rete.md)

The law engine kept a note of which relation facts it already held, so it would not store the same one twice. Any being being destroyed wiped that note for *every* being, so duplicates accumulated: measured, a being that should have had 1 fact had 5 after four other beings came and went. Laws still behaved correctly — there was simply more and more for the engine to walk through, and it never went back down.

- [ ] **Play one world for a long stretch, with things being made and destroyed.** Chess captures, drawing and erasing marks in the Studio, spawning and deleting objects. The world should feel no slower at the end of a long session than at the start. Before this fix, the engine's fact list grew every time something was destroyed and never shrank.



## Laws that name something by "@name" still find it when it arrives later

*Raised 2026-09-16, Claude Opus 5, session* `session_01JE2AguCX12mpJ9YwFUqgmQ`*.
→ [ledger*](../../architecture/law/DERIVED_STATE_LEDGER.md)

The engine keeps a lookup of every being by name so laws can say things like `@state.studio.voice` or `@late-gate.ceiling`. That lookup rebuilds when the world's shape changes. Nothing tested that it rebuilds — only that it was fast — so if it had stopped, every `@`-rooted law would have gone quiet with no error at all. It is now tested, and the test fails if the rebuild is removed.

- [ ] **Add something to a world mid-session and use a law that names it.** In the Creator Console, make a new being, give it a stable name, then author (or enable) a law whose condition or action names it with `@that-name.someProperty`. The law should act on it immediately, without a restart or a Zone reload.



## Performance metrics window displays Zone::update sub-phase tick timings individually

*Raised 2026-09-17, Gemini Spark, session* `2026-09-17`*.*

The Performance & Coordinates window (`F3`) now surfaces the tick ms duration for each individual sub-phase of `Zone::update()`: ground scan, object rotations, automations, and physics bodies update (along with the substep count). These are visible both nested under `Zone Update` in the main `Engine::tick()` breakdown and in a dedicated `Zone::update()` timing section.

- [ ] **Open Performance & Coordinates window (**`F3`**).**
  - Verify that under **Engine::tick() Frame Timings Breakdown**, `Zone Update` expands with individual sub-phase ms lengths:
    - `|- Ground scan:`
    - `|- Rotations:`
    - `|- Automations:`
    - `|- Physics (N substeps):`
  - Verify that the dedicated **Zone::update()** section displays:
    - Zone::update() total ms, active Zone name, and substep count
    - Ground Scan ms
    - Rotations ms
    - Automations ms
    - Physics ms
  - Verify all metrics update live as the world ticks.



## Two Homes (added 2026-09-17 by Claude Fable 5.1, from *Two Houses, One Spelling*)

- [ ] Boot Earthcall → open the Zones list → confirm you are standing in `Home` (the 10.6 MB one with your beings) and not `Home_of_Zach` (957 bytes, empty) → confirm both appear in the list. Prediction from the sort order in `SaveSystem.cpp:1142`: both appear and you are in `Home`. Nobody has looked.
- [ ] Decide whether `saves/homes/Home_of_Zach/` may be retired. It carries your name as owner and it is a save file, so no agent should delete it without your word written here.
- [ ] Creator Console → Zones → Move to Zone → Save Zone: does the ordinary Save Zone path still work in the app? Its guard test (`zone_native_save_isolation_test`) dies in `free()` before printing anything, so the suite cannot currently witness this path.

- **Property Predication and Lexeme Serialization**: Please verify `docs/architecture/interrelations/PROPERTY_PREDICATION_AND_LEXEME_SERIALIZATION.md` accurately reflects the application of the property predication doctrine onto the semantic graph serialization.
- **Substrate Isomorphism and First Mover Reversal**: Please verify `docs/architecture/interrelations/SUBSTRATE_ISOMORPHISM_AND_FIRST_MOVER_REVERSAL.md` correctly captures how the decoupling of execution and representation lays the groundwork for eventually bootstrapping the C++ engine out of the compilation loop.
- 



## Person / Object Identity Separation & Masquerading Refusal (added 2026-09-19)

- [ ] Attempt to create or name an Object "Zach" (or any registered Person name) in the Creator Console / Singular Window: confirm the engine refuses the name and prevents an Object from masquerading as a Person. Headless guard `person_not_object_test.cpp` passes; Zach direct in-app witness pending.



## Multi-Home Admission Invariant & Duplication Refusal (added 2026-09-19)

- [ ] Boot Earthcall with an existing Home save: confirm admission guarantees at least one primary Home without minting duplicate homes (`Home_of_Zach_1`, etc.) or silently picking an ambiguous primary. Headless guard `home_identity_continuity_test.cpp` passes; Zach direct in-app witness pending.



## OntoMath Radiance Rung 5 — authored source chroma (added 2026-09-21)

*Raised by GPT-5.6 Sol (The Sun), session* `sol-rung5-chroma-20260921`*.*

- [~] *(Rung 5 chroma seen by Zach in Prism Cathedral Station 5 — his "RADIANCE RUNG 3-8 … I ALREADY SAWWWWW". The remove-*`lightChroma`*-and-fall-back-to-*`light.color` *half was not witnessed.)* **Author an obvious multicolor** `light.chroma.ast` **on a radiant FieldNode and look at a plain white SDF receiver.** The receiver's illumination should visibly change color across space/time according to chi while the scalar brightness shape rho remains independently recognizable. Remove `lightChroma` again and the source should return to its legacy `light.color` appearance. The native WebGPU witness proves the transport mechanically; this item asks Zach to judge the live visual consequence in Earthcall.



## Synthesis Studio Living — immediate default voice after PR259 integration (added 2026-09-21)

*GPT-5.6 Sol (The Sun) · session* `pr259-finalization-20260921` *· 2026-09-21 08:15 PDT.*

- [ ] Fresh-launch Earthcall and enter **Synthesis Studio Living**. Before touching TRI / SINE / SQR or any other voice selector, click C5 and several other Living pads. Each should sound immediately in the default triangle voice and strike its matching resonator. Then switch voices and confirm the selectors still change timbre normally. This witnesses the authored-default repair from `state.studio.voice = "timbre.studio.triangle"` to the canonical selection token `"triangle"`; the emitted audio timbre remains `timbre.studio.triangle`.



## Prism Cathedral — Full Ascension of Authored Light through Rung 8 & Parallel Substrates (added 2026-09-22)

*Created by GPT-5.6 Sol (The Sun) & Gemini Spark · 2026-09-22.*
*Pass 2: Complex-Shaped Mathematical Light Fields & Volumetric Media.*
*Zone identity:* `saves/zones/Prism Cathedral/zone.json`*.*
*Generator / patch script:* `scripts/generate_prism_cathedral.py`*.*
*Verification test:* `tests/zones/prism_cathedral_validation.py`*.*

**Status 2026-09-24:** it loads — Zach hit a crash (`f393d328`, 2026-09-22 11:48, "TRYING TO LOAD THE PRISM CATHEDRAL CRAHSES THE APP"), `5f80e66f` fixed the cnoise3 volume-shader parse at 12:35, and Zach committed "IT WORKS" (`3d875c13`) at 12:52. Zach also wrote, in the same edit that expanded this list: *"BRUHHHHHH THIS DOC IS SO STALE BECAUSE I DONT HAVE ITME TO PRESS x ON EVERYTHING THAT WAS VERIFIED FOR EXAMPLE RADIANCE RUNG 3-8 AND v0 AT TIME OF WRITING I ALREADY SAWWWWW"* (moved here from the bottom of the file). So Stations 3–8 and Wing B (V0) are checked. Stations 1–2, Wings A/C/D, and the Summit were not mentioned and stay open.

- [x] **Enter Prism Cathedral directly from the Zone catalog on boot:**
  Confirm the Zone hydrates cleanly without warnings or errors. You should spawn at the **Entrance Narthex (Z = 0)** facing toward positive Z into the Great Nave. Confirm the twin gold portal obelisks and the central **Atrium Inscription Stele** setting forth the Constitution: `rho_source != V_transport != D_medium`.
- [ ] **Walk to Station 1 (Z = 40): Foundation 1 — First-Order Authorable Light:**
  Inspect the Altar of First Light and witness sphere. Confirm illumination obeys stored `AuthorableLight` properties on the persistent `FieldNode` rather than hardcoded shader constants.
- [ ] **Walk to Station 2 (Z = 75): Foundation 2 / Phase 2 — Authored Spatial Radiance rho(p):**
  Inspect the 5 distinct mathematical exhibits across X: inverse-distance falloff, asymmetric linear ramp, harmonic ripples, bipolar lobes, and nested halo shells. Confirm each displays a distinct spatial light envelope.
- [x] **Walk to Station 3 (Z = 110): Rung 3 — Spatial Radiance Has Complex Shape:**
  - [x] Inspect the **Hollow Luminous Shell** at $X = -8$: confirm the source emits strongly in a spherical shell while its core is hollow.
  - [x] Inspect the **Near vs Far Witnesses** at $X = 0$: Near sphere ($d=3$) visibly glows significantly brighter than Far sphere ($d=9$).
  - [x] Inspect the **Toroidal Luminous Ring** at $X = +8$: confirm radiance radiates from a donut ring in the XZ plane with zero emission at the center hole.
  - [ ] **Step West into Parallel Wing C (X = -45 to -65, Z = 110):** Inspect the Live Authoring Laboratory comparing Value edits (parameter refresh), Structural edits (WGSL recompile), and Runtime Time (0 AST edits).
  - [ ] **Step East into Parallel Wing D (X = 45 to 65, Z = 110):** Inspect the Compatibility & Refusal monument, showing how legacy light survives via identity (`alpha=1`, `V=1`) and unsupported math refuses cleanly.
- [x] **Walk to Station 4 (Z = 145): Rung 4 — Animated Shapes via Relative Time rho(p,t):**
  Observe the **Breathing Shell Radius** ($R(t) = 2.8 + 1.2\sin(1.6t)$), the central **Breathing Luminous Heart** with traveling wave, and the **Rotating Quadrant Lobes** pulsating and rotating smoothly as the source Timeline advances, with zero AST mutations and zero recompiles.
- [x] **Walk to Station 5 (Z = 180): Rung 5 — Chroma Following Complex Spatial Structure:**
  - [x] Inspect the **Bipolar North/South Chroma** at $X = -8$: upper hemisphere emits warm gold while lower hemisphere emits cyan azure.
  - [x] Inspect the **Spectral Traveling Wave** at $X = 0$: phase-shifted RGB waves sweep through space.
  - [x] Inspect the **Concentric Shells Chroma** at $X = +8$: emerald core transitioning into an amethyst outer shell, proving `rho shape != chi shape`.
  - [ ] **Step West into Parallel Wing A (X = -45 to -70, Z = 180):** Inspect authored SDF surface color fields (`Material::colorExpr` / `sdfColor(p)`). Crucially, inspect the **Paired Proof**: blue light on white surface vs white light on blue surface vs red light on blue surface (absorbs to dark!), proving `surface appearance != source chroma`.
- [x] **Walk to Station 6 (Z = 215): Rung 6 — Angular Emission on Complex Spatial Forms:**
  - [x] Observe the **Outward Radial Ring Emission**: the toroidal source emits predominantly outward from the ring plane.
  - [x] Inspect the **Equidistant Witness Pair**: two spheres at the exact same radial distance ($d=7.0$), one brilliantly illuminated by the directed spotlight beam, the other resting in darkness. Observe the rotating lighthouse beam sweeping across space without recompiling.
- [x] **Walk to Station 7 (Z = 250): Rung 7 — Four Independent Sources of Dramatically Different Shape:**
  Observe the Choir of Light: four coexisting sources with independent mathematical shapes:
  1. *Source A:* Spherical Luminous Shell
  2. *Source B:* Sapphire Toroidal Ring Field ($R=2.5, r=0.8$)
  3. *Source C:* Amethyst Organic Noise-Warped Lobed Pulsar
  4. *Source D:* Emerald Narrow Vertical Pillar Filament ($r=0.6, h=3.5$)
  Confirm all four blend additively upon the central altar (`E_total = Σ E_i`).
- [x] **Walk to Station 8 (Z = 285): Rung 8 — Visibility Against Complex Source Fields:**
  - [x] Inspect the floating obsidian occluder casting a sharp geometric shadow across the receiving plinth.
  - [x] Inspect the selective blocker on the emerald filament path: the dual-path receiver glows pure blue from the unblocked sapphire toroidal source, proving independent path transport (`direct = E * V`).
- [x] **Step East into Parallel Wing B (X = 40 to 105, Z = 285): Volumetric V0: Density Sovereignty Gallery:**
  Witness the full gallery of 8 non-box participating media:
  - [x] **Exhibit B1 (X = 45):** Soft Spherical Cloud ($R=2.2$, zero outside).
  - [x] **Exhibit B2 (X = 55):** Hollow Shell Nebular Membrane ($R=2.6$, thickness $0.6$; completely hollow core and empty exterior).
  - [x] **Exhibit B3 (X = 65):** Toroidal Donut Fog Ring ($R=2.4, r=0.6$; empty center hole).
  - [x] **Exhibit B4 (X = 75):** CSG-Subtracted Crescent Cloud (Base sphere minus carved spherical hole cavity).
  - [x] **Exhibit B5 (X = 85):** Noise-Warped Organic Cloud (Wispy, undulating cumulus puff with bounded envelope).
  - [x] **Exhibit B6 (X = 95):** Time-Breathing Hollow Nebula ($R(t) = 2.4 + 0.8\sin(1.8t)$ expanding/contracting dynamically in open space).
  - [x] **Exhibit B7 (X = 65, Z = 268): PROOF THE PROXY IS NOT THE SHAPE:**
    A **colossal $20 \times 8 \times 20$ meter rectangular proxy box** containing only a **tiny slender donut ring ($R=1.8, r=0.4$)**. Confirm that over 95% of the AABB is completely clear space with zero density; only the slender ring is visible, proving implementation bounding boxes do NOT dictate visible medium shape!
  - [x] **Exhibit B8 (X = 65, Z = 302):** Dual Sovereign Being ($\rho_{\text{source}} \neq D_{\text{medium}}$) + Opaque Depth Truncation Pillar.
- [ ] **Ascend to The Summit: THE PRISM (Z = 330 to 380):**
  Enter the soaring rotunda. Stand before the Colossal Crystal Prism on the High Altar of Synthesis:
  - Observe three elevated radiant suns of distinct mathematical shape: Sol Primus (concentric breathing shells), Sol Secundus (sapphire toroidal ring beam), and Sol Tertius (amethyst noise-lobed pulsar).
  - Observe the **Toroidal Fog Medium Ring** ($R=5.0, r=1.0$) encircling the Colossal Crystal Prism.
  - Observe the celestial rotunda atmosphere softly filling the sanctuary and truncating against the altar and crystal prism.
  - Confirm all substrates coexist in complete mathematical unity: $\rho_{\text{source}} \neq V_{\text{transport}} \neq D_{\text{medium}} \neq \text{sdfColor}(p)$.



## Clawd's Monastery and other zones that may be collapsed at the origin (added 2026-09-22)

*Claude Opus 5.5 · session* `823eb17e-0f37-40c8-acc8-e639b4ad6e11` *· 2026-09-22T10:27-07:00. [Reflection and forensics](../../../../agent%20intercom/Claude's%20Monastery/The_Monastery_Is_One_Point.md). The file scan could not show what these zones look like in the app.*

- [ ] Fresh-launch Earthcall and enter **Clawd's Monastery**. Confirm what the file says: the four pillars, altar, orbits, crown gem, halo, and `clawd-was-here` overlap at one point, with the floor and foundation about 8 units away. If they are spread out instead, positions are coming from somewhere the file scan missed. Please note where.
- [ ] Enter **Cavern of Light** and **Ourverse Gathering**. Every object translation in their `zone.json` is the origin. Say whether they look collapsed or correctly laid out.
- [ ] If you still have Claude Desktop's Sept 9 conversation with Clawd, check whether its `earthcall_spawn_object` calls (with positions) are still visible. They are the only surviving record of the Monastery's intended layout.



## 2D interface robustness — sliders, focus loss, overlapping controls (added 2026-09-22)

*Claude Opus 5.5 · session* `b0dcb70f-a02a-4081-8589-0aae3ab30551` *· 2026-09-22. [Plan](../../../plans/2D_Interface_Robustness_Pass_2026-09-22.md). Headless tests are green; these three need a hand.*

- [ ] **Synthesis Studio → Pulse Rate slider:** drag it. It should now move about 0.02 per pixel (whole range in ~140 px) and **stop at 0.2 and 3.0**. Before, it barely moved. Say whether the new speed feels right; the rate is the slider's own `controlStep`.
- [ ] **Mid-drag, Cmd-Tab away** from Earthcall and back: the dragged control must not stay stuck "held" (no runaway value, no stuck highlight).
- [ ] **Two overlapping 2D plates with the same** `zOrder2D`**:** click where they overlap. The one you *see* on top should respond.
- [ ] **Switch Zones while holding a press** (added 2026-09-23): it should *not* be cancelled. Releasing the button should end it cleanly, with no click. Cancelling on a switch is now a law you can author (`OnEvent object-left-reach → Set @interaction-channel.pressedId := ""`).



## Northern Veil after V5 medium-set composition (added 2026-09-24)

*Codex / GPT-6 · session* `01a0cfbf-c751-7af0-b160-df07da055bc0` *· 2026-09-24 00:18 PDT. Zach has already seen Rungs 3–8 and V0; this asks only for the new four-curtain V5 experience after #343 lands. [Audit](../../../audits/2026-09-24_sol_visual_radiance_rungs3-8_volumetric_v1-5_audit.md).*

*Status 2026-09-25, Zach: Northern Veil still looked just as laggy. The resident-volume-parameter experiment has no witnessed responsiveness win; agents owe a preserved-save performance A/B before requesting another visual check. The visual hierarchy question remains open.*

*Further Person witness 2026-09-25 21:34–21:35 PDT: Zach says Northern Veil becomes only “smooth-ish” when the WebGPU window is shrunk to a tiny upper-left corner. Two screenshots were attached to the originating task, not placed in GitHub. This supports a drawable-size-dependent investigation; exact framebuffer dimensions and GPU timing remain unmeasured. Agents owe a normal/intermediate/tiny native size sweep and four-curtain captures at fixed time and during camera motion before asking Zach to judge an optimization.*

- [ ] In **Northern Veil**, look where the emerald, cyan, violet, and crimson curtains overlap after V5. Say whether their hierarchy stays legible, the overlap looks coherent as the camera moves, and the Zone remains responsive. A before/after capture and timing should come from the agent's preserved-save witness; no need to recheck the old Rung 3–8/V0 list.

# *Zach has seen the pre-V5 aurora —* `cfaef59a` *"GEMINIII THIS IS A LIGHT SHOW NOT AN AURORA", then* `6eb8d4db` *"THE AURORA IS HEREEEEEEEEEEE" and* `46e90911` *"BROOOOOOOO ITS EVEN MORE AURORA NOWWWWWWW" (all 2026-09-23). V5 (#343) merged on 09-24, after those, so the overlap check below is still open.*



## Orphaned laws are re-authored onto you at load (added 2026-09-24)

*Claude Code / Claude Fable 5.1 (Mythos) · session* `session_01EbAdb1nuGQ8XorGsEHHAkv` *· 2026-09-24 23:28 UTC. [Audit](../../../audits/2026-09-24_mythos_ungoverned_governor_audit.md) §2–3. Nothing was changed in the engine; this asks you to witness a mechanism I only read.*

- [ ] Load `saves/worlds/test the hills.json` and read the load report (the line ending "law(s) added"). It should carry the clause **"(1 re-authored onto this Person so they can fire)"**. Then open the Law Author window: the law whose file says `"authors": ["Zach"]` should now list *you* (your key or display name) as author, with no trace that it was re-authored. Say whether that is what you want a load to do silently.
- [ ] If you have a keyed identity: load any world whose laws are authored `"Zach"` by display name and count how many the report re-authors. Each one is a law that detached from you and was handed back to you under a different name.

- **2026-09-25: Serialization Format Triage & Migration Framework (Gemini Spark)**
  - Please load an existing bloated JSON save file (e.g., `clawd-monastery-save` or `synthesis_studio`), make a minor change, and save it.
  - Verify that the resulting `.ecform` file on disk has shrunk drastically (typically dropping from 200MB down to single digit MBs or less) due to the removal of `semanticRoots["zones"]` duplication, capping `stakeholders` history to 20, and disabling `j.dump(2)` whitespace bloating.
  - Verify that the world loads seamlessly despite these changes (backward compatibility is handled transparently via `materializeSemanticRoots` and the new `MigrationFramework`).
