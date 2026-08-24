// Zone-as-being milestone test (governance made spatial, stage 1-2).
//
// The manifesto: "Zone is a name for a space that is its own, self-defined
// object" and "Every Person has a Home they fully own." This test proves the
// substrate half: a Zone is a LEGIBLE Singular — its name, colors, scope and
// owner are properties laws can read (and, where honest, write); zones join
// the Universe so quantifiers range over them (ForAny Zone ...), @-paths
// address them by name, and zone events carry the zone as a participant
// (@event.object) laws can testify about.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Core/EventBus.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <ctime>

namespace {
bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }
} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "zone_being_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "zone_being_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "zone_being_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        // ------------------------------------------------------------------
        // 1. The truthful surface: name, scope, owner.
        //
        // Background tint, draw colour, brush radius and strokes were removed
        // from Zone when the Zone system was rearchitected (3e6015e): they were
        // the state of a drawing TOOL, not of a space that is its own object,
        // and a Zone is not a canvas. What a Zone truthfully has is identity,
        // reach and ownership — so that is what this section asserts.
        // ------------------------------------------------------------------
        Zone home("Home", "default");
        home.setOwner("zack");

        PropertyValue v;
        assert(PropertyPath::parse("name").getValue(home, v) == PropertyPath::PathResult::Ok);
        assert(std::get<std::string>(v) == "Home");
        assert(PropertyPath::parse("owner").getValue(home, v) == PropertyPath::PathResult::Ok);
        assert(std::get<std::string>(v) == "zack");
        assert(PropertyPath::parse("scope").getValue(home, v) == PropertyPath::PathResult::Ok);
        assert(std::get<std::string>(v) == "Local");

        // A Zone's spatial mathematics is part of its surface: the fields it
        // carries are addressable, which is what lets a law read the space
        // itself rather than only the beings standing in it.
        assert(PropertyPath::parse("spatialField").getValue(home, v) == PropertyPath::PathResult::Ok);
        assert(PropertyPath::parse("spatialVectorField").getValue(home, v) == PropertyPath::PathResult::Ok);

        // A tool's state is not a Zone's: the drawing surface Zone used to
        // carry is gone, and asking for it must MISS rather than resolve.
        assert(PropertyPath::parse("color").getValue(home, v) == PropertyPath::PathResult::NoSuchProperty);
        assert(PropertyPath::parse("drawColor").getValue(home, v) == PropertyPath::PathResult::NoSuchProperty);

        // Identity and title are NOT slots: writes refuse.
        assert(PropertyPath::parse("name").setValue(home, PropertyValue(std::string("X"))) != PropertyPath::PathResult::Ok);
        assert(PropertyPath::parse("owner").setValue(home, PropertyValue(std::string("thief"))) != PropertyPath::PathResult::Ok);
        assert(home.owner() == "zack");

        // Ownership implies deletability by the owner, and survives copy.
        assert(home.isDeletable("zack"));
        Zone copied(home);
        assert(copied.owner() == "zack");

        // ------------------------------------------------------------------
        // 2. Kind precision: a Zone is a Singular of its own kind, NOT an
        //    Object. It descended from Object until the Zone rearchitecture
        //    (3e6015e); a space is not a thing standing in space, so a
        //    quantifier over Objects must no longer sweep up the zones the
        //    objects are standing in.
        // ------------------------------------------------------------------
        Object plain;
        assert(ConditionNode::matchesKind(home, ConditionNode::BeingKind::Zone));
        assert(!ConditionNode::matchesKind(home, ConditionNode::BeingKind::Object));
        assert(!ConditionNode::matchesKind(plain, ConditionNode::BeingKind::Zone));

        // ------------------------------------------------------------------
        // 3. Zones live in the Universe: quantifiers range over them and
        //    @-paths address them by name.
        // ------------------------------------------------------------------
        Zone commons("Commons", "default");   // unowned
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&plain);
            beings.push_back(&home);
            beings.push_back(&commons);
        });

        ECA::Event probe;
        auto someZoneOwned = ConditionNode::forAny(
            ConditionNode::BeingKind::Zone,
            ConditionNode::compare("owner", ConditionNode::Op::Eq,
                                   PropertyValue(std::string("zack")))).compile();
        auto allZonesOwned = ConditionNode::forAll(
            ConditionNode::BeingKind::Zone,
            ConditionNode::compare("owner", ConditionNode::Op::Ne,
                                   PropertyValue(std::string("")))).compile();
        assert(someZoneOwned(probe, plain));    // Home is zack's
        assert(!allZonesOwned(probe, plain));   // the Commons is nobody's

        // @Home.owner reads the named zone's owner from ANY subject.
        assert(lawGetValue(plain, PropertyPath::parse("@Home.owner"), v));
        assert(std::get<std::string>(v) == "zack");

        // ------------------------------------------------------------------
        // 4. Zone events testify: person-joined-zone carries the zone as
        //    @event.object, so a law can ask WHOSE ground was stepped on.
        // ------------------------------------------------------------------
        Object walker;
        walker.setPosition(glm::vec3(0.0f, 1.0f, 0.0f));
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&walker);
            beings.push_back(&home);
            beings.push_back(&commons);
        });

        Object author;
        LawManager lawMgr;
        lawMgr.connectToEventBus();
        auto greet = lawMgr.createLaw("hearth-greeting", {&author});
        greet->setConditionModel(ConditionNode::compare(
            "@event.object.owner", ConditionNode::Op::Eq,
            PropertyValue(std::string("zack"))));
        greet->setActionModel(ActionNode::set("position.y", PropertyValue(30.0)));
        lawMgr.bindTrigger(greet->getIdentifier(), "person-joined-zone");

        // Entering the unowned Commons: the guard condition fails.
        Core::EventBus::instance().publish(
            ECA::Event{"person-joined-zone", &walker, &commons, std::time(nullptr)});
        lawMgr.tick();
        assert(nearf(walker.getPosition().y, 1.0f));

        // Entering zack's Home: the law hears it and acts on the walker.
        Core::EventBus::instance().publish(
            ECA::Event{"person-joined-zone", &walker, &home, std::time(nullptr)});
        lawMgr.tick();
        assert(nearf(walker.getPosition().y, 30.0f));

        Universe::instance().setProvider({});   // leave no dangling refs
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("zone_being_test: all sections passed\n");
    return 0;
}
