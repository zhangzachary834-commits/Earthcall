# Property Binding Custody and Ourverse Convening

**How cross-Zone property custody and alias topology map onto the gathering Zone and filament Relations defined in the Ourverse, enabling ecumenical property bindings across jurisdictions.**

## 1. The Challenge of Cross-Zone Property Alias Topology

The `PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md` doctrine outlines a requirement for **alias topology**: if two distinct Properties share a value cell before a save, they must still share that cell after a load.

When both Properties reside within the same Zone (e.g., a Person's Home), local serialization handles this trivially. However, Earthcall's distributed nature means two Persons from different Homes might author a shared binding. For instance, Person A's lantern color might be bound to Person B's ambient light intensity.

This introduces the custody problem: where does the shared value cell reside?
- If it resides in A's Home, B loses the binding when A is offline.
- If it resides in B's Home, A loses the binding when B is offline.
- If they both keep local copies, synchronization desyncs and the unified alias topology is broken.

## 2. The Gathering Zone as the Custodial Substrate

The `OURVERSE.md` doctrine defines the Ourverse not as a physics bag, but as an ordering principle that provides a `gathering Zone`. This gathering Zone is explicitly unowned and serves as the nexus for cross-Zone filament Relations.

This gathering Zone provides the precise architectural solution for cross-Zone property custody. When Person A and Person B author a shared property binding that spans their respective Zones, the authoritative value cell is custodied within the gathering Zone of the Ourverse they mutually convene toward.

## 3. Filaments as Binding Conduits

The intersection functions as follows:

1. **The Filament Relation:** Person A's Zone and Person B's Zone both possess an undirected `filament` Relation connecting them to the shared gathering Zone.
2. **Custodial Transfer:** When the cross-Zone property binding is established, the `TransferPolicy` evaluates the mutual consent of A and B. Upon approval, the authoritative cell for the shared property is allocated within the gathering Zone.
3. **Alias Pointers:** The local Properties in A's Zone and B's Zone become alias pointers that resolve through their respective filaments into the gathering Zone.
4. **Change Fan-out:** When Person A mutates the property (e.g., changes the lantern color), the mutation is routed through the filament to the gathering Zone cell. The gathering Zone then fans out the semantic change notification to all dependent alias paths, including Person B's ambient light.

## 4. Ecumenical Convening and Persistence

Because the gathering Zone is independent of any single Person's Home, the alias topology survives the disconnection of either party. If Person B goes offline, the cell remains custodied in the gathering Zone. When Person B reconnects, their local alias pointer re-resolves via the filament, instantly syncing with the current state of the cell.

This fulfills the requirement in `PROPERTY_STORAGE_AND_ONTOMATH_BINDING.md` that custody connects "Zone ↔ bearer, qualified by PropertyPath", without hardcoding a C++ custodian enum. The custody arrangement is an authored structural consequence of the `convenes-toward` and `filament` Relations defined in the Ourverse.

By mapping property bindings onto the Ourverse's gathering mechanics, Earthcall allows deeply interconnected, multi-author state to persist without collapsing into centralized, unowned global variables or brittle peer-to-peer syncing. The shared property literally resides in the shared space between them.