// Relation persistence is a root codec: its endpoint identifiers survive even
// when no resolver can bind them during the first hydration pass.

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/Storage/Serialization/Relation/RelationSerialization.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>

int main() {
    Object source("relation-source");
    source.setObjectID("object.relation-source");
    Object target("relation-target");
    target.setObjectID("object.relation-target");

    Relation original("holds", source, target, true, 0.75f);
    original.events.push_back({42, "source-held-target", 0.25f});
    original.attachment.enabled = true;
    original.attachment.parentAnchor = {1.0f, 2.0f, 3.0f};
    original.attachment.childAnchor = {4.0f, 5.0f, 6.0f};
    original.attachment.inheritRotation = false;

    const nlohmann::json saved = relationToJson(original);
    assert(saved["entityA"] == source.getIdentifier());
    assert(saved["entityB"] == target.getIdentifier());
    assert(saved["events"].size() == 1);

    Relation unbound = relationFromJson(saved);
    assert(!unbound.hasEndpoints());
    assert(unbound.aId() == source.getIdentifier());
    assert(unbound.bId() == target.getIdentifier());

    Relation rebound = Relation::fromJson(saved, [&](const std::string& identifier) -> Singular* {
        if (identifier == source.getIdentifier()) return &source;
        if (identifier == target.getIdentifier()) return &target;
        return nullptr;
    });
    assert(rebound.a() == &source);
    assert(rebound.b() == &target);
    assert(rebound.directed);
    assert(std::fabs(rebound.getWeight() - 0.75f) < 1e-6f);
    assert(rebound.events.size() == 1);
    assert(rebound.events.front().description == "source-held-target");
    assert(rebound.attachment.enabled);
    assert(!rebound.attachment.inheritRotation);
    assert(std::fabs(rebound.attachment.parentAnchor.y - 2.0f) < 1e-6f);
    assert(std::fabs(rebound.attachment.childAnchor.z - 6.0f) < 1e-6f);

    std::puts("relation_serialization_topology_test: ALL OK");
    return 0;
}
