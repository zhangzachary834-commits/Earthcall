// Law graph milestone test (LAW_AND_CREATION_SYSTEM.md, commit 7):
//   the authoring surface, minus the pixels.
//
// The window itself needs eyes; what a test CAN prove is everything beneath
// it: the shared card layout (the same geometry the in-scene SDF graph uses),
// the law -> cards flatten, and — critically — the exact mutation path the
// editor drives: copy the model, navigate by child indices, edit a field,
// recommit, and the recompiled law behaves per the edit.

#include "Singularity/Screen/CardTreeLayout.hpp"
#include "Singularity/Screen/LawGraphWindow.hpp"
#include "ConstructedBeing/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {

bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "law_graph_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "law_graph_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "law_graph_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        using Rendering::layoutCardTree;

        // ------------------------------------------------------------------
        // 1. The shared layout: chain, binary, n-ary.
        // ------------------------------------------------------------------
        {
            // Chain 0 -> 1 -> 2: one column per depth, single row.
            std::vector<std::vector<int>> kids{{1}, {2}, {}};
            auto slots = layoutCardTree(3, 0,
                [&](int i) { return static_cast<int>(kids[i].size()); },
                [&](int i, int k) { return kids[i][k]; });
            assert(slots[0].depth == 0 && slots[1].depth == 1 && slots[2].depth == 2);
            assert(nearf(slots[0].row, 0.0f) && nearf(slots[2].row, 0.0f));
        }
        {
            // Root with three leaves: leaves stack 0,1,2; root centers at 1.
            std::vector<std::vector<int>> kids{{1, 2, 3}, {}, {}, {}};
            auto slots = layoutCardTree(4, 0,
                [&](int i) { return static_cast<int>(kids[i].size()); },
                [&](int i, int k) { return kids[i][k]; });
            assert(nearf(slots[1].row, 0.0f));
            assert(nearf(slots[2].row, 1.0f));
            assert(nearf(slots[3].row, 2.0f));
            assert(nearf(slots[0].row, 1.0f));   // average of children
            assert(slots[1].parent == 0 && slots[3].parent == 0);
        }

        // ------------------------------------------------------------------
        // 2. flattenLaw: the floor law becomes cards.
        // ------------------------------------------------------------------
        Object author;
        Law floorLaw("ground-rest");
        floorLaw.addAuthor(author);
        floorLaw.setConditionModel(ConditionNode::compare(
            "position.y", ConditionNode::Op::Lt, PropertyValue(0.0)));
        floorLaw.setActionModel(ActionNode::set("position.y", PropertyValue(0.0)));

        auto cards = Rendering::flattenLaw(floorLaw, "enters-world");
        assert(cards.size() == 4);   // law + event + condition + action
        assert(cards[0].kind == Rendering::LawCard::Kind::Law);
        bool sawEvent = false, sawCondition = false, sawAction = false;
        for (const auto& card : cards) {
            if (card.kind == Rendering::LawCard::Kind::Event) {
                sawEvent = card.label == "on: enters-world";
            }
            if (card.kind == Rendering::LawCard::Kind::Condition) {
                sawCondition = card.label.find("position.y") != std::string::npos &&
                               card.label.find("<") != std::string::npos;
            }
            if (card.kind == Rendering::LawCard::Kind::Action) {
                sawAction = card.label.find("set position.y") != std::string::npos;
            }
        }
        assert(sawEvent && sawCondition && sawAction);
        // Every non-law card hangs off the law card.
        assert(cards[0].children.size() == 3);

        // ------------------------------------------------------------------
        // 3. The editor's mutation path: copy -> navigate -> edit -> recommit.
        //    Change the floor from 0 to 5; the recompiled law obeys the edit.
        // ------------------------------------------------------------------
        Object obj;
        obj.setPosition(glm::vec3(0.0f, 3.0f, 0.0f));
        assert(floorLaw.applyTo(obj) == Law::ApplicationResult::ConditionsFailed);

        ConditionModel model = *floorLaw.conditionModel();          // copy
        ConditionNode* node = Rendering::conditionAt(model, {});    // navigate
        assert(node != nullptr);
        node->operand = PropertyValue(5.0);                         // edit
        floorLaw.setConditionModel(std::move(model));               // recommit

        assert(floorLaw.applyTo(obj) == Law::ApplicationResult::Applied);   // 3 < 5
        assert(nearf(obj.getPosition().y, 0.0f));

        // Same path on the action side: raise the floor's rest height.
        ActionModel action = *floorLaw.actionModel();
        ActionNode* actionNode = Rendering::actionAt(action, {});
        assert(actionNode != nullptr);
        actionNode->operand = PropertyValue(1.0);
        floorLaw.setActionModel(std::move(action));

        obj.setPosition(glm::vec3(0.0f, 2.0f, 0.0f));
        assert(floorLaw.applyTo(obj) == Law::ApplicationResult::Applied);
        assert(nearf(obj.getPosition().y, 1.0f));

        // Out-of-range navigation fails cleanly.
        ConditionModel model2 = *floorLaw.conditionModel();
        assert(Rendering::conditionAt(model2, {4}) == nullptr);

        // ------------------------------------------------------------------
        // 4. Discoverability: the substrate's vocabulary is enumerable, so
        //    the authoring UI offers real property names instead of quizzing.
        // ------------------------------------------------------------------
        Object probe;
        bool sawPosition = false, sawShapeParam = false;
        for (Property* property : probe.listProperties()) {
            if (property->name() == "position") sawPosition = true;
            if (property->name() == "shape.majorR") sawShapeParam = true;
        }
        assert(sawPosition && sawShapeParam);

        // Laws are legible the same way (metalaw targeting via picker).
        bool sawEnabled = false;
        for (Property* property : floorLaw.listProperties()) {
            if (property->name() == "enabled") sawEnabled = true;
        }
        assert(sawEnabled);

        // Clearing models is a real state (used by the editor's remove buttons).
        Law bare("bare");
        bare.addAuthor(author);
        bare.setActionModel(ActionNode::set("position.y", PropertyValue(0.0)));
        bare.clearActionModel();
        assert(!bare.hasActionModel());
        Object idle;
        idle.setPosition(glm::vec3(0.0f, -1.0f, 0.0f));
        assert(bare.applyTo(idle) == Law::ApplicationResult::NoAction);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("law_graph_test: ALL OK");
    return 0;
}
