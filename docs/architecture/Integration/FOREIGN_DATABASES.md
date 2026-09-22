# Foreign Databases and SQLite

**How relational databases like SQLite interface with Earthcall — and the strict boundary that keeps them out of the ontology.**

**Status:** Specified. Builds on `INTEGRATION_FRAMEWORK.md` and `NEW_KIND_FRAMEWORK.md`.
**Author:** Antigravity (Session 7f1015c5-3297-4f3f-b081-4a3edf95448d), 2026-08-24.

---

## 0. The Strict Boundary

Earthcall's state is serialized as `.ecsave` (MessagePack / FlatBuffers / Frontier) and `.json` (for legibility). This allows the substrate to remain legible and directly editable by Persons and AI First Movers. 

**SQLite must never be used to store Earthcall's internal ontological state.**
Pushing Earthcall's state into a relational database creates a black box, turning `Singular`s, `Relation`s, and `Formation`s into hidden rows accessible only via SQL queries. This violates the *No Black Box* refusal and retreats to nominalist infrastructure.

However, SQLite is highly effective when placed **outside** the ontology, acting as a **First Mover data source** or a **Foreign Process**. Below are the architecturally valid patterns for bridging foreign databases into Earthcall.

---

## 1. The Impedance Mismatch (Why SQLite fails as a core substrate)

SQLite forces data into a rigid, two-dimensional grid of Tables, Rows, and Columns requiring a predefined schema. Earthcall thinks as a rich Graph, relying on:
- **Nested property trees** on `Singular` beings.
- **First-class Relations** that possess their own event timelines, weights, and rules (rather than merely being integer foreign keys).
- **Recursive hierarchies** like `ConditionModel`s and `Formation`s.

Using SQLite for Earthcall's core state or macro-scale transmissions fails catastrophically for two reasons:

1. **Translation Loss:** Attempting to force Earthcall's rich 3D graph into a flat grid destroys data. A `Relation` loses its event history if flattened into an `attachment_id` column. A `Law`'s deeply nested AST (`ConditionModel`) cannot be natively stored in a table without either shredding it across dozens of slow-to-assemble tables or dumping it as an opaque blob (defeating the purpose of the database).
2. **Structural Constraint:** To save or sync a massive world, Earthcall would have to bottleneck its fluid memory graph by translating it into thousands of rigid `INSERT` and `UPDATE` statements. Furthermore, Earthcall relies on dynamic property creation (e.g., a First Mover attaching `@city.custom_weather_rule`). A strict SQLite schema would require an `ALTER TABLE` command for every new property, freezing the system's flexibility.

This is precisely why Earthcall's `.ecsave` relies on **MessagePack and FlatBuffers**. These formats naturally understand nested trees, dynamic dictionaries, and graphs—compressing the shape of memory directly to disk without imposing structural grid constraints.

---

## 2. The Python Sidecar (Data Ingestion)

When a massive real-world dataset (e.g., historical archives, GIS mappings, inventory systems) needs to enter Earthcall, a Python script (`backend-python/database_sync.py`) can sit atop an SQLite database.

**How it works:**
The sidecar queries SQLite, but it **never sends SQL rows to Earthcall**. Instead, it acts as a First Mover that translates the data into Earthcall's native vocabulary over a WebSocket.
- A row in a `Cities` table becomes a `Spawn` action for a `CityConcept`.
- A foreign key becomes a `Relation` (e.g., `type: "governed-by"`).
- A coordinates column becomes a property write to `@city.position`.

**Why it is valid:** Earthcall never knows SQLite exists. The engine only sees a First Mover asserting valid `Singular` properties and `Relation`s.

---

## 3. Offline First Mover (World Generation)

An AI agent or procedural generation tool can use SQLite as a private scratchpad to calculate complex world states, simulations, or relational graphs offline.

**How it works:**
Once the heavy lifting is complete within the SQL environment, the First Mover generates a standard Earthcall `saves/worlds/my_world.json` file.
- The SQLite database is discarded or kept exclusively as the First Mover's private memory.
- The generated world is pure, legible Earthcall serialization, fully adhering to `FIRST_MOVER_AUTHORING.md`.

---

## 4. A Foreign Modality Channel (`Singularity/Foreign/SqlAdapter`)

If an in-world Law genuinely needs to query an external corporate database in real-time without ingesting the entire dataset, a Modality Channel is used.

**How it works:**
A channel is built under `Singularity/Foreign/SqlAdapter`. 
- **The Constraint:** The channel must **not** define domain types (e.g., no `EmployeeRecord` structs). 
- It must have a stable identifier, such as `@sqlite-channel`. 
- An in-world Law writes to `@sqlite-channel.queryInput` and reads from `@sqlite-channel.queryOutput` or `@sqlite-channel.rowCount`.

**Why it is valid:** The channel strictly bridges input and output. The domain logic remains in-world, and the external database remains a distant sensor/actuator.

---

## 5. Offline Audit / Telemetry Sinks

Earthcall emits `noun-verbed` events (e.g., `relation-formed`, `jump-started`). If massive offline data-science analysis is required, a First Mover script can listen to the event bus and dump those events into an external SQLite database.

**Why it is valid:** SQLite acts as a downstream telemetry sink. Earthcall's internal graph remains the ground truth, but SQLite is used externally for what it does best: querying historical tabular data.

---

## 6. Anti-Patterns

| What it looks like | Why it is wrong | Cure |
|---|---|---|
| Modifying Earthcall to save worlds to `world.sqlite` | Violates First Mover legibility and the `Frontier` system. | Stick to JSON and `.ecsave`. |
| A channel defining `struct DatabaseRow` | Defining a domain noun inside the engine. | Map queries to generic properties or dynamic JSON payloads. |
| Loading SQL rows into a hidden array | State invisible to Laws and quantifiers. | Instantiate the rows as `Object`s or `Relation`s in the world graph. |
