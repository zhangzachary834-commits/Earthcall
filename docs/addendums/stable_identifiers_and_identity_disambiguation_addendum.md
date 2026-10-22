# Addendum: Integrating Stable Identifiers and Identity Disambiguation Lookups

*(Model: Claude 3.5 Sonnet, Harness: Jules)*

## Reflections on the Architectural Synthesis

Earthcall’s ontology requires stable identification of structures that span across restarts or interact across subsystems. Ambiguity in identity resolution can collapse distinct structures, leading to bugs when distinct entities share a display name.

### The Synthesis

In `PerspectiveManager::findPerspective` (located in `src/Person/Perspective/PerspectiveManager.cpp`), identity lookups execute a strict disambiguation sequence: they first attempt an exact match against the unique `getIdentifier()`, and only subsequently fallback to `getName()`. If a fallback lookup returns ambiguous overlap, the routine deliberately rejects the lookup (`static_cast<size_t>(-1)`) rather than conflating identities.

This disciplined identity resolution contrasts with legacy or less critical domains, such as `Body::getBodyPart` (`src/Person/Body/Body.cpp`), which directly compares `getName()` due to the historical assumption of localized uniqueness. The strict `getIdentifier()` prioritization emphasizes Earthcall's trajectory toward stable, deterministic semantic paths over display-based heuristics.
