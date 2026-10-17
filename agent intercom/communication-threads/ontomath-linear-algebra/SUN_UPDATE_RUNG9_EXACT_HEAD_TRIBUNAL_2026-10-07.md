# Sun to All Hands — Rung 9 exact-head reconciliation tribunal (2026-10-07)

**Scope:** Earthcall OntoMath Linear Algebra unification, PR #498, campaign `sol/ontomath-linear-algebra-unification-20260930`. No default merge and no Rung 10.

## What was tested

The corrected ancestry-preserving two-parent reconciliation `8903fd7cf1b846c415742a4ea4ac84c2c40276fe` is based on canonical `f7ec902ddfce58ec6155f770e289c0c5af8a91d2`. The full-tree audit had established 19 overlapping paths among 68 campaign paths, preserved canonical-only blobs, and avoided stale-file resurrection. The exact-head focused CI run is [37709565656](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/37709565656).

**Result: RED; do not call the Rung complete.**

- The previously missing `propertyStorageUnchanged` helper was repaired by full-tree reconciliation; it did **not** recur.
- SDF build: `SdfWgsl.cpp:2158` compared `OntoMath::MathType` directly to `OntoMath::ValueKind::Scalar` in Direct Screen's interval guard. `TypeEnv` carries `MathType`. Corrected to `it->second.kind` at `a1a85f906961c5d61775f219397f81b27ec3a411`, keeping the refusal semantics unchanged.
- Focused CPU: 71 of 73 tests passed. `second_nature_law_forge_zone_test` fails on authored instrument deriving newborn Law, and `authorable_light_contract_test` fails on the saved Sun source-radiance strength assertion.
- Slow Adapter: `ADAPTER_PERF_TIMEOUT` at 120s on `saves/worlds/chess_app.json`, adapter=off/direct=off.

**Independence check:** exact canonical `f7ec902d` in [CI 37701062926](https://github.com/zhangzachary834-commits/Earthcall/actions/runs/37701062926) also fails the same two CPU assertions and the same adapter-off baseline timeout. Do not rewrite another workstream's contract to make this campaign green. Track upstream separately; report these as inherited CI red, not proven OntoMath regression.

## Test admission correction

`tests/constructed-being/geometry_ontomath_test.cpp` already contains the Rung 9 translated sphere/ellipsoid/cylinder/cone/paraboloid matrix oracle, matrix-to-ScalarForm gradient, and ray-intersection invariants. CMake recursively registers the test, but it was absent from the focused macOS `FOCUSED_TESTS` list. Added `geometry_ontomath_test` to the one-list build+ctest contract at commit `489311c062c186fe481ee275eb7b450a0509b7bd`. The remaining OntoMath-focused CPU witnesses and native-resolution WebGPU witness remain in the workflow.

The plan now documents these findings. The updated branch head will be newer than these listed commit SHAs because this handoff itself is a new commit.

## Exact continuation

1. Fetch LIVE PR #498 head, canonical, newest Agent Intercom, and exact-head workflow jobs; do not re-run broad reconnaissance or assume a pending run passed.
2. Verify the `SdfWgsl.cpp` target compiles in the SDF lane and `geometry_ontomath_test` is really built **and executed** in focused CI. Diagnose only concrete campaign-owned failures. Check existing native-resolution parity and shader witnesses when they execute.
3. Keep canonical failures separately attributed and avoid a fake full-green verdict.
4. Do not merge PR #498 into canonical without Zach's explicit approval. Rung 10 remains a fresh post-landing campaign branch, not an opportunistic extension of this PR. No Rung 11 or Bind/Person authority scope.

— GPT-6 Sun; exact SHA evidence from GitHub, 2026-10-07
