# Addendum: Interaction as Law, Spatial UI, and Mathematical Geometry

*(Original synthesis: Gemini / Jules; source-verified corrections: Sun Squid Intake III, 2026-10-23)*

## Architectural direction versus implemented mechanism

Earthcall's `docs/architecture/law/INTERACTION_AS_LAW.md` defines a control as a spatial being plus an authored Law responding to sensed interaction. This is a **design rule for authored controls**, not a claim that all existing interfaces have already become spatial Objects. The lived `earthcall_webgpu` app still has native Law Graph and Creator Console windows; see `.Jules/palette.md`.

1. **Law-governed interaction.** The `InteractionChannel` senses pointer, wheel, and keyboard activity; `ControlPatterns` contains first-mover archetype factories. Authored `Object`, `Law`, `ConditionModel`, `ActionModel`, and `Formation` are the intended building blocks for controls rather than introducing a new domain-level `Button` or `Widget` kind. This does not mean every current ImGui control is already a Law-authored spatial being.

2. **OntoMath and hit-test correspondence.** The intended mathematical substrate can describe authored visual forms and transformations. However, a guarantee that every nonlinear warp automatically yields a **perfectly matching** interactive hit area is not established by the cited Interaction-as-Law implementation. A unified visual/picking expression requires an explicit lowering contract and executable witnesses for the same transformed boundary on both paths. Until those exist, distinguish the proposed correspondence from current hit testing; do not assert that all UI geometry already runs as one native GPU AST.

3. **Transparent authoring.** The No Black Box principle calls for visible property paths, Law effects, and refusal reasons. Palette's legitimate target is the native Law Graph, Creator Console, and Law Line, not the abandoned root `web_ui/` fossil. Any illustrative syntax (such as `@[id].enabled := false`) must be checked against the actual parser before being presented as executable Law Line syntax.

**Witness boundary:** A headless interaction-channel test proves the sensing path, not native-window usability, pixel/picking parity, or end-to-end Person-authored control behavior. Those need their own tests and, where appropriate, a manual app witness.
