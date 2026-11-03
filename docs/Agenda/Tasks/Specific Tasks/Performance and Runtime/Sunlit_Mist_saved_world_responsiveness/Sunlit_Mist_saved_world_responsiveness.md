# Restore saved-world responsiveness in Sanctuary of Sunlit Mist

**Opened by:** Zach's 2026-09-25 report that **Sanctuary of Sunlit Mist** becomes unresponsive sooner while the separate **Sanctuary of Beginnings** remains around 60 FPS; recorded by Codex / GPT-6, session `01a0cfbf-c751-7af0-b160-df07da055bc0`, 2026-09-25 12:57 PDT.

**Evidence:** [Saved-scene A/B audit](../../../../../audits/rendering_optimization/2026-09-25_sunlit_mist_saved_scene_ab.md). Both the original feature commit and current revision have high volume-only GPU-synchronized cost. The small native correctness test passes but does not test responsiveness. The user's time-to-unresponsive observation is open, neither disproved nor yet attributed to a code change.

**Candidate isolation:** In response to Zach's clarification that the scene became *basically* unresponsive after Codex's work, the exact resident-cache parent `c645315d` and implementation `4874573c` were measured A–B–B–A on the identical saved mist. Full-frame hashes matched at 640×360 and 1280×720. B reduced one ring allocation, but showed no repeatable frame-time win; both versions slowed during sustained runs. The candidate is withheld from main pending a whole-app causal witness.

1. Reproduce the Sunlit Mist saved world in the WebGPU app without changing the save. Record hardware, drawable resolution, camera/time, cold-load duration, first-frame compilation, and frame-time trace through the point of unresponsiveness. Repeat on a quiet machine beside the separate, reportedly 60 FPS Sanctuary of Beginnings as a control, both at default eye and while moving.
2. Separately time CPU projection/serialization, shader compile/pipeline creation, volume fragment work, surface acquisition, and GPU completion. Record memory and cache growth. Distinguish sustained cost from a first-load stall and from machine-load drift.
3. Count per-fragment view samples and occluder SDF evaluations. Seek a mathematically conservative visibility certificate derived from authored geometry or exact shared-work reuse, with a normal exact fallback. Preserve the soft penumbra and the authored source/medium/occluder meanings. A Sanctuary-specific hardcoded AABB gate was tested, changed image pixels, and yielded no speed win; do not transplant it.
4. Require native image parity across several cameras, or explicitly justify any intentional transport change, and a repeatable native timing win at the saved world's actual display resolution. Keep the long-duration app trace and Zach's visual/responsiveness judgment as separate gates.

**Related Sun work:** [V5 and Rung 8 follow-ups](../../Rendering%20and%20OntoMath/Visual_radiance_V5_and_Rung_8_followups/Visual_radiance_V5_and_Rung_8_followups.md) and the [two-movement crystallization](../../../../../audits/2026-09-24_two_sol_movements_rendering_crystallization.md). This task addresses the later Antigravity-authored Sanctuary as a consumer of the Sol substrate, without assigning its measured cost to V5.

**Signed:** Codex / GPT-6 · session `01a0cfbf-c751-7af0-b160-df07da055bc0` · 2026-09-25 12:57 PDT
