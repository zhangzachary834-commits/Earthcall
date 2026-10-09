#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "json.hpp"
#include <cassert>
#include <iostream>

using json = nlohmann::json;

int main() {
    std::cout << "Testing ActionModel...\n";
    {
        ActionNode setNode = ActionNode::set("position.x", 10.5);
        assert(setNode.kind == ActionNode::Kind::Set);
        assert(setNode.path.toString() == "position.x");
        assert(std::get<double>(setNode.operand) == 10.5);

        ActionNode addNode = ActionNode::add("health", -10.0);
        assert(addNode.kind == ActionNode::Kind::Add);
        assert(addNode.path.toString() == "health");
        assert(std::get<double>(addNode.operand) == -10.0);
    }
    {
        ActionNode seq = ActionNode::sequence({
            ActionNode::set("a", 1.0),
            ActionNode::set("b", 2.0)
        });
        assert(seq.kind == ActionNode::Kind::Sequence);
        assert(seq.children.size() == 2);
        assert(seq.children[0].kind == ActionNode::Kind::Set);

        ActionNode par = ActionNode::parallel({
            ActionNode::add("c", 3.0)
        });
        assert(par.kind == ActionNode::Kind::Parallel);
        assert(par.children.size() == 1);
    }
    {
        ActionNode original = ActionNode::sequence({
            ActionNode::set("pos", 42.0),
            ActionNode::publish("event-type", "@subject", "@object")
        });
        json j = original.toJson();
        ActionNode restored = ActionNode::fromJson(j);
        assert(restored.kind == ActionNode::Kind::Sequence);
        assert(restored.children.size() == 2);
        assert(restored.children[0].kind == ActionNode::Kind::Set);
        assert(restored.children[0].path.toString() == "pos");
        assert(restored.children[1].kind == ActionNode::Kind::Publish);
        assert(restored.children[1].eventType == "event-type");
        assert(restored.children[1].publishSubject == "@subject");
    }
    {
        ActionNode setNode = ActionNode::set("score", 200.0);
        auto executor = setNode.compile();
        assert(executor != nullptr);
    }
    {
        ActionNode node = ActionNode::set("health", 50.0);
        assert(node.describe() == "set health 50");

        ActionNode glowNode = ActionNode::set("glow", 1.0);
        assert(glowNode.describe() == "set glow 1");

        ActionNode colorNode = ActionNode::set("color", glm::vec3(1.0f, 0.0f, 0.0f));
        assert(colorNode.describe() == "set color (1, 0, 0)");

        ActionNode toolNode = ActionNode::set("activeTool", "3DShapeGenerator");
        assert(toolNode.describe() == "set activeTool 3DShapeGenerator");

        ActionNode titleNode = ActionNode::set("title", "Hello World");
        assert(titleNode.describe() == "set title \"Hello World\"");

        ActionNode boolNode = ActionNode::set("enabled", true);
        assert(boolNode.describe() == "set enabled true");

        ActionNode addNode = ActionNode::add("health", -10.0);
        assert(addNode.describe() == "add -10 to health");

        ActionNode scaleNode = ActionNode::scale("speed", 2.0);
        assert(scaleNode.describe() == "scale speed by 2");

        ActionNode propNode = ActionNode::addProperty("target", "custom", 42.0);
        assert(propNode.describe() == "grant property 'custom' = 42");

        auto rev = node.reversibility();
        assert(rev.exact == false);
        assert(!rev.obstacles.empty());
    }
    std::cout << "ActionModel tests passed!\n";
    return 0;
}
