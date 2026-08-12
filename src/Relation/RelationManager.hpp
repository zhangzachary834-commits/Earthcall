#pragma once

#include <vector>
#include <string>
#include <memory>
#include <ctime>
#include "Relation.hpp"
#include "json.hpp"
#include "Singularity/Core/EventBus.hpp"

// Forward declaration for the event
// struct RelationCreatedEvent;
struct RelationCreatedEvent {
    std::shared_ptr<const Relation> relation;
    std::time_t timestamp;

    explicit RelationCreatedEvent(std::shared_ptr<const Relation> r)
        : relation(std::move(r)), timestamp(std::time(nullptr)) {}
};

// Centralized container/utility class for working with collections of Relation
// objects. This abstraction makes it easy to add/remove/query relations,
// as well as serialize the entire relation graph to JSON for persistence.
class RelationManager {
public:
    // Add a new relation to the set (no duplicate checks for now).
    // void add(const Relation& r);
    void add(const std::shared_ptr<Relation>& r);

    // Remove a relation; returns true if a matching relation was found and
    // erased.
    // bool remove(const Relation& r);
    bool remove(const std::shared_ptr<Relation>& r);

    // Remove all relations connecting the two entities. If `type` is not
    // empty, only relations of that type are removed. Returns true if at
    // least one relation was deleted.
    bool removeBetween(const std::string& a, const std::string& b, const std::string& type = "");

    // Query helpers ------------------------------------------------------
    // std::vector<Relation> getRelationsOf(const std::string& entity) const;
    // std::vector<Relation> getRelationsBetween(const std::string& a, const std::string& b) const;
    std::vector<std::shared_ptr<Relation>> getRelationsOf(const std::string& entity) const;
    std::vector<std::shared_ptr<Relation>> getRelationsBetween(const std::string& a, const std::string& b) const;
    std::vector<std::shared_ptr<Relation>> getRelationsOfType(const std::string& type) const;

    // Semantic Traversal Utilities (Logos Phase 4)
    // Semantic Traversal Utilities (Logos Phase 4)
    std::vector<std::string> findAdjacentEntities(const std::string& entityId, const std::string& relationType = "") const;

    // Checks if adding a directed relation from start to target of the given type would form a cycle
    bool wouldFormCycle(const std::string& start, const std::string& target, const std::string& relationType) const;

    // (De)Serialization --------------------------------------------------
    nlohmann::json toJson() const;
    void loadFromJson(const nlohmann::json& j);

    // Access underlying storage (read-only)
    // const std::vector<Relation>& getAll() const { return relations; }
    const std::vector<std::shared_ptr<Relation>>& getAll() const { return relations; }

private:
    std::vector<std::shared_ptr<Relation>> relations;
}; 
