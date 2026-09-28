# Msgpack Migration Root and Saving Context

**How the `.ecform` serialization strategy ensures historical data continuity and physical world presence.**

**Status:** Conceptual interrelation.
**Connected Systems:**
*   `SaveSystem` and `ZoneManager`

---

## The Interrelation

Earthcall's save data (`.ecform` and `.ecsave` files) is not simply dumped as raw strings. The data is wrapped in a `MigrationRoot` JSON object and then encoded to binary using `nlohmann::json::to_msgpack` before being written to disk, and carefully unpacked via `nlohmann::json::from_msgpack` during reads.

This mechanism directly serves the goal of preserving a Person's soul formation and saving context. The `MigrationRoot` acts as a durable envelope that preserves the continuous history of a world or a Person. If Earthcall's engine evolves, the unparsed raw ontology remains intact within the msgpack binary, meaning no historical meaning is lost to format rot. It ensures that the exact ontological state authored by a Person—even if it contains unrecognized legacy kinds—can be loaded, migrated, and represented back in the engine without discarding user truth.

This strict handling of binary-encoded migration roots guarantees that what is saved to disk accurately reflects the live execution context, bridging the gap between active simulation and dormant storage.
