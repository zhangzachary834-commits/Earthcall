// Object set-to-set creation milestone test (LAW_AND_CREATION_SYSTEM.md, commit 6):
//   "select set → capture → instantiate elsewhere with a mapping applied."
//
// Exercises: captureFrom (centroid-relative poses, geometry recipes,
// provenance), PropertyMapping derivation (PerMember and Mean), concept JSON
// round-trip, ConceptRegistry, and the unification claim — creation IS a law
// application: a Spawn law fired by an event births objects into a World,
// and an unauthored law cannot create.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Form/Object/Creation/ObjectConcept.hpp"
#include "ZonesOfEarth/World/World.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {

bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "object_concept_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "object_concept_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "object_concept_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        Object author;

        // ------------------------------------------------------------------
        // 1. Capture: two columns become a concept.
        // ------------------------------------------------------------------
        Object colA, colB;
        colA.setShape(Object::ShapeKind::Torus, [] {
            Object::ShapeParams p; p.majorR = 0.4f; p.minorR = 0.1f; return p; }());
        colB.setShape(Object::ShapeKind::Torus, [] {
            Object::ShapeParams p; p.majorR = 0.6f; p.minorR = 0.2f; return p; }());
        colA.setPosition(glm::vec3(-1.0f, 0.0f, 0.0f));
        colB.setPosition(glm::vec3(1.0f, 0.0f, 0.0f));

        auto concept = ObjectConcept::captureFrom({&colA, &colB}, "twin-columns", &author);
        assert(concept->members().size() == 2);
        // Poses are centroid-relative: the concept is placeable anywhere.
        assert(nearf(glm::vec3(concept->members()[0].relativeTransform[3]).x, -1.0f));
        assert(nearf(glm::vec3(concept->members()[1].relativeTransform[3]).x, 1.0f));

        // ------------------------------------------------------------------
        // 2. Derivation: each newborn's majorR = 1.5x its source's (PerMember),
        //    every newborn's minorR = mean of the source set (Mean).
        // ------------------------------------------------------------------
        PropertyMapping perMember;
        perMember.source = PropertyPath::parse("shape.majorR");
        perMember.transform = CurveModel::polynomial({0.0, 1.5});
        perMember.target = PropertyPath::parse("shape.majorR");
        concept->addMapping(perMember);

        PropertyMapping meanMap;
        meanMap.source = PropertyPath::parse("shape.minorR");
        meanMap.transform = CurveModel::polynomial({0.0, 1.0});   // identity
        meanMap.target = PropertyPath::parse("shape.minorR");
        meanMap.agg = PropertyMapping::Aggregate::Mean;
        concept->addMapping(meanMap);

        std::vector<Object*> sources{&colA, &colB};
        auto newborns = concept->instantiate(
            glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 0.0f)), &sources);
        assert(newborns.size() == 2);
        assert(nearf(newborns[0]->getPosition().x, 9.0f));    // placed, structure kept
        assert(nearf(newborns[1]->getPosition().x, 11.0f));
        assert(nearf(newborns[0]->getShapeParams().majorR, 0.6f));   // 0.4 * 1.5
        assert(nearf(newborns[1]->getShapeParams().majorR, 0.9f));   // 0.6 * 1.5
        assert(nearf(newborns[0]->getShapeParams().minorR, 0.15f));  // mean(0.1, 0.2)
        assert(nearf(newborns[1]->getShapeParams().minorR, 0.15f));

        // Every newborn's origin is recorded.
        assert(!concept->provenance().toJson().empty());

        // ------------------------------------------------------------------
        // 3. The concept's text survives serialization.
        // ------------------------------------------------------------------
        auto reloaded = ObjectConcept::fromJson(concept->toJson());
        assert(reloaded->getIdentifier() == concept->getIdentifier());
        assert(reloaded->members().size() == 2);
        assert(reloaded->mappings().size() == 2);
        auto again = reloaded->instantiate(glm::mat4(1.0f), &sources);
        assert(again.size() == 2);
        assert(nearf(again[1]->getShapeParams().majorR, 0.9f));

        // ------------------------------------------------------------------
        // 4. Creation IS a law application: a Spawn law births into a World.
        // ------------------------------------------------------------------
        ConceptRegistry::instance().add(concept);
        assert(ConceptRegistry::instance().find(concept->getIdentifier()) == concept);

        World world;
        LawManager mgr;
        mgr.connectToEventBus();

        auto birthLaw = mgr.createLaw("raise-columns");   // deliberately unauthored
        ActionNode spawn;
        spawn.kind = ActionNode::Kind::Spawn;
        spawn.conceptId = concept->getIdentifier();
        birthLaw->setActionModel(spawn);

        const std::size_t alpha = mgr.rete().addAlphaNode(
            "type == birth-signal",
            [](const ReteFact& f) { return f.type == "birth-signal"; });
        mgr.rete().bindLawToAlpha(birthLaw->getIdentifier(), alpha);

        // Unauthored: the event arrives but nothing may enter the world.
        Core::EventBus::instance().publish(
            ECA::Event{"birth-signal", &world, nullptr, std::time(nullptr)});
        auto records = mgr.tick();
        assert(!records.empty() &&
               records.front().result == Law::ApplicationResult::Unauthored);
        assert(world.objects().empty());

        // Authored: the same signal now births both columns into the world.
        birthLaw->addAuthor(author);
        Core::EventBus::instance().publish(
            ECA::Event{"birth-signal", &world, nullptr, std::time(nullptr)});
        records = mgr.tick();
        assert(!records.empty() &&
               records.front().result == Law::ApplicationResult::Applied);
        assert(world.objects().size() == 2);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("object_concept_test: ALL OK");
    return 0;
}
