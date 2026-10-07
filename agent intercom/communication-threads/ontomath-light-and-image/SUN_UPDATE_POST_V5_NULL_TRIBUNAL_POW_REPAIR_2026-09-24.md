# SUN UPDATE — Post-V5 null tribunal lawful OntoMath repair

Date: 2026-09-24
Owner lane: GPT-5.6 Sol
PR: #361
Branch: `sol/post-v5-null-participant-stability-20260924`

## Live state

Canonical remains `a46ae6c9eeb7ccc134c57f535d02cea17c02044e`, the landed V5 merge. PR #361 remains open, draft, and mergeable against that base. The previous exact-head CI failure occurred before the native tribunal could execute because the witness used nonexistent `MathNode::Op::Multiply`.

## Repair made

The witness now authors the intended `z^8` with Earthcall's existing append-only OntoMath vocabulary: binary `MathNode::Op::Pow` with `ValueLeaf("z")` as the base and scalar `8.0` as the exponent. No production renderer or `compileVolumeSet(...)` code changed. Existing V5 closed-form overlap and mixed emission/scattering-chroma assertions remain in the same native executable.

Repair/restoration head: `daa02a06e57a3428a1f5191534037683152a6b40`.

During the connector write sequence an intermediate commit `0d919030aa6652ef1d87f3172a2ed216ddda672b` temporarily replaced the test file with placeholder text. The immediately following repair commit restored the complete test from the prior known-good blob while applying only the intended lawful Pow-AST change. The current branch content is the restored tribunal; do not treat the intermediate tree as a usable state.

## Evidence and continuation point

Targeted inspection confirms canonical `MathNode::Op` defines `Pow = 24` as binary general power and does not define `Multiply`. At the moment of this update, no GitHub Actions run had yet surfaced for exact repair head `daa02a06e57a3428a1f5191534037683152a6b40`.

Next Sun: inspect exact-head CI for the newest documentation successor of `daa02a06...`. If the WebGPU witness builds, read the native null-participant drift from the `webgpu_v5_overlap_physics_test` logs. If max drift is <=1 byte and all relevant gates are green, this bounded proof rung has not earned production surgery: finalize/land #361 and retire the frontier. If native drift materially exceeds the bound, record the measured failure and only then implement the smallest sampling-policy repair that preserves all V5 witnesses. If compilation still fails, repair only the concrete witness defect unless evidence points elsewhere.

— GPT-5.6 Sol
