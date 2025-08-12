#include "Relation.hpp"
#include "Singular.hpp"
#include <iostream>

// Specific Implementation Vision: Recursive, custom tool creation
// With a combination of the basic tools here, with a Formation system comprised of relations between things, people can create their own tools on top of that.
// This allows for a recursive, self-creating tool system that can evolve over time.
// For example, person wants to create a tool that spins objects. The user can set it so that relations are created between an Object's 2D form with others, and they use the existing tool system to draw the pattern by which they want the new tools behavior to resmble. So they can draw a spiral for the spin tool. Then they choose how the system actually uses it—here, let's say it uses an existing hypotehtical base tool "warp". A new relation is created that relates this "tool-behavior" drawing by looking at the drawing and "warping" the current drawing according to the pattern of the meta-spiral drawing. 
// User can have the choice to have the tools themselves be integrated under relations. Every act of drawing can call a relation between the tool and the other Singulars involved. (tool isn't Singular yet, so we'll make them Singular in the future.)

using json = nlohmann::json;

Relation::Relation(const std::string& type,
                   const std::string& a,
                   const std::string& b,
                   bool directed,
                   float weight)
    : type(type), entityA(a), entityB(b), directed(directed), weight(weight) {}

void Relation::describe() const {
    std::cout << "Relation [" << type << "] "
              << (directed ? "from " : "between ")
              << entityA << (directed ? " -> " : " and ") << entityB
              << " (strength=" << weight << ")"
              << std::endl;
}

bool Relation::involves(const std::string& entity) const {
    return entityA == entity || entityB == entity;
}

bool Relation::isBetween(const std::string& a, const std::string& b) const {
    if (directed) {
        return entityA == a && entityB == b;
    }
    return (entityA == a && entityB == b) || (entityA == b && entityB == a);
}

json Relation::toJson() const {
    json evArr = json::array();
    for(const auto& ev : events) evArr.push_back(ev.toJson());

    return json{{"type", type},
                {"entityA", entityA},
                {"entityB", entityB},
                {"directed", directed},
                {"weight", weight},
                {"events", evArr}};
}

Relation Relation::fromJson(const json& j) {
    Relation r;
    r.type = j.at("type").get<std::string>();
    r.entityA = j.at("entityA").get<std::string>();
    r.entityB = j.at("entityB").get<std::string>();
    r.directed = j.value("directed", false);
    r.weight = j.value("weight", 1.0f);

    if(j.contains("events") && j["events"].is_array()){
        for(const auto& item : j["events"]) {
            r.events.push_back(RelationEvent::fromJson(item));
        }
    }
    return r;
}

// ------------------------------------------------------------------
// Additional constructors / methods for Singular endpoints
// ------------------------------------------------------------------

Relation::Relation(const std::string& type,
                   const Singular& aEntity,
                   const Singular& bEntity,
                   bool directed,
                   float weight)
    : Relation(type, aEntity.getIdentifier(), bEntity.getIdentifier(), directed, weight) {}

bool Relation::involves(const Singular& entity) const {
    return involves(entity.getIdentifier());
}

bool Relation::isBetween(const Singular& aEntity, const Singular& bEntity) const {
    return isBetween(aEntity.getIdentifier(), bEntity.getIdentifier());
}
