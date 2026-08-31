#include "ControlPatterns.hpp"

#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <memory>

namespace Singularity {
namespace Input {

namespace {

// The channel every archetype reads its gesture from. Written once so a rename
// is one edit rather than six, and so the dependency is visible: the control
// laws know the channel by NAME, exactly as a Person's law text does — not by
// pointer, not by include.
constexpr const char* kChannel = "@interaction-channel";

std::string channelPath(const char* leaf) {
    return std::string(kChannel) + "." + leaf;
}

// A Piecewise defined everywhere over `variable`. Interval bounds are what
// give an authored function its DURATION or its RANGE; a control response that
// holds everywhere says so by carrying one unbounded piece.
OntoMath::Piecewise everywhere(OntoMath::ScalarForm form, const std::string& variable) {
    OntoMath::Piecewise f = OntoMath::Piecewise::continuous(
        OntoMath::MathNode::fromLegacyExpression(std::move(form)));
    f.inputVariable = variable;
    return f;
}

// Every archetype opens the same way: a first-mover law with a stable slug,
// authored, listening for one event kind. setLawIdentifier and NOT setObjectID
// — Law overrides getIdentifier() to return _lawId, so setObjectID writes a
// field nothing reads back and leaves the law answering to a generated "law-N"
// that changes between runs. Every first mover built in C++ made that mistake
// once.
std::shared_ptr<Law> beginPattern(const std::string& name, const std::string& id,
                                  const std::string& eventType, Singular& author) {
    auto law = std::make_shared<FirstMoverLaw>(name);
    law->setLawIdentifier(id);
    law->addAuthor(author);
    law->setActivation(Law::Activation::OnEvent);
    law->setScope(Law::Scope::Subject);
    law->ecaLoop().eventType = eventType;
    return law;
}

ConditionNode inCategory(const char* categoryId) {
    return ConditionNode::related(Control::kInstanceOf, categoryId);
}

} // namespace

// ---------------------------------------------------------------------------
// The taxonomy
// ---------------------------------------------------------------------------

void seedControlCategories(CategoryManager& categories, Singular& author) {
    auto root = categories.create(Control::kCategoryRoot);
    if (!root) return;

    const char* leaves[] = {Control::kCategoryButton, Control::kCategoryToggle,
                            Control::kCategorySlider, Control::kCategoryTuner,
                            Control::kCategoryKey};
    for (const char* leaf : leaves) {
        auto child = categories.create(leaf);
        if (!child) continue;
        // subcategory-of is DIRECTED: the child claims the parent, never the
        // reverse. Direction is what keeps the graph acyclic and what makes
        // "is x a control" a question with an answer (AUTHORED_CATEGORIES.md
        // §7). Re-seeding adds a duplicate edge rather than a contradiction,
        // which is why this is safe to call on every boot.
        Universe::instance().addRelation(std::make_shared<Relation>(
            Control::kSubcategoryOf, *child, *root, true, 1.0f));
    }

    // A category is a CLAIM about what things are. Sign it.
    Universe::instance().addRelation(std::make_shared<Relation>(
        "authored-by", *root, author, true, 1.0f));
}

bool makeControl(Object& being, const std::string& categoryId,
                 const CategoryManager& categories, double initial, double step) {
    auto category = categories.get(categoryId);
    // No category, no membership. Minting one here would be authoring a
    // taxonomy on the Person's behalf, from inside a helper, unsigned.
    if (!category) return false;

    being.setDynamicProperty(Control::kValue, PropertyValue(initial));
    being.setDynamicProperty(Control::kStep, PropertyValue(step));
    being.setDynamicProperty(Control::kOn, PropertyValue(false));

    Universe::instance().addRelation(std::make_shared<Relation>(
        Control::kInstanceOf, being, *category, true, 1.0f));
    return true;
}

// ---------------------------------------------------------------------------
// §6a — Button
// ---------------------------------------------------------------------------

std::shared_ptr<Law> createButtonLaw(Singular& author) {
    auto law = beginPattern("Control: Button", "control-button-law",
                            "object-clicked", author);
    law->setConditionModel(inCategory(Control::kCategoryButton));
    // Subject token "" = the law's subject, which under Scope::Subject is the
    // being that was clicked. The button announces; it does not act.
    law->setActionModel(ActionNode::publish(Control::kActivated));
    return law;
}

// ---------------------------------------------------------------------------
// §6b — Toggle
//
// ONE law, and the reason is a finding rather than a preference.
//
// This was authored first as the obvious PAIR — "if off, turn on" and "if on,
// turn off" — on the grounds that the branch belongs in the condition calculus
// and that a Person should be able to disable the off-switch without disabling
// the on-switch. Both of those are true, and the pair does not work, because
// the law network CASCADES: the on-law writes controlOn, the write marks the
// Rete state fact dirty, the dirty fact re-activates the off-law's Compare
// terminal in the next chain round, and the off-law fires within the same
// tick. One click, both laws, no net change. kMaxChainRounds bounds the loop;
// it does not make it wrong less often.
//
// The general rule this is an instance of: TWO LAWS WHOSE ACTIONS SATISFY EACH
// OTHER'S CONDITIONS ARE A LOOP, not a branch. A branch needs a condition its
// own action cannot invalidate — which for a toggle is impossible by
// definition, because flipping the state IS the action.
//
// So the flip goes into the mathematics, where it cannot re-trigger anything:
// controlOn := 1 - o. The condition tests only category membership, which no
// action here touches, so the state change re-activates nothing. bool is an
// arithmetic alternative of PropertyValue, so it reads as 0/1 and the written
// double coerces back through lawSetValue's coerceLike.
// ---------------------------------------------------------------------------

std::shared_ptr<Law> createToggleLaw(Singular& author) {
    auto law = beginPattern("Control: Toggle", "control-toggle-law",
                            "object-clicked", author);
    law->setConditionModel(inCategory(Control::kCategoryToggle));

    OntoMath::ScalarForm flip = OntoMath::ScalarForm::constant(1.0).plus(
        OntoMath::ScalarForm::variable("o").scaled(-1.0));
    MathBindings bindings{{"o", PropertyPath::parse(Control::kOn)}};
    law->setActionModel(ActionNode::sequence(
        {ActionNode::map(Control::kOn, everywhere(std::move(flip), "o"),
                         std::move(bindings)),
         ActionNode::publish(Control::kActivated)}));
    return law;
}

// ---------------------------------------------------------------------------
// §6c — Slider
// ---------------------------------------------------------------------------

std::shared_ptr<Law> createSliderLaw(Singular& author) {
    auto law = std::make_shared<FirstMoverLaw>("Control: Slider");
    law->setLawIdentifier("control-slider-law");
    law->addAuthor(author);
    // Dragging is a LEVEL, not an edge: it is true for as long as the Person
    // holds the pointer down on the slider, and the value must move on every
    // tick of that. WhileTrue is the activation phase built for exactly this,
    // and publishing a per-frame "still dragging" event instead would be the
    // event-as-level bug.
    law->setActivation(Law::Activation::WhileTrue);
    law->setScope(Law::Scope::Everyone);
    law->setConditionModel(ConditionNode::all(
        {inCategory(Control::kCategorySlider),
         ConditionNode::compare("@world.pointerPressedOn", ConditionNode::Op::Eq,
                                PropertyValue(true))}));

    // dcontrolValue/dt = d · s — the RATE form. Flow integrates it each tick,
    // so the value the Person sees is the exact antiderivative of what was
    // authored rather than a per-frame delta accumulated by hand.
    OntoMath::ScalarForm rate =
        OntoMath::ScalarForm::variable("d").times(OntoMath::ScalarForm::variable("s"));
    MathBindings bindings{
        {"d", PropertyPath::parse(channelPath("dragX"))},
        {"s", PropertyPath::parse(Control::kStep)},
    };
    law->setActionModel(
        ActionNode::flow(Control::kValue, everywhere(std::move(rate), "d"),
                         std::move(bindings)));
    return law;
}

// ---------------------------------------------------------------------------
// §6d — Tuner
// ---------------------------------------------------------------------------

std::shared_ptr<Law> createTunerLaw(Singular& author) {
    auto law = beginPattern("Control: Tuner", "control-tuner-law",
                            "object-scrolled", author);
    law->setConditionModel(inCategory(Control::kCategoryTuner));

    // controlValue := v + s·n. The wheel is the one traditional input that
    // arrives as a signed quantity rather than a gesture, so it reaches
    // authored mathematics with nothing in between.
    OntoMath::ScalarForm next =
        OntoMath::ScalarForm::variable("v").plus(
            OntoMath::ScalarForm::variable("s").times(OntoMath::ScalarForm::variable("n")));
    MathBindings bindings{
        {"v", PropertyPath::parse(Control::kValue)},
        {"s", PropertyPath::parse(Control::kStep)},
        {"n", PropertyPath::parse(channelPath("scrollY"))},
    };
    law->setActionModel(
        ActionNode::map(Control::kValue, everywhere(std::move(next), "n"),
                        std::move(bindings)));
    return law;
}

// ---------------------------------------------------------------------------
// §6e — Key command
// ---------------------------------------------------------------------------

std::shared_ptr<Law> createKeyCommandLaw(Singular& author, const std::string& key) {
    auto law = beginPattern("Control: Key \"" + key + "\"",
                            "control-key-" + key + "-law", "key-pressed", author);
    law->setConditionModel(ConditionNode::all(
        {inCategory(Control::kCategoryKey),
         ConditionNode::compare(channelPath("lastKey"), ConditionNode::Op::Eq,
                                PropertyValue(key))}));
    law->setActionModel(ActionNode::publish(Control::kActivated));
    return law;
}

// ---------------------------------------------------------------------------
// §6f — Hover response
// ---------------------------------------------------------------------------

std::shared_ptr<Law> createHoverResponseLaw(Singular& author) {
    auto law = std::make_shared<FirstMoverLaw>("Control: Hover response");
    law->setLawIdentifier("control-hover-response-law");
    law->addAuthor(author);
    law->setActivation(Law::Activation::WhileTrue);
    law->setScope(Law::Scope::Everyone);
    law->setConditionModel(ConditionNode::compare(
        "@world.pointerOver", ConditionNode::Op::Eq, PropertyValue(true)));
    // NO ACTION MODEL, on purpose. What being pointed at should LOOK like is
    // the authored half, and a channel that answered it would have taken the
    // ontology's job (INTERACTION_AS_LAW.md §3). Law::applyTo answers NoAction
    // and the law sits in the Law Graph as a written invitation — a condition
    // with an empty right-hand side, waiting for a Person to fill it in.
    //
    // Disabled on registration for the same reason: a law with nothing to do
    // should not be sweeping the world every tick until someone gives it
    // something to do.
    law->setEnabled(false);
    return law;
}


// ---------------------------------------------------------------------------
// Art & Stroke Patterns
// ---------------------------------------------------------------------------

void seedArtCategories(CategoryManager& categories, Singular& author) {
    auto root = categories.create(Control::kCategoryArtRoot);
    if (!root) return;

    auto stroke = categories.create(Control::kCategoryStroke);
    if (stroke) {
        Universe::instance().addRelation(std::make_shared<Relation>(
            Control::kSubcategoryOf, *stroke, *root, true, 1.0f));
    }
    Universe::instance().addRelation(std::make_shared<Relation>(
        "authored-by", *root, author, true, 1.0f));
}

std::shared_ptr<Law> createStrokeDrawingLaw(Singular& author) {
    auto law = std::make_shared<FirstMoverLaw>("Art: Draw Stroke");
    law->setLawIdentifier("art-stroke-draw-law");
    law->addAuthor(author);
    law->setActivation(Law::Activation::WhileTrue);
    law->setScope(Law::Scope::Everyone);
    law->setConditionModel(ConditionNode::all({
        ConditionNode::compare("@interaction-channel.leftDown", ConditionNode::Op::Eq, PropertyValue(true)),
        ConditionNode::compare("@creation-channel.active3DMode", ConditionNode::Op::Eq, PropertyValue(std::string("Draw"))),
        ConditionNode::compare("@interaction-channel.dragging", ConditionNode::Op::Eq, PropertyValue(true))
    }));

    ActionNode createSegment = ActionNode::create(1, "art.stroke.segment", {
        ActionNode::set("scale", PropertyValue(glm::vec3(0.08f))),
        ActionNode::set("color", PropertyValue(glm::vec3(1.0f, 0.8f, 0.2f))),
        ActionNode::addRelation("", Control::kCategoryStroke, Control::kInstanceOf)
    });
    createSegment.spawnPlacementPath = PropertyPath::parse("@interaction-channel.pointerWorld");
    law->setActionModel(createSegment);
    return law;
}

std::shared_ptr<Law> createStrokeAcousticLaw(Singular& author) {
    auto law = beginPattern("Art: Stroke Acoustic Reaction", "art-stroke-sound-law",
                            "object-hover-entered", author);
    law->setConditionModel(inCategory(Control::kCategoryStroke));
    law->setActionModel(ActionNode::playAudio("acoustic.frequency", "acoustic.amplitude", "crystal"));
    return law;
}

std::shared_ptr<Law> createStrokeGlowLaw(Singular& author) {
    auto law = std::make_shared<FirstMoverLaw>("Art: Stroke Illumination");
    law->setLawIdentifier("art-stroke-glow-law");
    law->addAuthor(author);
    law->setActivation(Law::Activation::WhileTrue);
    law->setScope(Law::Scope::Everyone);
    law->setConditionModel(ConditionNode::all({
        inCategory(Control::kCategoryStroke),
        ConditionNode::compare("@world.pointerOver", ConditionNode::Op::Eq, PropertyValue(true))
    }));
    law->setActionModel(ActionNode::set("color", PropertyValue(glm::vec3(1.0f, 1.0f, 1.0f))));
    return law;
}

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

const std::vector<std::string>& controlPatternLawIds() {
    static const std::vector<std::string> ids{
        "control-button-law", "control-toggle-law",       "control-slider-law",
        "control-tuner-law",  "control-hover-response-law",
    };
    return ids;
}

void syncRegisterControlPatterns(LawManager& laws, CategoryManager& categories,
                                 Singular& author) {
    seedControlCategories(categories, author);

    struct Entry {
        std::shared_ptr<Law> law;
        const char* trigger;   // nullptr for continuous laws
    };
    const std::vector<Entry> entries{
        {createButtonLaw(author), "object-clicked"},
        {createToggleLaw(author), "object-clicked"},
        {createTunerLaw(author), "object-scrolled"},
        {createSliderLaw(author), nullptr},
        {createHoverResponseLaw(author), nullptr},
    };

    for (const auto& entry : entries) {
        if (!entry.law) continue;
        const std::string id = entry.law->getIdentifier();
        // First-wins. A world that loaded its own version of one of these —
        // edited, retargeted, disabled — keeps it: re-seeding must never
        // overwrite a Person's revision of a law they were handed.
        if (laws.find(id)) continue;
        laws.add(entry.law);
        if (entry.trigger) laws.bindTrigger(id, entry.trigger);
    }
}

} // namespace Input
} // namespace Singularity
