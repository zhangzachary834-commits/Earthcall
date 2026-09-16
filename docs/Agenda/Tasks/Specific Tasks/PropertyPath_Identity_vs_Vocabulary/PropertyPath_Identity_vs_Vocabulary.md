# PropertyPath identity vs vocabulary

**Status:** ✅ done and verified (reconciled 2026-09-16)  
**Section in the To-Do list:** Architectural Actualization → Singular · Relation · Formation → Property · PropertyPath  
**Human origin:** Zach raised the architectural tension that Property lookup caches spellings through string interning while Singular beings are individuated by stable identity, and warned that root resolution could repeat Earthcall's earlier “is versus is-called” failures.

## Resolution

The two layers now have distinct jobs rather than sharing one notion of identity.

- `PropertyPath` interns **property vocabulary** (`shape.color.r`, `image.pixelWidth`, etc.) as `StringId`s so the hot path can compare compact integers without heap allocation or repeated string comparisons.
- A qualified Law root (`@material.clay`, `@state.studio.voice`, etc.) resolves to an actual `Singular*` through the Universe referent map using the being's stable `getIdentifier()`; dotted identifiers are matched longest-first before the remaining segments are offered to property lookup.
- Therefore an interned property spelling does not become the identity of the Singular that owns it: the root chooses **which being**, while the interned suffix chooses **which property vocabulary on that being**.
- Duplicate or same-spelled language beings are handled by the LanguageSystem's identity-aware path: exact stable ids and `@<exact-id>` select one Lexeme; plain spelling remains only a convenience/default binding and reports ambiguity.

This preserves the optimization without making “called X” equivalent to “is X.”

## Regression witnesses

The architecture is guarded at the seams where a spelling/identity collapse would become observable:

1. `tests/law/dynamic_property_reachability_test.cpp` writes a plain dynamic key, a flat dotted key (`image.pixelWidth`), and a nested component reached through a `PropertyDict`/`Singular*` (`region.tint.g`), then requires the listening Laws to fire; simple value readback is explicitly not accepted as the verdict.
2. `tests/law/referent_map_invalidation_test.cpp` guards the qualified-root cache against going stale when named beings arrive or the relation/world structure changes.
3. `tests/language/language_identity_reference_test.cpp` guards exact Lexeme identity and duplicate-spelling behavior, while the Terminal CI smoke exercises `@lexeme.christ` through mutating commands.
4. Focused GitHub CI includes these witnesses, so a future optimization that re-collapses owner identity into a spelling cache fails loudly.

## Historical bug closed during the same thread

The Phase-2 PropertyPath rewrite briefly announced writes to dotted dynamic properties under only their final path segment, so a write could read back correctly while the Rete kept the old fact and the Law stayed deaf. Commit `2b69221c` merged the end-to-end Law-firing witness after the announcement path was repaired; the task index should no longer describe dotted dynamic writes as an open failure.

## Remaining work that is *not* this task

PropertyPath governance, TransferPolicy, and the broader no-black-box programme remain separate. Likewise, cryptographic/self-certifying `SingularId` work and language semantic identity can continue evolving without changing the separation above: stable being identity chooses the root; `StringId` is a local vocabulary/cache key below that root.
