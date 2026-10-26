# §6dk — Scratch-lifetime intervention; qualified correctness, no accepted speedup

**Original implementation:** Antigravity / Gemini / session `50cf28df-09e9-4c2b-a4d5-1f796abbc73a` / 2026-10-25T18:55:00-07:00.

**Corrective review:** Codex / GPT-6.1 Sol / session `01a148a7-bd85-7550-a416-0e859a729c1a` / 2026-10-25T19:24:08.355403-07:00. Zach requested independent review; [§6dl's executed evidence](GPU_RAY_TRANSPORT_SCRATCH_LIFETIME_REVIEW_6DL_2026-10-25.md) supersedes the original batch-win, physical768-byte-saving and tail-removal interpretations. The original report and machine record are preserved in that review bundle. No numerical implementation or Earthcall production behavior was changed by the review.

Sparkly delayed reconstruction of member knots until after roots, narrowed member-knot storage to70entries and placed96-entry merge storage after the disjoint early return. Old`bounds` aliases`knots`; it was not a second array. The unchanged32-band limit needs at most69member knots; the unchanged48-piece limit needs96merge endpoints. Source scopes do not establish physical allocation savings.

Raw native evidence replays on32784queries for both variants: guarded packet/output parity,0normalfallbacks,247326singles/1799overlaps,12102protected affine matches,192holdouts at both reference resolutions and existing budget/capacity/fallback controls. Strict bit parity is to the guarded candidate reference; Current is a different integration baseline. The1e-5CPU/GPU AST check is distinct from the integrated current-or-better RGB/T gates.

The actual trimmed candidate is`cpFastCompact` in mode3. Its recorded resident costs lose Current: disjoint+0.686/+12.483/+2.447%, tile-cert+0.113/+0.947/+1.141%, at repeats1/2/4. The previously advertised tiny BATCH1wins are from instrumented capture runs and dispatch unchanged`cpProducer`; they do not measure scratch trimming. Host tile-certificate preparation/initial upload is outside the mode3timing. No matched before/after series proves removal of historical tail drift.

Fresh §6dlXcode profiles show both trimmed producers spill2.22KiB, up from1.95KiB. Registers fall206→202and208→204, but L1cache-pressure influence remains~100%; register-pressure influence stays0%. This intervention preserved tested answers while failing its measured spill-reduction objective.

The two source-hashed captures remain `/private/tmp/earthcall-6dk-capture-disjoint/disjoint-trimmed.gputrace` and `/private/tmp/earthcall-6dk-capture-cert/cert-trimmed.gputrace`; exact producer hashes are unchanged in the machine record. See §6dlfor raw exports, replay, scoped next steps and remaining evidence requirements. All3shipping gates OPEN; no Earthcall src/default/save/app/FPS or Person acceptance change. Native lane RELEASED.
