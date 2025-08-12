#pragma once

#include <vector>
#include <string>
#include "Relation.hpp"
#include "json.hpp"
#include "Core/EventBus.hpp"

// Forward declaration for the event
struct RelationCreatedEvent;

// Centralized container/utility class for working with collections of Relation
// objects. This abstraction makes it easy to add/remove/query relations,
// as well as serialize the entire relation graph to JSON for persistence.
class RelationManager {
public:
    // Add a new relation to the set (no duplicate checks for now).
    void add(const Relation& r);

    // Remove a relation; returns true if a matching relation was found and
    // erased.
    bool remove(const Relation& r);

    // Remove all relations connecting the two entities. If `type` is not
    // empty, only relations of that type are removed. Returns true if at
    // least one relation was deleted.
    bool removeBetween(const std::string& a, const std::string& b, const std::string& type = "");

    // Query helpers ------------------------------------------------------
    std::vector<Relation> getRelationsOf(const std::string& entity) const;
    std::vector<Relation> getRelationsBetween(const std::string& a, const std::string& b) const;

    // (De)Serialization --------------------------------------------------
    nlohmann::json toJson() const;
    void loadFromJson(const nlohmann::json& j);

    // Access underlying storage (read-only)
    const std::vector<Relation>& getAll() const { return relations; }

private:
    std::vector<Relation> relations;
}; 