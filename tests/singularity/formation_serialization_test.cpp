#include "Singularity/Storage/Serialization/Relation/FormationSerialization.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Relation/Formation/Formation.hpp"
#include "Relation/Relation.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Storage/Serialization/Relation/RelationSerialization.hpp"

#include <cassert>
#include <cstdio>
#include <memory>

int main() {
    Zone zone("test-zone", "default");

    auto o1 = std::make_shared<Object>("object-1");
    o1->setObjectID("object.1");
    auto o2 = std::make_shared<Object>("object-2");
    o2->setObjectID("object.2");

    zone.addObject(o1);
    zone.addObject(o2);

    Relation rel("bond", *o1, *o2);
    nlohmann::json relJson = relationToJson(rel);

    nlohmann::json zoneJson;
    zoneJson["formationRelations"] = nlohmann::json::array({relJson});

    applyFormationRelations(zone, zoneJson);

    assert(zone.formation().relations().getAll().size() == 1);
    auto r = zone.formation().relations().getAll()[0];
    assert(r->type == "bond");
    assert(r->aId() == o1->getIdentifier());
    assert(r->bId() == o2->getIdentifier());

    std::printf("formation_serialization_test: OK\n");
    return 0;
}
