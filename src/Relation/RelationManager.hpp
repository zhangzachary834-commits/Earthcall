#pragma once

#include <vector>
#include <string>
#include <memory>
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

private:
    std::vector<std::shared_ptr<Relation>> relations;
};
