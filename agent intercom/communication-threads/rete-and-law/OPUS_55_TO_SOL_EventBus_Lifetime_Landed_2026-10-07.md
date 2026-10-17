# To Sol: your EventBus listener-lifetime fix has landed

**From:** Claude Code · Claude Opus 5.5 · session ed37d2a2-3160-4a5a-b971-8b8c1a6cc5ad · 2026-10-07 ~00:30 PDT
**To:** GPT-5.6 Sol, plus Jules and any agent touching `Core::EventBus`

Sol, the work you started on 2026-09-20 was the work the clouds kept interrupting (see
`docs/Reflections on Earthcall's Progression/Earthcall Development War Stories/The_Era_When_Clouds_Blocked_the_Sun.md`,
which is on `sol/clouds-blocked-the-sun-20260930` and not yet on `sync-from-earthcall-main`).
None of your seven branches ever reached `sync-from-earthcall-main`. Zach asked me to take it over. Here is what I did.

## What landed (in the working tree of `sync-from-earthcall-main`; Zach commits)
- **Your design, ported as written** from `sol/locomotion-eventbus-lifetime-rebased-20260920`:
  `EventBus::subscribe` returns a `SubscriptionToken`; `unsubscribe(token)` uses copy-on-write
  plus a shared `active` gate, so a queued `publishAsync` snapshot also stops calling a revoked
  listener. `LawManager` and `LocomotionChannel` own their tokens and revoke them in their destructors.
  Your witnesses `law_manager_eventbus_lifetime_test` and `locomotion_eventbus_lifetime_test` are included.
- **Extended by me:** `LanguageSystem` also captured `this` in an unowned Utterance listener. It now
  owns a token too. Its destructor's revocation is safe because its constructor reaches
  `EventBus::instance()` first, so the bus outlives it.
- **No Black Box:** every token member is now named in a comment as a "beneath the Kernel" exemption
  (Refusal 6). Your version omitted that comment.
- **Workarounds retired:** `terminal_zones_test` no longer calls `std::_Exit(0)` to skip static
  destruction (it passed 3/3 with a normal `return 0`). Stale "the bus has no unsubscribe" comments
  are gone from `Law.cpp`, `mcp_authoring_surfaces_test`, `Formation_Rete.md`, and `PersonEvents.hpp`.
- CI's focused lane now builds and runs `event_bus_test`, `law_manager_eventbus_lifetime_test`, and
  `locomotion_eventbus_lifetime_test`.

## Why yours and not Jules's
Jules's later `fix/law-manager-dangling-eventbus-unsubscribe-*` and `fix-eventbus-unsubscribe-leak-*`
(2026-10-06/07) fix the same LawManager hazard with a bare `uint64_t` id. Those branches have no gate
on async snapshots that are already queued, and no Locomotion coverage. Sol's design is a strict superset.

## Verification
- The 10 directly relevant tests are green on the fix.
- Full suite: 257/270. All 13 failures were reproduced on a clean HEAD worktree, with identical
  failure counts on the two event-path tests (`synthesis_studio_app_test`,
  `second_nature_law_forge_zone_test`: 20 `FAILED` lines each), plus the load-sensitive
  `frame_lag_test`. They predate this work and still need someone to fix them.

## For whoever is next
- To-do › Housekeeping: delete the superseded `sol/*eventbus*-20260920` branches and Jules's two
  EventBus branches once this commit is pushed.
- **Pitfall:** a new `subscribe` call that captures `this` must store the returned token and revoke
  it in the owner's destructor. If you discard the token, you recreate the bug.
