# Earthcall Web Designer Modality Task

## Status
- **Date**: 2026-10-14
- **Status**: ✅ Completed and Verified (Hardened Person-exclusive stewardship, unauthored substrate vocabulary, in-world Lexeme denotes scanning, browser trust boundary sanitization, relational precedence DAG, and 14/14 tests passed)
- **Harness/Agent**: Antigravity / Gemini 3.8 Flash ("Sparkly Guy") · session `b5354f7e-b110-44af-9393-61ae139053de`

## Objective
Establish the Earthcall Web Designer Modality as a genuine Earthcall Law-authoring domain with all architectural gift shops removed:
- **Person-Exclusive Stewardship**: Authorship strictly means human stewardship. `authored-by` endpoints are strictly `Person*`s. The "Zach Plush" / `s_modalityFirstMover` surrogate proxy object is completely deleted.
- **Unauthored Substrate Infrastructure**: Modality initialization (`WebDesignerMetalaws::syncRegister(LawManager& laws)`) takes no author parameter. Runtime initializer is not human author; built-in substrate vocabulary is unauthored infrastructure (`law->isAuthored() == false`).
- **Enduring Authorship across Absence**: Human authorship persists after authoring; a Person's current presence is not required at execution time, and no surrogate cubes or objects babysit Laws.
- **Multi-Person Isolation**: Distinct Persons (e.g. Alice and Bob) maintain strictly isolated authorship provenance; no process-global authorship leak.
- **In-World Lexeme Relation Scanning**: Input text matches in-world `Relation("denotes", *lexeme, *law)` via greedy longest match across Lexemes; private tokenizers and static dictionaries removed; does not hijack Terminal's `LawSentence` or `SyntacticParser`.
- **Browser Trust Boundary Hardened**: Strict foreign-medium validation without restricting human meaning (`sanitizeHtmlTag`, `normalizeCssToken`, `sanitizeHref` with explicit scheme allowlist, and `sanitizeCssValue` to neutralize `</style>` and comments).
- **Universalized Properties**: Meaning generalized above `web.*` (`role`, `level`, `layout.flow`, `layout.align`, `surface.variant`, `subordinate`, `prominence`); `web.*` strictly reserved for medium-specific browser parameters.
- **Relational Ordering & Provenance**: Parent-child via `web-contains` relations; sibling sequence derived topologically from `precedes` relations via Kahn's algorithm (strictly overriding numerical `web.order`); authorship provenance preserved via `authored-by` relations.
- **Thin Realizer & Authored Theme Tokens**: Emits CSS variables directly from root `theme.*` properties; minimal fallback tokens; zero hardcoded toast UI.
- **Storage Governance & World State Independence**: Direct `std::ofstream` removed; persistence routed through `VirtualFileSystem` (`memory://`) and `FileChannel` (`file.path`, `file.content`, `file.atomicWrite`); pure realization returns HTML without requiring storage; channel destruction leaves site intact in world state.

## Evidence & Verification
- **Test Suite**: `tests/singularity/web_designer_modality_test.cpp` (all 14 tests passing).
- **Prototype Artifact**: `web_ui/designer_prototype.html` and `saves/web_designer/prototype.html`.
- **Governed VFS Export**: `saves/web_designer/test_governed_vfs.html`.
- **Core Architecture Document**: [WEB_DESIGNER_MODALITY.md](../../../../architecture/Integration/WEB_DESIGNER_MODALITY.md).

## Future Rungs
1. Live WebSocket bidirectional sync between browser RealWebView and `WebDesignerChannel` during interactive authoring sessions.
2. Direct OntoMath SDF rasterization / embedding into authored canvas/SVG elements.
3. Form input concepts (text field, toggle, range slider) bound directly to `PropertyPath`.
