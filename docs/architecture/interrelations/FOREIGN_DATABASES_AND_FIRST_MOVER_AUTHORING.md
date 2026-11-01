# Foreign Databases and First Mover Authoring

## Interrelated Documents
- `docs/architecture/Integration/FOREIGN_DATABASES.md`
- `docs/architecture/law/FIRST_MOVER_AUTHORING.md`

## The Interrelation

`FOREIGN_DATABASES.md` lays out the strict boundary between Earthcall's legible internal state and external relational databases, explicitly forbidding SQLite from acting as the ontological data store.

`FIRST_MOVER_AUTHORING.md` describes how external agents or scripts (First Movers) can interact with and construct the Earthcall ontology by creating JSON `.ecsave` data and directly asserting properties and Relations.

These two concepts interrelate because the only valid way for a foreign relational database (like SQLite) to influence Earthcall's world state is by assuming the role of a First Mover. A database is not permitted to secretly hold back-end state. Instead, when data from an external database enters Earthcall, it must be translated by an intermediate process (the First Mover script) from rigid rows and columns into Earthcall's native vocabulary of beings, properties, and relationships.

## Thoughts on Integration and Interrelation

When viewing database ingestion through the lens of First Mover Authoring, it ceases to be a simple data-sync problem and becomes an act of cross-boundary authorship. The python script or side-car connecting to the SQLite database is not just a connector; it is an active author translating a foreign schema into a native ontology.

This paradigm ensures that all state within Earthcall remains legible and manipulable. Since the foreign data must be explicitly mapped into Properties and Relations upon entry, the engine's Rete network (which monitors for changes) and Law ecosystem (which reacts to facts) can engage with the ingested data flawlessly. The external database remains outside the system's awareness, providing computational heavy-lifting offline, while its conclusions are brought into the light of the Earthcall simulation as fully-fledged, legible assertions authored by a First Mover.
