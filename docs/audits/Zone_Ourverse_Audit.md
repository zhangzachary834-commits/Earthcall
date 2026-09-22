# Zone Implementation Audit Report (Updated Paradigm)

This report analyzes how the current `ZonesOfEarth` implementation in Earthcall's C++ codebase measures against the updated ontological vision set out in `EarthcallOurverse.md`.

> [!WARNING]
> The current codebase deviates significantly from the `EarthcallOurverse.md` manifesto, primarily by treating `Zone` and `Home` as rigid, hardcoded C++ domain nouns rather than authored `Singulars` connected via `Formations` and `Laws`. This directly violates Refusal #1 ("No new C++ class for a domain noun").

## 1. Nature of a Zone
**Manifesto Vision (Updated):**
- Zones are fields that handle spatial/jurisdictional existence.
- "Zones are Singulars, not Objects, but can gain Object-like components by owning Objects."

**Current Implementation (`Zone.hpp`):**
- `Zone` is unconditionally hardcoded as `class Zone : public Object`. 
- **Violation:** `Zone` inherits from `Object` rather than `Singular`. It acts as a monolithic C++ class loaded with domain-specific properties like `float drawR`, `std::unique_ptr<BrushSystem> brushSystem`, `strokes`, and `designSystem`. A Zone should just be a `Singular` and gain spatial/visual properties by owning authored `Objects` in its `Formation`, rather than hardcoding C++ struct fields.

## 2. The Global Ourverse
**Manifesto Vision (Updated):**
- `Ourverse` is a `Singular` that owns one primary gathering `Zone` to represent ecumenical telos.
- `Ourverse` is a larger system innately owning a `Relation/Formation` of `Laws` that ensures proper order between Zones (preventing isolation, preventing consolidation, guarding against Community-level laws) and makes Zones reachable to each other.

**Current Implementation (`Ourverse.hpp`):**
- `Ourverse` inherits from `Singular`. *(This part aligns with the new paradigm!)*
- **Violation:** `Ourverse` does not own a primary ecumenical `Zone`, nor does it govern `Zone`s through a `Relation/Formation` of `Laws`. Instead, it merely acts as a hardcoded container holding `std::vector<Zone> zones;` and `std::vector<Home> homes;`. The mechanisms for reachable Zones and MetaLaw guardrails are entirely missing.

## 3. Overlapping Zones and Field Sharing
**Manifesto Vision:**
- "Zones can exist inside other zones. Zones can also overlap with other zones. Multiple zones that share a field can have the field itself be a Zone."

**Current Implementation:**
- `Zone.hpp` uses a single `std::string _parentZoneName;` to model hierarchical embedding, which strictly limits it to a tree structure. 
- There is no native ontological structure for Zones to "overlap". Overlapping would need to be implemented via `Relation`s or shared `Formation` members, but currently, `ZoneManager` just manages a flat vector of Zones and switches between them (`switchTo(size_t index)`), treating them more like independent video game levels than a unified, overlapping field.

## 4. Homes and Persons
**Manifesto Vision:**
- "A Home is a Zone that is a digital dwelling space for at least one Person."

**Current Implementation (`Home.hpp`):**
- `Home` inherits from `Zone`.
- **Violation:** `Home` includes hardcoded state fields: `std::vector<Person*> _persons;` and `std::vector<Object*> _objects;`. In Earthcall's architecture, a Person's presence or ownership in a space should be mediated entirely by the `Formation` system or a `Relation` to the Home, never by hardcoded arrays of pointers within the C++ domain noun. `Zone` already has a `Formation _formation`, so these vectors are entirely redundant and philosophically incorrect.

## 5. Law Attachment & Jurisdiction
**Manifesto Vision:**
- "Zones handle everything with respect to jurisdiction. The law hierarchy considers the zones."
- "Laws are all inherently attached to at least one Zone."

**Current Implementation (`Law.hpp` & `LawContext.hpp`):**
- Laws have no structural attachment to `Zone` objects. `LawContext` just implicitly evaluates against whatever zone `ZoneManager::active()` returns. There is no concept of jurisdictional boundaries, MetaLaw constraints based on Zone hierarchies, or resolving conflicts between overlapping jurisdictions.

## 6. Freedom of Movement (Exit Locks)
**Manifesto Vision:**
- "Nobody can be forced to stay in another person's zone against their will. Zone exit lock rejection is guaranteed."

**Current Implementation:**
- There is no implementation of exit lock rejection, movement authorization, or Person guards preventing forced retention within a Zone.

---

### Conclusion & Recommendations
The current `ZonesOfEarth` system is built like a traditional game engine's scene manager, completely circumventing Earthcall's ontological architecture. 

To align with the updated manifesto, the system needs a substantial refactor:
1. `Zone` must inherit from `Singular`, not `Object`.
2. Strip hardcoded arrays (`_persons`, `_objects` in `Home`) and domain logic (`brushSystem`, drawing states in `Zone`) and port them into authored `Object`s within a `Zone`'s `Formation`.
3. `Ourverse` must drop `std::vector<Zone>` and instead manage a `Relation/Formation` of `Laws` to govern the reachability and order of Zones.
4. Zone overlapping and embedding must be represented via `Relation`s or `Formation` structures, rather than a monolithic `_parentZoneName` string.
5. `Law` must be structurally attached to `Zone`s to enforce true jurisdictional evaluation.
