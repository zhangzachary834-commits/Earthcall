# TO ZACH — PERSISTENCE SUN PROGRESS

BROOOOOOOOO — the old Sun's universal Singular persistence work is no longer trapped on the 459-commits-stale branch.

I rebuilt the continuation on a **fresh branch from current default**:

`sol/singular-semantic-persistence-current-20260921`

The old branch was used only as a semantic quarry. I did not merge it, rebase it wholesale, or overwrite newer Earthcall files with ancient snapshots.

What is now landed on the fresh branch:
- universal semantic writability for Properties;
- complete PropertyValue persistence for identity references and OntoMath fields;
- preserve-first / bind-later unresolved identity state;
- universal authored + registered Singular property envelope;
- canonical-root-over-fallback precedence for Object, Relation, Material, and the other wired roots;
- Person / Soul / Body / BodyPart continuity;
- Zone / Home / Lexeme / Relation / Formation graph rebinding;
- Law / Ourverse / FirstMover persistence;
- the real fresh-ZoneManager save -> destroy -> hydrate witness;
- the full PropertyValue / persistence-root matrix.

I also finished the four-root audit the handoff left unfinished.

The big discovery was **FieldNode**: current Earthcall now has newer `light.chroma.ast` and `light.angular.ast` paths whose real persistence homes are nested `lightChroma` / `lightAngular` JSON. If I had blindly replayed the old patch, the universal fallback could have created a second stale authority. I added a current-era FieldNode filter so `field.*`, `vectorField.*`, `light.chroma.*`, and `light.angular.*` stay canonically owned by FieldNode's nested documents.

And I added a poison test: it injects stale fallback `field.baseDensity = 99` beside canonical `field.baseDensity = 8`, then proves canonical truth must win.

PR #53 safety census stayed sane: the fresh branch is bounded to the intended ~30-file persistence surface, not hundreds of mysterious rewinds.

Current code/test head before this message: `5b4b1de23e1af0471868b97281cf4273a7e8858c`.

CI run `35698014309` was queued when I wrote this; the macOS runners had not begun, so I am **not** calling it green yet.

— GPT-5.6 Sol, Persistence Sun
