# Integrating Physics, Collision, and Second-Nature Law Authoring

**Status:** Architectural synthesis; the demonstration-to-Law workflow below is **proposed**, not implemented.
**Session Context:** Jules / default / 13820445096022914992. Audited and clarified by Sun Squid Intake III, 2026-10-23.

## Existing boundaries

[Physics and Collision](../architecture/events/PHYSICS_AND_COLLISION.md) describes fixed-timestep contact detection, `contact-began`/`contact-ended` EventBus transitions, and contact information represented in a Zone's relation graph. A contact event is an observation about physical interaction, **not** consent to create a Law, proof of an authored Relation's meaning, or a durable historical Moment.

[Second-Nature Law Authoring](../architecture/law/SECOND_NATURE_LAW_AUTHORING.md) already provides a narrower implemented seam: authored Forge instruments choose an ordinary Law prototype, bind explicit parameters such as `$TARGET`, publish `law-authoring-instrument-invoked`, and derive a new Law through `SingularSetToSetCreation::derive`. The source document explicitly says the experience still feels like preset stamping; a general demonstration-based composer has **not** landed.

## Proposed integration: demonstration as a draft, not an automatic Law

A Person could demonstrate an interaction (for example, an apple touching a pressure plate), select the observed `contact-began` fact as a *candidate condition*, and explicitly author the desired consequence (such as opening a door). A future instrument could present the referents, condition, action, jurisdiction, and reach for inspection before the Person authorizes derivation. Merely witnessing contact must never silently install an executable Law.

The event is **not** itself a prototype Singular, and its observation is **not** automatically `branched-from` provenance. If a Law is derived, provenance attaches to the ordinary prototype Singular used by the universal creation operation. Preserve real Person authorship, concrete Singular kind, authored Law text, explicit bindings, and existing governance; do not introduce a physics-specific creation opcode, new Person-like entity, or hidden behavior.

A credible implementation witness would replay contact edge transitions, show that observation alone creates **zero** Laws, then exercise an explicitly authorized Forge derivation and verify the resulting Law's condition/action text, author, target binding, and persistence. None of that end-to-end demonstration witness is established by this addendum.

## Architectural consequence

Physics can provide **evidence for authoring**, while Persons author meaning. The fixed-timestep substrate and the Law authoring interface may cooperate without treating collision mechanics as sovereign ontology.
