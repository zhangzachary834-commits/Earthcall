# Integrating Fossil Verification and the Unified Brush System

In Earthcall, the C++ codebase is expected to reflect the active ontology without carrying dead, deprecated concepts. When architectural shifts occur—such as replacing legacy hardcoded UI tools with the Unified Brush System—the remnants of the old architecture (fossils) must be explicitly removed.

To ensure these fossils do not inadvertently return, Earthcall integrates explicit verification through `tests/law/law_file_tree_fossil_test.cpp`.

## Retiring Legacy Concepts

Previously, the engine relied on a hardcoded UI and drawing paradigm, represented by fields such as `_useLegacy2DTools`. When the Unified Brush System was introduced, it provided a general, relation-based approach to materials and ontomath manipulation.

The older tools (like `configureStrokeTool`, `eraseLegacyStrokeSegments`, and `deleteLegacyStrokesAt` in `src/Singularity/FirstMoverOntology/FirstMoverWindowTools/Tool.cpp`) and files (like `Legacy/DesignSystem.cpp`) became ontological dead ends. They violated the principle that new kinds and behaviors should be authored in Law rather than hardcoded in C++.

## Automated Fossil Verification

Removing dead code is standard engineering practice. Earthcall's integration goes a step further by explicitly codifying the *absence* of these structures as a testable invariant.

In `tests/law/law_file_tree_fossil_test.cpp`, the test actively scans the source tree:

1. **File Absence:** It asserts that files like `src/Singularity/FirstMoverOntology/Legacy/DesignSystem.cpp` do not exist.
2. **Textual Absence:** It reads `Tool.cpp` as raw text and uses `content.find(fossil) != std::string::npos` to assert that specific strings, such as `"eraseLegacyStrokeSegments"` and `"configureStrokeTool"`, are entirely absent from the file.

By integrating fossil verification, Earthcall guarantees that the transition to the Unified Brush System is irreversible in the C++ layer. If a developer accidentally restores a legacy file or re-adds a hardcoded legacy method during a rebase, the test suite will fail. This enforces the structural integrity of the codebase, ensuring that the engine remains a clean vessel for the authored ontology.
