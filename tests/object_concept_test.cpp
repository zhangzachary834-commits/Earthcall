// Object set-to-set creation milestone test (LAW_AND_CREATION_SYSTEM.md, commit 6):
//   "select set → capture → instantiate elsewhere with a mapping applied."
//
// Exercises: captureFrom (centroid-relative poses, geometry recipes,
// provenance), PropertyMapping derivation (PerMember and Mean), concept JSON
// round-trip, ConceptRegistry, and the unification claim — creation IS a law
// application: a Spawn law fired by an event births objects into a World,
// and an unauthored law cannot create.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Form/Object/Creation/ObjectConcept.hpp"
#include "Singularity/TransferPolicy.hpp"
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
            [](const FactPtr& f) { return f->type == "birth-signal"; });
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

        // ------------------------------------------------------------------
        // 5. Exact mathematics in the derivation: the mapping transform is
        //    the full OntoMath piecewise algebra — undefined transfers
        //    NOTHING (bounded domains are honored, never zero-filled).
        // ------------------------------------------------------------------
        Object src;
        src.setPosition(glm::vec3(0.0f, 3.0f, 0.0f));
        std::vector<Object*> srcSet{&src};
        auto squared = ObjectConcept::captureFrom(srcSet, "squared", &author);
        {
            PropertyMapping m;
            m.source = PropertyPath::parse("position.y");
            m.target = PropertyPath::parse("position.y");
            m.hasExact = true;
            OntoMath::Piecewise f;                        // y = x^2 for x in [0, 5]
            f.inputVariable = "x";
            OntoMath::Piecewise::Piece piece;
            piece.hasLo = true;
            piece.lo = 0.0;
            piece.hasHi = true;
            piece.hi = 5.0;
            piece.mathNode = OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::variable("x", 2.0));
            f.pieces.push_back(std::move(piece));
            m.exact = std::move(f);
            squared->addMapping(std::move(m));
        }
        auto exactBorn = squared->instantiate(glm::mat4(1.0f), &srcSet);
        assert(exactBorn.size() == 1);
        assert(nearf(exactBorn[0]->getPosition().y, 9.0f));      // 3^2, exact

        src.setPosition(glm::vec3(0.0f, 7.0f, 0.0f));            // outside [0,5]
        auto outside = squared->instantiate(glm::mat4(1.0f), &srcSet);
        assert(nearf(outside[0]->getPosition().y, 0.0f));        // untouched (template pose)

        // The exact transform survives serialization like everything else.
        auto rebornConcept = ObjectConcept::fromJson(squared->toJson());
        assert(rebornConcept->mappings().size() == 1);
        assert(rebornConcept->mappings()[0].hasExact);

        // ------------------------------------------------------------------
        // 6. Governed transfer: properties reach set-to-set only through
        //    the Singularity gate — and the gate is itself a legible being
        //    that ordinary LAWS govern.
        // ------------------------------------------------------------------
        src.setPosition(glm::vec3(0.0f, 3.0f, 0.0f));
        auto& policy = TransferPolicy::instance();

        assert(!policy.canTransfer(PropertyPath::parse("name")));       // gated
        assert(policy.canTransfer(PropertyPath::parse("position.y"))); // kernel
        assert(policy.canTransfer(PropertyPath::parse("shape.r")));    // governable-open

        // A LAW closes the shape gate: transfer access is governed state.
        // The policy must be reachable in the Universe for @-paths.
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&policy);
        });
        auto gateLaw = mgr.createLaw("close-the-shape-gate", {&author});
        gateLaw->setActionModel(ActionNode::set(
            "@transfer-policy.gate.shape", PropertyValue(false)));
        assert(gateLaw->applyTo(src) == Law::ApplicationResult::Applied);
        assert(!policy.canTransfer(PropertyPath::parse("shape.r")));   // closed by law

        // A closed gate means the mapping is SKIPPED during instantiation.
        src.setShape(Object::ShapeKind::Sphere, Object::ShapeParams{});
        auto radiusTaker = ObjectConcept::captureFrom(srcSet, "radius-taker", &author);
        {
            PropertyMapping m;
            m.source = PropertyPath::parse("shape.r");
            m.target = PropertyPath::parse("shape.fillet");
            m.transform = CurveModel::polynomial({0.0, 1.0});
            radiusTaker->addMapping(std::move(m));
        }
        assert(PropertyPath::parse("shape.r").setValue(src, PropertyValue(0.9f)) == PropertyPath::PathResult::Ok);
        assert(PropertyPath::parse("shape.fillet").setValue(src, PropertyValue(0.0f)) == PropertyPath::PathResult::Ok);
        auto gatedBorn = radiusTaker->instantiate(glm::mat4(1.0f), &srcSet);
        PropertyValue fv;
        double filletValue = -1.0;
        assert(PropertyPath::parse("shape.fillet").getValue(*gatedBorn[0], fv) == PropertyPath::PathResult::Ok &&
               propertyValueToNumber(fv, filletValue));
        assert(!nearf(static_cast<float>(filletValue), 0.9f));   // did NOT transfer

        // ...and a law reopens it; now the same derivation carries.
        auto openLaw = mgr.createLaw("open-the-shape-gate", {&author});
        openLaw->setActionModel(ActionNode::set(
            "@transfer-policy.gate.shape", PropertyValue(true)));
        assert(openLaw->applyTo(src) == Law::ApplicationResult::Applied);
        assert(policy.canTransfer(PropertyPath::parse("shape.r")));
        auto openBorn = radiusTaker->instantiate(glm::mat4(1.0f), &srcSet);
        assert(PropertyPath::parse("shape.fillet").getValue(*openBorn[0], fv) == PropertyPath::PathResult::Ok &&
               propertyValueToNumber(fv, filletValue));
        assert(nearf(static_cast<float>(filletValue), 0.9f));    // transferred

        // The Kernel floor holds: no law closes position.
        auto tyrant = mgr.createLaw("close-position", {&author});
        tyrant->setActionModel(ActionNode::set(
            "@transfer-policy.gate.position", PropertyValue(false)));
        tyrant->applyTo(src);                                    // write refused inside
        assert(policy.canTransfer(PropertyPath::parse("position.y")));
        assert(!policy.setOpen("position", false));              // even directly

        // Policy state round-trips with the world.
        const auto policyJson = policy.toJson();
        policy.setOpen("shape", false);
        policy.loadFromJson(policyJson);
        assert(policy.canTransfer(PropertyPath::parse("shape.r")));

        // ------------------------------------------------------------------
        // 7. The set's STRUCTURE is captured and reborn: an inter-member
        //    relation ("pillar attached to beam") becomes a fresh relation
        //    between the corresponding newborns, every instantiation.
        // ------------------------------------------------------------------
        Object pillar, beam;
        pillar.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        beam.setPosition(glm::vec3(2.0f, 0.0f, 0.0f));

        RelationManager graph;
        graph.add(std::make_shared<Relation>("attached-to", pillar, beam, true, 0.8f));
        Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
            for (const auto& rel : graph.getAll()) {
                if (rel) out.push_back(rel.get());
            }
        });
        RelationManager rebornGraph;   // where newborn relations register
        Universe::instance().setRelationRegistrar(
            [&](std::shared_ptr<Relation> rel) { rebornGraph.add(std::move(rel)); });

        std::vector<Object*> pairSet{&pillar, &beam};
        auto structure = ObjectConcept::captureFrom(pairSet, "pillar-beam", &author);
        assert(structure->relationTemplates().size() == 1);
        assert(structure->relationTemplates()[0].type == "attached-to");
        assert(structure->relationTemplates()[0].directed);

        auto twins = structure->instantiate(
            glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, 0.0f, 0.0f)), &pairSet);
        assert(twins.size() == 2);
        assert(rebornGraph.getAll().size() == 1);
        const Relation& rebornRel = *rebornGraph.getAll().front();
        assert(rebornRel.type == "attached-to");
        assert(rebornRel.directed);
        assert(nearf(rebornRel.getWeight(), 0.8f));
        assert(rebornRel.entityA == twins[0]->getIdentifier());   // NEWBORN ids,
        assert(rebornRel.entityB == twins[1]->getIdentifier());   // not the sources'

        // The structure survives serialization like all concept text.
        auto rebornStructure = ObjectConcept::fromJson(structure->toJson());
        assert(rebornStructure->relationTemplates().size() == 1);
        assert(rebornStructure->relationTemplates()[0].type == "attached-to");

        Universe::instance().setRelationProvider({});
        Universe::instance().setRelationRegistrar({});
        Universe::instance().setProvider({});
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("object_concept_test: ALL OK");
    return 0;
}
