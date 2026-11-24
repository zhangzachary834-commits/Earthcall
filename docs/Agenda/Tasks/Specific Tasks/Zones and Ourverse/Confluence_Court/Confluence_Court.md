# Confluence Court

Zach asked for a Zone whose bounds overlap another Zone, where a shared Singular can share memory with distinct Singulars. Sixth-Two Sun authored a court with two walks, two controls, and one common lamp. This is the first authored example from this pass on `sync-from-earthcall-main` at `ce1e4f60b`. It continues the [ActiveZones integration plan](../../../../../plans/ACTIVE_ZONES_AND_CROSS_ZONE_INTEGRATION_PLAN_2026-11-09.md) and uses the [Zone bounds model](../../../../../plans/ZONES_AS_MATHEMATICAL_BOUNDS_PLAN_2026-09-23.md).

## Viability

| Capability | Result |
|---|---|
| One Singular inside intersecting Zones | Landed. `ZoneManager::locate` derives all containing Zones from the coordinates of one resident Object. Two active scopes can also encounter one in-memory pointer without duplicating it in the Law working set. |
| Distinct Singulars sharing a Property live | Landed. Law actions support strong sharing and weak observations across active Zones. Section 20.5 of `property_memory_access_test` exercises independently stored boards in memory. |
| Saving and reopening a shared Property | Landed when the aliases hydrate in one Zone cohort. Separate Zone files get separate cell-remapping tables. A deliberate control split two of the Court's lens records across independent loads and got distinct cells even with the same serialized key. |
| Independently owned sibling stores sharing a durable Singular or cell | Open. Ordinary Object definitions hydrate locally. A repeated id or cell key across files does not unify the referent. Cross-store custody and a durable reference to the owning store remain necessary for this shape. |

The Nov. 23 referent-cache, PropertyPath-root, Formation-lifetime, and TransferPolicy-index branches were inspected as candidate work outside default. Their relevant changes concern lookup, invalidation, or policy throughput. The storage cohort boundary still applies to the shipped branch.

## The legacy seams and their effect

- `ZoneManager::switchTo` still fills a single primary presentation bag with its own Objects and inherited ancestor pointers. Rendering, picking, and legacy physics use that bag. The Court therefore uses one continuous scene, which is visible in the parent and both sibling views. The whole scene remains visible beyond each walk’s mathematical bounds.
- `containsBeing` and Zone-qualified PropertyPaths use the Zone's actual storage and Formation reach. Mathematical presence comes from `locate`. Mounting the two sibling stores makes their Laws available. Zone-qualified routes to the lamp still depend on the inherited view or a custody address. The working example addresses the canonical court or the event subject. The test also switches into both sibling views, where both qualified paths resolve to the same lamp.
- The existing authority lookup can select one jurisdiction from active membership and residence. The single-valued `@world.zoneId` also projects one result from the location set. Arbitrating overlapping authority remains open. The Court has one custody store. A follow-up runtime probe found that switching into a child view changes the policy target Zone for unchanged memory and can stop a parent-scoped deny rule applying. Custody-stable authorization remains open.
- Native Save Zone formerly serialized inherited parent pointers as child definitions. This pass projects those ancestor pointers out of the child's semantic save and its embedded materials, while keeping child-local or explicitly child-designated Objects. The save guard counts the projected definitions so inherited geometry cannot mask accidental erasure of persisted local Objects. Save the Court to persist a change to its shared cell. Legacy conglomerate World sidecars are outside the verified workflow.

## What is authored

The parent is a dimensional Zone with axes bound to `position.x/y/z`. Copper Walk covers `-6.5 ≤ x ≤ 1.5`, Indigo Walk `-1.5 ≤ x ≤ 6.5`. The single common lens at `x = 0` lies in both. Copper and indigo lenses at `x = ±3.4` lie in their respective walk. The two side lenses are strong aliases of `confluence.lampLevel`; the common lens is a weak, read-only observer of that cell. The click Law changes the state through whichever side lens received the click. The projection Law changes each lens's own Material color through its Object color property.

The first seeds are the three [Zone identities](../../../../../../saves/zones/) `confluence-court`, `confluence-court-copper`, and `confluence-court-indigo`, and the two [Law roots](../../../../../../saves/laws/) `confluence-turn-lamp` and `confluence-show-lamp`. The parent holds 17 Object definitions and eight Material definitions. The Law roots and Zone seeds record Zach's existing Person identifier as the authority for the injection and Codex / GPT-6 / Sixth-Two Sun as injector. Object and Zone provenance also lives on authored Properties. These are first seeds in the checkout. Earthcall's normal missing-identity delivery copies them into a verified data home when launched from the checkout.

## Witnesses

- `confluence_court_test` loads those exact files into a temporary data root, mounts both walks, checks mathematical membership and one referent, uses the pointer's press/release path from each side, and checks cell identity, weak write refusal, and the actual Material-backed color. It saves both views and the Court, then repeats location and behavior checks in a fresh harness. It checks both preservation of a legitimate child object and refusal of an erasing save.
- Release `earthcall_webgpu` built. `confluence_court_test`, `property_memory_access_test`, `dimensional_zone_test`, and `stale_sidecar_cleanup_test` passed. The GLFW-based `zone_switch_invalidation_test` also passed with access to the Mac display services after a sandboxed initialization timeout.
- An offscreen run through the production Metal/WebGPU renderer captured both authored states at 1024×640. The copper, common, and indigo regions changed by 2,436, 4,226, and 2,436 pixels, respectively, following the same authored click event. This used a fixed preview camera with the seed scene.
- Live Person verification is on the [Person Verification List](../../../For%20Zach/Person%20Verification%20List.md#confluence-court).

*Codex · GPT-6 · Sixth-Two Sun · session `01a1fc29-5539-7f12-bc32-783dde165cfc` · 2026-11-23T20:14:25-08:00.*

## Sixth-One audit

The [Sixth-One audit](../../../../../audits/CONFLUENCE_COURT_SIXTH_ONE_AUDIT_2026-11-23.md) corrected loss of seed provenance during native Save Zone and extended the witness to normal entry, the common lamp, Material paint, explicit child membership, and the boot memory rules. Six focused tests passed after the correction.

*Codex · GPT-6.1 Sol · Sixth-One Sun · session `01a1fc29-5539-7f12-bc32-783dde165cfc` · 2026-11-23T21:38:27-08:00.*
