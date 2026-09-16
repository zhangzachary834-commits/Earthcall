#pragma once

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>
#include "Relation.hpp"
#include "json.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Time/Moment/Moment.hpp"

struct RelationCreatedEvent {
    std::shared_ptr<const Relation> relation;
    Moment timestamp;

    explicit RelationCreatedEvent(std::shared_ptr<const Relation> r)
        : relation(std::move(r)), timestamp(Moment::now()) {}
};

// Centralized container/utility class for working with collections of Relation
// objects. This abstraction makes it easy to add/remove/query relations,
// as well as serialize the entire relation graph to JSON for persistence.
class RelationManager {
public:
    // ---------------------------------------------------------------------
    // A being can leave the world while relations still point at it.
    //
    // That is not an error — control_patterns_test does it deliberately
    // (`cats.remove(category.control.button)` with instance-of edges still in
    // the graph), and the engine's answer is supposed to be "the edge is
    // simply skipped." But a Relation holds its endpoints as raw pointers and
    // `aId()`/`bId()` call a VIRTUAL getIdentifier() through them, so reading
    // such an edge is `__cxa_pure_virtual` — an abort, from a query. Every
    // graph walk (`isBetween`, `involves`, the Related condition) hits it.
    //
    // So the graph is told. `forgetBeingEverywhere` drops the pointer and
    // keeps the NAME, which returns the relation to the same state a save
    // holds before its endpoints are resolved — a state every reader already
    // handles. The relation itself survives: an edge to a being that has left
    // is still an authorship no one revoked.
    //
    // Anchored on the MANAGER rather than on Relation itself, deliberately.
    // Relations are value-copied (fromJson returns one by value), so a
    // per-Relation registry misses copies and its static teardown races the
    // shared_ptrs that own them. Managers are few, stable, and own the
    // relations that are actually reachable.
    // ---------------------------------------------------------------------
    static void forgetBeingEverywhere(const Singular* being);

    RelationManager();
    RelationManager(const RelationManager& other);
    RelationManager(RelationManager&& other) noexcept;
    RelationManager& operator=(const RelationManager& other);
    RelationManager& operator=(RelationManager&& other) noexcept;
    ~RelationManager();

    void add(const std::shared_ptr<Relation>& r);

    bool remove(const std::shared_ptr<Relation>& r);

    bool removeBetween(const Singular& a, const Singular& b, const std::string& type = "");
    bool removeBetween(const std::string& a, const std::string& b, const std::string& type = "");

    // Drop every relation that still names a being that is about to leave.
    bool removeInvolving(const Singular* being);

    std::vector<std::shared_ptr<Relation>> getRelationsOf(const Singular& being) const;
    std::vector<std::shared_ptr<Relation>> getRelationsOf(const std::string& identifier) const;
    std::vector<std::shared_ptr<Relation>> getRelationsBetween(const Singular& a, const Singular& b) const;
    std::vector<std::shared_ptr<Relation>> getRelationsBetween(const std::string& a, const std::string& b) const;
    std::vector<std::shared_ptr<Relation>> getRelationsOfType(const std::string& type) const;

    std::vector<std::string> findAdjacentEntities(const std::string& entityId, const std::string& relationType = "") const;

    bool wouldFormCycle(const Singular* start, const Singular* target, const std::string& relationType) const;
    bool wouldFormCycle(const std::string& start, const std::string& target, const std::string& relationType) const;

    nlohmann::json toJson() const;
    void loadFromJson(const nlohmann::json& j, const RelationEndpointResolver& resolve = {});

    const std::vector<std::shared_ptr<Relation>>& getAll() const { return relations; }

    // ---------------------------------------------------------------------
    // Every relation that could involve this being — FORMATION_RETE.md §8 rung 4.
    //
    // The `Related` condition used to find a subject's edges by walking EVERY
    // relation in the world, once per candidate per tick. It is the dominant
    // scoping idiom in the tree (132 saved laws name `category.chess.piece`
    // through `instance-of`), and measured against an identical law reading a
    // plain property it cost 3.1x at 400 beings, the gap widening with the
    // world. This answers from an index instead, in O(degree).
    //
    // A CANDIDATE list, not a verdict: callers must still check which end the
    // being is and whether the edge's type and far end match. That is what lets
    // a stale entry cost a wasted check rather than a wrong answer.
    //
    // Keyed TWO ways, and both are load-bearing:
    //   by POINTER    — a bound endpoint; immune to the being being renamed.
    //   by IDENTIFIER — a bound endpoint's current name, or an UNBOUND or
    //                   FORGOTTEN endpoint's kept name. Stable Identifiers is a
    //                   non-negotiable: that name IS the being, and a Zone reload
    //                   recreates beings under the same names. Keying by pointer
    //                   alone would make every such edge invisible (the regression
    //                   related_identity_endpoint_test guards).
    // Because the identifier key is captured while the endpoint is still bound,
    // an edge forgotten WITHOUT this manager being told is still found again by
    // name. `out` is cleared first.
    // ---------------------------------------------------------------------
    void relationsInvolving(const Singular& being, std::vector<Relation*>& out) const;

private:
    std::vector<std::shared_ptr<Relation>> relations;

    // CACHE STAMP for the index above — private bookkeeping, not a change
    // signal. Nothing outside this class reads it. It is NOT a second
    // world-level revision beside Universe::structuralRevision (one was started
    // and reverted on 2026-09-14 at Zach's question); it only tells this
    // manager's own index that its own vector changed.
    //
    // COMPLETENESS, which is the whole risk: `relations` is private, so every
    // write is in RelationManager.cpp. Each of them calls touch() — the four
    // copy/move members, add, remove, both removeBetween, removeInvolving,
    // loadFromJson — and forgetBeingEverywhere touches every live manager.
    // IF YOU ADD A WRITE TO `relations`, CALL touch(), and add a section to
    // tests/law/relation_endpoint_index_test.cpp that goes through it. An index
    // that misses an insertion hides an edge from every law, silently.
    void touch() { ++_generation; }
public:
    // HOW MANY TIMES THIS GRAPH HAS CHANGED. Not a new change system — this is
    // the stamp the endpoint index already keys on, surfaced so a consumer can
    // ask "has the graph moved since I looked?" without walking it. Every write
    // to `relations` bumps it (see the note above), including the ones that
    // announce nothing on the EventBus: loadFromJson, copy and move assignment,
    // and forgetBeingEverywhere when an endpoint actually moves.
    std::size_t generation() const { return _generation; }
private:
    void rebuildEndpointIndex() const;
    std::size_t _generation = 0;
    mutable std::size_t _indexedGeneration = static_cast<std::size_t>(-1);
    mutable std::unordered_map<const Singular*, std::vector<Relation*>> _byEndpoint;
    mutable std::unordered_map<std::string, std::vector<Relation*>> _byIdentifier;
};
