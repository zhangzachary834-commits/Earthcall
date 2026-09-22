# Law Author Library and Relation Graph

**Status:** implementation complete; Person visual verification required.
**Section in the To-Do list:** Law · Kernel · Governance

## Intent

Zach asked for separate windows opened from Law Author that make a large law world legible: a grouped library and a graphical graph of Laws joined by Relations. This extends his earlier authoring-window concern: a Person should not have to decode a long, flat register or infer whether two Laws are truly connected.

`LawGraphWindow.cpp` now provides a **Law Library**, grouping Laws by authored category
beings and their `instance-of` / `subcategory-of` Relations; and a **Law Relation Graph**,
drawing only first-class `Relation` beings whose two endpoints are `Law` Singulars.
Selecting a Law in either window focuses it in the main authoring window. Search matches
category display names, category identifiers, Law names, and Law identifiers. Laws may
appear in more than one category, and Laws without a membership edge remain visible under
Uncategorized.

Zach's correction governs the implementation: a category is a raw Singular–Relation DAG,
not automatically a Formation. Formationhood requires at least three members and a
non-hub topology; the Library therefore reads category Relations directly and neither
creates nor infers Formations.

As the first commissioned taxonomy, Codex authored 11 Chess Law category roots on behalf
of Zach: Chess Laws; Picking & Interaction; Movement; Pawn Movement; Piece Movement;
Castling; Capture; Check & King Safety; Promotion; Turn State; and Draw & Game Conclusion.
Ten `subcategory-of` Relations form the acyclic hierarchy and 79 `instance-of` Relations
place all 69 Chess Laws, including intentional cross-membership. The category author is
recorded as First Mover `codex-gpt5`, acting on behalf of Zach; the Laws retain their
existing `grok-4.6` authorship.

The graph intentionally does not infer edges from shared names, properties, or events. This extension preserves Zach's request for graphical understanding while keeping the picture faithful to Earthcall's relation ontology: a visual connection means a Relation exists in the world.

## Verification

Verified on 2026-09-07:

- `./build/chess_app_test` completed `ALL OK`; the category roots, hierarchy edge, and
  Law membership survived load, and all 69 authored Chess Laws were categorized. Existing
  movement, capture, illegal-path, and distant-check scenarios remained green.
- `cmake --build build --target earthcall_webgpu chess_app_test -j8` compiled and linked
  both the WebGPU application and focused test after the final UI/search changes.
- `jq empty` accepted `chess_app.json`, `chess_app.ecform`, and the Chess Zone identity;
  all three carry 79 memberships, while the two world saves carry the 11 category roots.

The visual layout and interaction await a Person in the running application; the required
protocol is in the Person Verification List.

Recorded by Codex, session `01a07a4c-2b09-7f62-b975-3a23084ddeaf`, 2026-09-07 00:44 PDT.
