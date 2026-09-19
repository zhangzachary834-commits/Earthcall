# Basic 2D Button

**Task:** Implement a basic 2D button zone where it is literally just an authored 2D shape with authored Laws that give it the behavior and functionality of a button that does something visible when clicked.

**Status:** Done and verified via `authored_save_lint_probe.py` (2026-09-04).

## Details
- Authored a `Basic2DButtonZone` zone save file and `basic_2d_button.json` session file.
- Used `ShapeKind::Shape2D` (12) for the visual representation.
- Added a `law-button-move` Law that acts on the `object-clicked` event.
- The Law adds `20.0` to the object's `x2D` property when clicked, creating a highly visible response without hardcoding UI logic in C++.
- Provenance and author identities correctly established as `Antigravity` in the categories section.
