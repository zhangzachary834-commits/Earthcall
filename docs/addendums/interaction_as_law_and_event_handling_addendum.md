# Addendum: Integrating Interaction-as-Law and Event Handling Architecture

## The seam between sensing and meaning

[Interaction-as-Law](../architecture/law/INTERACTION_AS_LAW.md) separates **Sense**, which remains the engine's first-mover responsibility, from **Decide** and **Act**, which are expressed by Laws. A control is a being plus a Law that gives interaction with that being its meaning; the input channel must not hardcode a separate ontology of buttons.

### InteractionChannel publishes semantic edges

The `interaction-channel` is stepped from `Engine::update`. It records registered input levels (such as `hoveredId`, `pressedId`, and `focusedId`) and publishes transition events such as `object-hover-entered` and `object-clicked`. A click requires press and release on the same being within the click-slop threshold. Events name the affected Singular and, where applicable, the pointing Person; no known Person means no published interaction edge. Laws may respond with `Activation::OnEvent` or read levels with `Activation::WhileTrue`.

### EventBus delivery is not EventHandler governance

[Event Bus vs Event Handler](../architecture/events/EVENT_BUS_VS_EVENT_HANDLER.md) distinguishes the `EventBus`, which distributes events, from the optional `EventHandler` convenience layer, which organizes named callback registrations. The source explicitly permits direct `EventBus` subscriptions. **Interaction sensing does not depend on a centralized `EventHandler`**, and registering a callback there does not itself convert hardware input into a Rete fact, authorize a Law, or determine what a control means.

The architectural boundary is therefore: **engine senses and publishes; EventBus distributes; Laws interpret and act**. An `EventHandler` may organize imperative subscribers without becoming a second source of semantic authority. This is an architectural account, not a claim that every proposed authored control or manual UI validation has shipped.

---

**Interrelated Documents:**
* [Interaction-as-Law](../architecture/law/INTERACTION_AS_LAW.md)
* [Event Bus vs Event Handler](../architecture/events/EVENT_BUS_VS_EVENT_HANDLER.md)
