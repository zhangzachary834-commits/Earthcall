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

    // -----------------------------------------------------------------------
    // Law: physics-acoustics-decay
    // -----------------------------------------------------------------------
    auto decay = std::make_shared<Law>("physics-acoustics-decay");
    decay->setObjectID("physics-acoustics-decay");
    decay->setActivation(Law::Activation::WhileTrue);
    decay->setScope(Law::Scope::Everyone);
    
    ConditionNode timeCond;
    timeCond.kind = ConditionNode::Kind::Compare;
    timeCond.path = PropertyPath::parse("time.sinceApplied");
    timeCond.operand = PropertyValue(0.5);
    timeCond.op = ConditionNode::Op::Gt;
    decay->setConditionModel(timeCond);

    decay->setActionModel(ActionNode::destroy());
    laws.push_back(decay);

    // -----------------------------------------------------------------------
    // Law: physics-acoustics-occlusion
    // -----------------------------------------------------------------------
    auto occ = std::make_shared<Law>("physics-acoustics-occlusion");
    occ->setObjectID("physics-acoustics-occlusion");
    occ->setActivation(Law::Activation::WhileTrue);
    occ->setScope(Law::Scope::Everyone);

    ConditionNode occCond;
    occCond.kind = ConditionNode::Kind::Compare;
    occCond.path = PropertyPath::parse("acoustic.isSoundEmitter");
    occCond.operand = PropertyValue(std::string("true"));
    occCond.op = ConditionNode::Op::Eq;
    occ->setConditionModel(occCond);

    auto occNode = std::make_unique<OntoMath::MathNode>();
    occNode->op = OntoMath::MathNode::Op::ValueLeaf;
    occNode->variableName = "c";

    // If occluded (c == 1.0), cutoff = 5000.0 (muffled)
    // If not occluded (c == 0.0), cutoff = 22000.0 (open)
    // Cutoff = 22000.0 - (c * 17000.0)
    auto occScaled = std::make_unique<OntoMath::MathNode>();
    occScaled->op = OntoMath::MathNode::Op::Scale;
    
    auto occMultiplier = std::make_unique<OntoMath::MathNode>();
    occMultiplier->op = OntoMath::MathNode::Op::ScalarLeaf;
    occMultiplier->scalarForm = OntoMath::ScalarForm::constant(17000.0);
    
    occScaled->children.push_back(std::move(occMultiplier));
    occScaled->children.push_back(std::move(occNode));
    
    auto occBase = std::make_unique<OntoMath::MathNode>();
    occBase->op = OntoMath::MathNode::Op::ScalarLeaf;
    occBase->scalarForm = OntoMath::ScalarForm::constant(22000.0);
    
    auto occSub = std::make_shared<OntoMath::MathNode>();
    occSub->op = OntoMath::MathNode::Op::Sub;
    occSub->children.push_back(std::move(occBase));
    occSub->children.push_back(std::move(occScaled));

    MathBindings occBindings;
    occBindings["c"] = PropertyPath::parse("world.occlusionToCamera");
    
    ActionNode occAction = ActionNode::flow("acoustic.lowpassCutoff", OntoMath::Piecewise::continuous(occSub), occBindings);
    occ->setActionModel(occAction);
    laws.push_back(occ);

    // -----------------------------------------------------------------------
    // Law: physics-acoustics-vibrato (LFO)
    // -----------------------------------------------------------------------
    auto vibrato = std::make_shared<Law>("physics-acoustics-vibrato");
    vibrato->setObjectID("physics-acoustics-vibrato");
    vibrato->setActivation(Law::Activation::WhileTrue);
    vibrato->setScope(Law::Scope::Everyone);

    ConditionNode vibCond;
    vibCond.kind = ConditionNode::Kind::Compare;
    vibCond.path = PropertyPath::parse("acoustic.isSoundEmitter");
    vibCond.operand = PropertyValue(std::string("true"));
    vibCond.op = ConditionNode::Op::Eq;
    vibrato->setConditionModel(vibCond);

    // base + sin(t * 15.0) * 20.0
    // sinusoid takes (amplitude, frequency, phase, bias, var)
    // frequency in Hz is 15.0 / 2pi = 2.387324
    auto lfoNode = std::make_unique<OntoMath::MathNode>();
    lfoNode->op = OntoMath::MathNode::Op::ScalarLeaf;
    lfoNode->scalarForm = OntoMath::ScalarForm::sinusoid(20.0, 2.38732414, 0.0, 0.0, "t");

    auto baseNode = std::make_unique<OntoMath::MathNode>();
    baseNode->op = OntoMath::MathNode::Op::ValueLeaf;
    baseNode->variableName = "base";

    auto addNode = std::make_shared<OntoMath::MathNode>();
    addNode->op = OntoMath::MathNode::Op::Add;
    addNode->children.push_back(std::move(baseNode));
    addNode->children.push_back(std::move(lfoNode));

    MathBindings vibBindings;
    vibBindings["t"] = PropertyPath::parse("time.sinceApplied");
    vibBindings["base"] = PropertyPath::parse("acoustic.baseFrequency");

    ActionNode vibAction = ActionNode::flow("acoustic.frequency", OntoMath::Piecewise::continuous(addNode), vibBindings);
    vibrato->setActionModel(vibAction);
    laws.push_back(vibrato);

    return laws;
}

} // namespace Physics
