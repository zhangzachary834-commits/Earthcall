# Foreign Databases and First Mover Authoring

**Status:** Conceptual interrelation.

## Interrelated Documents
* `../Integration/FOREIGN_DATABASES.md`
* `../law/FIRST_MOVER_AUTHORING.md`

## Thoughts on Integration and Interrelation
In Earthcall, external relational databases like SQLite are explicitly forbidden from acting as the internal ontological state store; they can only influence the world state by assuming the role of a First Mover, such as by creating JSON `.ecsave` data.
