# Illustrative examples — NOT loadable saves

The JSON in this directory is **prose in JSON's clothing**. It shows the shape an
Integration mapping is meant to take; none of it will load. It lived in
`saves/fixtures/` until 2026-08-13 and was moved here because a mockup sitting
beside real save files eventually gets read as one.

| File | Illustrates |
|---|---|
| `foreign_zone_dump.json` | a macOS Accessibility tree projected into an isolated Zone (K0–K3) |
| `metalaw_behavior_synthesis.json` | a Person-authored metalaw vetting laws proposed by the ML First Mover |

## Why they don't load

Both were written against an invented vocabulary. Nothing in `src/` parses any of it:

- `metalaw_behavior_synthesis.json` uses `conditions` / `actions` arrays with string
  discriminators — `property-match`, `event-match`, `spawn-law`, `"type": "metalaw"`.
  Real laws serialize as `authoredLaws.laws[].conditionModel` / `.actionModel`, with
  **integer** `kind` fields off the append-only enums. `saves/fixtures/inference_law.json`
  is the worked example.
- `foreign_zone_dump.json` puts `zoneId` at the root and gives objects a `concept` key;
  neither matches the Zone save schema.
- Neither carries an `injectedBy` envelope, so even in the correct schema
  `FIRST_MOVER_AUTHORING.md` §8c would load them quarantined.

## What a real `metalaw_behavior_synthesis` still needs

Deferred deliberately — this is a design question, not a transcription job:

1. The correct integer `ActionNode::Kind` for spawning a law. The enum is append-only
   and two `ConditionNode::Kind` values (12, 13) are burned; read the enum, do not guess.
2. An `injectedBy` envelope naming the First Mover that wrote it and the Person it acted
   for — `{"mover": "<the AI or tool>", "onBehalfOf": "zacharyzhang"}`, per §8b. The mover
   is *not* the Person.
3. **An answer to the authority question.** `kAuthoredCeiling` is clamped to 0 on every
   path that reads a file, and `Law::ApplicationResult::AuthorityDenied` exists precisely
   because lower authority may not govern higher. A metalaw loaded from disk at authority
   0, governing laws that are also at authority 0, may not be expressible as a save file
   at all — it may have to be constructed in-process. Resolve this before writing the
   fixture; it determines whether the fixture can exist.
