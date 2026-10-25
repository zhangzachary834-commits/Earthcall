# Addendum: Foreign Integration and Governable Behavior Reconstruction

*Architectural synthesis; the behavior-reconstruction mechanisms below are a proposal, not a claim of shipped runtime behavior.*

## A foreign surface is not yet a native law

The [Integration Guide](../architecture/Integration/INTEGRATION_GUIDE.md) describes APIs and overlays through which external Web or desktop applications may exchange data with Earthcall's creative tools. This is a boundary for communication and representation, not automatic evidence that Earthcall understands an application's hidden semantics.

The [Behavior Reconstruction](../architecture/Integration/BEHAVIOR_RECONSTRUCTION.md) document explicitly calls its work **Phase 3** and lists proposed components. Its plan is to capture accessibility/state transitions over time (`dt`), feed observations to an external Python classifier, and expose classifier parameters through an `InferenceLawBridge`. The plan also sketches a metalaw-governed path for proposing K4 Laws from observations. Do not describe that proposal as a completed end-to-end implementation.

## Authority, evidence, and the No Black Box constraint

An inferred behavior is a hypothesis derived from external observations; it is not an authored Law merely because a model produced it. The proposed bridge must make its assumptions, model settings, confidence, and provenance inspectable and governable. A Person retains authority to accept, reject, or author the resulting Law. The external classifier is not an ontological authority or a substitute for native `Formation` and `Relation` semantics.

The architectural connection is therefore **transport → observations → inspectable inference → human-authorized law**, with the implementation and verification gates explicitly separated.

## Linked references

- [Integration Guide](../architecture/Integration/INTEGRATION_GUIDE.md)
- [Behavior Reconstruction (Phase 3 proposal)](../architecture/Integration/BEHAVIOR_RECONSTRUCTION.md)
