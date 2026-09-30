# Soul Formation and Saving Context

The `Soul` in Earthcall is represented as a `Formation` that tracks the course of a Person's life and interactions across the digital substrate. This concept intrinsically binds with the `SaveContext` and `SaveSystem` systems.

While `SaveSystem` provides the persistence mechanism (e.g. `SaveSystem::saveRoot()` or writing via JSON)—ensuring that per-Singular histories survive process shutdown—the `Soul` Formation provides the semantic, relational structure over those saves. The ledger of a Person's actions, relationships formed, and zones visited is not merely a data dump in a save file, but rather the very nodes that compose the Soul Formation when loaded back into active memory.

Thus, `SaveContext` coordinates the physical storage substrate mapping, and the Soul Formation is the teleological graph instantiated from it upon load.
