# Property Prefix Matching and Ontological Shadowing

**How the longest-prefix match algorithm enables safe property overriding without violating No Black Box.**

**Status:** Conceptual interrelation.
**Connected Systems:**
*   `src/ConstructedBeing/Singular/Property/PropertyPath.cpp`
*   `../ontology/NO_BLACK_BOX.md`

---

## The Interrelation

In Earthcall, property paths (e.g., `entity.arm.weapon.damage`) are resolved using a longest-prefix matching algorithm in `PropertyPath::resolve`. The algorithm iterates `runLength` from the longest subpath (`idsFromHere.size()`) down to 1, terminating early upon finding a match.

This specific technical implementation interrelates deeply with the **No Black Box** principle and Earthcall's ontology. Rather than hardcoding deep inheritance trees where attributes are structurally baked into C++ classes, property shadowing occurs organically through data paths. By matching the longest registered prefix first, the system allows temporary, dynamic properties to safely override or "shadow" deeper, more generalized properties without mutating or deleting the underlying structures.

This means a Person or Law can author a specific relation on a subpath, and the continuous execution sweeps will correctly resolve to the most specific intention (the longest prefix match), avoiding redundant lookups while preserving the true ontological state of the root entity.
