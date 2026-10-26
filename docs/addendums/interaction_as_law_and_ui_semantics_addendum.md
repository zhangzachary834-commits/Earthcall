# Addendum: Integrating Interaction as Law and UI Semantic Projection

*(Model: Gemini 2.0 Pro, Harness: Jules, Session ID: 948281034)*

## The Rejection of Traditional UI State

Earthcall's architecture fundamentally rejects traditional bespoke UI classes and event handlers in favor of "Interaction as Law." Interactive elements are treated as spatial Objects, and their behaviors are governed entirely by authored Laws rather than hardcoded C++ widget logic.

This paradigm ensures that the UI is not a separate, privileged domain but part of the shared, authorable ontology.

## The Interrelation with the No Black Box Principle

When creating Person-facing interfaces (such as `LawGraphWindow.cpp` or tooltip mechanisms), there is often a tension between presenting a clean, conventional UX and accurately reflecting the underlying engine semantics.

The "No Black Box" principle dictates that Persons must be able to see and author the exact boundaries and mechanics of the system. Therefore, generic UX polish that abstracts away Earthcall's true operations is considered harmful, as it obscures the truth of how the system functions.

## The Semantic Projection Solution

To resolve this tension, UX enhancements must explicitly project Earthcall's true semantic operations.

For example, when displaying a tooltip to explain an action, the interface should not use a vague, generic label like "Disable Object." Instead, it must expose the exact Earthcall syntax, using bracketed property paths such as `@[id].enabled := false`.

By displaying the literal `PropertyPath` that the system uses to execute the change, the UI acts as a true semantic projection. It aids user understanding by teaching them the actual language of the engine (aiding natural language law authoring) rather than manufacturing a fake layer of UI state that they must later unlearn.
