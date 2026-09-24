#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <memory>
#include <string>
#include <vector>

class CategoryManager;
class Object;
class Singular;

namespace Singularity {
namespace Input {

// The five archetype controls, as LAW TEXT (INTERACTION_AS_LAW.md §6).
//
// Every factory here returns a Law whose behaviour lives entirely in a
// ConditionModel / ActionModel tree — the same trees a Person authors in the
// Law Graph, the same trees that serialize. Nothing in this file decides what
// a button does in C++; it writes down a sentence and hands it over.
//
// That constraint is not stylistic. createShapeGenerator3DLaw exists as a
// factory for exactly this reason: while the same law was C++ inlined in
// Engine::initLogic, it shipped for a day unable to fire at all, and no test
// could have caught it, because reaching the code meant booting a window.
// A control law a Person cannot read, edit, or disable from the Law Graph is
// not a control law — it is the UI framework these refusals exist to prevent,
// wearing a law's name.
//
// Refusal #1 and refusal #3, together: there is no Control class and no
// ControlKind enum. A control KIND is a category being (AUTHORED_CATEGORIES.md
// §10) and a control INSTANCE is any being with an instance-of edge to one.
namespace Control {

// The authored vocabulary. These are DYNAMIC properties — granted with
// ActionNode::AddProperty, or stamped onto a being/concept directly — not C++
// members on anything. Singular's dynamic-property fallback makes them resolve
// through PropertyPath exactly like registered ones, so law text does not know
// or care which kind it is reading.
inline constexpr const char* kValue = "controlValue";
inline constexpr const char* kMin   = "controlMin";
inline constexpr const char* kMax   = "controlMax";
inline constexpr const char* kStep  = "controlStep";
inline constexpr const char* kOn    = "controlOn";
inline constexpr const char* kLabel = "controlLabel";

// The category beings. Namespaced identifiers, so they cannot collide with an
// Object in the same path space and law text can name them (root resolution
// matches longest-first — Material set this pattern with "material.<name>").
inline constexpr const char* kCategoryRoot   = "category.control";
inline constexpr const char* kCategoryButton = "category.control.button";
inline constexpr const char* kCategoryToggle = "category.control.toggle";
inline constexpr const char* kCategorySlider = "category.control.slider";
inline constexpr const char* kCategoryTuner  = "category.control.tuner";
inline constexpr const char* kCategoryKey    = "category.control.key-command";
inline constexpr const char* kCategoryArtRoot = "category.art";
inline constexpr const char* kCategoryStroke  = "category.art.stroke";

// The membership edge, and the one a category uses to sit under another.
inline constexpr const char* kInstanceOf    = "instance-of";
inline constexpr const char* kSubcategoryOf = "subcategory-of";

// What a control PUBLISHES when it is worked. A button does not act; it
// announces. What activation MEANS is a separate authored law that hears this
// and conditions on which being it was — which is how one button comes to have
// any number of independently authorable, independently disableable
// consequences, something a click handler could never be.
inline constexpr const char* kActivated = "control-activated";

} // namespace Control

// Mint the category beings and the subcategory-of edges that place them under
// category.control. Idempotent — CategoryManager::create is first-wins, so a
// world that loaded its own control categories keeps them.
//
// The edges are registered through Universe::addRelation, which means they
// land wherever the engine pointed the relation registrar. No registrar = the
// hierarchy is not made, which is the standing rule: structure is never
// silently dropped somewhere unfindable.
void seedControlCategories(CategoryManager& categories, Singular& author);

// Admit a being into a control category and grant it the vocabulary it needs.
// This is the gesture a Person makes with the tools, available to C++ callers
// and tests: an instance-of edge plus a few AddProperty grants. It mints
// nothing and defines nothing.
//
// `initial` seeds controlValue; `step` seeds controlStep. Returns false when
// the category being does not exist — a control cannot join a category nobody
// authored, and guessing one into existence here would be minting a taxonomy
// on the Person's behalf.
bool makeControl(Object& being, const std::string& categoryId,
                 const CategoryManager& categories,
                 double initial = 0.0, double step = 1.0);

// ------------------------------------------------------------------
// The archetypes. Each is authored BY `author` — "nothing enters the world
// without an author" is structural (Law::applyTo answers Unauthored and
// refuses), and a first mover is not exempt.
// ------------------------------------------------------------------

// object-clicked -> Publish control-activated. §6a.
std::shared_ptr<Law> createButtonLaw(Singular& author);

// object-clicked -> flip controlOn, as ONE law with the flip in the
// mathematics. Authored first as the obvious pair ("if off, turn on" / "if on,
// turn off"), which does not work: the law network cascades, so the on-law's
// write re-activates the off-law's condition in the next chain round and one
// click nets no change. The full account is in the .cpp; the rule it is an
// instance of is that two laws whose actions satisfy each other's conditions
// are a loop, not a branch. §6b.
std::shared_ptr<Law> createToggleLaw(Singular& author);

// WhileTrue, driven by drag: controlValue integrates the authored rate. §6c.
std::shared_ptr<Law> createSliderLaw(Singular& author);

// object-scrolled -> controlValue := v + s·n. §6d.
std::shared_ptr<Law> createTunerLaw(Singular& author);

// key-pressed on the focused being, matching one key -> Publish
// control-activated. `key` is matched against
// @interaction-channel.lastKey. §6e.
std::shared_ptr<Law> createKeyCommandLaw(Singular& author, const std::string& key);

// WhileTrue over every being the pointer is on. Left with no action model on
// purpose: the FEEDBACK is the authored half, and the engine has no business
// deciding what "pointed at" should look like. Registered so it appears in the
// Law Graph as a written invitation rather than as absent machinery.
std::shared_ptr<Law> createHoverResponseLaw(Singular& author);

// Seed the categories and register every archetype into `laws`, binding each
// to the event that wakes it. Idempotent — safe on every boot and after a
// world load, which is what makes these first movers rather than save data.
// Seed art tool categories (category.art and category.art.stroke).
void seedArtCategories(CategoryManager& categories, Singular& author);

// WhileTrue drawing law that spawns stroke Singulars along @interaction-channel.pointerWorld.
std::shared_ptr<Law> createStrokeDrawingLaw(Singular& author);

// OnEvent law that triggers acoustic response on object-hover-entered for strokes.
std::shared_ptr<Law> createStrokeAcousticLaw(Singular& author);

// WhileTrue law that illuminates strokes when pointer is over them.
std::shared_ptr<Law> createStrokeGlowLaw(Singular& author);

void syncRegisterControlPatterns(LawManager& laws, CategoryManager& categories,
                                 Singular& author);

// The identifiers the archetypes answer to, for callers that need to find,
// disable, or govern them (the Law Graph, tests, a metalaw).
const std::vector<std::string>& controlPatternLawIds();

} // namespace Input
} // namespace Singularity
