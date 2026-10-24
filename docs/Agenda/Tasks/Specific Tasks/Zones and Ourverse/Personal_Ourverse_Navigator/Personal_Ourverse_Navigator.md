# Personal Ourverse Navigator

**Zach's idea (2026-10-12), recorded verbatim in spirit and left for its own session:** a "beautiful personal Ourverse navigator with little 2D/3D islands representing each world." This is what a Person meets when Earthcall.app opens, and what a fresh data home's first run should grow into.

## Where it attaches (already built, 2026-10-12)
- When the installed app first creates a data home, it copies `Earthcall.app/Contents/Resources/Foundation/` (sourced from `scripts/macos/Foundation/`, which doesn't exist yet) into it. See `SaveSystem::chooseDataRoot(…, foundation)` and the [macOS app task](../../Housekeeping%20and%20Build/macOS_App_Bundle/macOS_App_Bundle.md).
- Today an empty home boots into the engine's default Zones, so the navigator has no prerequisite and is purely additive.

## Constraints to carry into that session
- **Interface:** it is Law plus set-to-set aimed at the pointer, not a widget (`docs/architecture/law/INTERACTION_AS_LAW.md`).
- **Form:** islands are authored form (OntoMath, CSG, Relations), never a new `ShapeKind` or C++ class (Refusals 1 and 3; `Design/Building 2D and 3D Apps with Earthcall Guide.md`).
- **Meaning:** worlds are Zones, and the Ourverse is the vessel of unity, not an object bag (`docs/architecture/ourverse/OURVERSE.md`).
- **Authorship:** the Foundation ships no Person, key or Home of Zach's. Anything seeded carries an author, under `injected_by` conventions.

*Recorded by Claude Code / Claude Opus 5.5 / session `16d04786-8407-4a70-a57c-fa5c11094c91` / 2026-10-12. The idea is Zach's.*
