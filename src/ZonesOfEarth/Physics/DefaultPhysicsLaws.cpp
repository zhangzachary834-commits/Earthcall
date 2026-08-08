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

    // 3. Acoustics: Convert collision into sound-emitter spawning
    auto acoustics = std::make_shared<Law>("physics: acoustics");
    acoustics->setObjectID("physics-acoustics");
    acoustics->setActivation(Law::Activation::OnEvent);
    acoustics->ecaLoop().eventType = "physics.collision";
    acoustics->setScope(Law::Scope::Everyone);

    ActionNode spawnEmitter = ActionNode::spawn("concept-sound-emitter");
    acoustics->setActionModel(spawnEmitter);
    laws.push_back(acoustics);

    // 4. Acoustics Envelope: ADSR WhileTrue law for sound-emitters
    auto adsr = std::make_shared<Law>("physics: acoustics envelope");
    adsr->setObjectID("physics-acoustics-envelope");
    adsr->setActivation(Law::Activation::WhileTrue);
    adsr->setScope(Law::Scope::Everyone);

    ConditionNode conceptCond;
    conceptCond.kind = ConditionNode::Kind::Related;
    conceptCond.relationType = "generated-from";
    conceptCond.otherId = "concept-sound-emitter";

    adsr->setConditionModel(conceptCond);

    // Envelope: 1.0 - (t * 2.0)  -> decays over 0.5 seconds
    auto tNode = std::make_unique<OntoMath::MathNode>();
    tNode->op = OntoMath::MathNode::Op::ValueLeaf;
    tNode->variableName = "t";

    auto twoNode = std::make_unique<OntoMath::MathNode>();
    twoNode->op = OntoMath::MathNode::Op::ScalarLeaf;
    twoNode->scalarForm = OntoMath::ScalarForm::constant(2.0);

    // t * 2.0
    auto tScaled = std::make_unique<OntoMath::MathNode>();
    tScaled->op = OntoMath::MathNode::Op::Scale;
    tScaled->children.push_back(std::move(twoNode));
    tScaled->children.push_back(std::move(tNode));

    auto oneNode = std::make_unique<OntoMath::MathNode>();
    oneNode->op = OntoMath::MathNode::Op::ScalarLeaf;
    oneNode->scalarForm = OntoMath::ScalarForm::constant(1.0);

    // 1.0 - (t * 2.0)
    auto subNode = std::make_shared<OntoMath::MathNode>();
    subNode->op = OntoMath::MathNode::Op::Sub;
    subNode->children.push_back(std::move(oneNode));
    subNode->children.push_back(std::move(tScaled));

    MathBindings adsrBindings;
    adsrBindings["t"] = PropertyPath::parse("time.sinceApplied");

    ActionNode envelopeAction = ActionNode::flow("acoustic.amplitude", OntoMath::Piecewise::continuous(subNode), adsrBindings);
    adsr->setActionModel(envelopeAction);
    laws.push_back(adsr);

    // 5. Acoustics Decay: Destroy the emitter after 0.5s
    auto decay = std::make_shared<Law>("physics: acoustics decay");
    decay->setObjectID("physics-acoustics-decay");
    decay->setActivation(Law::Activation::WhileTrue);
    decay->setScope(Law::Scope::Everyone);

    ConditionNode timeCond;
    timeCond.kind = ConditionNode::Kind::Compare;
    timeCond.op = ConditionNode::Op::Gt;
    timeCond.path = PropertyPath::parse("time.sinceApplied");
    timeCond.operand = 0.5;

    ConditionNode decayAll;
    decayAll.kind = ConditionNode::Kind::All;
    decayAll.children.push_back(conceptCond);
    decayAll.children.push_back(timeCond);

    decay->setConditionModel(decayAll);

    ActionNode destroyAction = ActionNode::destroy();
    decay->setActionModel(destroyAction);
    laws.push_back(decay);

    return laws;
}

} // namespace Physics
