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
        assert(PropertyPath::parse("shape.fillet").getValue(inside, fillet));
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
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("law_model_test: ALL OK");
    return 0;
}
