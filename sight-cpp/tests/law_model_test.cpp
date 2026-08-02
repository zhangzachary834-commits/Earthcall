// Law model milestone test (LAW_AND_CREATION_SYSTEM.md, commit 3):
//   "port one hard-coded law to a model; save, load, verify it still fires."
//
// The ported law is the ground-rest rule — today hard-coded in the physics
// substrate (objects rest at the ground plane) — re-expressed as an authored,
// serializable Law: IF position.y < 0 THEN position.y := 0.
//
// Exercises: ConditionNode (Compare/InRegion/All/Any/Not) and ActionNode
// (Set/Add/Drive/Sequence) compilation, Law::applyTo through models,
// CurveModel evaluation, and the commit's central claim — JSON round-trip
// preserves BEHAVIOR, not just descriptions.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "Form/Object/Object.hpp"
#include "Person/Person.hpp"
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
    // Object's constructor initializes face textures (GL calls) — hidden context.
    if (!glfwInit()) {
        std::fprintf(stderr, "law_model_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "law_model_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "law_model_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        Object author;   // any Singular can author (Person governance comes later)

        // ------------------------------------------------------------------
        // 1. The ported floor law: IF position.y < 0 THEN position.y := 0.
        // ------------------------------------------------------------------
        Law floorLaw("ground-rest");
        floorLaw.addAuthor(author);
        floorLaw.setConditionModel(
            ConditionNode::compare("position.y", ConditionNode::Op::Lt, PropertyValue(0.0)));
        floorLaw.setActionModel(ActionNode::set("position.y", PropertyValue(0.0f)));

        Object obj;
        obj.setPosition(glm::vec3(2.0f, -5.0f, 1.0f));
        assert(floorLaw.applyTo(obj) == Law::ApplicationResult::Applied);
        assert(nearf(obj.getPosition().y, 0.0f));
        assert(nearf(obj.getPosition().x, 2.0f));   // only y clamped

        obj.setPosition(glm::vec3(0.0f, 2.0f, 0.0f));
        assert(floorLaw.applyTo(obj) == Law::ApplicationResult::ConditionsFailed);
        assert(nearf(obj.getPosition().y, 2.0f));   // above ground: untouched

        // ------------------------------------------------------------------
        // 2. The commit's central claim: behavior survives save/load.
        // ------------------------------------------------------------------
        nlohmann::json saved = floorLaw.toJson();
        std::shared_ptr<Law> restored = Law::fromJson(saved);
        assert(restored->getIdentifier() == floorLaw.getIdentifier());

        Object fresh;
        fresh.setPosition(glm::vec3(0.0f, -3.0f, 0.0f));
        // Unauthored after load — the loader reattaches authors; until then
        // the law cannot fire. Structural: nothing acts without an author.
        assert(restored->applyTo(fresh) == Law::ApplicationResult::Unauthored);
        assert(nearf(fresh.getPosition().y, -3.0f));

        restored->addAuthor(author);
        assert(restored->applyTo(fresh) == Law::ApplicationResult::Applied);
        assert(nearf(fresh.getPosition().y, 0.0f));   // same behavior as before saving

        // Model text is structurally identical after the round-trip.
        assert(restored->toJson()["conditionModel"] == saved["conditionModel"]);
        assert(restored->toJson()["actionModel"] == saved["actionModel"]);

        // ------------------------------------------------------------------
        // 3. InRegion: a shape IS the condition (projection mode's landing site).
        // ------------------------------------------------------------------
        Law regionLaw("golden-zone");
        regionLaw.addAuthor(author);
        regionLaw.setConditionModel(ConditionNode::inRegion(
            geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f))));  // r=1 at origin
        regionLaw.setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.9f)));

        Object inside;
        inside.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        assert(regionLaw.applyTo(inside) == Law::ApplicationResult::Applied);
        PropertyValue fillet;
        assert(PropertyPath::parse("shape.fillet").getValue(inside, fillet) == PropertyPath::PathResult::Ok);
        assert(nearf(std::get<float>(fillet), 0.9f));

        Object outside;
        outside.setPosition(glm::vec3(5.0f, 0.0f, 0.0f));
        assert(regionLaw.applyTo(outside) == Law::ApplicationResult::ConditionsFailed);

        // Region survives serialization too (SdfNode JSON round-trip).
        std::shared_ptr<Law> regionRestored = Law::fromJson(regionLaw.toJson());
        regionRestored->addAuthor(author);
        Object inside2;
        inside2.setPosition(glm::vec3(0.2f, 0.1f, 0.0f));
        assert(regionRestored->applyTo(inside2) == Law::ApplicationResult::Applied);

        // ------------------------------------------------------------------
        // 4. Drive — the gradient law: rotation.y = f(position.x), f = 90x.
        // ------------------------------------------------------------------
        Law driveLaw("turn-with-x");
        driveLaw.addAuthor(author);
        driveLaw.setActionModel(ActionNode::drive(
            "rotation.y", CurveModel::polynomial({0.0, 90.0}), "position.x"));

        Object turner;
        turner.setPosition(glm::vec3(0.5f, 0.0f, 0.0f));
        assert(driveLaw.applyTo(turner) == Law::ApplicationResult::Applied);
        assert(nearf(turner.getRotationEulerDegrees().y, 45.0f, 1e-2f));

        // ------------------------------------------------------------------
        // 5. Combinators + sequences compile and behave.
        // ------------------------------------------------------------------
        ECA::Event probeEvent;
        probeEvent.type = "test";
        Object probeObj;
        probeObj.setPosition(glm::vec3(0.0f, 5.0f, 0.0f));

        auto above = ConditionNode::compare("position.y", ConditionNode::Op::Gt, PropertyValue(0.0));
        auto below = ConditionNode::compare("position.y", ConditionNode::Op::Lt, PropertyValue(0.0));
        assert(ConditionNode::all({above, ConditionNode::negate(below)}).compile()(probeEvent, probeObj));
        assert(ConditionNode::any({below, above}).compile()(probeEvent, probeObj));
        assert(!ConditionNode::all({above, below}).compile()(probeEvent, probeObj));

        ActionNode seq = ActionNode::sequence({
            ActionNode::set("position.y", PropertyValue(1.0f)),
            ActionNode::add("position.y", 2.0),
            ActionNode::scale("position.y", 3.0)
        });
        seq.compile()(probeEvent, probeObj);
        assert(nearf(probeObj.getPosition().y, 9.0f));   // (1 + 2) * 3

        // Sequence round-trips as a tree.
        ActionNode seqBack = ActionNode::fromJson(seq.toJson());
        Object probeObj2;
        seqBack.compile()(probeEvent, probeObj2);
        assert(nearf(probeObj2.getPosition().y, 9.0f));

        // ------------------------------------------------------------------
        // 6. CurveModel JSON fidelity.
        // ------------------------------------------------------------------
        CurveModel wave = CurveModel::sinusoid(2.0, 0.5, 0.25, 1.0);
        CurveModel waveBack = CurveModel::fromJson(wave.toJson());
        for (double x : {0.0, 0.3, 1.7, -2.2}) {
            assert(std::fabs(wave.evaluate(x) - waveBack.evaluate(x)) < 1e-12);
        }
        // ------------------------------------------------------------------
        // 7. ConditionNode Description Formatting
        // ------------------------------------------------------------------
        auto condBool = ConditionNode::compare("state.enabled", ConditionNode::Op::Eq, PropertyValue(true));
        assert(condBool.describe() == "state.enabled == true");

        auto condBoolFalse = ConditionNode::compare("state.enabled", ConditionNode::Op::Eq, PropertyValue(false));
        assert(condBoolFalse.describe() == "state.enabled == false");

        auto condString = ConditionNode::compare("player.activeTool", ConditionNode::Op::Eq, PropertyValue(std::string("3DShapeGenerator")));
        assert(condString.describe() == "player.activeTool == \"3DShapeGenerator\"");

        auto condDouble = ConditionNode::compare("position.y", ConditionNode::Op::Gt, PropertyValue(3.14));
        assert(condDouble.describe() == "position.y > 3.14");

        auto condDoubleTrunc = ConditionNode::compare("position.y", ConditionNode::Op::Eq, PropertyValue(5.0));
        assert(condDoubleTrunc.describe() == "position.y == 5");

        auto condPath = ConditionNode::comparePaths("position.y", ConditionNode::Op::Lt, "target.position.y");
        assert(condPath.describe() == "position.y < target.position.y");

        // ------------------------------------------------------------------
        // 8. Evaluation of Condition Data Types & Console Feed Execution
        // ------------------------------------------------------------------
        // We ensure that string conditions are properly evaluated and actions correctly affect the world
        // (logging that they fired via the Law's ApplicationResult logic).
        Law strLaw("tool-checker");
        strLaw.addAuthor(author);
        strLaw.setConditionModel(ConditionNode::compare("activeTool", ConditionNode::Op::Eq, PropertyValue(std::string("3DShapeGenerator"))));
        strLaw.setActionModel(ActionNode::set("position.y", PropertyValue(100.0f)));

        Soul pSoul("TestSubject");
        Body pAvatar = Body::createBasicAvatar("TestVoxel");
        Person person(pSoul, std::move(pAvatar), "default");
        strLaw.addAuthor(person);
        
        // Ensure property can be accessed
        PropertyValue valOut;
        PropertyPath::parse("activeTool").setValue(person, PropertyValue(std::string("None")));

        // Condition fails
        assert(strLaw.applyTo(person) == Law::ApplicationResult::ConditionsFailed);
        assert(person.position.y == 0.0f); 

        PropertyPath::parse("activeTool").setValue(person, PropertyValue(std::string("3DShapeGenerator")));
        
        assert(strLaw.applyTo(person) == Law::ApplicationResult::Applied);
        
        // ------------------------------------------------------------------
        // 9. Spawning Action Behavior
        // ------------------------------------------------------------------
        {
            // Create a test concept (a simple cube)
            Object sourceObj;
            sourceObj.setShape(Object::ShapeKind::Cube);
            std::vector<Object*> sources = { &sourceObj };
            auto cubeConcept = ObjectConcept::captureFrom(sources, "TestCubeShape");
            
            // Register it
            ConceptRegistry::instance().add(cubeConcept);
            std::string conceptId = cubeConcept->getIdentifier();
            
            // Create a world
            World testWorld;
            size_t initialSize = testWorld.getOwnedObjects().size();
            
            // Spawn action: target is the world
            ActionNode spawnAction = ActionNode::spawn(conceptId, "");
            
            ECA::Event spawnEvent;
            spawnEvent.type = "testSpawn";
            // Run action
            spawnAction.compile()(spawnEvent, testWorld);
            
            assert(testWorld.getOwnedObjects().size() == initialSize + 1);
            Object* spawnedObj = testWorld.getOwnedObjects().back().get();
            assert(spawnedObj->getShapeKind() == Object::ShapeKind::Cube);
        }

        printf("All core Law model tests passed.\n");

        // Now test a float mismatch (near)
        Law floatLaw("height-checker");
        floatLaw.addAuthor(author);
        floatLaw.setConditionModel(ConditionNode::compare("position.y", ConditionNode::Op::Near, PropertyValue(100.0f)));
        floatLaw.setActionModel(ActionNode::set("activeTool", PropertyValue(std::string("FloatSatisfied"))));

        assert(floatLaw.applyTo(person) == Law::ApplicationResult::Applied);
        (void)PropertyPath::parse("activeTool").getValue(person, valOut);
        assert(std::get<std::string>(valOut) == "FloatSatisfied");
        // ------------------------------------------------------------------
        // 10. Multiple-Condition and Multiple-Action Laws
        // ------------------------------------------------------------------
        {
            Law multiLaw("multi-law");
            multiLaw.addAuthor(author);
            multiLaw.addAuthor(person);
            
            // Multiple conditions: activeTool == "FloatSatisfied" AND position.y > 50.0
            std::vector<ConditionNode> conditions;
            conditions.push_back(ConditionNode::compare("position.y", ConditionNode::Op::Gt, PropertyValue(50.0f)));
            conditions.push_back(ConditionNode::compare("activeTool", ConditionNode::Op::Eq, PropertyValue(std::string("FloatSatisfied"))));
            multiLaw.setConditionModel(ConditionNode::all(std::move(conditions)));

            // Multiple actions: sequence of Sets
            std::vector<ActionNode> actions;
            actions.push_back(ActionNode::set("position.x", PropertyValue(42.0f)));
            actions.push_back(ActionNode::set("position.z", PropertyValue(42.0f)));
            multiLaw.setActionModel(ActionNode::sequence(std::move(actions)));

            // Before applying, x and z should be 0 (from defaults) and y is ~100 (from previous test)
            assert(multiLaw.applyTo(person) == Law::ApplicationResult::Applied);
            assert(nearf(person.position.x, 42.0f));
            assert(nearf(person.position.z, 42.0f));
            
            // Verify failure condition (AND fails if one is false)
            PropertyPath::parse("activeTool").setValue(person, PropertyValue(std::string("WrongTool")));
            assert(multiLaw.applyTo(person) == Law::ApplicationResult::ConditionsFailed);
        }
        // ------------------------------------------------------------------
        // 5. Verification of multiple-condition (All, Any) and multiple-action (Sequence) laws
        // ------------------------------------------------------------------
        {
            Law multiLaw("multi-tester");
            multiLaw.addAuthor(author);
            multiLaw.addAuthor(person);

            // Condition: (y < 0) AND (x > 10)
            auto c1 = ConditionNode::compare("position.y", ConditionNode::Op::Lt, PropertyValue(0.0));
            auto c2 = ConditionNode::compare("position.x", ConditionNode::Op::Gt, PropertyValue(10.0));
            multiLaw.setConditionModel(ConditionNode::all({c1, c2}));

            // Action: Sequence( set y=0, set x=10 )
            auto a1 = ActionNode::set("position.y", PropertyValue(0.0f));
            auto a2 = ActionNode::set("position.x", PropertyValue(10.0f));
            multiLaw.setActionModel(ActionNode::sequence({a1, a2}));

            Object testObj;
            
            // Fails x condition
            testObj.setPosition(glm::vec3(5.0f, -5.0f, 0.0f));
            assert(multiLaw.applyTo(testObj) == Law::ApplicationResult::ConditionsFailed);
            
            // Fails y condition
            testObj.setPosition(glm::vec3(15.0f, 5.0f, 0.0f));
            assert(multiLaw.applyTo(testObj) == Law::ApplicationResult::ConditionsFailed);
            
            // Passes both
            testObj.setPosition(glm::vec3(15.0f, -5.0f, 0.0f));
            assert(multiLaw.applyTo(testObj) == Law::ApplicationResult::Applied);
            assert(nearf(testObj.getPosition().y, 0.0f));
            assert(nearf(testObj.getPosition().x, 10.0f));

            // Test Any
            multiLaw.setConditionModel(ConditionNode::any({c1, c2}));
            
            // Passes y condition only
            testObj.setPosition(glm::vec3(5.0f, -5.0f, 0.0f));
            assert(multiLaw.applyTo(testObj) == Law::ApplicationResult::Applied);
            assert(nearf(testObj.getPosition().y, 0.0f));
            assert(nearf(testObj.getPosition().x, 10.0f));
        }

    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("law_model_test: ALL OK");
    return 0;
}
