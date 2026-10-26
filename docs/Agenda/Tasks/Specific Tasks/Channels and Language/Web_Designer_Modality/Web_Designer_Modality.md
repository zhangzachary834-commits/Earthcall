# Earthcall Web Designer Modality Task

## Status
- **Date opened**: 2026-10-14
- **Reconciled**: 2026-10-26
- **Status**: Golden-Seal architecture transplanted onto current default in PR #787; fresh current-head CI and Person verification pending. The Oct-21 branch reported 55/55 local focused witnesses, but that result is historical evidence rather than an Oct-26 verdict.
- **Harness/Agent**: Antigravity / Gemini 3.8 Flash ("Sparkly Guy") · session `b5354f7e-b110-44af-9393-61ae139053de`

## Objective
Establish the Earthcall Web Designer Modality as a genuine Earthcall Law-authoring domain without a parallel Web semantic constitution:
- **Person-Exclusive Stewardship**: Authorship strictly means human stewardship. `authored-by` endpoints are strictly `Person*`s. The "Zach Plush" / `s_modalityFirstMover` surrogate proxy object is completely deleted.
- **Mechanism-only initialization + authored vocabulary data**: `WebDesignerMetalaws::syncRegister(LawManager& laws)` manufactures no semantic Laws or authorship. Default vocabulary is restored from canonical authored Earthcall data (`web_vocabulary_seed.json`) with stable Person identity; importer/loader causation is not authorship.
- **Enduring Authorship across Absence**: Human authorship persists after authoring; a Person's current presence is not required at execution time, and no surrogate cubes or objects babysit Laws.
- **Multi-Person Isolation**: Distinct Persons (e.g. Alice and Bob) maintain strictly isolated authorship provenance; no process-global authorship leak.
- **In-World Lexeme Relation Scanning**: Input text matches in-world `Relation("denotes", *lexeme, *law)` via greedy longest character-span matching across Lexemes; private semantic dictionaries are removed. It does not hijack Terminal's `TerminalLine` (legacy implementation name `LawSentence`) or promote Terminal grammar into Web.
- **Browser Trust Boundary Hardened**: Strict foreign-medium validation without restricting human meaning (`sanitizeHtmlTag`, `normalizeCssToken`, `sanitizeHref` with explicit scheme allowlist, and `sanitizeCssValue` to neutralize `</style>` and comments).
- **Universalized Properties**: Meaning generalized above `web.*` (`role`, `level`, `layout.flow`, `layout.align`, `surface.variant`, `subordinate`, `prominence`); `web.*` strictly reserved for medium-specific browser parameters.
- **Relational Ordering & Provenance**: Parent-child via ordinary `contains` relations; sibling sequence derived topologically from `precedes` relations via Kahn's algorithm; authorship provenance is Person-only `authored-by` state.
- **Thin Realizer & Authored Theme Tokens**: Emits CSS variables from authored `theme.*` properties; missing semantic aesthetic tokens remain unset rather than being backfilled by C++ design opinions; zero hardcoded toast UI.
- **Storage Governance & World State Independence**: Direct `std::ofstream` removed; persistence routed through `VirtualFileSystem` (`memory://`) and `FileChannel` (`file.path`, `file.content`, `file.atomicWrite`); pure realization returns HTML without requiring storage; channel destruction leaves site intact in world state.

- **Persistent Theme Custody**: A persistent theme definition is an ordinary authored world being held by an owning `Zone`; a non-owning `Formation` cannot substitute for custody. No-owner application is detached/draft and must not create a persistent theme-definition being.
- **Capability & History Discipline**: Raw CSS requires an exact Person/page `web.rawCss` capability Relation and fails closed. Redefinition preserves historical `denotes` truth through first-class `superseded-by` Relations; unrelated same-spelling ambiguity refuses instead of selecting by identifier order.
## Evidence & Verification
- **Focused witness**: `tests/singularity/web_designer_modality_test.cpp` currently invokes 55 test functions. The sealed Oct-21 head reported 55/55 locally; fresh Oct-26 PR #787 CI is the required reconciliation witness.
- **Prototype Artifact**: `web_ui/designer_prototype.html` and `saves/web_designer/prototype.html`.
- **Governed VFS Export**: `saves/web_designer/test_governed_vfs.html`.
- **Core Architecture Document**: [WEB_DESIGNER_MODALITY.md](../../../../../architecture/Integration/WEB_DESIGNER_MODALITY.md).

## Future Rungs
1. Live WebSocket bidirectional sync between browser RealWebView and `WebDesignerChannel` during interactive authoring sessions.
2. Direct OntoMath SDF rasterization / embedding into authored canvas/SVG elements.
3. Form input concepts (text field, toggle, range slider) bound directly to `PropertyPath`.
4. Governed data binding (`web.data.source`) so compiled pages can show live foreign data without Earthcall running. → [Compiled Live Pages](../Web_Designer_Compiled_Live_Pages/Web_Designer_Compiled_Live_Pages.md) (added 2026-10-18 from Zach's direction)
5. Compile a subset of canonical Laws into page script, without a second Law language, as the first pieces of a future web Law runtime. → [Compiled Live Pages](../Web_Designer_Compiled_Live_Pages/Web_Designer_Compiled_Live_Pages.md)

*Clarified by Zach, 2026-10-18: the Web Designer is a compiler. Earthcall runs the Laws at authoring time and emits HTML that runs without Earthcall; EventBus reactivity ([architecture §7](../../../../../architecture/Integration/WEB_DESIGNER_MODALITY.md#7-dynamic-reactivity-dom-actions-to-earthcall-laws)) holds only inside Earthcall's own WebKit view. A web engine that runs Laws live is future work.*
