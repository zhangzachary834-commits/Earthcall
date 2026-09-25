# Codex to the Sol Suns — one less recurring Screen debt

- **To:** the Sol Suns carrying Prophetic SDF performance and V5 visual radiance
- **From:** Codex / GPT-6, session `01a0cfbf-c751-7af0-b160-df07da055bc0`
- **Date and time:** 2026-09-25 11:50 PDT
- **Base:** `c645315d`, isolated `volume-param-residency` worktree
- **Full audit:** [Cheap work the renderer can stop doing now](../../../docs/audits/rendering_optimization/2026-09-25_non_prophetic_renderer_fruit.md)

SUNS: Zach asked what ordinary renderer work we can remove while you pursue the deeper Prophetic question. The narrow answer is now in code: a V5 medium set's authored float parameters retain a pipeline-local GPU buffer and upload only when the bytes change. Camera, Timeline, and instances still travel through the frame ring. A second change stops allocating a temporary byte vector merely to compare unchanged plural radiance sources. These are Screen execution details; they carry no new authority over OntoMath, visibility, or Person-authored Law.

The native fused-medium witness uploaded 20 parameter bytes on its first draw and **0** on the next identical draw, with identical center pixels. Timeline-only movement uploaded **0** authored-parameter bytes and still changed the image; a numeric `E_v` edit uploaded fresh values and changed the pixels without regenerating WGSL. `webgpu_object_test` and `webgpu_v5_overlap_physics_test` pass on the native Mac GPU. I did not measure a Northern Veil FPS delta. The existing heavy 4,500-object probe already gets 3,000 meshes and 1,500 fields through two draw calls and five ring suballocations; it does not exercise volumes, so do not claim this patch improves that number.

For your next pass, please keep three ledgers separate. First, **semantic proof economics**: count exact samples saved per branch and record test in the authored Perlin ray, preserving the exact fallback and the Person's field. Second, **visual truth**: keep V5's shared transmittance, #361's null-participant stability, named failure, and Northern Veil's fixed-camera/cold-reload witness. Third, **ordinary Screen cost**: time the saved scene's medium gathering and 24 per-frame Piecewise serializations, bind-group construction, shader compilation, and GPU pass. A revision-led medium reader is attractive only when in-place edits and lifetime invalidation are proved; the present exact byte comparison is deliberately conservative. Cache resident bytes and pipeline count over long authoring sessions before designing eviction.

The next Sun can use the task [Visual radiance V5 and Rung 8 follow-ups](../../../docs/Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Visual_radiance_V5_and_Rung_8_followups/Visual_radiance_V5_and_Rung_8_followups.md) and the audit above as the current gate list. Treat this small win as room cleared around your deeper work, never as a substitute for a cheaper exact answer at the ray.

— **Codex / GPT-6**, session `01a0cfbf-c751-7af0-b160-df07da055bc0`, 2026-09-25 11:50 PDT

**12:19 PDT A/B correction:** The exact-parent versus candidate native A/B is now in the linked audit. In a synthetic four-medium 128×128 set, the candidate removed one ring allocation and 96 ring bytes per stable frame, but median synchronized frame times were 0.130 ms (parent) and 0.133 ms (candidate), with overlapping ranges. Numeric edits showed no material regression. Do not advertise an FPS gain or merge this merely because upload bytes fell; profile the saved Northern Veil Zone first, and discard the extra resident cache if that benefit is absent. — Codex / GPT-6, same session
