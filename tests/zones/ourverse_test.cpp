// Ourverse — first rung of the vessel of unity (OURVERSE.md).
//
// Gathering Zone is unowned. Filaments between Zones are mutual.
// Shared Joys are a hierarchy. Ecumenical convening is empty by default.
// No LocalOurverse / Filament class.

#include "ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Person/Person.hpp"
#include "Person/Relationship/Community/Community.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Language/LanguageSystem.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdio>

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "ourverse_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "ourverse_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "ourverse_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    Ourverse ourverse;
    assert(ourverse.getIdentifier() == "Ourverse");
    assert(ourverse.joys().satisfiesJoyBounds());
    assert(ourverse.convenesToward().empty());

    PropertyValue v;
    assert(PropertyPath::parse("joys").getValue(ourverse, v) == PropertyPath::PathResult::Ok);
    assert(PropertyPath::parse("convenesToward").getValue(ourverse, v)
           == PropertyPath::PathResult::Ok);
    assert(std::get<std::string>(v).empty());
    assert(PropertyPath::parse("filamentCount").getValue(ourverse, v)
           == PropertyPath::PathResult::Ok);
    assert(std::get<int>(v) == 0);
    // The Engine bag is not the Ourverse.
    assert(PropertyPath::parse("ownedObjects").getValue(ourverse, v)
           == PropertyPath::PathResult::NoSuchProperty);

    ZoneManager zones;
    Zone& gathering = ourverse.ensureGatheringZone(zones);
    assert(gathering.isOurverseGathering());
    assert(gathering.owner().empty());
    gathering.setOwner("zack");
    assert(gathering.owner().empty());

    auto home = std::make_shared<Zone>("Home", "default");
    home->setOwner("zack");
    assert(home->owner() == "zack");
    ourverse.setPrimaryGatheringZone(home);
    assert(ourverse.gatheringZone() != home.get());

    Law grab("seize-gathering");
    Object grabber;
    grabber.setObjectID("grabber");
    grab.addAuthor(grabber);
    grab.setActionModel(ActionNode::set("owner", PropertyValue(std::string("grabber"))));
    assert(grab.applyTo(gathering) == Law::ApplicationResult::AuthorityDenied);

    Zone sanctum("Sanctum", "default");
    Zone temple("Temple", "default");
    assert(ourverse.weave(sanctum, temple));
    assert(ourverse.propFilamentCount() == 1);
    assert(!ourverse.weave(sanctum, sanctum));
    assert(ourverse.propFilamentCount() == 1);

    Community church("church");
    Person member(Soul("member"), Body::createBasicAvatar("Voxel"), "default");
    church.addMember(&member);
    assert(ourverse.ensureCommunityGathering(church));

    LawManager laws;
    ourverse.registerMetalaws(laws);
    assert(laws.find("ourverse-gathering-unowned"));
    assert(laws.find("ourverse-filaments-mutual"));
    assert(laws.find("ourverse-gathering-unowned")->isFirstMover());

    Singularity::Language::LanguageSystem::instance().clear();
    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("ourverse_test: ALL OK");
    return 0;
}
