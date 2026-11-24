# Sixth-One audit of Confluence Court

Codex · GPT-6.1 Sol · Sixth-One Sun · session `01a1fc29-5539-7f12-bc32-783dde165cfc` · 2026-11-23T21:38:27-08:00

Zach asked Sixth-One Sun to audit Sixth-Two Sun's completed work. The human request being evaluated was a working example of intersecting Zones sharing a Singular which shares Property memory with other Singulars, with legacy limitations disclosed. This audit continues [Sixth-Two's Court record](../Agenda/Tasks/Specific%20Tasks/Zones%20and%20Ourverse/Confluence_Court/Confluence_Court.md). The checkout is `/private/tmp/earthcall-default-worktree.oNOOLW/Earthcall`, branch `sync-from-earthcall-main`, base `ce1e4f60b`, with the local Court changes.

## Finding corrected

**Save Zone discarded first-seed provenance.** The five seeds carry `authors` and `injected_by`, while `zoneToJson` represents the runtime Zone without those envelope fields. The native writer previously wrote that projection directly, losing both fields on the first save. It already preserved the injection envelope of Law and Material roots. The native Zone writer now carries the prior identity's two provenance fields into its output. The Court test compares them before and after Save Zone. A control build with only this correction removed fails at the missing `authors` field. The corrected build passes.

The rule list and count remain governed by the two compiled First Mover Laws from Sixth-Two's earlier pass. The extended Court test installs those Laws through their production registration entry point, so its read, write, and save checks execute with the same committed device rules present at boot.

## Additional witnesses

- Ordinary entry to the parent Court loads its two Laws and works with only one active Zone. The test clicks the common lamp and confirms no state change, then clicks each side and observes all three state and appearance projections.
- Appearance checks read all three color components and the actual Material face texture bytes. Object color readback alone could agree while the displayed paint was stale. The Material albedo is a separate multiplier, so the test checks paint bytes for the authored tint.
- A child-designated Object survives the inherited-geometry projection. A child-local Object also survives, and inherited geometry cannot mask an attempted empty save over that persisted Object.
- The test retains the original checks for pointer identity through both switched sibling views, deduplicated ActiveZones reach, mathematical overlap, weak write refusal, two strong owners, save/reload, and the control showing independent Zone loads allocate different cells.

## Execution

Release `earthcall_webgpu` built after the correction. Six focused CTest targets passed in 0.57 seconds with isolated temporary saves: `confluence_court_test`, `property_memory_access_test`, `dimensional_zone_test`, `stale_sidecar_cleanup_test`, `zone_native_save_isolation_test`, and `zone_material_closure_test`. The material test initially lacked its frozen `visible_cube` fixture in the sparse checkout. Restoring that exact tracked fixture allowed its 23 closure checks to run.

The earlier native Metal captures remain Sixth-Two's dated evidence. This pass verifies behavior and persistence and leaves live Person acceptance on the [verification list](../Agenda/Tasks/For%20Zach/Person%20Verification%20List.md#confluence-court). The new seeds remain in this checkout. They have not yet been delivered into the inhabited data home by launching this checkout.

## Completion boundary

The single-custody Court fulfills the authored example's bounded contract. Its three lens Objects hydrate together in the parent's cell cohort. The child Zones provide intersecting mathematical bounds and inherited presentation views. Physical membership is still distinct from storage membership and jurisdiction.

The broad architecture has unfinished work. Independently saved sibling stores allocate their own Object definitions and cell cohorts. Durable cross-store identity, custody, overlapping jurisdiction arbitration, and rendering/picking across arbitrary intersecting Zones remain open. The Court uses the working parent-store route. No new authority decision for those open questions was introduced in this audit.

The general Property access bypasses identified in the earlier audit, including AddProperty and RemoveProperty, also remain outside the two device inspection Laws and this Court example. Completing universal memory AAA requires further work.


## Review of the audit's architectural assurance

The previous review establishes the Court's bounded storage and interaction behavior. Calling that review independent overstated its separation from the original implementation. It reused the original fixture and most of its acceptance conditions. The provenance control is a specific adversarial witness. Broader assurance requires counterexamples to the assumptions behind the fixture.

A fresh isolated probe now demonstrates a concrete permission hazard. With the same common lens and cell in the parent storage home, a read produces target Zone `confluence-court` when the parent is the primary view. After `switchTo(Copper Walk)`, `residenceOf(common)` still returns the parent, but the same read produces target Zone `confluence-court-copper`. A deny rule for reads of `confluence.lampLevel` with `targetZoneId = confluence-court` refuses the first read and allows the second. The probe uses the actual seed roots, normal switch path, and `PropertyPath::getValue` through `TransferPolicy`.

The cause is `resolveZoneForBeing` in `TransferPolicy.cpp`. It chooses the first active Zone whose `containsBeing` includes the target, ahead of consulting residence. Child views inherit the parent's Object pointers. Consequently, the presentation selection can change which scoped policy is applicable to unchanged memory. The original Court contains no such authored denial, so its lamp behavior remains valid. Its success establishes no custody-stable authorization guarantee.

Sixth-Two's statement that the Court takes its permissions from one custody store was too strong. The Court has one custody store, while policy context remains dependent on active membership. Both audits disclosed overlapping jurisdiction as unfinished, but the earlier wording failed to name this observable consequence.

The engine's policy resolution is unchanged by this review. Choosing whether all applicable Zones, an explicit act jurisdiction, custody, or another authored resolution governs is part of the unresolved authority contract. This witness should be preserved when that contract is implemented. Cross-store persistence, overlap authority, universal memory AAA, and live Person acceptance remain open.

*Codex · GPT-6.1 Sol · Sixth-One Sun · session `01a1fc29-5539-7f12-bc32-783dde165cfc` · 2026-11-23T21:41:42-08:00.*
