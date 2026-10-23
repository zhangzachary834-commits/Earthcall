# Stable Identifiers and Ambiguous Display Names

**Scope:** Current canonical lookup implementations; identity and display spelling are distinct concepts.

## What the code currently guarantees

`PerspectiveManager::findPerspective(nameOrId)` in `src/Person/Perspective/PerspectiveManager.cpp` first scans for an exact `getIdentifier()` match, then scans for a `getName()` match. This makes exact identifiers take precedence, but **the current name fallback returns the first match**: it does not detect duplicate display names or reject ambiguity. `hasPerspective` checks identifiers, not names.

`Body::getBodyPart(name)` in `src/Person/Body/Body.cpp` currently compares `getName()` only; `removeBodyPart(name)` also matches names. These are current-code observations, not desirable identity guarantees. Proposed fixes in other open PRs are not assumed to have landed in canonical.

## Required stronger contract

An unambiguous resolver should match stable identifiers before display labels, and reject or explicitly disambiguate a label shared by multiple objects. The same-name case needs a regression witness for both lookup and removal, so that presentation labels never silently become ontological identity. This addendum documents the gap rather than claiming that canonical already enforces it.
