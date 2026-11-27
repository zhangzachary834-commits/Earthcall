# Domain test suites for Earthcall

**Implementation brief for Claude Opus and Claude Sonnet · 2026-11-20**

Zach wants a developer who changes one system to be able to finish the work with one trustworthy command. The command should build and run the relevant existing tests, include the few consumers most likely to break, and explain how it chose them. The [original domain-suite proposal](../Agenda/Tasks/Specific%20Tasks/Housekeeping%20and%20Build/Domain_Test_Suites/Domain_Test_Suites.md) supplies the wider domain map. This plan specifies the first working increment, the evidence needed to promote it, and Zach’s subsequent direction on 3D physics.

## Division of work

**Sonnet owns implementation.** Make the first deliverable a working Properties and TransferPolicy suite in one reviewable PR. Own the CMake registration, local runner, focused runner tests and usage instructions. Submit subsequent increments separately so each can be reviewed and used on its own.

**Opus owns the independent check.** Before seeing the selector output, derive expected test names for the cases below from the code, test assertions and existing CI. Review consumer coverage, try to make the runner give a false green, and compare its receipts with those expectations. Own the CI migration after the local pilot works. Use one editor per shared file and trade findings in the PR. The duo can work in parallel on the selector and its independent expectations without creating further agents.

## 1. Establish the baseline on the checkout you receive

Read the repository instructions and the three companions they name. Record the source revision, working-tree changes relevant to the pilot, generator, build profile, CMake/CTest versions and runner environment. Reuse the supported [configuration recipe](../BUILD_AND_ENVIRONMENT.md#build). Use an isolated fixture/data home for checks that write.

Compare the C++ stems under `tests/` with `ctest --test-dir <build> --show-only=json-v1` after configuration. Inspect duplicate stems, missing or extra registrations, and scripted checks outside the C++ glob. On the planning checkout, the main build listed 323 tests, another build listed 319, five had the label `lag`, and the focused CI list held 88 targets. Refresh those numbers. The [registration loop](../../CMakeLists.txt) has special handling for wrappers, working directories, device-specific targets and serial timing. The [current workflow](../../.github/workflows/earthcall-ci.yml) also runs a Terminal smoke program, foreign DOM binaries from two working directories, and specialist jobs with their own arguments and artifacts.

## 2. Land the smallest useful local runner

Expose these commands from the checkout root. They describe the proposed interface.

```sh
scripts/check suite properties --explain
scripts/check affected --explain
scripts/check affected
scripts/check affected --base <base-ref> --head <head-ref> --build-dir <build>
```

1. **Register the six pilot witnesses.** Select `property_memory_access_test`, `property_path_precalc_test`, `no_black_box_test`, `dynamic_property_reachability_test`, `transfer_policy_test` and `transfer_policy_branch_reactivity_test` for the joint Property/TransferPolicy handoff. The last witness runs a Law that writes policy and a second Law that branches on it. Keep individual suite labels available so the receipt can explain which contract brought each witness in. Append labels to the existing CTest registrations. Preserve existing labels and execution properties.
2. **Keep two small pieces of information authoritative.** CTest labels own test membership. A versioned path map owns the contracts affected by each path, including explicit consumer relationships. A PropertyPath edit, for example, reaches Law authors through registered dynamic paths and TransferPolicy through guarded access. Match the union of applicable rules and deduplicate test names. Use the same resolved names for the build and the run. Work with the repo’s supported CMake tooling and publish any minimum version required by the runner.
3. **Make the diff match the developer’s work.** With no refs, include staged, unstaged and untracked non-ignored files. With PR refs, use the merge base and both sides of renames, including deleted paths. Handle spaces and unusual filenames through NUL-delimited Git output. A changed test selects itself and its assigned suite. For a missing merge base, shared registration or fixture change, or unknown production path, select a documented broad fallback and print the cause. During the partial rollout, use the existing CI baseline and report the uncovered domain explicitly. Once the whole inventory is classified, use the full applicable lane. Show a distinct result for a genuinely empty diff or an explicitly docs-only route.
4. **Build before running.** Check that the configured tree belongs to this checkout. Refresh its registration when inputs change. Resolve exact names, build the corresponding targets, and run them through CTest so wrapper arguments, fixtures and working directories survive. Verify expected registrations individually and reconcile requested names against executed outcomes. A single matching test can make `--no-tests=error` succeed while five intended tests are missing. Give absent registrations, build failures, failed assertions, skips and unavailable environments distinct results. Required evidence still outstanding must be visible to the caller and CI.
5. **Make the receipt useful.** In explain mode, print changed path → matched rule → suite → exact test names, including fallback and environment requirements. In run mode add the commands, profile, selected/built/executed counts, individual exceptions, and separate configuration, build and execution times. A developer should be able to copy the failing invocation.

## 3. Accept or repair the pilot

Opus should test the following against a disposable checkout or temporary Git fixture, keeping the active developers’ edits intact.

| Input or fault | Required observation |
|---|---|
| Edit only `PropertyPath.cpp`. Then repeat with its public header. | The six pilot witnesses are selected. The header also takes its declared shared-consumer route. The receipt identifies the Law-side tests. |
| Stage one change, leave another unstaged, and add a new test in a path containing spaces. Repeat with a rename across domains and a deletion. | The union contains both domains and the new test. A removed path still selects surviving consumers. |
| Supply an outdated or foreign build tree, remove one expected registration, then try an empty required suite. | The runner refreshes a stale matching tree or explains the failure. A foreign tree, unresolved registration or empty required suite cannot report success. |
| Edit an unknown production file, a shared test helper, a docs-only file, then try a missing PR base. | Each case takes the documented route, with its reason. A Git error cannot become an empty diff. |
| Force a selected assertion to fail in a disposable copy. Exercise an existing wrapped test and a serial test when wiring those lanes. | A failing witness propagates a nonzero result. CTest retains its wrapper, working directory, serial semantics and existing labels. |
| Reconstruct a real shipped regression from the [shipped-bug table](../BUILD_AND_ENVIRONMENT.md#tests-that-exist-because-of-what-got-past-the-suite-not-to-pad-it). | Show that the historical production change selects its consumer witness, and demonstrate red on faulty behavior and green after repair. Record the revision or the exact controlled reversal. |

Publish one clean build and repeated warm measurements with toolchain and machine details. Use two minutes as the initial warm pilot target. If the target is missed, identify whether configure, core rebuild, link or execution dominates and report the cost with all six witnesses present. Report sample count with timing statistics. Pilot acceptance requires an actual successful build and run through the new entry point, the negative cases above, and an independently reviewed selection.

## 4. Expand and promote in CI

| Increment | Work | Evidence before advancing |
|---|---|---|
| Persistence | Route codec, schema, storage and hydration changes to round-trip, older-save, unsaved-preservation and Zone boot witnesses. | Load a frozen older world in an isolated home, make a bounded edit, save and reopen through the production route. Check identity, authorship, Relations and unaffected content. |
| OntoMath and Screen | Give mathematical evaluation, CPU shader generation, software WebGPU, native Metal and isolated Release timing independent execution requirements. Route an emitter edit to an authored world that invokes it, such as Gyroid Reliquary. | Preserve CPU refusal checks and the appropriate backend comparison. Record acquisition failures explicitly. A timing result needs its matched correctness run. |
| Remaining domains | Use the parent proposal’s map for Singulars, Relations, Person, Identity, Zones, Time, Law, Rete and channels. Attach authored-world tests to the contracts they exercise. Register applicable Python/Node checks with their interpreter, fixtures and build prerequisites. Apply the physics dispositions below. | Every discovered test has an execution lane or a named reason and owner for exclusion. New registrations and unmapped production paths are visible in CI. |
| CI rollout | Run the selector in shadow beside existing jobs first. Add a small common PR smoke set for boot, Law firing, authority refusal and frozen-save load. Add a scheduled full applicable run and a release entry point. | Diff exact test identities, special invocations, environment variables, thresholds and expected artifacts. Retain Terminal input/output assertions, both DOM working directories, shader diagnostics, timing jobs and Slow Adapter scenarios. Retire a duplicate list only after the intended checks run on their assigned triggers. |

For the full run, account for every registered test across capable runners, including tests excluded from a platform’s default build. Publish selected, passed, failed, skipped, blocked and unscheduled counts with named exceptions. Keep native claims tied to the observed backend. An escaped defect should improve both the witness and the rule that selects it.

## 5. Zach’s decision on hardcoded 3D physics

**Zach intends to retire the hardcoded 3D simulator completely.** Its source directory already carries his retirement marker. Preserve coverage of behavior that ships during the transition. Give simulator-specific tests a temporary `legacy-3d-physics` route that can disappear with the implementation. Classify assertions by what they protect. [Zach’s retirement marker](../../src/ZonesOfEarth/Physics/RETIRE%20EVERYTHING%20IN%20THIS%20FOLDER!!!!!!%20-%20Zach)

| Existing witness or code | How the duo should treat it |
|---|---|
| `collision_dispatcher_test`, `physics_toggle_test`, legacy simulation loops | Route changes to these mechanisms through the temporary suite. At retirement, identify assertions whose sole subject disappeared and remove those alongside it in the separately scoped retirement change. |
| `authored_physics_laws_test` | The current test includes a C++ factory, named Laws, boot wiring and Law execution. Map the factory-specific checks to legacy coverage. Identify which general Law/Flow behavior has an enduring consumer, and carry that evidence through a Person-authored fixture when the factory retires. |
| `ground_plane_test` and saves carrying physics-era data | Preserve any continuing Zone, authorship or compatibility promise independently. The regression in which the second spawned being became the floor by list position belongs in the retirement review. A migration witness should establish what happens to an existing authored world. |
| `webgpu_v5_overlap_physics_test` | Keep it under Screen/volumetric transport. Its subject is overlapping media and emitted light. Verify that retirement of the simulator leaves this test selected for its renderer. |
| `src/Singularity/Physical/` and `serial_adapter_test` | Keep the existing physical Sense/Act channel separately selectable. The serial test exercises a dummy adapter lifecycle. Record that level of evidence accurately. |

Zach floated a future Singularity anchor for real-world physical quantities and laws and currently leans toward a fresh design if pursued. That remains an open architecture question. The suite rollout commissions classification and routing only. Simulator removal, a move into Singularity and implementation of a new physical anchor each require their own scope.

## 6. Carry the discipline beyond routing

After the pilot, put durable safeguards in their owning suite. Start with one reviewed historical fixture for persisted enum numbers and burned slots. For a Rete or cache change, compare against an independent exact path after mutation, removal and Zone switching. For flaky failures, retain the first failure and reproduction details, and give quarantined checks an owner and review date. Add each safeguard with a demonstrated failure it catches.

Update the Build and Environment guide and the short agent router with the verified command when it lands. In the handoff report, include the revision, exact selected witnesses, real commands, outcomes, measured costs, remaining environments and the reviewer’s counterexample results. Record a Person check only when an actual control, appearance or experience needs Zach’s judgment.

**Prepared by Codex · GPT-6 · session `01a1fcd4-1765-7893-a097-489b8ea4db36` · 2026-11-20T22:53:49-08:00.**
