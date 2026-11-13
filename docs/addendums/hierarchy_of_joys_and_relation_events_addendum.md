# Integrating Hierarchy of Joys and Relation Events

**Status:** Conceptual integration; the event publication exists, but no automatic Hierarchy-of-Joys Law reaction is established here.
**Connected documents:** [Hierarchy of Joys](../architecture/ontology/HIERARCHY_OF_JOYS.md) and [Relation Event System](../architecture/events/RELATION_EVENT_SYSTEM.md).

## Existing structure

A Hierarchy of Joys is a rooted Formation of Lexemes, tagged `hierarchy-of-joys`, ordered by directed `grounds` Relations. The root and the edge direction determine rank; the hierarchy is not an opaque Person string or a special C++ kind.

`Formation::addRelation` validates a candidate and, when admitted, delegates to `RelationManager::add`. When that manager **inserts a new Relation**, it publishes a C++ `RelationCreatedEvent` containing a pointer to the Relation and a timestamp, and separately publishes a string-typed ECA `relation-formed` event with the Relation as subject. The ECA echo is the current Law-facing trigger. Duplicate additions and refused edges do not publish a new-relation event; `Formation::retainRelation` uses a different path and does not provide this publication guarantee.

## Proposed authored bridge

An authored Law could react to `relation-formed`, inspect the Relation's type and endpoints, and establish that the Relation belongs to the particular Person's `hierarchy-of-joys` Formation before deriving a new consequence. **The event itself does not carry a Formation tag or prove membership, ownership, consent, or authority.** A `grounds` edge elsewhere in the world is not automatically a change to this Person's telos.

Changes to roots, removal or rebinding of edges, and reordering of an existing hierarchy are not covered merely by listening for newly inserted Relations. Those operations need their own explicit, witnessed observation contract before anyone can claim complete teleological change tracking.

Any capability grant or Zone response is a possible authored Law, **not** an existing automatic behavior. It must respect the ordinary Law/TransferPolicy and Person-consent boundaries rather than treating a preference event as blanket permission.
