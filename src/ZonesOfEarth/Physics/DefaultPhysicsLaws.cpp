#include "DefaultPhysicsLaws.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "Singularity/OntoMath/CurveModel.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

namespace Physics {

std::vector<std::shared_ptr<Law>> createDefaultPhysicsLaws() {
    std::vector<std::shared_ptr<Law>> laws;

    // 1. Gravity: Flow action over velocity (velocity := velocity + g * dt)
    auto gravity = std::make_shared<Law>("physics: gravity");
    gravity->setObjectID("physics-gravity");
    gravity->setActivation(Law::Activation::WhileTrue);
    gravity->setScope(Law::Scope::Everyone);
    
    auto gNode = std::make_shared<OntoMath::MathNode>();
    gNode->op = OntoMath::MathNode::Op::VectorConstruct;
    
    auto makeConstant = [](double val) {
        auto node = std::make_unique<OntoMath::MathNode>();
        node->op = OntoMath::MathNode::Op::ScalarLeaf;
        node->scalarForm = OntoMath::ScalarForm::constant(val);
        return node;
    };
    
    gNode->children.push_back(makeConstant(0.0));
    gNode->children.push_back(makeConstant(-9.81));
    gNode->children.push_back(makeConstant(0.0));
    
    ActionNode gAction = ActionNode::flow("velocity", OntoMath::Piecewise::continuous(gNode), MathBindings{});
    gravity->setActionModel(gAction);
    laws.push_back(gravity);

    // 2. Integration: Flow action over position (position := position + velocity * dt)
    auto integration = std::make_shared<Law>("physics: kinematics");
    integration->setObjectID("physics-kinematics");
    integration->setActivation(Law::Activation::WhileTrue);
    integration->setScope(Law::Scope::Everyone);

    auto vNode = std::make_shared<OntoMath::MathNode>();
    vNode->op = OntoMath::MathNode::Op::ValueLeaf;
    vNode->variableName = "v";

    MathBindings intBindings;
    intBindings["v"] = PropertyPath::parse("velocity");
    
    ActionNode intAction = ActionNode::flow("position", OntoMath::Piecewise::continuous(vNode), intBindings);
    integration->setActionModel(intAction);
    laws.push_back(integration);

    // 3. Acoustics: Convert collision into audio.synthesize
    auto acoustics = std::make_shared<Law>("physics: acoustics");
    acoustics->setObjectID("physics-acoustics");
    acoustics->setActivation(Law::Activation::OnEvent);
    acoustics->ecaLoop().eventType = "physics.collision";
    acoustics->setScope(Law::Scope::Everyone);

    ActionNode playAction = ActionNode::playAudio("acoustic.frequency", "acoustic.amplitude", "sine");
    acoustics->setActionModel(playAction);
    laws.push_back(acoustics);

    return laws;
}

} // namespace Physics
