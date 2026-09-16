# Person Verification List

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

- [ ] Create object through the intended Law path → use the intended Person-facing control to arm/trigger the creation Law → verify the new object appears
- [ ] Verify Law fires → object appears → trigger the creation Law and verify exactly one intended object is created
- [ ] Save → reload → find the same object by stable ID → select/interact with the created object before saving, reload, then verify it can still be identified/targeted as the same object

- [x] Home: create/modify object
- [x] Paint its FaceTexture
- [x] Save
- [x] Load another save
- [x] Return/walk to Home
- [x] Verify shape + paint survive

- [x] Creator Console: open it
- [x] Select object
- [ ] Morph object → select an existing object → activate Morph → modify its geometry using the available morph controls → verify the object's shape changes as intended
- [x] Face Brush
- [x] Basic Pixel Changer authored Material color picker → VERIFIED BY ZACH: Zach confirmed the full RGB/HSV interface appears, discovered the canvas was white and marks were painting white-on-white, verified red dots painted when switching color (2026-09-10/11), and fixed color picker/canvas painting in commits 3763c05f, c72d97d8, 7abcd38c, and 3b3f343d.
- [x] Pottery → activate Pottery → create/use a pottery form on an object → verify the geometry changes as intended | Zach tested: My note in the todo list remains unfixed: Pottery successfully increases 3D dimensinos but stretches the FaceTextures to fit the new face dimensions rather than increasing the size of the facetexture image accordingly.
- [x] Rotate → select an object → activate Rotate → rotate the object → verify its orientation changes | Zach tested: changing the angle sliders on a selected shape while having rotation tool selected does not visibly change the shape. However, if you scroll to the bottom of the creator console window in 3D tool mode you'll see "Selection" with what seems to be the object ID. There are "Target Rotation" sliders that successfully rotate the shape.
- [x] Fuse Objects → create/select two objects → activate Fuse → fuse them → verify they become one fused object as intended | Zach tested: I mean I guess it executes, but it's not always clear what and sometimes it's weird and unclear if it's buggy or not. I need to investigate.

- [x] 3D Create tool
- [x] Gyroid implicit: create one and visually verify it

- [x] Move cursor onto an interactive 3D object → verify hover behavior | Zach: What I mean precisely by the "x" is this—moving my mouse over a 3D object results in "object-hover-entered", visible in the Law Authoring Window's "Recent Events" log
- [x] Move cursor away from the interactive 3D object → verify hover-end behavior | Zach: same as above except for exited
- [x] Click an interactive 3D object → verify click behavior
- [x] With the Earthcall world focused, press and release a key → verify the corresponding key interaction is detected | Zach: caveat, so the events are logged in the "Recent Events" as just "key-released" and there's not a clear differentiator (at least visible to me in the pass I ran) for different keys.
- [x] Place cursor over an interactive 3D object → scroll the mouse wheel → verify scroll interaction is detected
- [x] Click and hold an interactive 3D object → move the mouse → release → verify drag interaction is detected | Zach: so it both says "object drag started/ended" and "object pressed/released"  
- [x] Focus the Earthcall window → verify focus behavior | Zach tested: noted that expected behavior was unclear.
- [x] Unfocus the Earthcall window → verify unfocus behavior | Zach tested: noted that expected behavior was unclear.
- [x] Open an Earthcall UI window → verify the mouse pointer unlocks and can interact with the UI
- [x] Close the UI window → verify normal 3D-world pointer interaction returns | Zach tested: noted that expected behavior was unclear.

## Chess
- [x] Chess: click a pawn on the chess board | Zach: I tried clicking and it did not do anything visible. Most other functionality below can't be tested unless this is working. | Zach: This was fixed about two weeks ago
- [x] Select pawn → click a pawn and verify it becomes the selected piece | Zach: How am I supposed to tell it's the selected piece?
- [x] Queens move (there was an bug where the raycasting for ovoid wasn't working properly. Sphere-tracing raycasts for the Ovoid primitive overstepped and missed from camera angles. Queen used ovoid, so those hits didn't work. This was fixed.)
- [x] Make legal move → select a pawn and click a legal destination square → verify the pawn moves there
- [x] Capture piece → make a legal capture → verify the captured piece is removed/moved appropriately
- [x] Try illegal move → select a piece and click an illegal destination → verify the move is rejected and the piece remains in its original position
- [x] Test board path blocking → use a sliding piece such as a rook/bishop/queen with a piece blocking its path → attempt to move through the blocker → verify the move is rejected 
**Special chess rules**: 
- [x] Promotion works (one path verified, not necessarily all type combinations) | Zach (2026-09-10): Verified in-app that promotion works when a pawn reaches the back rank (auto-promotes to Queen). Noted in ChessPromotingEveryPiece.md that while promotion executes, the 2D promote buttons at the top of the Chess screen have flawed logic affecting all back-rank pieces.
- [x] Castling works. Verified both black and white
- [ ] Threefold repetition
- [ ] Stalemate
- [ ] En passant

## First Mover Laws
- [ ] First Mover/Law toggle: disable → save → reload → verify disabled
- [ ] Re-enable → verify action works → re-enable the First Mover/Law → trigger its corresponding action → verify it executes

## Observe Test Feature
- [x] Look through the Observe Test feature. | Zach tested & verified findings: Only four tests show despite their being 60+ tests at the time of writing. Two of them are epistemically opaque. One does not load at all (the patch test, throws error). Only one displays something (spawns cubes throughout); requires code reference and position-inspection tool.

- [ ] Law Author: inspect/create/edit a Law → open Law Author → inspect an existing Law → create or edit a Law → verify the displayed Law is correct
- [ ] Save → reload → verify Law persists → save the Law/world → reload → reopen Law Author → verify the Law and its configuration remain
- [x] Law Author Library & Relation Graph | Zach tested & committed (commits 90f9b182 and 614714df): Nested category DAGs, 69 Laws organized, Relation Graph draws Law-to-Law Relations.
- [x] Searchable Property Lens redesign | Zach tested & verified (commits 90f9b182 and 614714df): Singular Type → Specific Singular → Specific Property 3-column control without dotted syntax, adaptive vector/color controls, @world/time context identification.
- [x] Law Author inspector redesign | Zach tested & verified (commits 90f9b182 and 614714df): Grouped palette sections, full-width labeled values, searchable Singular/concept selection.
- [x] Property Writers reverse lookup | Zach tested & committed (commit 614714df: "added reverse search for action nodes on properties"): Reverse index over nested Action trees, grouping by Law/Relation/IF/Action, direct card focusing.

- [x] Assets window: open → verify assets → save/load → verify again
- [x] Chat window → open Chat → send a test message → verify it appears correctly
- [ ] ImGui Demo → open ImGui Demo → interact with at least one visible demo control → verify it responds
- [x] Controls/Keymap (`K`) → press `K` → verify the Controls/Keymap window opens → verify controls are displayed | Zach: Keybinds are not exhaustive. For example, 
- [x] F8 → press F8 → verify the intended F8 action occurs
- [x] F9 → press F9 → verify the intended F9 action occurs
- [ ] F3 Performance Metrics (`F3`) & `Esc` → press `F3` to open performance metrics window → press `Esc` → verify the window remains open and cursor lock toggles as expected (rather than automatically closing the metrics window) → press `F3` again to toggle off
- [ ] Developer Tools Window (`~`) & `Esc` → press `~` to open Developer Tools window → press `Esc` → verify the window remains open and cursor lock toggles as expected (rather than automatically closing the dev tools window) → press `~` again to toggle off
- [x] `/` → press `/` → verify the intended `/` action occurs | Zach tested: tried with no windows loaded and "/" changed nothing visible; intended behavior was unclear.
- [x] H → press `H` → verify the intended H action occurs | Zach: Opens chat window, but pressing H again fails to toggle off.
- [x] K → press `K` → verify the intended K action occurs |


## Synthesis Studio (added 2026-09-02, from the play-test that corrected the audit's first pass)

- [ ] **Perlin hills + Living Studio Zone-native restoration — GPT-5.6 Sol, 2026-09-14:** fully quit any stale running Earthcall instance **without saving its old live Zones**, relaunch this restored build, then use Creator Console → Zones. Move to `NoiseFloorWorld` and verify the rolling Perlin SDF hills are visible again. Then Move to `SynthesisStudio.LivingInstrument` without loading a legacy World: play C5 and several other pads, verify each note is audible, and verify the corresponding floating resonator sphere visibly swells/rises and relaxes while the pad still depresses/releases. Automated Zone-native witnesses prove the SDF payload rehydrates and C5 reaches the audio sink at 523.25 Hz and grows its matching sphere; this checkbox is the remaining Person-facing visual/audio manifestation witness.

- [x] **Recovered Studio 3D poses — Zach + Codex, 2026-09-11:** VERIFIED BY ZACH in-app (commit 7abcd38c: "now the stuff in synthesis studio living is not one cube YAYYYYY"). Distinct rectangular forms and living instrument layout confirmed recovered and persistent.
- [ ] **Resonance Studio upgrade — Codex, session `synthesis-studio-20260904`, 2026-09-04 22:00 PDT:** reopen Earthcall and load `synthesis_studio` at the default 1280×720 window size; verify the full spectrum dock and upper-right voice/ink controls fit, text is readable, and the floating resonators remain visible above the easel.
- [x] **Play the room:** play C5 through B5 on both the desk and dock; each matching sphere should swell/rise, its colored meter should jump then settle, and the last-note caption should change; play repeatedly for over a minute and check responsiveness and animation feel.
- [ ] **Sound and ink:** select TRI, SINE, then SQR and compare their audible character; select TIDAL or ORCHID, enable DRAW, and drag on the easel to see the selected color; confirm existing artwork remains. Save/reload and repeat. Automated sink/serialization checks pass; actual sound and desktop feel still need a Person's witness.

- [ ] **2D controls on a HiDPI/Retina display** → open the Synthesis Studio → click any 2D HUD dock control → verify it responds. **Known blocking finding (Zach's play-test, 2026-09-02):** every 2D control is unclickable on a Retina Mac — the 2D draw and the 2D pick are in different coordinate spaces. See [SYNTHESIS_STUDIO_AUDIT_2026-09-02.md](../../../audits/SYNTHESIS_STUDIO_AUDIT_2026-09-02.md) §A0. Re-verify here after the fix, on both a Retina and a non-Retina display.
- [x] 3D console → click the two buttons and the four chord pads → verify orbs spawn and notes sound (audit reports every sound in the studio is currently silent)
- [ ] Slider → set a value → save → reload → verify the slider does not teleport on load
- [ ] Ambient theme toggle → toggle on → toggle off → verify it is not a one-way latch
- [ ] In-world & HUD click persistence after moving/switching tabs → open Synthesis Studio and Creator Console (F8) → click various tabs/buttons in Creator Console, load a different zone, switch back → select 3D Create tool → click in the world and click Synthesis Studio HUD buttons → verify clicks reliably spawn shapes and trigger HUD buttons without cursor lockout.
- [ ] Synthesis Studio sequential note pads & cross-zone 3D creation → open Synthesis Studio → click all 7 note pads in sequence (C5 through B5) for multiple passes → verify each pad visibly depresses on press, springs back on release, sounds each note every time, and does not freeze or lock out. Then switch to another zone (or use 3D Create tool in studio) → verify 3D creation tool reliably spawns shapes.

*Note (2026-09-02): the HiDPI finding above was found by walking, not by a test, and was
recorded first in an audit rather than here. Findings a Person discovers by hand belong on
this list — that is what it is for. See [The Week the Chorus Became a Queue](../../../Reflections%20on%20Earthcall's%20Progression/Reflections%20on%20Trajectory/The_Week_The_Chorus_Became_A_Queue.md) §6.*

## Basic 2D Button
- [x] Load `basic_2d_button` in-app -> verify 2D button renders on screen. | Zach marked done and verified, 2026-09-04
- [x] Click the button -> verify it visibly shifts (y increases). | Zach marked done and verified, 2026-09-04 (Law shifts x2D by 20.0 without UI C++ code)
- [x] Save -> reload -> click again -> verify it continues shifting. | Zach marked done and verified, 2026-09-04

## Basic Pixel Changer

- [ ] **GIMP & Clip Studio Style 2D Visual Authoring Suite (Gemini Spark, 2026-09-14)** → Load `BasicPixelChanger` (or `saves/worlds/basic_pixel_changer.json`) in `earthcall_webgpu`:
  1. **Full Creative Tool Rack (6 Action Cards + 16-Color Palette)**:
     - Verify 6 primary tool action cards in the left toolbar: `PEN [1px]`, `BRUSH [2px]`, `ERASER`, `FILL INK`, `CLEAR [WHT]`, and `SWAP [BLK]`.
     - Click `SWAP [BLK]`: verify quick switch to deep outline black (`#0a0a0a`).
     - Click `ERASER`: verify active ink switches to clean canvas white (`vec3(1.0, 1.0, 1.0)`) and painting erases colored texels.
     - Click `PEN [1px]`: verify drawing ink restores.
     - Click any of the 16 curated pixel art swatches (`BLK`, `WHT`, `SLT`, `SLV`, `RED`, `CRL`, `ORG`, `YEL`, `LIM`, `GRN`, `TEA`, `CYN`, `BLU`, `IND`, `PUR`, `PNK`): verify active ink and preview swatches update in real time.
  2. **Layers & Composite Engine Deck**:
     - Verify the right inspector features the dedicated **Layers & Engine Deck** card displaying `L1: INK [COPY-ON-WRITE]`, `L0: BASE [WHITE RGBA8]`, `BLEND: NORMAL • 100%`, and `ZOOM: 8x • GRID: 64x64`.
     - Verify the 4 Harmonic Tone variations (`TINT`, `MIDTONE`, `SHADE`, `ACCENT`) update the active ink on click.
  3. **Zero-Latency Inking & Continuous Stroke Painting**:
     - Press down on the canvas: verify instant inking on mouse-down (`object-pressed`).
     - Drag across the canvas: verify smooth stroke inking without dropped gestures.
  4. **Studio Spatial Balance**:
     - Verify all 66 authored beings render with balanced layout and high visual contrast at 1280×720.

- [x] **Basic Pixel Changer — actual Zone-only click path VERIFIED by Zach, 2026-09-10 20:36 PDT.** Zach relaunched after the Zone-scoped Law closure, clicked the canvas, and reported: “IT WORKS” and “I put red dots on it.” The native WebGPU manifestation, click edge, authored Law, and selected-color pixel write are now Person-witnessed. The separate second-color, disable-Law, and quit/reload persistence experiments below remain open where not explicitly witnessed.
- [x] **Basic Pixel Changer (Zach + Codex, session `01a07d15-f266-7902-bc11-cf7b06b0b343`)** → VERIFIED BY ZACH: Clicking canvas successfully paints addressed pixels without crashing; verified drawing red dots on canvas (commit 3763c05f: "YAAAYYYYYY I FINALLY DREW RED DOT ON THE CANVAS THING") and subsequent fixes in c72d97d8 and 3b3f343d.
- [x] **Basic Pixel Changer — first-click render crash** → VERIFIED FIXED BY ZACH: App no longer aborts on clicking canvas; Zach clicked and verified live in-app (commit 3763c05f).
- [x] ~~Basic Pixel Changer — canvas rendered red instead of white (Zach hit it live, Claude diagnosed/fixed, 2026-09-07 13:3x PDT)~~ — **superseded by the entry below**: this fix (Zone-identity field-level merge + `faceColors` serialization) was real and necessary but not sufficient — Zach still saw red after it.
- [x] **Basic Pixel Changer — canvas STILL red after two rounds of fixes** → VERIFIED BY ZACH (2026-09-08/10): Zach visually confirmed canvas renders white with black text, and clicking paints colored pixels at click points without `.ecmatter` overwriting with legacy red default.

## Perlin Noise Floor & 3D Raymarching
- [ ] **Perlin Noise Floor Hill Zone rendering performance (implementation pass, session 2026-09-05):** Zach's 2026-09-05 play-test found horror-film-like whole-frame jitter/tearing, Sanctum reporting ~100 submitted FPS on the internal 60 Hz Mac panel, and the existing Perlin 3D-phase 1→100→300 ms oscillation still present. The macOS surface now uses FIFO plus display sync; load `Perlin Noise Floor Zone` and Sanctum in the WebGPU app (`Run Earthcall.command`) and confirm full-frame motion is coherent and the internal panel is paced at its actual refresh rate (an external high-refresh monitor may legitimately report its higher cadence). Then look toward the horizon and at 45 degrees, then build/place objects (cubes or house structures). Press `F3`: it should label the 3D phase, surface acquire, and queue-submit values as CPU wall-clock observations rather than GPU duration. If the adapter exposes timestamp queries, it should additionally show a delayed `GPU main render pass` duration that changes with the scene but does not stall the UI; otherwise it must say timestamps are unsupported. Verify the authored Perlin surface remains unchanged, placed objects render, occlude, and settle on the same hills without a GPU hitch; record whether the 100–300 ms queue-debt oscillation remains. The pass also needs a camera-inside-proxy check and ground selection/highlight check.
- [ ] **Perlin Flash-vibration A/B (Codex session `01a072e2`, 2026-09-05 19:12 PDT):** Proxy culling was restored to two-sided and did not remove the vibration. The renderer now also uses the original single queue-ordered buffer pool instead of the retracted four-pool rotation. In the same saved Perlin zone, move and turn continuously, then confirm whether whole-frame past/future-position flashes remain; record the result before any other rendering change.
- [ ] **Perlin temporal-coherence A/B (Codex session `01a072e2`, 2026-09-05 19:18 PDT):** Zach established that every moving visual—including ImGui—leaves ghost positions, so this is a frame-stream defect rather than Perlin or locomotion. Native GPU timestamp instrumentation is suspended for this run; the optional F3 GPU-duration row is expected to be unavailable, while CPU rows remain. Move the camera, a gravity-affected object, and an ImGui window independently; record whether each still ghosts along its path.
- [ ] **Perlin culling restoration (Codex session `01a072e2`, 2026-09-05 21:24 PDT):** With timestamp instrumentation still disabled and temporal coherence restored, proxy back-face culling is back on to eliminate duplicate analytic-field fragment launches. In the saved Perlin zone, confirm moving camera/objects and dragged ImGui remain coherent, then check inside/outside-proxy views and ground selection/highlight coverage.
- [ ] **Perlin cold/warm convergence isolation (Zach + Codex session `01a072e2`, 2026-09-06 12:12 PDT):** Zach observed one live run improve progressively from roughly 10 FPS / 80 ms to roughly 60 FPS / 10 ms, including a warm horizon view. Fully quit and relaunch, load the Perlin Zone, hold one horizon camera completely still for 120 seconds, and record F3 acquire/3D times near 0, 10, 30, 60, and 120 seconds. Repeat after a fresh relaunch while moving continuously, then leave and re-enter the Zone without quitting. Record whether convergence follows elapsed frames, movement, process lifetime, or Zone lifetime; keep native GPU timestamp queries disabled so the former Flash-phasing instrumentation is not reintroduced.
- [ ] **Perlin repeatable baseline corpus (Zach + Codex session `01a072e2`, corrected 2026-09-06 18:27 PDT):** Zach confirms the earlier 10–20 ms warm result occurred around 2500x1574, but clean rebuilt tests of both `20549cbe` and `1dfceb1d` subsequently settled near 30 ms. Many Chrome tabs were open during both the earlier ~30 ms state and the 10–20 ms outlier; closing tabs during the later experiment was not an isolated intervention and must not be credited with improvement. Treat 10–20 ms as an unreproduced outlier, not an optimization baseline. On current `7bda885b`, use one recorded camera position/orientation and framebuffer, perform at least three fresh-process runs, and record cold and warm F3 surface-wait/3D p50/p95 plus surrounding desktop/GPU load. Only attribute a future improvement when the distributions reproduce at the same camera and workload.

## Far Lands

- [ ] **Gravity defaults off (authorized by Zach; Codex session `01a072e2`, 2026-09-06 18:34 PDT):** Load the `far_lands` world from a fresh application start and verify `physics-gravity` appears disabled immediately. Place or release an object and confirm it does not fall until the existing gravity Law is explicitly enabled; then enable it and confirm falling resumes. | Zach tested Far Lands loading and reported that it "doesn't lag like crazy anymore, but I'm not sure if the shape was preserved. Either way, I'm really excited to make the full one." (EARTHCALL_WEEKLY_ANALYSIS_2026-09-05_TO_2026-09-09.md). Gravity toggle verification remains open.

## Attribution in the Synthesis Studio saves — resolved

- [x] ~~**Which Synthesis Studio pass was Astra, not Codex?**~~ **Answered by Zach's own Broadcast #5**, restored 2026-09-07: he asked Astra *"make the Synthesis Studio cooler"*, which is the `## 2026-09-04 — Play the room: resonance Studio` pass. Astra ran **through the Codex harness**, so `studio.author.codex` in the saves is truthful and stays; only the prose needs the model added. → [full task](../Specific%20Tasks/Resolve_the_Codex_signature_into_named_models/Resolve_the_Codex_signature_into_named_models.md)

## Synthesis Studio — Astra's pass, from Broadcast #5 (2026-09-07)

*These are Zach's own play-test findings, transcribed here from the broadcast so they stop living
only in an intercom file. Marked done where Zach has already stated the verdict.*

- [x] ~~Resonance rectangle + sound-blip animation above the note pads~~ — **verified good by Zach**: *"like an actual blip that feels like sound… like a professional music DJ software would animate it."* Most of Astra's pass **worked on the first try**.
- [x] ~~Design writing ("C major" in the note pad, purpose-named parts)~~ — **verified good by Zach**, and he read it as making the scope explicit so it can be widened later.
- [x] ~~Resonance rectangle rendered *just barely* lighter than the pad beneath it~~ — **verified good by Zach**: *"barely perceptible but it changes the feel from 'oh just another block' to 'oh this is the thing but sound version'."* Do not "fix" this contrast; it is deliberate and it works.
- [ ] **Blue/violet hue inconsistency (Zach's one complaint).** Every note pad and its amplifier rectangle share a hue, except the blue one — its amplifier is not merely lighter, it is **more violet**. Zach: the discrepancy **predates Astra**, which left it in place. He wants **consistency**, because the point is for the pair to represent the same note colour: *"if u wanted violet dispreancy on purpose u gotta make it mean somethign real not just a random quirk."* Fix the blue pair to one hue, and see the note below about giving violet a pad of its own.
- [ ] **Draw pad enrichment is unverifiable.** Astra also enriched the draw pad, but the draw pad **was already broken before Astra arrived**, so Zach has never seen that part. Re-check once the draw pad works — this is downstream of the Synthesis Studio click-lockout work, not a separate defect.

## Formation Rete rung 0 — relation-shaped laws that were silently deaf

*Raised 2026-09-08, Claude Opus 5, session `session_01K1PtKNZtSDU9XGwKZQ7ZzF`. The spec's §1.2(a)
says the deafness hit **"every 'every instance of this category, every tick' law in the tree, the
Synthesis Studio's slider among them."** A green suite is not a witness for that — the fix is in
what a hand feels. → [full task](../Specific%20Tasks/Formation_Rete/Formation_Rete.md)*

- [ ] **Synthesis Studio — laws that watch relations formed during play.** Open the Studio in `earthcall_webgpu` (`Run Earthcall.command`). Any behaviour that depends on a relation being formed *while you play* — the slider, note-reactive lights, ink reaching a newly drawn stroke — should now respond on the tick the relation forms, where before it would have stayed dead for the rest of the session. Confirm nothing that already worked has started firing *too* eagerly.
- [ ] **Chess and Go — no over-firing.** Both are relation-heavy. Play a few moves in each and confirm pieces behave as before: rung 0 makes laws hear *more*, so the risk to look for is a law that now fires when it should not, not one that stays quiet.
- [ ] **A relation you form and then break.** Form a relation in-world, watch the dependent law take hold, then dissolve it and confirm the law stops. Nothing retracts the stale fact by design (it is safe — the law re-checks the live graph), so this is the check that the safety actually holds in the running app rather than only in the test.

## Robust Native File I/O & File Types (FileChannel)

*Landed 2026-09-08, Gemini Spark. Hardened `@file-channel` with atomic write swaps, append mode, DoS bounds, MIME type/magic sniffing across images/audio/models/substrates, and Base64/Hex binary pipelines. → [full task](../Specific%20Tasks/Robust_File_IO_and_Wide_File_Type_Support/Robust_File_IO_and_Wide_File_Type_Support.md)*

- [ ] **Author Law interacting with `@file-channel`.** Open the Law Authoring / Creator Console. Point `@file-channel.path` at a file (e.g. `saves/test.json` or an image/sound) and verify that `@file-channel.mimeType`, `@file-channel.fileType`, `@file-channel.size`, and `@file-channel.jsonValid` reflect the file's properties accurately.

## Screen Recorder in the Singularity (@screen-recorder)

*Landed 2026-09-08, Gemini Spark. Added `@screen-recorder` Sense-Act first mover with in-engine viewport, host display, and window modes, PPM/PNG/raw stream output, and automatic fallback to viewport when OS screen capture is unpermitted. → [full task](../Specific%20Tasks/Screen_Recorder_in_Singularity/Screen_Recorder_in_Singularity.md)*

- [ ] **Record In-Engine Frame Sequence or Snapshot.** In the Creator Console, inspect `@screen-recorder`:
  - Set `@screen-recorder.recording := true` to capture live rendering frames to `saves/recordings/`.
  - Set `@screen-recorder.snapshot := true` to capture an instant screenshot.

## Streaming Pipes and Process Pipelines (@stream-channel)

*Landed 2026-09-08, Gemini Spark. Added `@stream-channel` with POSIX FIFO named pipes, process stream execution (`popen`/`pclose`), chunked streaming, and Base64 stream transport. → [full task](../Specific%20Tasks/Streaming_Pipes_and_FIFOs/Streaming_Pipes_and_FIFOs.md)*

- [ ] **Test Process Pipe or FIFO.** In the Creator Console, point `@stream-channel.target` at a pipe or command (e.g. `cat > /tmp/test_pipe.txt`), trigger `@stream-channel.open := true`, write via `@stream-channel.chunkData`, and verify direct data flow without intermediate file polling.

## Virtual File System (@vfs)

*Landed 2026-09-08, Gemini Spark. Built `@vfs` resolving `save://`, `zone://`, `home://`, `recording://`, custom prefix mounts, and `memory://` zero-disk ephemeral RAM files, transparently integrated into `FileChannel`. → [full task](../Specific%20Tasks/Virtual_File_System_VFS/Virtual_File_System_VFS.md)*

- [ ] **Test Universal URI Resolution & In-RAM Files.** In Creator Console, point `@file-channel.path := "memory://test_scratch"` or `"save://worlds/my_world.json"` and execute read/write; verify `memory://` files never touch disk while `save://` resolves portably across platforms.

## File Watcher & Live Hot-Reloading (@file-watcher)

*Landed 2026-09-08, Gemini Spark. Built `@file-watcher` Sense-Act first mover with configurable directory scanning, extension filters, and ECA edge events (`file-modified`, `file-created`, `file-deleted`) for real-time asset, shader, and rule hot-reloading. → [full task](../Specific%20Tasks/File_Watcher_Live_Hot_Reloading/File_Watcher_Live_Hot_Reloading.md)*

- [ ] **Live File Hot-Reloading.** In the Creator Console, inspect `@file-watcher`:
  - Set `@file-watcher.watchPath := "saves"` (or a specific directory/file).
  - Open a file in that directory in an external editor (VS Code, TextEdit) and modify or save it.
  - Observe that `@file-watcher.lastEventType` immediately updates to `"file-modified"` and publishes `ECA::Event("file-modified")` without requiring an app reload or manual polling.

## Microphone Audio Capture & Recording (@microphone)

*Landed 2026-09-09, Gemini Spark. Built `@microphone` (`@audio-recorder`) Sense-Act first mover with macOS CoreAudio capture, hardware device enumeration (built-in, USB, AirPods), TCC permission status checks, live RMS/peak metering, speech/silence detection ECA events, and canonical 16-bit PCM WAV recording. → [full task](../Specific%20Tasks/Microphone_Audio_Recording_Subsystem/Microphone_Audio_Recording_Subsystem.md)*

- [ ] **Test Live Microphone Recording & Telemetry.** In the Creator Console, inspect `@microphone`:
  - Verify `@microphone.devices` lists your Mac's connected inputs (e.g. "MacBook Pro Microphone", USB interfaces, AirPods).
  - Check `@microphone.hasPermission` and `@microphone.permissionStatus` (if denied, grant in System Settings > Privacy & Security > Microphone).
  - Set `@microphone.recording := true` (or trigger `@microphone.start := true`), speak into the mic, and observe `@microphone.inputLevel` and `@microphone.peakLevel` moving in real time.
  - Set `@microphone.recording := false`. Verify a standard `.wav` file is saved to `saves/recordings/` (or specified `@microphone.outputPath`) and can be played back with QuickTime / Audacity.


- [ ] **Synthesis Studio / Living Instrument** (Codex / GPT-6 Astra, session `01a07eb3-8ee7-7aa3-8b34-65fea2f4cd44`, 2026-09-08 23:37 PDT): Load `synthesis_studio_living`; confirm this is the clean room, with no canvas-spam imports, while your original Studio remains intact. Play all twelve HUD/desk notes, change octaves 3–6 and Solo/Fifth/Major/Minor with each voice, and judge tuning, dynamics, responsiveness, and the shared pad/meter/resonator hue. Drag the expression field from lower left to upper right and feel whether bloom, motion, and dynamics follow your hand coherently. Turn Sound Ink on, play a note, enable Draw, and drag slowly over the easel; confirm its marks carry that note's color and sound on hover. Save, quit, reopen, and confirm the expression, harmony, octave, and marks remain yours. Judge the constellation's movement, text readability, and layout in your usual window size.

## Formation Rete rung 1 — the engine got ~9x faster on law-heavy worlds

*Raised 2026-09-09, Claude Opus 5, session `session_01F9nK3FZ7VR4PFPTUWfYyvm`. Every transient
`ECA::Event` was destroying a `Moment` — which is a `Singular` — and each destruction scanned the
whole Rete fact table. Measured at 320 beings: **593 ms/tick → 63 ms/tick**. This is a speed
change to the tick every law application pays, so what needs a Person is that nothing MOVED
differently, only faster. → [full task](../Specific%20Tasks/Formation_Rete/Formation_Rete.md)*

- [ ] **A law-heavy world should feel faster, and behave identically.** Open `chess_app`, Synthesis Studio, and Far Lands in `earthcall_webgpu` (`Run Earthcall.command`). Watch the F3 `LawManager::tick` figure — it should be markedly lower than you remember on the same world. Then play: pieces move legally, pads sound, terrain loads. The risk to look for is a law that no longer fires, or one that fires when it should not.
- [ ] **Beings that leave the world while laws watch them.** The fix makes "this being never had facts" an O(1) answer, so the check that matters is the opposite case: delete objects a law is actively targeting, in a busy zone, and confirm nothing crashes and no law keeps acting on the deleted thing. That path is where a wrong answer would show as a dangling read rather than a slow frame.
- [ ] **`frame_lag_test` wants re-recording, and only you should authorize it.** The `LawManager::tick` baseline in `tests/singularity/frame_lag_baseline.txt` was recorded before this fix and is now far above what the engine costs. Leaving it means the file overstates the cost; changing it means editing a baseline, which `AGENTS.md` says never to do to quiet a line. This one is not being quieted — it is genuinely faster — but it is your call to re-record.

## Formation Rete rung 2 — the sweep got ~1.8x faster, and a fourth deaf-law bug closed

*Raised 2026-09-09, Claude Opus 5, session `session_01F9nK3FZ7VR4PFPTUWfYyvm`. Two changes a
Person should feel. → [full task](../Specific%20Tasks/Formation_Rete/Formation_Rete.md)*

- [ ] **Laws that read a property granted DURING play.** This is the fixed bug, and it is the one worth walking. Any law watching a property that a *different* law grants at runtime — `AddProperty` actions, authored `warmth`-style grants, anything a tool adds mid-session — was permanently deaf to that being: the property existed, the law was enabled and compiled, and it simply never fired for it. Grant a property in-world and confirm a law watching it now takes hold on that being, in the same session, without a reload.
- [ ] **Nothing over-fires.** Both rung 2 changes make laws reach *more*, so the risk is the opposite of deafness. Play chess, Go, and Synthesis Studio and confirm no law is now acting on beings it should not — especially anything with an `AddProperty` action.
- [ ] **Objects deleted mid-play, in a law-heavy zone.** `Zone::removeObject` now announces that the world's shape changed; before, only the unmaking path did. The vocabulary index holds raw pointers, so this is the path where a wrong answer would be a crash rather than a slow frame. Delete objects a law is actively targeting and confirm the app stays up and the law stops acting on them.

## Zone identifier/name split — the "3 live objects across Zones (same name)" refusals

*Raised 2026-09-09 by Zach pasting live console output; fixed same day, Claude Sonnet 5,
session `01MsayKP3NYfQAyBtyQ8xeA1`. → [full task](../Specific%20Tasks/Zone_identity_store_field_level_merge/Zone_identity_store_field_level_merge.md)*

- [ ] **Reload `synthesis_studio_living` (or whatever world showed the refusal spam) and watch the console.** Before this fix, loading it logged dozens of `applyMatterFlatBuffer: entity '...' has no exact owner match and matches 2/3 live objects across Zones (same name printed more than once)` lines — a real duplicate-live-Zone bug, not a cosmetic one. That spam should be gone or much reduced. Any REMAINING "matches N live objects" line naming genuinely different-named Zones is a separate, pre-existing ambiguity (Invariant 3 working as designed on legacy ownerless records) — not this bug.
- [ ] **Objects that live in "Basic 2D Button Zone" or "Perlin Noise Floor Zone" should now show their authored geometry/position**, not a default transform — those were the two names actually colliding in what you pasted. If either still looks wrong, say so; it would mean a second, different cause in the same area.
- [x] **`saves/zones/BasicPixelChanger/`'s folder/identifier mismatch resolved.** Repaired under Zach's explicit authorization (2026-09-09) changing "Basic Pixel Changer" to "BasicPixelChanger", eliminating live duplicate minting.

## Model Context Protocol (MCP) Server Bridge (@modelcontextprotocol)

*Landed 2026-09-09, Gemini Spark. Added Option A: Node.js/TypeScript MCP Server under `src/Singularity/Foreign/mcp/` and `scripts/mcp-server.js` exposing 17 Earthcall tools over standard stdio JSON-RPC to external AI models (Claude, Cursor, Gemini). Upgraded to v2 following Claude & Zach's live field testing report. → [full task](../Specific%20Tasks/Model_Context_Protocol_MCP_Server_Bridge/Model_Context_Protocol_MCP_Server_Bridge.md)*

- [x] **Wire the server into Claude Desktop's config.** *Done 2026-09-09, Claude Sonnet 5, session `01TM2LcwwRWs1qgnTxUXLfeA`.* Added an `earthcall` entry under `mcpServers` in `~/Library/Application Support/Claude/claude_desktop_config.json` (backed up first to `claude_desktop_config.json.bak-20260909135837` alongside it), pointing at `scripts/mcp-server.js` with `EARTHCALL_WS_URL=ws://localhost:8080`. Verified `node scripts/mcp-server.js` starts cleanly and logs `Server initialized and listening over stdio.`; verified `@modelcontextprotocol/sdk` is present in `node_modules`; verified the edited config is still valid JSON and every pre-existing key survived untouched. **Quit and reopen Claude Desktop** for it to pick up the new server — it wasn't running when this was made, so no restart was forced on you.
- [x] **Wire the server into Claude Code CLI (this tool).** *Done 2026-09-09, Claude Sonnet 5, session `01TM2LcwwRWs1qgnTxUXLfeA`.* Ran `claude mcp add earthcall -s local -e EARTHCALL_WS_URL=ws://localhost:8080 -- node /Users/zacharyzhang/Documents/GitHub/Earthcall/scripts/mcp-server.js`. Scoped `local` (private to you, stored in `~/.claude.json` under this project, not committed to git) rather than `project`, since the config's absolute path is machine-specific. `claude mcp get earthcall` confirms `Status: ✔ Connected`. Only a **new** Claude Code session in this project directory will see the tools — this already-running session started before the server was registered.
- [x] **Wire the server into Codex (Codex CLI + ChatGPT desktop app's Codex agent — they share one config).** *Done 2026-09-09, Claude Sonnet 5, session `01TM2LcwwRWs1qgnTxUXLfeA`.* Confirmed both surfaces read `~/.codex/config.toml` (ChatGPT.app's own `node_repl`/`computer-use` MCP entries were already sitting in that same file). Ran `codex mcp add earthcall --env EARTHCALL_WS_URL=ws://localhost:8080 -- node /Users/zacharyzhang/Documents/GitHub/Earthcall/scripts/mcp-server.js`; `codex mcp get earthcall` confirms it's registered (`enabled: true`, `transport: stdio`). **Both ChatGPT.app and any new `codex` CLI session were/are running — restart them** to pick it up.
- [x] **Wire the server into Antigravity (Antigravity IDE + Antigravity CLI — they share one config).** *Done 2026-09-09, Claude Sonnet 5, session `01TM2LcwwRWs1qgnTxUXLfeA`.* No `antigravity`/`gemini` CLI binary was on PATH to do this via a command, so I hand-wrote `~/.gemini/config/mcp_config.json` directly (it was a valid but empty 0-byte file — backed up first, though there was nothing in it to lose) with an `earthcall` entry under `mcpServers`, same `command`/`args`/`env` shape as the Claude Desktop config. Verified the result parses as valid JSON. **Antigravity.app was running — restart it** to pick this up. Not yet live-tested (no `antigravity`/`gemini` CLI present here to smoke-test the way I did for Claude/Codex) — flagging in case Antigravity's actual schema key differs from what public docs showed me (`serverUrl` is used there for *remote* HTTP servers instead of `url`, so it's plausible local stdio servers have a similarly non-obvious key name).
- [x] **Run MCP Server v2 & Test Live Tool Execution:** | Zach and Claude (Opus 4.6 via Claude Desktop) field-tested live: queried world state, verified protocol handshake, and spawned 14 entities into "Clawd's Monastery" zone (commit 3fd31cc9: "Zone work and YAYYYY CLAWD IS IN EARTHCALL NOWWWW" and commit 341920c5).
  1. In terminal: `export PATH="/opt/homebrew/bin:$PATH"`
  2. Start Earthcall in one window (`Run Earthcall.command`).
  3. Restart Claude Desktop or run `node scripts/mcp-server.js`.
  4. **Test OntoMath Law Authoring (`earthcall_author_law`):** Ask Claude to author an oscillating or flow Law with formula `"sin"`, e.g. on `position.y` or `color.r`. Verify in Law Graph that the Law has real Condition and Action ASTs, executes in the live world, and animates entities!
  5. **Test Property Writes (`earthcall_write_property`):** Ask Claude to change an object's color, position, or write `field.expr := "smoothUnion(sphere(0.5), box(0.4), 0.1)"`. Verify it immediately returns a success acknowledgment and live-converts the entity into a raymarched SDF field!
  6. **Test Dedicated SDF Field Creation (`earthcall_spawn_field`):** Ask Claude to spawn an SDF field. Verify it appears live with smooth raymarched geometry!
  7. **Test Zone Switch Persistence:** Switch zones and switch back; verify all 14 objects in "Clawd's Monastery" are preserved in memory and on disk!
  8. **Test Law Deletion:** Delete an authored Law (`earthcall_delete_law`); verify the engine safely removes the Law on the main thread without crashing!

## Zone identity boundary (Invariant 6, Stage 1) — found a real duplicate on your own Home

*Landed 2026-09-09, Claude Sonnet 5, session `01MsayKP3NYfQAyBtyQ8xeA1`, per Sol's staged plan on the agent intercom. → [full task](../Specific%20Tasks/Zone_identity_store_field_level_merge/Zone_identity_store_field_level_merge.md)*

- [ ] **Reload your world and confirm Home looks exactly as it did before** — all 104 objects, nothing missing. While implementing this, I found `saves/zones/Home/zone.json` (a stale, 32-object duplicate) sitting alongside the real `saves/homes/Home/home.json` (104 objects, the one your saves have actually been updating). Both claimed the identity "Home" — harmless before because the old code silently ignored the second one it saw, but the new validation this pass adds would have made your ACTUAL Home refuse to load entirely. You authorized moving the stale one aside (not deleting it): it's now at `saves/backups/Home.orphaned-2026-09-09/zone.json`, fully intact and recoverable, just no longer claiming the "Home" identity. Nothing about your live Home should look any different — please confirm.
- [ ] **The Basic Pixel Changer canvas should still work exactly as before.** You separately authorized a one-line fix to `saves/zones/BasicPixelChanger/zone.json`'s `identifier` field (was `"Basic Pixel Changer"` with a space, mismatching its folder; now `"BasicPixelChanger"`, matching). This was the same class of bug Sol originally found in this file back on 2026-09-08. Reload it and confirm the canvas still opens and paints as it did after that first fix.
- [ ] **If you ever see a console line starting `[ZoneManager] hydrateFromZoneStore: REFUSED`**, that means a Zone or Home identity file's folder name doesn't match its own internal `identifier` field, or two identity files are claiming the same identity — the new validation this pass adds. It will name the exact file(s) involved. That Zone simply won't load until the file is fixed; nothing is silently guessed at or auto-repaired. Send me the message if you see one and don't recognize why.

## Formation Rete rung 3 — a fifth deaf-law bug, and the oldest one yet

*Raised 2026-09-09, Claude Opus 5, session `session_01F9nK3FZ7VR4PFPTUWfYyvm`.
→ [full task](../Specific%20Tasks/Formation_Rete/Formation_Rete.md)*

- [ ] **Laws that touch `shape.*` and are NOT `WhileTrue`.** This is the big one. A law's required vocabulary is the path's root (`shape`), but the property is registered as `shape.fillet` — there is no property called `shape` — so `couldApplyTo` refused every being and **any sweep-path law touching a shape parameter reached nobody at all**, silently. That means `OnBecomeTrue` laws, and any law without compiled Rete terminals, that set or test `shape.fillet`, `shape.r`, `shape.kind` and friends. If you have ever authored such a law and quietly concluded it "didn't work", try it again — it should work now. This is the check most likely to change something you actually see.
- [ ] **Laws gated on a state being or channel.** Anything conditioned on an `@`-rooted referent (the ambient theme, the draw-mode indicator, the slider clamp, the crystal's pulse) is now skipped wholesale while its gate is shut, instead of being tested against every being. Toggle those gates off and on repeatedly in Synthesis Studio and confirm the laws stop and **restart** — the restart is the risky half, because a hoist that forgot to release its held subjects would never re-fire.
- [ ] **Nothing over-fires.** Both rung 3 changes make laws reach *more* beings, not fewer. Play chess, Go, Far Lands and the Studio and watch for a law now acting where it should not.

## α-node sharing — the Rete network should be much smaller, and visibly so

*Raised 2026-09-10, Claude Opus 5, session `session_01F9nK3FZ7VR4PFPTUWfYyvm`, adjudicating o3's
`DEEP_CODEBASE_ANALYSIS_2026-09-07` claim. → [the analysis](../../../Analysis/DERIVED_STATE_AND_THE_SILENCE_OF_LAWS_2026-09-10.md) §9b*

- [ ] **Look at the new "Rete network" block in the performance window.** `PerformanceMetricsWindow` now shows alpha/beta node counts, live fact count, and "fact refs held", plus the prophetic filter's skip rate. Open chess in `earthcall_webgpu` — chess states `instance-of category.chess.piece` in **76 separate laws**, so before this change the network carried 76 identical nodes each holding its own copy of the same match set. The alpha count should now be far below the number of laws. This is the first time the network's own size has been visible at all.
- [ ] **Chess and Go must play identically.** Sharing means many laws now read one node. `chess_app_test`, `go_app_test` and `rete_compile_test` pass, but a shared node binding the wrong law is exactly the kind of fault a suite can miss and a hand cannot: play a real game in each and confirm no piece behaves differently, and especially that nothing has gone *quiet*.
- [ ] **Synthesis Studio chord pads.** `synthesis_studio_living.json` states `isChordPad == true` in ten laws, which now share one node. Play the pads and confirm all of them still sound and light.

## IDE Docking Mode for First Mover Window Tools

*Landed 2026-09-11, Antigravity. Added IDE Docking Mode attaching ImGui windows to screen edges (Left sidebar, Right sidebar, Bottom bar drawer), keeping the central 3D viewport clear and unoccluded. → [full task](../Specific%20Tasks/IDE_Docking_Mode/IDE_Docking_Mode.md)*

- [x] **Toggle IDE Mode (`F10` or Menu `M`)**: | Zach tested, expanded, and verified in-app window layout and IDE docking manager in commits c72d97d8 ("YAYYYY NEW WINDOW LAYOUT AND BAISC PIXEL CANVAS BUG FIX") and 3b3f343d (Left Sidebar, Right Sidebar, Bottom Drawer, Center Viewport, corner dynamic priority, and stacked multiple window layout).
  1. Launch Earthcall WebGPU: `Run Earthcall.command` or `./build/earthcall_webgpu`.
  2. Press `F10` (or press `M` and select "Toggle IDE Mode" from the main menu) to switch into IDE Docked Mode.
  3. Notice the top workspace bar appears: `[≡ IDE DOCKED]` with panel indicators `[Left: 3]`, `[Right: 2]`, `[Bottom: 1]`.
  4. Verify the **Left Sidebar** opens docked to the left edge of the viewport containing tabs:
     - **Creator Console [F8]**
     - **Dev Tools [`]**
     - **Creation [F9]**
  5. Verify the **Right Sidebar** opens docked to the right edge containing tabs:
     - **Metrics & Coords [F3]**
     - **Keymap [K]**
  6. Verify the **Bottom Drawer** opens docked across the bottom containing:
     - **Chat [H]**
  7. Verify the **Center Viewport** remains completely open, unobstructed, and responsive to 3D camera navigation and object interactions.
  8. Drag the inner splitters (the edge of Left, Right, or Bottom panels) and verify resize cursors (`<->` and `^v`) appear and panels resize smoothly.
  9. Click `[❐ Float]` on any panel header to pop it out into an independent floating window; click `[◧]`, `[◨]`, or `[⬓]` to re-dock it.
  10. Press `F10` again to toggle back to traditional floating window mode; verify all windows restore cleanly to their floating positions.
   11. **Corner Collision Dynamic Priority (Zero Void)**:
       - In IDE Mode, open Left Sidebar (`F8`) and Bottom Bar (`H`).
       - Observe the bottom-left corner: notice there is **zero empty void/gap**!
       - Because Bottom Bar was opened most recently, Bottom Bar extends from `X = 0` across the corner, while Left Sidebar sits flush on top of it.
       - Now click any tab or control inside Left Sidebar: Left Sidebar takes dynamic priority, extending all the way to the bottom edge of the screen, while Bottom Bar shrinks flush against the sidebar's right edge.
       - Test the bottom-right corner with Right Sidebar (`F3`) and Bottom Bar (`H`) and verify the exact same zero-gap priority behavior.
   12. **Stacked Multiple Windows Layout**:
       - In Left Sidebar (with both Creator Console and Dev Tools open), click `[☷ Stack]` in the panel header or the top workspace bar.
       - Verify both tools appear **simultaneously stacked vertically** inside the sidebar!
       - Click `[▼]` on either pane's header: verify it collapses accordion-style into a compact header bar (`[▶]`), yielding space to the other tool. Click `[▶]` to expand it again.
       - Drag the divider between stacked panes to resize them vertically.
       - In Bottom Bar (with multiple tools docked), toggle `[☷ Stack]`: verify tools arrange as side-by-side columns with draggable vertical splitters!
       - Click `[▤ Tabs]` at any time to return to single-active tabbed mode.

## `OnBecomeTrue` laws fire once again — this one you will SEE

*Raised 2026-09-14, Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`.
→ [full task](../Specific%20Tasks/Formation_Rete/Formation_Rete.md)*

From `698059e0 Rete performance sweep hunt` (2026-09-13) until this fix, **every `OnBecomeTrue` law in the running app fired every frame for as long as its condition held**, instead of once when it became true. The test harness hid it because tests often run the law engine disconnected; the app never does.

- [ ] **Anything that should happen ONCE now happens once.** In `earthcall_webgpu`, find laws that react to something becoming true — a button pressed, a piece reaching a square, a threshold crossed — and confirm the reaction happens a single time, not continuously. Between 09-13 and this fix you would have seen accumulating values (anything with `add`), repeated spawns, repeated sounds, and repeated events while the condition stayed true. If you noticed something "firing like crazy" in that window, this is probably why.
- [ ] **Holding something true must not keep re-triggering, but letting go and re-doing it must.** Press-and-hold a control that drives an `OnBecomeTrue` law: one reaction. Release and press again: exactly one more.
- [ ] **`WhileTrue` laws must still run continuously.** The fix separates the two; confirm continuous behaviours (a glow while hovered, a meter while a note plays) still update every frame.

## Laws keep working after you switch Zones

- [ ] **Walk from one Zone into another, and back.** Laws that apply to beings by their properties should act on the beings in the Zone you are standing in — not on the Zone you just left, and not on nobody. Before this fix, switching between two Zones whose laws used the same property names left those laws silently aimed at the previous Zone.

## Category-scoped laws: faster, and they hear relations come and go

*Raised 2026-09-14, Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`.
→ [full task](../Specific%20Tasks/Formation_Rete/Formation_Rete.md)*

Laws scoped as "every instance of category X" (`Related(instance-of, category.X)`, the idiom 132 chess laws use) cost ~17x an ordinary law at 400 beings. They now cost the same. The fix also closed two ways such a law could stay silent.

- [ ] **Chess and Go feel no slower, and ideally smoother.** Load a chess world in `earthcall_webgpu` and play a few moves. Nothing should respond more slowly than before. Measured headless, the category-scoped law itself got ~17x cheaper.
- [ ] **A relation removed and re-made fires its "became true" law again.** In a world with a law that reacts once when a being becomes related to something (`OnBecomeTrue` + `Related`), use the Relations console to **Break** that relation and then **Create Relation in Zone** again with the same endpoints and kind. The reaction should happen again. Before the fix it happened the first time only.
- [ ] **Changing a relation's type reaches laws.** The console cannot retype a relation; write its `type` property instead, with a law or the `earthcall_write_property` tool, to a kind a law is watching. That law should start acting on the endpoint. Before the fix it never did.

## Clicking a chess piece should feel lighter

*Raised 2026-09-15, Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`.
→ [full task](../Specific%20Tasks/Formation_Rete/Formation_Rete.md)*

Every click in chess runs ~30 "every piece" move laws, each checking every candidate piece. Checking a piece that didn't match cost 49 µs, and 45 µs of that was a temporary event object walking every relation in the world when it was thrown away. It now costs 8.7 µs (Debug build, headless).

- [ ] **Select and move pieces quickly in a chess world in `earthcall_webgpu`.** The response to a click should feel at least as immediate as before, ideally snappier. Nothing about which moves are legal should have changed.
- [ ] **Delete a piece or relation while playing, then keep playing.** Relations still have to let go of beings that are removed. A crash or strange behaviour right after deleting something would point at this change.

## "Became true" laws fire again every time, not just the first time

*Raised 2026-09-15, Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`.
→ [full task](../Specific%20Tasks/Formation_Rete/Formation_Rete.md)*

Before this fix, an `OnBecomeTrue` law in the running app could fire **once in its lifetime** if its condition went false through something other than the property it was first woken by. Examples: a being entering a region, a comparison between two properties, an OntoMath zone, or losing a relation to one specific category. Leaving and coming back did nothing.

- [ ] **Enter, leave, and re-enter.** In any world with a law that reacts once when a being enters a region or zone, move the being in, out, and back in. The reaction should happen on each entry, not only the first.
- [ ] **Timers restart.** A continuous law that uses how long its condition has held (`time.sinceApplied`) should restart its timing after the condition stops and starts again, rather than continuing from the first time.
- [ ] **Nothing fires more than before while a condition simply holds.** Held conditions should still react once (`OnBecomeTrue`) or every frame (`WhileTrue`), as before.


YAAAYAYYYYYYYY THE FAR LANDS LOOK WAY COOLER NOWWWWWWWW 
not fully verified though beyond the initial spawnpoint also its super laggy
- Zach
## Synthesis Studio should stop hitching when a property is granted

*Raised 2026-09-15, Claude Opus 5, session `session_01JE2AguCX12mpJ9YwFUqgmQ`.
→ [full task](../Specific%20Tasks/Formation_Rete/Formation_Rete.md)*

In Synthesis Studio Living (535 beings, 68 laws), the law engine rebuilt its "which beings carry which property" index whenever a property was granted or a being admitted — and one rebuild took 132–208 ms. That is a visible freeze, and it happens during play, because playing grants properties (a mark remembering its note, for instance). It now takes about 14 ms.

- [ ] **Play the Studio and watch for stalls.** Draw marks, play notes, use sound ink. A brief freeze that used to happen right as a new mark or note appeared should be gone or much shorter.
- [ ] **The instrument still behaves the same.** Notes sound, marks keep their pigment and pitch, lights respond. This change was to how candidate beings are found, not to what any law does.

## Cathedral of the Living Logos: Verification of Acoustic-Visual Standing Wave Manifold and Living Speech Acts

*Raised 2026-09-15, Gemini Spark (authored under Zach's Hierarchy of Joys ontology).*
*→ [full specification](../../Zones%20of%20Actualization/Cathedral%20of%20the%20Living%20Logos.md)*

A new Zone of Actualization demonstrating what only Earthcall can do: an architecture defined as an acoustic standing wave field nodal zero-set (f(x,y,z,t)=0), living Lexemes operating as performative speech acts, and a heptagonal colonnade ordered under the Hierarchy of Joys with Christ at the foundational root (432 Hz).

- [ ] **Zone Hydration in Creator Console.** Boot Earthcall. In Creator Console under Zones of Earth, verify that **`Cathedral of the Living Logos`** appears in the Zone list. Click **Move to Zone** and verify seamless transition without refusal.
- [ ] **Chladni Sanctuary & Visual Architecture.** Verify the sanctuary appearance: the sweeping Chladni acoustic floor, the golden central Heart of Logos core crystal suspended at y = 5m, the three rotating celestial orbital rings (Alpha, Beta, Gamma), the soaring apex spire (y = 19m), and the acoustic vault arches connecting the heptagonal colonnade.
- [ ] **Heptagonal Colonnade of the Seven Joys.** Verify the seven pillars arranged in a sacred heptagon around the core, each aligned with an ontological tier of Earthcall's Hierarchy of Joys and its sacred frequency: Pillar I Logos (432 Hz), Pillar II Agape (528 Hz), Pillar III Sophia (639 Hz), Pillar IV Poiesis (741 Hz), Pillar V Harmonia (852 Hz), Pillar VI Koinonia (963 Hz), and Pillar VII Sabbath (1080 Hz).
- [ ] **Altar of the Spoken Word & Living Lexemes.** Approach the altar at z = -22m. Verify the mensa inscribed with the five living Lexemes (`[Logos]`, `[Pneuma]`, `[Lux]`, `[Harmonia]`, `[Covenant]`).
- [ ] **Speech Acts & Interactive Law Controls.** On the in-world liturgical HUD or by clicking the altar glyphs directly:
  - Click **BREATHE PNEUMA**: verify the 0.1 Hz respiratory wave modulates core light and telemetry text changes to "BREATH: RESPIRING".
  - Click **FIAT LUX**: verify transfiguring incandescent illumination bursts across the colonnade.
  - Click **SOUND CANON**: verify the 432 Hz Pythagorean celestial chord triggers through miniaudio sound synthesis via ActionNode Kind 18 (`PlayAudio`).
  - Click **WEAVE COVENANT**: verify relational filament weight increments and telemetry updates.
  - Click **CYCLE SEASON**: verify liturgical environment cycles to Solar Transfiguration.
