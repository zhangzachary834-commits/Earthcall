# Integrating Structural Revisions and Transient Fact Retraction

Origin: Jules architectural interrelation proposal; corrected against Earthcall canonical `59232506f781`.

## Structural revisions govern seeding

In `src/ZonesOfEarth/AuthorsOfLaw/Law.cpp`, the connected `LawManager::tick()` compares `Universe::instance().structuralRevision()` against `_lastSeededStructuralRevision`. Only a changed revision initiates the sweep over `Universe::instance().beings()` calling `seedStateFacts`. Within that operation, `_seededBeingPointers` avoids repeating initial seeding for a previously seen being. This reduces unnecessary full-world sweeps; it is not a claim that every property or relation change necessarily increments the structural revision. Separate dirty-property and relation-event handling are required for those changes.

## Retraction preserves state facts

`ReteNetwork::retractFirst(count)` consumes a prefix of the fact stream after an evaluation round. It retains state facts in that prefix, identifies non-state facts for removal, updates affected alpha/beta memories and the agenda, cleans dirty-fact references, and appends the unconsumed suffix. The current implementation constructs a `std::vector<FactPtr> new_facts` and finishes with `_facts = std::move(new_facts)`. **It does not contain a `_transientFactCount` O(1) shortcut or in-place compaction of the existing fact vector.** The fast return is for `count == 0`.

## Interrelation and performance boundary

The structural-revision guard limits how often broad seeding is attempted; prefix retraction independently keeps event facts from persisting after consumption while retaining state facts. These mechanisms operate at different stages of the Law/Rete lifecycle, and a revision bump does not itself call `retractFirst`. Any future compaction optimization must preserve fact ordering, state persistence, downstream node/agenda invalidation, and dirty-fact consistency, with measured witnesses rather than assumed allocation savings.
