# Foreign Databases and First Mover Authoring

**Status:** Conceptual interrelation; not a claim that every proposed adapter exists.

## Interrelated Documents
- [Foreign Databases](../Integration/FOREIGN_DATABASES.md)
- [First Mover Authoring](../law/FIRST_MOVER_AUTHORING.md)

## Boundary and connection
SQLite and other relational databases may be **external sources, private scratchpads, foreign modality channels, or telemetry sinks**, but must not become Earthcall's internal ontological state store. Their rows are not automatically Singulars, Relations, or Laws.

A First Mover may author valid native serialization (for example, a world JSON file), or a foreign process may translate external observations into qualified channel actions. The receiving Earthcall substrate must still recognize the authored entities, paths, and applicable authority boundaries. **Reading a database does not itself confer First Mover authority**, and generating a JSON world is not the same thing as generating a binary `.ecsave` file.

The foreign-database document sketches Python sidecars and SQL modality adapters as architectural patterns; do not report those examples as an already integrated, universal ingestion pipeline. First Mover save-path grants, scoping, and quarantine are described separately in the authoring specification, while its proposed named-section attestation remains unfinished.
