# Domain test suites

**Proposal commissioned by Zach, 2026-11-20.** Zach wants a developer working on one part of Earthcall to have an affordable, repeatable way to check the surrounding system. This task applies the [engineering discipline](../../../../../ENGINEERING_DISCIPLINE.md) to test selection. Codex proposes the divisions, routing rules and rollout below.

## Starting point

The [Opus and Sonnet implementation handoff](../../../../../plans/DOMAIN_TEST_SUITES_IMPLEMENTATION_2026-11-20.md) specifies the pilot, adversarial checks and CI rollout. Zach subsequently clarified that he intends to retire the hardcoded 3D simulator completely. Its remaining tests need a temporary route during that transition.

On this checkout, 323 C++ test sources matched the 323 names in the configured `build` inventory. A second local configuration, `build_release`, listed 319. Five tests carried the single CTest label `lag`. The focused macOS workflow listed 88 targets, using the same list to build and run them. Separate jobs cover SDF verification, SDF performance and the Slow Adapter. Standalone Python and JavaScript checks also exist. Counts describe registration on this checkout. Execution time and pass rates still need measurement.

## Suggested divisions

Each independently selectable suite corresponds to a contract a change can break. A witness can carry several labels. These divisions concern test selection and allow files to stay where they are.

| Suite | Representative witnesses already in the tree |
|---|---|
| Singulars, Properties and paths | `singular_copy_move_test`, `property_memory_access_test`, `no_black_box_test` |
| Relations and Formations | `endpoint_register_test`, `formation_topology_test`, `relation_endpoint_index_test` |
| Person, Body, Identity and authority | `body_part_test`, `identity_ledger_test`, `first_mover_test`, `transfer_policy_test` |
| Zones, Homes and Ourverse | `dimensional_zone_test`, `world_switch_test`, `zone_boot_hydration_relations_test` |
| Time, Moments and Events | `timeline_test`, `time_moment_test`, `event_test`, `slow_adapter_clock_test` |
| Law execution and creation | `law_creation_test`, `action_spawn_test`, `metalaw_test` |
| Rete, reactivity and derived state | `prophetic_rete_test`, `rete_index_sync_test`, `zone_switch_invalidation_test` |
| Storage, codecs and restart | `serialization_compat_test`, `save_roundtrip_test`, `unsaved_preserve_test` |
| OntoMath and geometric proofs | `ontomath_test`, `sdf_range_soundness_test`, `volume_zero_proof_test` |
| Screen, materials and rendering | `paint_test`, `sdf_wgsl_parameter_refresh_test`, `webgpu_sdf_parity_test` |
| Interaction, Language and Terminal | `interaction_channel_test`, `law_sentence_test`, `law_line_zone_test` |
| Audio and physical Sense/Act channels | Select Audio and real-device interfaces independently. Examples include `infrasound_floor_test` and the dummy adapter lifecycle in `serial_adapter_test`. |
| Legacy 3D simulator (temporary) | Route `collision_dispatcher_test` and legacy toggle checks until retirement. Review mixed witnesses for enduring Law, Zone and save contracts. |
| Foreign, Network and First Mover interfaces | `foreign_actuation_test`, `web_socket_server_test`, `mcp_first_mover_bridge_test`, Node bridge checks |
| Engine boot, tick and execution substrate | `event_bus_test`, `native_bytecode_vm_test`, `law_manager_eventbus_lifetime_test` |

Tag authored-world scenarios across these suites. A Law Line change deserves its authored Zone boot test. A geometry-emitter change deserves a world such as Gyroid Reliquary that exercises the production emitter. Select worlds by the feature each actually exercises.

## What runs after a change

1. **During iteration**, run the reproducer. **Before handing off a domain change**, build and run its quick suite plus selected tests of dependent domains. Give the runner an explain mode that prints the changed paths, selected contracts, targets and reasons. Local selection includes staged, unstaged and new files. PR selection uses the merge base and handles deletions and both ends of a rename.
2. **On each PR**, add a small shared smoke suite covering ordinary boot, an authored Law firing, a refused action and a frozen save loading. Add relevant integration or native jobs when the contract needs them. Changes to shared headers, codecs, test fixtures, boot, registration or routing use explicit wider rules. Unknown production paths select broad coverage and report the missing rule. A declared suite selecting zero tests is an error.
3. **On a scheduled run and before release**, run the full applicable inventory across supported environments and compare failures with the suites that each change selected. A missed regression earns a routing rule as well as a repair. Track skipped, unavailable and never-scheduled tests by name.

Keep resource and evidence labels independent of domain. A Screen change can have CPU compiler checks, software WebGPU output, native Metal output, and an isolated Release timing run. Give real-device work a suitable runner. Record Person acceptance separately when the question concerns feel or appearance. Preserve `RUN_SERIAL`, fixture working directories and Python wrappers already attached to CTest registrations.

## Pilot and acceptance

Start with Properties and TransferPolicy since an edit there can affect authored Law reads and writes, authority decisions, and reactive consumers. This command previews six existing registrations in the current checkout. It provides a starting set for the pilot.

```sh
ctest --test-dir build -N -R '^(property_memory_access_test|property_path_precalc_test|no_black_box_test|dynamic_property_reachability_test|transfer_policy_test|transfer_policy_branch_reactivity_test)$'
```

Give tests overlapping CTest labels in the existing registration path. Keep the small path-to-contract map reviewable. Use the configured inventory to derive one exact selection, build the corresponding targets, then execute those tests through CTest. Include script-only checks with their interpreter and fixture requirements. Reconfigure when sources change and check source-to-registration coverage. Compare the runner's output against the present CI coverage before migrating any job. Preserve bespoke job assertions and artifacts as well as test names. [CTest labels](https://cmake.org/cmake/help/latest/prop_test/LABELS.html)

Measure warm build, test execution and clean CI build separately. Trial a two-minute warm handoff target for the pilot and publish median and p95 once enough runs exist. Exercise the selector with a Property-only edit, a shared header, an added test, an unmapped source and a deleted file. Replay a shipped regression to see whether its consumer test would have been selected. Expand to persistence and Screen once the pilot gives trustworthy selections and measured costs.

## Companion disciplines to adopt

- Turn a repeated refusal into an executable check where its meaning is mechanical. Start with a test comparing the persisted enum assignments and burned slots against a small reviewed historical fixture.
- For a codec change, load a frozen older world in an isolated data home, edit it through the ordinary entry point, save and reopen it. Compare concrete identity, authorship, Relations and unaffected authored content.
- For an index, cache or prophetic filter, compare with the exact path after mutation, removal and Zone switching. Keep the independent reference small enough to run routinely.
- For each escaped defect, preserve a minimal reproducer at the boundary that missed it. Confirm it goes red against the faulty revision, assign it to a suite, and check that the original changed path selects it.
- Report the first flaky failure with the runner, seed, profile and artifact needed to reproduce it. Give quarantined tests an owner and review date. For timing regressions, keep an isolated matched baseline and its correctness checks.

**Codex · GPT-6 · session `01a1fcd4-1765-7893-a097-489b8ea4db36` · 2026-11-20T22:39:28-08:00**
