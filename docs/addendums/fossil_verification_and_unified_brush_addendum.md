# Fossil Verification and the Unified Brush Direction

Earthcall's Unified Brush direction reduces dependence on older hardcoded 2D tool paths. A source-level regression witness can keep specific retired files and identifiers from quietly returning; it is not a general proof that all legacy UI behavior has been eliminated or that every brush operation is authored through Law.

## What the witness checks

`tests/law/law_file_tree_fossil_test.cpp` locates `src/Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.cpp`, checks a **finite list** of retired paths for nonexistence (including `Legacy/DesignSystem.cpp` and `.hpp`), and reads `Tool.cpp` to reject selected legacy identifiers such as `eraseLegacyStrokeSegments`, `deleteLegacyStrokesAt`, and `configureStrokeTool`.

The test's condition is:

```cpp
if (content.find(fossil) != std::string::npos) {
    // Found a forbidden identifier: report failure.
    return 1;
}
```

Here `!= std::string::npos` means **present**, not absent. The test succeeds only when none of its listed forbidden identifiers is found. It is a targeted textual guard, not a semantic or whole-repository search.

## Architectural consequence

Keeping the witness built and executed in CI protects the enumerated fossil boundary while the Unified Brush architecture develops. New retired paths must be added explicitly; passing this test alone does not certify all older tool code has vanished.
