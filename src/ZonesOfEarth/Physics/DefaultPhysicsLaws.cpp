#include "DefaultPhysicsLaws.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "Singularity/OntoMath/CurveModel.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

namespace Physics {

namespace {

// A one-piece, everywhere-defined function of an authored ScalarForm.
//
// Every acoustic law here is "this quantity IS f(t)" — a value, not a rate —
// so each is one ScalarLeaf over a multivariate ScalarForm. That is also the
// only correct way to write scalar multiplication in this algebra: a Term
// carries its own coefficient, so 20·t is ScalarForm::variable("t", 1, 20).
//
// The MathNode::Op::Scale route (ScalarLeaf(20) ⊗ ValueLeaf("t")) that these
// laws used to take is TYPE-INVALID: Op::Scale evaluates only (double, vec3)
// and (vec3, double). Two scalars fall through to nullopt, which poisons the
// enclosing Sub, which makes the whole Piecewise undefined, which makes the
// action write nothing — silently, for every value of t. MathNode::typeOf
// would have said so; nothing calls it.
std::shared_ptr<OntoMath::MathNode> scalarNode(OntoMath::ScalarForm form) {
    auto node = std::make_shared<OntoMath::MathNode>();
    node->op = OntoMath::MathNode::Op::ScalarLeaf;
    node->scalarForm = std::move(form);
    return node;
}

// "the subject is a sound emitter" — the guard every acoustic law needs.
ConditionNode isSoundEmitter() {
    ConditionNode c;
    c.kind = ConditionNode::Kind::Compare;
    c.path = PropertyPath::parse("acoustic.isSoundEmitter");
    c.operand = PropertyValue(std::string("true"));
    c.op = ConditionNode::Op::Eq;
    return c;
}

} // namespace

std::vector<std::shared_ptr<Law>> createDefaultPhysicsLaws() {
    std::vector<std::shared_ptr<Law>> laws;

    // 1. Gravity: Flow action over velocity (velocity := velocity + g * dt)
    auto gravity = std::make_shared<Law>("physics: gravity");
    gravity->setLawIdentifier("physics-gravity");
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
    integration->setLawIdentifier("physics-kinematics");
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

    // -----------------------------------------------------------------------
    // Law: physics-acoustics — a collision becomes a sound emitter.
    //
    // EDGES, NOT LEVELS. This fires on "contact-began", the false->true edge
    // Physics::updateBodies synthesizes from its touching-pair set, and not on
    // "collision", which is republished every frame an overlap persists. On
    // the level, with Scope::Everyone and no condition, this law was a
    // per-frame spawner: two boxes resting against each other minted roughly
    // 120 Objects a second, forever.
    //
    // Scope::Subject, not Everyone: the being this is about is the one that
    // was struck — the event's subject — not every being in the world. Under
    // Scope::Everyone the drain sweeps the whole Universe and applies the law
    // once per being, so a single collision spawned an emitter for every
    // object, law, relation and material in the world.
    //
    // NOTE FOR THE CALLER: an OnEvent law hears nothing until it is bound.
    // LawManager::add compiles Rete terminals only for continuous laws, so
    // whoever registers this must also call
    //     lawManager.bindTrigger(law->getIdentifier(), "contact-began");
    // and give the law an author (an unauthored law returns Unauthored from
    // applyTo and never fires). ecaLoop().eventType below records the
    // intended trigger; it does not perform the binding.
    // -----------------------------------------------------------------------
    auto acoustics = std::make_shared<Law>("physics: acoustics");
    acoustics->setLawIdentifier("physics-acoustics");
    acoustics->setActivation(Law::Activation::OnEvent);
    acoustics->ecaLoop().eventType = "contact-began";
    acoustics->setScope(Law::Scope::Subject);

    ActionNode spawnEmitter = ActionNode::spawn("concept-sound-emitter");
    acoustics->setActionModel(spawnEmitter);
    laws.push_back(acoustics);

    // -----------------------------------------------------------------------
    // Law: physics-acoustics-envelope — ADSR, as one law.
    //
    // There were TWO laws with this stable id, both pushed, both writing
    // acoustic.amplitude with different formulas every tick. Law text naming
    // "physics-acoustics-envelope" resolved to whichever matched first and the
    // other was unaddressable — it could not be inspected, edited or disabled.
    // An envelope is one intention; this is one law.
    //
    // Map, not Flow. Flow means `path := path + f(bindings)·dt` — the authored
    // model is a RATE, integrated each tick. An envelope is a VALUE at time t.
    // Written as Flow, `amplitude += (1 - 2t)·dt` starting at 1.0 RISES to
    // ~1.25 by t = 0.5: the integral of the intended curve, not the curve.
    //
    //   attack  t in [0, 0.05)      amplitude = 20·t        (0 -> 1)
    //   decay   t in [0.05, 0.5]    amplitude = 1.111 - 2.222·t   (1 -> 0)
    //   outside                     UNDEFINED — the Piecewise writes nothing,
    //                               which is the envelope ending, not failing.
    // -----------------------------------------------------------------------
    auto envelope = std::make_shared<Law>("physics: acoustics envelope");
    envelope->setLawIdentifier("physics-acoustics-envelope");
    envelope->setActivation(Law::Activation::WhileTrue);
    envelope->setScope(Law::Scope::Everyone);
    envelope->setConditionModel(isSoundEmitter());

    OntoMath::Piecewise::Piece attackPiece;
    attackPiece.hasLo = true; attackPiece.lo = 0.0;  attackPiece.includeLo = true;
    attackPiece.hasHi = true; attackPiece.hi = 0.05; attackPiece.includeHi = false;
    attackPiece.mathNode = scalarNode(OntoMath::ScalarForm::variable("t", 1.0, 20.0));

    OntoMath::Piecewise::Piece decayPiece;
    decayPiece.hasLo = true; decayPiece.lo = 0.05; decayPiece.includeLo = true;
    decayPiece.hasHi = true; decayPiece.hi = 0.5;  decayPiece.includeHi = true;
    decayPiece.mathNode = scalarNode(
        OntoMath::ScalarForm::constant(1.11111111)
            .plus(OntoMath::ScalarForm::variable("t", 1.0, -2.22222222)));

    OntoMath::Piecewise envelopeFunc;
    envelopeFunc.inputVariable = "t";
    envelopeFunc.pieces.push_back(attackPiece);
    envelopeFunc.pieces.push_back(decayPiece);

    MathBindings envBindings;
    envBindings["t"] = PropertyPath::parse("time.sinceApplied");

    envelope->setActionModel(
        ActionNode::map("acoustic.amplitude", envelopeFunc, envBindings));
    laws.push_back(envelope);

    // -----------------------------------------------------------------------
    // Law: physics-acoustics-vibrato — an LFO on the carrier frequency.
    //
    // Map, not Flow, for the same reason as the envelope: an LFO is a value at
    // time t. As Flow, `frequency += (base + 20·sin(...))·dt` ramped 880 Hz to
    // ~1320 Hz in half a second — a monotonic pitch sweep, not a vibrato.
    //
    //   frequency = baseFrequency + 20·sin(2π·2.387·t)      (±20 Hz at ~15 rad/s)
    //
    // `base` is read from acoustic.baseFrequency, which the sound-emitter
    // concept now actually seeds — the law bound it before anything set it,
    // so the read failed and the action wrote nothing on every tick.
    // -----------------------------------------------------------------------
    auto vibrato = std::make_shared<Law>("physics: acoustics vibrato");
    vibrato->setLawIdentifier("physics-acoustics-vibrato");
    vibrato->setActivation(Law::Activation::WhileTrue);
    vibrato->setScope(Law::Scope::Everyone);
    vibrato->setConditionModel(isSoundEmitter());

    MathBindings vibBindings;
    vibBindings["t"] = PropertyPath::parse("time.sinceApplied");
    vibBindings["base"] = PropertyPath::parse("acoustic.baseFrequency");

    vibrato->setActionModel(ActionNode::map(
        "acoustic.frequency",
        OntoMath::Piecewise::continuous(scalarNode(
            OntoMath::ScalarForm::sinusoid(20.0, 2.38732414, 0.0, 0.0, "t")
                .plus(OntoMath::ScalarForm::variable("base")))),
        vibBindings));
    laws.push_back(vibrato);

    // -----------------------------------------------------------------------
    // Law: physics-acoustics-occlusion — sound muffles behind geometry.
    //
    //   lowpassCutoff = 22000 - 17000·c,  c = @world.occlusionToCamera in {0,1}
    //   open  -> 22000 Hz     blocked -> 5000 Hz
    //
    // Map, not Flow: a cutoff is a value, not a rate. As Flow the cutoff
    // climbed ~366 Hz per frame without bound.
    //
    // `c` is a WORLD READING, registered by the Audio channel (see
    // MathBinding.hpp and AudioSystem::setupAudioEventListeners). The law
    // substrate does not know what occlusion is; it knows only that some
    // channel answers this name.
    // -----------------------------------------------------------------------
    auto occlusion = std::make_shared<Law>("physics: acoustics occlusion");
    occlusion->setLawIdentifier("physics-acoustics-occlusion");
    occlusion->setActivation(Law::Activation::WhileTrue);
    occlusion->setScope(Law::Scope::Everyone);
    occlusion->setConditionModel(isSoundEmitter());

    MathBindings occBindings;
    occBindings["c"] = PropertyPath::parse("@world.occlusionToCamera");

    occlusion->setActionModel(ActionNode::map(
        "acoustic.lowpassCutoff",
        OntoMath::Piecewise::continuous(scalarNode(
            OntoMath::ScalarForm::constant(22000.0)
                .plus(OntoMath::ScalarForm::variable("c", 1.0, -17000.0)))),
        occBindings));
    laws.push_back(occlusion);

    // -----------------------------------------------------------------------
    // Law: physics-acoustics-decay — the emitter stops existing when it stops
    // sounding. The envelope reaches zero at t = 0.5; this unmakes the being.
    //
    // THE GUARD IS NOT OPTIONAL. This law is Scope::Everyone and its action is
    // Destroy. With `time.sinceApplied > 0.5` as its ONLY condition it says
    // "destroy every being in the world half a second after its condition
    // starts holding" — it was inert only because time.sinceApplied does not
    // resolve inside a condition today (see below), and would have emptied the
    // world the moment that was fixed.
    //
    // KNOWN INERT, NOT MY FILE: Law::applyTo evaluates conditionsSatisfied()
    // BEFORE it arms Universe::OnsetScope, and lawGetTime returns false
    // without an onset — so `time.sinceApplied` never resolves in a condition
    // and this law never holds. The fix is to arm the onset scope before the
    // condition phase, in Law.cpp. Until then, emitters are spawned and never
    // unmade. Reported, deliberately not worked around here: a second,
    // parallel clock authored onto the emitter would collide with that fix.
    // -----------------------------------------------------------------------
    auto decay = std::make_shared<Law>("physics: acoustics decay");
    decay->setLawIdentifier("physics-acoustics-decay");
    decay->setActivation(Law::Activation::WhileTrue);
    decay->setScope(Law::Scope::Everyone);

    ConditionNode spent;
    spent.kind = ConditionNode::Kind::Compare;
    spent.path = PropertyPath::parse("time.sinceApplied");
    spent.operand = PropertyValue(0.5);
    spent.op = ConditionNode::Op::Gt;

    ConditionNode decayCond;
    decayCond.kind = ConditionNode::Kind::All;
    decayCond.children.push_back(isSoundEmitter());
    decayCond.children.push_back(spent);
    decay->setConditionModel(decayCond);

    decay->setActionModel(ActionNode::destroy());
    laws.push_back(decay);

    return laws;
}

} // namespace Physics
