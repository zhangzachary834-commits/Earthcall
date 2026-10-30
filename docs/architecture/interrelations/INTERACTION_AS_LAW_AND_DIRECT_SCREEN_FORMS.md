# Interaction-as-Law and Direct Screen Forms

**Status:** Architectural cross-check — two implemented seams; their direct-screen interaction bridge is **not yet established**.

**Sources:** [Interaction as Law](../law/INTERACTION_AS_LAW.md), [OntoMath](../mathematics/ONTOMATH_FRAMEWORK.md), [Direct Screen forms](../../Agenda/Tasks/Specific%20Tasks/Rendering%20and%20OntoMath/Direct_Screen_Forms/Direct_Screen_Forms.md), [native verification](../../audits/DIRECT_SCREEN_FORMS_NATIVE_VERIFICATION_2026-10-04.md), [ScreenChannel](../../../src/Singularity/Screen/ScreenChannel.hpp), [InteractionChannel](../../../src/Singularity/Input/Interaction/InteractionChannel.hpp).

## What exists

- **Interaction-as-Law:** `InteractionChannel` senses pointer gestures and picks spatial `Object` bearers; it publishes events such as `object-clicked` that authored Laws can interpret. A control need not be a special `Button` or `Widget` kind.
- **Direct Screen manifestation:** `ScreenChannel::manifestOutput` renders authored typed color/opacity fields at physical framebuffer sample centers. Their source may be any addressable Singular, without requiring an `Object`, mesh, or texture as the authored carrier. The production renderer calls this path.
- **Screen observation:** `ScreenChannel::senseOutput` can sample selected pixels of the completed viewport on an explicit request. This is an observation contract, **not** automatic pointer picking or a screen-region click dispatcher.

## The missing interrelation

A mathematical region drawn directly onto the screen is **not thereby an interactable target** of the current spatial-Object picking channel. In particular, rendering a field and publishing `object-clicked` for a spatial `Object` do not prove that a pointer intersection with a screen-only region generates a Law event. Nor does native pixel-parity verification establish hit-test parity.

**Proposed bridge, not a shipped guarantee:** preserve the authored region selector as meaning; convert pointer coordinates from window points to physical framebuffer samples with explicit viewport bounds; evaluate membership against that selector; publish a provenance-bearing observation/event through the existing Interaction/Law machinery. Specify overlap, occlusion, focus, event ordering, and UI-capture behavior before declaring it complete. Do not invent a bespoke widget type, duplicate event system, or separate source of region truth.

This synthesis therefore joins two real capabilities and identifies their remaining boundary. It does **not** claim a fully interactive direct-screen UI is already implemented, or that compatibility HUD/ImGui surfaces have vanished.
