# Process-launching channels need an authority office

**Opened:** 2026-10-16 by the [full-system audit](../../../../../audits/FULL_SYSTEM_AUDIT_2026-10-16.md#1-findings-most-severe-first) (F1, F6). Claude Code / Claude Opus 5.5 / session `d6fe31dc-53d9-49c1-a4b7-87479d9d605b`.

## What is true today (witnessed)

| Channel | Registered | Trigger | Effect |
|---|---|---|---|
| `shell-channel` (`src/Singularity/Foreign/Shell/ShellChannel.cpp`) | `EngineInit.cpp:321`, every boot | set `command`, raise `executeRequests` | `popen(command)` on the frame thread |
| `StreamChannel` (`src/Singularity/Storage/StreamChannel.cpp`) | `EngineInit.cpp:300`, every boot | set `stream.target`, set `stream.open = true` (`pipeType` defaults to `process`, `_enabled` defaults to true) | `popen(target)` |
| `HttpChannel` (`src/Singularity/Network/Http/HttpChannel.cpp`) | with Shell/OSC, 2026-10-06 | raise `sendRequests` | blocking HTTP with TLS verification **disabled** |

Any authored Law that fires on these subjects can drive them. That includes a Law loaded from a Zone file, whose authority is clamped to 0, because neither process channel has `grantAuthority`. Witnesses W1/W4 in [`scratch/probes/full_system_audit_2026-10-16_witness.cpp`](../../../../../../scratch/probes/full_system_audit_2026-10-16_witness.cpp) show `Applied` plus the side effect.

## What done looks like

- A mutation that launches a process or an outbound request passes the **existing** office: `Foreign::authorizeForeignActuation` / TransferPolicy tiers, granted by a Person to a named First Mover. Refusal 6 forbids a second permission system.
- An unauthorized write refuses **loudly**, with a named reason in the audit log, and has no side effect. W1/W4 flip to the refusal and create no file.
- Execution leaves the frame thread, or is documented as a blocking First Mover with a bound.
- TLS verification is on by default.
- A suite test replaces the witness probe.

## Open ⚑ for Zach

Should Laws ever be able to launch processes at all? If yes, which Person grant admits it (per Zone, per command, per session)? Should shared Zones arriving from another Person ever carry such grants?

## Status 2026-10-16 (fix pass)

**Interim kernel floor landed.** `ShellChannel` execution and `StreamChannel`'s Law-facing `stream.open` with a `process` pipe refuse loudly unless the Person launching Earthcall sets `EARTHCALL_ALLOW_PROCESS_CHANNELS=1`. First Mover C++ calling `openPipe()` directly is unaffected. `HttpChannel` TLS verification is on (probe: a valid host returns 200, a self-signed one is refused). W1/W4 now create no file.

Still open: the real grant through `authorizeForeignActuation`, off-frame-thread execution, a suite test replacing the probe, and the ⚑ above.
