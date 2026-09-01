#include "RelationManager.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include <algorithm>

// Event structure for when a new Relation is created
/*struct RelationCreatedEvent {
    const Relation& relation;
    std::time_t timestamp;
    
    RelationCreatedEvent(const Relation& r) 
        : relation(r), timestamp(std::time(nullptr)) {}
};*/

/*void RelationManager::add(const Relation& r) {
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
        ev.deltaWeight  = r.getWeight();
        it->addEvent(ev);

        // Optional: update aggregate weight (could use running average, etc.)
        it->setWeight(it->getWeight() + r.getWeight());
    } else {
        // New relation – copy and create initial event
        Relation newRel = r;
        RelationEvent ev{std::time(nullptr), r.type, r.getWeight()};
        newRel.events.push_back(ev);
        relations.push_back(std::move(newRel));
        
        // Trigger event for new relation creation
        RelationCreatedEvent event(newRel);
        Core::EventBus::instance().publish(event);
    }
}*/
#include <unordered_set>
#include <iostream>

void RelationManager::add(const std::shared_ptr<Relation>& r) {
    if (!r) return;
    if (!r->hasEndpoints()) {
        std::cerr << "RelationManager::add - Rejecting relation '" << r->type
                  << "' with unbound Singular endpoints.\n";
        return;
    }

    if (r->type == "subcategory-of" && wouldFormCycle(r->a(), r->b(), "subcategory-of")) {
        std::cerr << "RelationManager::add - Rejecting cycle in subcategory-of: "
                  << r->aId() << " -> " << r->bId() << std::endl;
        return; // reject cyclic relation
    }

    const Relation& input = *r;
    // Check if an equivalent relation already exists (same type & endpoints)

    auto it = std::find_if(relations.begin(), relations.end(), [&](const std::shared_ptr<Relation>& otherPtr) {
        if (!otherPtr) return false;
        const Relation& other = *otherPtr;
        bool sameType = other.type == input.type;
        bool sameDir  = other.directed == input.directed;

        if (!sameType || !sameDir) return false;

        if (input.directed) {
            return other.a() == input.a() && other.b() == input.b();
        }
        bool matchForward  = other.a() == input.a() && other.b() == input.b();
        bool matchBackward = other.a() == input.b() && other.b() == input.a();
        return matchForward || matchBackward;
    });

    if (it != relations.end()) {
        // Existing relation – append an event capturing this interaction
        RelationEvent ev;
        ev.timestamp    = std::time(nullptr);
        ev.description  = input.type;
        ev.deltaWeight  = input.getWeight();
        (*it)->addEvent(ev);

        // Optional: update aggregate weight (could use running average, etc.)
        (*it)->setWeight((*it)->getWeight() + input.getWeight());

        // Re-binding an existing pair carries fresh attachment geometry: the
        // caller measured localOffset from the beings' current transforms, so
        // the incoming pose supersedes the stored one. Dropping it left the
        // stale offset silently in force.
        if (input.isAttachment()) {
            (*it)->attachment = input.attachment;
        }
    } else {
        // New relation – create initial event
        RelationEvent ev{std::time(nullptr), input.type, input.getWeight()};
        r->events.push_back(ev);
        relations.push_back(r);

        // Trigger event for new relation creation
        RelationCreatedEvent event(r);
        Core::EventBus::instance().publish(event);

        // ECA echo so LAWS can hear it (string-typed events are what the
        // Rete network binds): subject is the newborn relation — itself a
        // Singular whose endpoints laws can read.
        ECA::Event echo;
        echo.type = "relation-formed";
        echo.subject = r.get();
        echo.timestamp = std::time(nullptr);
        Core::EventBus::instance().publish(echo);
    }
}

/*bool RelationManager::remove(const Relation& r) {
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
}*/

bool RelationManager::remove(const std::shared_ptr<Relation>& r) {
    if (!r) return false;
    const Relation& target = *r;
    auto it = std::find_if(relations.begin(), relations.end(), [&](const std::shared_ptr<Relation>& otherPtr) {
        if (!otherPtr) return false;
        const Relation& other = *otherPtr;
        return other.type == target.type &&
               other.a() == target.a() &&
               other.b() == target.b() &&
               other.directed == target.directed;
    });
    if (it != relations.end()) {
        auto removed = *it;
        relations.erase(it);

        ECA::Event echo;
        echo.type = "relation-destroyed";
        echo.subject = removed.get();
        echo.timestamp = std::time(nullptr);
        Core::EventBus::instance().publish(echo);
        return true;
    }
    return false;
}

bool RelationManager::removeBetween(const Singular& a, const Singular& b, const std::string& type) {
    auto oldSize = relations.size();
    relations.erase(std::remove_if(relations.begin(), relations.end(), [&](const std::shared_ptr<Relation>& r) {
        if (!r) return false;
        bool matchesEntities = r->isBetween(a, b);
        bool matchesType = type.empty() || r->type == type;
        if (matchesEntities && matchesType) {
            ECA::Event echo;
            echo.type = "relation-destroyed";
            echo.subject = r.get();
            echo.timestamp = std::time(nullptr);
            Core::EventBus::instance().publish(echo);
            return true;
        }
        return false;
    }), relations.end());
    return relations.size() != oldSize;
}

bool RelationManager::removeBetween(const std::string& a, const std::string& b, const std::string& type) {
    auto oldSize = relations.size();
    relations.erase(std::remove_if(relations.begin(), relations.end(), [&](const std::shared_ptr<Relation>& r) {
        if (!r) return false;
        bool matchesEntities = r->isBetween(a, b);
        bool matchesType = type.empty() || r->type == type;
        if (matchesEntities && matchesType) {
            ECA::Event echo;
            echo.type = "relation-destroyed";
            echo.subject = r.get();
            echo.timestamp = std::time(nullptr);
            Core::EventBus::instance().publish(echo);
            return true;
        }
        return false;
    }), relations.end());
    return relations.size() != oldSize;
}

bool RelationManager::removeInvolving(const Singular* being) {
    if (!being) return false;
    auto oldSize = relations.size();
    relations.erase(std::remove_if(relations.begin(), relations.end(), [&](const std::shared_ptr<Relation>& r) {
        if (!r || !r->involves(being)) return false;
        ECA::Event echo;
        echo.type = "relation-destroyed";
        echo.subject = r.get();
        echo.timestamp = std::time(nullptr);
        Core::EventBus::instance().publish(echo);
        return true;
    }), relations.end());
    return relations.size() != oldSize;
}

/*std::vector<Relation> RelationManager::getRelationsOf(const std::string& entity) const {
    std::vector<Relation> result;
    for (const auto& r : relations) {
        if (r.involves(entity)) result.push_back(r);
    }
    return result;
}*/

std::vector<std::shared_ptr<Relation>> RelationManager::getRelationsOf(const Singular& being) const {
    std::vector<std::shared_ptr<Relation>> result;
    for (const auto& r : relations) {
        if (r && r->involves(being)) result.push_back(r);
    }
    return result;
}

std::vector<std::shared_ptr<Relation>> RelationManager::getRelationsOf(const std::string& identifier) const {
    std::vector<std::shared_ptr<Relation>> result;
    for (const auto& r : relations) {
        if (r && r->involves(identifier)) result.push_back(r);
    }
    return result;
}

/*std::vector<Relation> RelationManager::getRelationsBetween(const std::string& a, const std::string& b) const {
    std::vector<Relation> result;
    for (const auto& r : relations) {
        if (r.isBetween(a, b)) result.push_back(r);
    }
    return result;
}*/

std::vector<std::shared_ptr<Relation>> RelationManager::getRelationsBetween(const Singular& a, const Singular& b) const {
    std::vector<std::shared_ptr<Relation>> result;
    for (const auto& r : relations) {
        if (r && r->isBetween(a, b)) result.push_back(r);
    }
    return result;
}

std::vector<std::shared_ptr<Relation>> RelationManager::getRelationsBetween(const std::string& a, const std::string& b) const {
    std::vector<std::shared_ptr<Relation>> result;
    for (const auto& r : relations) {
        if (r && r->isBetween(a, b)) result.push_back(r);
    }
    return result;
}

std::vector<std::shared_ptr<Relation>> RelationManager::getRelationsOfType(const std::string& type) const {
    std::vector<std::shared_ptr<Relation>> result;
    for (const auto& r : relations) {
        if (r && r->type == type) result.push_back(r);
    }
    return result;
}

nlohmann::json RelationManager::toJson() const {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& r : relations) {
        if (!r) continue;
        arr.push_back(r->toJson());
    }
    return arr;
}

void RelationManager::loadFromJson(const nlohmann::json& j, const RelationEndpointResolver& resolve) {
    relations.clear();
    if (!j.is_array()) return;
    for (const auto& item : j) {
        // Provenance may name a being that is not in this world yet. Keep the
        // identifier property; live graphs still refuse unbound edges in add().
        relations.push_back(std::make_shared<Relation>(Relation::fromJson(item, resolve)));
    }
} 


std::vector<std::string> RelationManager::findAdjacentEntities(const std::string& entityId, const std::string& relationType) const {
    std::vector<std::string> adjacent;
    for (const auto& relPtr : relations) {
        if (!relPtr) continue;
        const Relation& rel = *relPtr;
        if (!relationType.empty() && rel.type != relationType) continue;

        if (rel.aId() == entityId) {
            adjacent.push_back(rel.bId());
        } else if (!rel.directed && rel.bId() == entityId) {
            adjacent.push_back(rel.aId());
        }
    }
    return adjacent;
}

bool RelationManager::wouldFormCycle(const Singular* start, const Singular* target, const std::string& relationType) const {
    if (!start || !target) return false;
    if (start == target) return true;

    std::vector<const Singular*> queue = {target};
    std::unordered_set<const Singular*> visited = {target};

    while (!queue.empty()) {
        const Singular* current = queue.back();
        queue.pop_back();

        for (const auto& relPtr : relations) {
            if (!relPtr) continue;
            const Relation& rel = *relPtr;
            if (rel.type == relationType && rel.a() == current) {
                if (rel.b() == start) return true;
                if (rel.b() && visited.insert(rel.b()).second) {
                    queue.push_back(rel.b());
                }
            }
        }
    }
    return false;
}

bool RelationManager::wouldFormCycle(const std::string& start, const std::string& target, const std::string& relationType) const {
    if (start.empty() || target.empty()) return false;
    if (start == target) return true;
    
    // Trace target's outgoing relations to see if we can reach start
    std::vector<std::string> queue = {target};
    std::unordered_set<std::string> visited = {target};
    
    while (!queue.empty()) {
        std::string current = queue.back();
        queue.pop_back();
        
        for (const auto& relPtr : relations) {
            if (!relPtr) continue;
            const Relation& rel = *relPtr;
            if (rel.type == relationType && rel.aId() == current) {
                if (rel.bId() == start) return true;
                if (visited.insert(rel.bId()).second) {
                    queue.push_back(rel.bId());
                }
            }
        }
    }
    return false;
}
