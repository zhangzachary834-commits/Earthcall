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
