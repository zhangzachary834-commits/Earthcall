# Continuous cube visibility witness

**Follow-up, 2026-10-05 11:55 PDT:** Zach subsequently tested both flying above the floor and the camera-forward probe with no visible cube. The confirmed defect and fix are in [Create active-Zone routing](LAW_CREATE_ACTIVE_ZONE_ROUTING_FIX_2026-10-05.md): the resolver preferred inactive `World`, which the original test harness omitted. The direct-render observations below remain fixture evidence; they were insufficient to diagnose the real birth destination. A full Engine viewport capture now verifies the fix.

Codex / GPT-6.1 Sol / session `01a10992-828e-7e80-890c-c64b09141e18` / 2026-10-05 11:28 PDT.

Zach confirmed that the corrected continuous Law fires and the Object count grows, but no cube appears below him. This pass distinguishes successful creation from sensory visibility rather than supplying another speculative grammar change.

The production locomotion channel places the camera at `Person.position + (0, Body.eyeHeight, 0)` and writes Person.position back as camera position minus eye height. The Person position is at the feet. The supplied Create expression therefore places the new cube's centre three units beneath the feet. The Object position bridge updates the transform; EngineRender passes that transform into the renderer before drawing every owned Object. A ground-level observer can have correctly positioned newborns underneath an opaque floor.

## Executed witness

Run `python3 scratch/probes/law_line_visibility_probe.py` after building `earthcall_webgpu`. It compiles a standalone probe against the production app objects, leaving CMake and inhabited saves untouched.

The probe boots an isolated copy of LawLine's seed and compiler roots. Its test-only Person is called Zach and receives a fixed fixture public-key identifier; it does not load or authenticate Zach's real key. The speaking-Person wire matches the interaction channel. It reads the actual `examples/law_line_cubes_below.txt`, submits it through Terminal and authored compiler Metalaws, then ticks the real LawManager. One newborn cube is authored by that fixture Person at `(0, -3, 3)` for Person position `(0, 0, 3)`.

The production WebGPU renderer draws that newborn while a camera at the fixture's actual eye height looks straight down. Four 64×64 native frames are read back: empty, cube alone, floor alone, and floor plus the same cube. The fixture floor's top is Y=0. Its position and size make it an ordinary opaque geometric occluder; no visibility filter is authored.

Result: **PASS**. The cube alone changes **1,452 framebuffer bytes** relative to empty. Floor plus cube differs from floor alone in **zero bytes**. Thus this actual created cube renders, and the floor hides every pixel even with the observer looking directly toward it. PNG artifacts are identity format conversions of the GPU readback, not reconstructed pictures.

- [Native result](../../scratch/verification/law-line-visibility-2026-10-05/result.json)
- [Cube without floor](../../scratch/verification/law-line-visibility-2026-10-05/cube-without-floor.png)
- [Same cube beneath floor](../../scratch/verification/law-line-visibility-2026-10-05/cube-beneath-floor.png)
- [Execution log](../../scratch/verification/law-line-visibility-2026-10-05/probe.log)

The WebGPU target builds and `law_line_zone_test` passes. No production engine code, user-authored Law, or inhabited save was changed. Temporary fixture saves contain copied roots plus the test-authored Law and newborn; retained verification artifacts contain pixels, measurements, and the log.

## View-relative follow-up

Zach replied that changing the offset to `(0, 3, 0)` still showed no cube. Floor occlusion therefore remains a possible explanation for the original placement, not a sufficient diagnosis of his scene. Positive Y places a cube overhead rather than along the viewing direction.

`examples/law_line_visible_probe.txt` authors an OnBecomeTrue Law that creates one gold cube at `my.cameraPos + my.cameraForward * 3` and marks it with `visibilityProbe: true`. Engine::tick refreshes those registered Person perception fields from the camera before evaluating Laws. In first-person perspective this places the centre directly along the view ray. Aim approximately level so it does not put the probe below the ground.

The native probe submits this additional sentence through the same Terminal/Metalaw path. It checks the exact position, proves a second tick adds no further probe, and then draws it with the floor present from a horizontal first-person camera. Result: **PASS**, **2,393 framebuffer bytes** change compared with the same floor-only view. [Native front-probe image](../../scratch/verification/law-line-visibility-2026-10-05/front-probe.png). This separate diagnostic keeps the requested below-feet semantics intact.

## Practical boundary

This is native GPU evidence for the test placement and floor, not a capture of Zach's scene. Both the MCP connector and a direct read-only WebSocket query failed to provide a live snapshot; desktop inventory had no Earthcall app entry, and the desktop tool refused Terminal access. If Zach is grounded, press F in the viewport to enable flight, hold Space to rise more than three units above the floor, then look down: subsequently created cubes should lie above the floor and become visible. Alternatively, press 1 for first-person perspective, aim level and submit the one-shot view probe. If it remains invisible, obtain a scene screenshot and the newest Object's position before attributing it to floor occlusion. The current expression is three world units, not a conversion of three feet.

Remaining live checks are in [Person Verification](../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md). Preserve exact below-position semantics; do not silently replace below-feet with an above-ground or camera-relative placement in the compiler.
