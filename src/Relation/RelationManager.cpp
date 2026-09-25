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

#include <unordered_set>

// Leaked on purpose: a manager can be destroyed during static teardown, and a
// registry destroyed before it would be erased from after its own death.
namespace {
std::unordered_set<RelationManager*>& liveManagers() {
    static auto* live = new std::unordered_set<RelationManager*>();
    return *live;
}
} // namespace

void RelationManager::forgetBeingEverywhere(const Singular* being) {
    if (!being) return;
    // O(1) for the common case — a transient Moment, or any being no relation
    // holds. Without this every Singular destructor walked every relation in
    // every live manager (Relation.hpp, struct Endpoint, has the measurement).
    // Guarded by tests/relation/endpoint_register_test.cpp.
    if (!Relation::mayBeEndpoint(being)) return;
    for (RelationManager* manager : liveManagers()) {
        if (!manager) continue;
        bool changed = false;
        for (const auto& relation : manager->relations) {
            if (!relation) continue;
            // POINTER compare only: `being` is mid-destruction and may not be
            // dereferenced (see Singular::notifyBeingReleased).
            if (relation->a() == being || relation->b() == being) {
                relation->forgetEndpoint(being);
                changed = true;
            }
        }
        // Touch ONLY if an endpoint actually moved. This callback fires for
        // EVERY Singular destructor — and every transient ECA::Event is a
        // Moment is a Singular, one per alpha predicate per fact. Touching
        // unconditionally invalidated the endpoint index on every one of them,
        // forcing an O(relations) rebuild on the next query: measured, an
        // indexed category-scoped law ran ~25% SLOWER than scanning every
        // relation. The same trap as the transient-Moment fact scan
        // (DERIVED_STATE_AND_THE_SILENCE_OF_LAWS §4), set again by the code
        // meant to make things faster.
        //
        // When it did move: the identifier key would still find the edge, but
        // the pointer key names a being that is being destroyed, and could
        // match a new being allocated at the same address. Rebuild.
        if (changed) manager->touch();
    }
}

void RelationManager::forgetTypeLexemeEverywhere(
    const Singularity::Language::Lexeme* lexeme) {
    if (!lexeme) return;

    // Deliberately rare-path O(total live Relations): unlike endpoints, type
    // Lexemes are released only by LanguageSystem removal/eviction/clear.
    // Keeping this out of Singular::~Singular preserves the endpoint register's
    // O(1) fast path for transient Moments and ordinary beings.
    for (RelationManager* manager : liveManagers()) {
        if (!manager) continue;
        for (const auto& relation : manager->relations) {
            if (relation) relation->forgetTypeLexeme(lexeme);
        }
    }
}

RelationManager::RelationManager() { liveManagers().insert(this); }

RelationManager::RelationManager(const RelationManager& other)
    : relations(other.relations) {
    liveManagers().insert(this);
    touch();   // index members are not copied; build our own on first query
}

RelationManager::RelationManager(RelationManager&& other) noexcept
    : relations(std::move(other.relations)) {
    liveManagers().insert(this);
    touch();
    other.touch();   // `other` just lost its relations
}

RelationManager& RelationManager::operator=(const RelationManager& other) {
    if (this != &other) { relations = other.relations; touch(); }
    return *this;
}

RelationManager& RelationManager::operator=(RelationManager&& other) noexcept {
    if (this != &other) { relations = std::move(other.relations); touch(); other.touch(); }
    return *this;
}

RelationManager::~RelationManager() { liveManagers().erase(this); }

// ---------------------------------------------------------------------------
// The endpoint index (FORMATION_RETE.md §8 rung 4). See the header for why it
// is keyed two ways and why it only ever proposes.
// ---------------------------------------------------------------------------
void RelationManager::rebuildEndpointIndex() const {
    _byEndpoint.clear();
    _byIdentifier.clear();
    for (const auto& owned : relations) {
        Relation* r = owned.get();
        if (!r) continue;
        if (Singular* a = r->a()) _byEndpoint[a].push_back(r);
        if (Singular* b = r->b(); b && b != r->a()) _byEndpoint[b].push_back(r);
        // aId()/bId(): a bound endpoint's live name, an unbound one's kept name.
        // Reading it here also fills the endpoint's cached id, which is what
        // lets Relation::Endpoint::forget() keep the name when the being dies.
        // Bound pointers are live here: forgetBeingEverywhere nulls every
        // pointer to a dying being before it is freed — the same guarantee the
        // unindexed scan in the Related condition already relies on.
        const std::string aId = r->aId();
        const std::string bId = r->bId();
        if (!aId.empty()) _byIdentifier[aId].push_back(r);
        if (!bId.empty() && bId != aId) _byIdentifier[bId].push_back(r);
    }
    _indexedGeneration = _generation;
}

void RelationManager::relationsInvolving(const Singular& being, std::vector<Relation*>& out) const {
    out.clear();
    if (_indexedGeneration != _generation) rebuildEndpointIndex();
    if (auto it = _byEndpoint.find(&being); it != _byEndpoint.end()) out = it->second;
    const std::string id = being.getIdentifier();
    if (id.empty()) return;
    if (auto it = _byIdentifier.find(id); it != _byIdentifier.end()) {
        for (Relation* r : it->second) {
            // Degrees are small; a linear check keeps the list duplicate-free
            // when an edge is known both by pointer and by name.
            if (std::find(out.begin(), out.end(), r) == out.end()) out.push_back(r);
        }
    }
}


void RelationManager::add(const std::shared_ptr<Relation>& r) {
    if (!r) return;
    if (!r->hasEndpoints()) {
        std::cerr << "RelationManager::add - Rejecting relation '" << r->typeLabel()
                  << "' with unbound Singular endpoints.\n";
        return;
    }

    // Zach's constitutive-Relation rule: if an authored Relation-kind says its
    // substance is an engine invariant, the graph may not store a contrary
    // assertion. This is deliberately BELOW the spelling layer. A same-named
    // Relation-kind with another unique id and no opcode is unaffected.
    switch (r->evaluateConstitutive()) {
        case Relation::ConstitutiveStatus::Violated:
            std::cerr << "RelationManager::add - REFUSED Relation kind '"
                      << r->typeLabel() << "' [" << r->type << "] between '"
                      << r->aId() << "' and '" << r->bId()
                      << "': its authored constitutive opcode evaluates false.\n";
            return;
        case Relation::ConstitutiveStatus::Invalid:
            std::cerr << "RelationManager::add - REFUSED Relation kind '"
                      << r->typeLabel() << "' [" << r->type << "] between '"
                      << r->aId() << "' and '" << r->bId()
                      << "': its authored constitutive opcode cannot be evaluated "
                         "from the supplied beings.\n";
            return;
        case Relation::ConstitutiveStatus::Holds:
        case Relation::ConstitutiveStatus::NotApplicable:
            break;
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
        touch();

        // Trigger event for new relation creation
        RelationCreatedEvent event(r);
        Core::EventBus::instance().publish(event);

        // ECA echo so LAWS can hear it (string-typed events are what the
        // Rete network binds): subject is the newborn relation — itself a
        // Singular whose endpoints laws can read.
        ECA::Event echo("relation-formed", r.get(), nullptr, std::time(nullptr));
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
        touch();

        ECA::Event echo("relation-destroyed", removed.get(), nullptr, std::time(nullptr));
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
            ECA::Event echo("relation-destroyed", r.get(), nullptr, std::time(nullptr));
            Core::EventBus::instance().publish(echo);
            return true;
        }
        return false;
    }), relations.end());
    touch();
    return relations.size() != oldSize;
}

bool RelationManager::removeBetween(const std::string& a, const std::string& b, const std::string& type) {
    auto oldSize = relations.size();
    relations.erase(std::remove_if(relations.begin(), relations.end(), [&](const std::shared_ptr<Relation>& r) {
        if (!r) return false;
        bool matchesEntities = r->isBetween(a, b);
        bool matchesType = type.empty() || r->type == type;
        if (matchesEntities && matchesType) {
            ECA::Event echo("relation-destroyed", r.get(), nullptr, std::time(nullptr));
            Core::EventBus::instance().publish(echo);
            return true;
        }
        return false;
    }), relations.end());
    touch();
    return relations.size() != oldSize;
}

bool RelationManager::removeInvolving(const Singular* being) {
    if (!being) return false;
    auto oldSize = relations.size();
    relations.erase(std::remove_if(relations.begin(), relations.end(), [&](const std::shared_ptr<Relation>& r) {
        if (!r || !r->involves(being)) return false;
        ECA::Event echo("relation-destroyed", r.get(), nullptr, std::time(nullptr));
        Core::EventBus::instance().publish(echo);
        return true;
    }), relations.end());
    touch();
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
    std::vector<Relation*> candidates;
    // O(degree) candidate lookup via endpoint index
    relationsInvolving(being, candidates);
    if (candidates.empty()) return result;

    result.reserve(candidates.size());
    std::unordered_set<Relation*> candSet(candidates.begin(), candidates.end());

    // Single pass over relations with early break once all K candidates are matched
    for (const auto& r : relations) {
        if (!r) continue;
        if (candSet.count(r.get())) {
            result.push_back(r);
            if (result.size() == candSet.size()) break;
        }
    }
    return result;
}

std::vector<std::shared_ptr<Relation>> RelationManager::getRelationsOf(const std::string& identifier) const {
    std::vector<std::shared_ptr<Relation>> result;
    if (identifier.empty()) return result;
    if (_indexedGeneration != _generation) rebuildEndpointIndex();

    auto it = _byIdentifier.find(identifier);
    if (it == _byIdentifier.end() || it->second.empty()) return result;

    const auto& candidates = it->second;
    result.reserve(candidates.size());
    std::unordered_set<Relation*> candSet(candidates.begin(), candidates.end());

    // Single pass over relations with early break once all K candidates are matched
    for (const auto& r : relations) {
        if (!r) continue;
        if (candSet.count(r.get())) {
            result.push_back(r);
            if (result.size() == candSet.size()) break;
        }
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
    touch();   // before the early return: an empty or malformed load still replaced the graph
    if (!j.is_array()) return;
    for (const auto& item : j) {
        // Provenance may name a being that is not in this world yet. Keep the
        // identifier property; live graphs still refuse unbound edges in add().
        relations.push_back(std::make_shared<Relation>(Relation::fromJson(item, resolve)));
    }
    touch();
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

    // Pre-build adjacency list to avoid O(E) scan per visited node
    std::unordered_map<const Singular*, std::vector<const Singular*>> adj;
    for (const auto& relPtr : relations) {
        if (!relPtr) continue;
        const Relation& rel = *relPtr;
        if (rel.type == relationType && rel.a() && rel.b()) {
            adj[rel.a()].push_back(rel.b());
        }
    }

    std::vector<const Singular*> queue = {target};
    std::unordered_set<const Singular*> visited = {target};

    while (!queue.empty()) {
        const Singular* current = queue.back();
        queue.pop_back();

        auto it = adj.find(current);
        if (it != adj.end()) {
            for (const Singular* next : it->second) {
                if (next == start) return true;
                if (visited.insert(next).second) {
                    queue.push_back(next);
                }
            }
        }
    }
    return false;
}

bool RelationManager::wouldFormCycle(const std::string& start, const std::string& target, const std::string& relationType) const {
    if (start.empty() || target.empty()) return false;
    if (start == target) return true;
    
    // Pre-build adjacency list to avoid O(E) scan per visited node
    std::unordered_map<std::string, std::vector<std::string>> adj;
    for (const auto& relPtr : relations) {
        if (!relPtr) continue;
        const Relation& rel = *relPtr;
        if (rel.type == relationType) {
            std::string aId = rel.aId();
            std::string bId = rel.bId();
            if (!aId.empty() && !bId.empty()) {
                adj[aId].push_back(bId);
            }
        }
    }

    // Trace target's outgoing relations to see if we can reach start
    std::vector<std::string> queue = {target};
    std::unordered_set<std::string> visited = {target};
    
    while (!queue.empty()) {
        std::string current = queue.back();
        queue.pop_back();
        
        auto it = adj.find(current);
        if (it != adj.end()) {
            for (const std::string& next : it->second) {
                if (next == start) return true;
                if (visited.insert(next).second) {
                    queue.push_back(next);
                }
            }
        }
    }
    return false;
}
