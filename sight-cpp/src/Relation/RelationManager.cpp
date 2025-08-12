#include "RelationManager.hpp"
#include <algorithm>

// Event structure for when a new Relation is created
struct RelationCreatedEvent {
    const Relation& relation;
    std::time_t timestamp;
    
    RelationCreatedEvent(const Relation& r) 
        : relation(r), timestamp(std::time(nullptr)) {}
};

void RelationManager::add(const Relation& r) {
    // Check if an equivalent relation already exists (same type & endpoints)
    auto it = std::find_if(relations.begin(), relations.end(), [&](const Relation& other) {
        bool sameType = other.type == r.type;
        bool sameDir  = other.directed == r.directed;

        if (!sameType || !sameDir) return false;

        if (r.directed) {
            // Directed: order matters
            return other.entityA == r.entityA && other.entityB == r.entityB;
        }
        // Undirected: order independent
        bool matchForward  = other.entityA == r.entityA && other.entityB == r.entityB;
        bool matchBackward = other.entityA == r.entityB && other.entityB == r.entityA;
        return matchForward || matchBackward;
    });

    if (it != relations.end()) {
        // Existing relation – append an event capturing this interaction
        RelationEvent ev;
        ev.timestamp    = std::time(nullptr);
        ev.description  = r.type;
        ev.deltaWeight  = r.weight;
        it->addEvent(ev);

        // Optional: update aggregate weight (could use running average, etc.)
        it->weight += r.weight;
    } else {
        // New relation – copy and create initial event
        Relation newRel = r;
        RelationEvent ev{std::time(nullptr), r.type, r.weight};
        newRel.events.push_back(ev);
        relations.push_back(std::move(newRel));
        
        // Trigger event for new relation creation
        RelationCreatedEvent event(newRel);
        Core::EventBus::instance().publish(event);
    }
}

bool RelationManager::remove(const Relation& r) {
    auto it = std::find_if(relations.begin(), relations.end(), [&](const Relation& other) {
        return other.type == r.type &&
               other.entityA == r.entityA &&
               other.entityB == r.entityB &&
               other.directed == r.directed;
    });
    if (it != relations.end()) {
        relations.erase(it);
        return true;
    }
    return false;
}

bool RelationManager::removeBetween(const std::string& a, const std::string& b, const std::string& type) {
    auto oldSize = relations.size();
    relations.erase(std::remove_if(relations.begin(), relations.end(), [&](const Relation& r) {
        bool matchesEntities = r.isBetween(a, b);
        bool matchesType = type.empty() || r.type == type;
        return matchesEntities && matchesType;
    }), relations.end());
    return relations.size() != oldSize;
}

std::vector<Relation> RelationManager::getRelationsOf(const std::string& entity) const {
    std::vector<Relation> result;
    for (const auto& r : relations) {
        if (r.involves(entity)) result.push_back(r);
    }
    return result;
}

std::vector<Relation> RelationManager::getRelationsBetween(const std::string& a, const std::string& b) const {
    std::vector<Relation> result;
    for (const auto& r : relations) {
        if (r.isBetween(a, b)) result.push_back(r);
    }
    return result;
}

nlohmann::json RelationManager::toJson() const {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& r : relations) {
        arr.push_back(r.toJson());
    }
    return arr;
}

void RelationManager::loadFromJson(const nlohmann::json& j) {
    relations.clear();
    if (!j.is_array()) return;
    for (const auto& item : j) {
        relations.push_back(Relation::fromJson(item));
    }
} 