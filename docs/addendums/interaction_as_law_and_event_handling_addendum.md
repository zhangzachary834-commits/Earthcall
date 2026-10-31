# Addendum: Integrating Interaction-as-Law and Event Handling Architecture

## Thoughts on Integration and Interrelation

When examining the mechanisms of user interaction in Earthcall through the lenses of [Interaction-as-Law](../architecture/law/INTERACTION_AS_LAW.md) and [Event Bus vs Event Handler](../architecture/events/EVENT_BUS_VS_EVENT_HANDLER.md), a unified philosophy of ontological UI emerges.

### From C++ Callbacks to Authored Laws
The [Interaction-as-Law](../architecture/law/INTERACTION_AS_LAW.md) document dictates that standard user interfaces (like buttons or tuners) must not exist as hardcoded black boxes, but rather as authored Laws reacting to pointer events. This represents a shift from imperative C++ callbacks to declarative, relational rule evaluation.

### The Event Handler as the Mechanism
The [Event Bus vs Event Handler](../architecture/events/EVENT_BUS_VS_EVENT_HANDLER.md) document describes how events are routed and managed. While Interaction-as-Law moves the *logic* of UI out of C++, the *sensing* of these interactions (e.g., `object-clicked`, `object-hover-entered`) relies on the centralized `EventHandler`. By registering core channel events via the `EventHandler`, the engine ensures that pointer actions are cleanly captured and translated into the semantic facts required by the Rete network to evaluate interaction Laws.

This interrelation ensures that the engine efficiently manages the delivery of hardware inputs, while completely deferring the meaning and reaction of those inputs to the authored, continuous conversation of Earthcall's Laws.

---

**Interrelated Documents:**
* [Interaction-as-Law](../architecture/law/INTERACTION_AS_LAW.md)
* [Event Bus vs Event Handler](../architecture/events/EVENT_BUS_VS_EVENT_HANDLER.md)
