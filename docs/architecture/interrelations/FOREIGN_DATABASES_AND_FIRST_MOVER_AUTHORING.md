# Foreign Databases and First Mover Authoring

## Source documents
- [Foreign Databases](../Integration/FOREIGN_DATABASES.md)
- [First Mover Authoring](../law/FIRST_MOVER_AUTHORING.md)

## The boundary

SQLite and other relational stores may be **foreign data sources, scratchpads, query services, or telemetry sinks**, but must not become the hidden authoritative store of Earthcall's internal ontology. The authored world remains legible as Singulars, Relations, Formations, properties, and Laws.

The integration specification names **four distinct valid paths**. They must not be collapsed into a rule that every foreign database is itself a First Mover:

1. **Live ingestion sidecar:** a process translates rows into native creation/property/Relation operations and submits them through an authorized interface. The *process* performs the boundary crossing; the database does not thereby become an ontological being.
2. **Offline world generation:** a human or AI First Mover writes an Earthcall-compatible world serialization from external data. JSON is a legible authoring representation; `.ecsave` is not simply a JSON file with a different extension.
3. **Foreign modality channel:** an in-world Law can query an external service through registered channel input/output properties without importing its entire database as native world state.
4. **Outbound audit/telemetry:** a foreign store may receive copies of events for analysis without becoming the authoritative world graph.

## Authority and provenance

First Mover serialization injection is a privileged authorship boundary, **not** an automatic exemption from all checks. [First Mover Authoring](../law/FIRST_MOVER_AUTHORING.md) §8 documents the built register, scoped grants, quarantine, and save-path enforcement **while a FirstMoverSession is active**; it also distinguishes this from engine/in-world saves and notes remaining attestation work. An adapter must not invent Person standing, authority, or provenance merely because it can translate a foreign schema.

A useful integration review asks separately: **where is foreign computation performed; who authorizes its effect; which native facts are actually materialized; and which information remains external?** Native graph queries see only materialized native facts; Laws may also read registered foreign-channel outputs without turning them into Relations. A channel's external answer is not secretly an authored Relation until an authorized Law or other explicit operation makes it one.
