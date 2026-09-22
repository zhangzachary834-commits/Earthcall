// The Decide/Act half of INTERACTION_AS_LAW.md, held to §11a.
//
// The claim under test is the framework's whole thesis: that a GUI is the
// existing Law system and the existing set-to-set creation, aimed — with no
// widget class, no control kind enum, and no UI event path. So this test
// never constructs a control. It clicks a being, and asks whether the world's
// own graph made that mean something.
//
// The case that matters most is #6. A concept captured from a control used to
// reproduce its geometry and its properties and NOT its membership in its
// category, because RelationTemplate only remembered edges whose far end was
// also inside the captured set. A hundred instantiated buttons, none of them
// buttons, all of them looking right.

#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Singular/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/RelationManager.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Input/Interaction/ControlPatterns.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

using namespace Singularity::Input;

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) {
        ++g_failures;
        std::printf("  FAILED: %s\n", what.c_str());
        return;
    }
    std::printf("  ok: %s\n", what.c_str());
}

bool nearf(double a, double b, double eps = 1e-4) { return std::fabs(a - b) < eps; }

// control-activated events, by subject.
std::vector<std::string> g_activated;

double valueOf(Singular& being) {
    PropertyValue v;
    double n = 0.0;
    if (!lawGetValue(being, PropertyPath::parse(Control::kValue), v)) return 1e30;
    return propertyValueToNumber(v, n) ? n : 1e30;
}

bool onOf(Singular& being) {
    PropertyValue v;
    if (!lawGetValue(being, PropertyPath::parse(Control::kOn), v)) return false;
    const bool* b = std::get_if<bool>(&v);
    return b && *b;
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "control_patterns_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "control_patterns_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "control_patterns_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    std::printf("Running control patterns test...\n");

    Core::EventBus::instance().subscribe<ECA::Event>([](const ECA::Event& e) {
        if (e.type == Control::kActivated) {
            g_activated.push_back(e.subject ? e.subject->getIdentifier() : "null");
        }
    });

    Object author;
    author.setObjectID("first-mover");

    CategoryManager cats;
    LawManager laws;
    InteractionChannel channel;

    // The world's relation graph: one RelationManager standing in for the
    // active Zone's Formation, read by Related conditions and written by
    // makeControl and by concept instantiation.
    RelationManager graph;
    Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
        for (const auto& rel : graph.getAll()) {
            if (rel) out.push_back(rel.get());
        }
    });
    Universe::instance().setRelationRegistrar(
        [&](std::shared_ptr<Relation> rel) { graph.add(std::move(rel)); });

    std::vector<Object*> world;
    std::vector<std::unique_ptr<Object>> newborns;
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        for (Object* obj : world) beings.push_back(obj);
        for (const auto& born : newborns) {
            if (born) beings.push_back(born.get());
        }
        for (const auto& cat : cats.getAll()) {
            if (cat) beings.push_back(cat.get());
        }
        beings.push_back(&channel);
        beings.push_back(&author);
    });

    syncRegisterControlPatterns(laws, cats, author);
    channel.installWorldReadings();
    laws.connectToEventBus();
    Universe::instance().setClock(0.0, 1.0 / 60.0);

    const auto click = [&](Object& being) {
        Core::EventBus::instance().publish(
            ECA::Event{"object-clicked", &being, nullptr, std::time(nullptr)});
        laws.tick();
    };
    const auto scroll = [&](Object& being, float notches) {
        channel.scrollY = notches;
        Core::EventBus::instance().publish(
            ECA::Event{"object-scrolled", &being, nullptr, std::time(nullptr)});
        laws.tick();
        channel.scrollY = 0.0f;
    };

    // ------------------------------------------------------------------
    // 1. Every archetype is registered, authored, and carries law TEXT.
    //    Unauthored is a refusal Law::applyTo makes structurally, and a
    //    first mover is not exempt from it — createShapeGenerator3DLaw
    //    shipped for a day failing exactly this.
    // ------------------------------------------------------------------
    {
        bool allPresent = true, allAuthored = true, allText = true;
        for (const std::string& id : controlPatternLawIds()) {
            Law* law = laws.find(id);
            if (!law) {
                std::printf("    missing: %s\n", id.c_str());
                allPresent = false;
                continue;
            }
            if (!law->isAuthored()) {
                std::printf("    unauthored: %s\n", id.c_str());
                allAuthored = false;
            }
            // A control law with no readable condition tree is C++ wearing a
            // law's name. The hover-response law is the deliberate exception:
            // it has a condition and NO action, because the feedback is the
            // authored half (§6f).
            if (!law->hasConditionModel()) {
                std::printf("    opaque: %s\n", id.c_str());
                allText = false;
            }
        }
        check(allPresent, "every archetype is registered under a stable identifier");
        check(allAuthored, "every archetype is authored — the structural gate");
        check(allText, "every archetype carries a readable condition tree");

        laws.add(nullptr);   // tolerated, and must not disturb the register
        syncRegisterControlPatterns(laws, cats, author);
        int buttonLaws = 0;
        for (const auto& law : laws.getAll()) {
            if (law && law->getIdentifier() == "control-button-law") ++buttonLaws;
        }
        // First-wins within a session. NOT across a save: these are
        // FirstMoverLaws, so LawManager::toJson skips them and the seeded
        // version returns on the next run (INTERACTION_AS_LAW.md §12).
        check(buttonLaws == 1, "re-seeding is first-wins — one registration, not two");
    }

    // ------------------------------------------------------------------
    // 2. The button. Clicking it activates it; clicking anything else
    //    does not, and no C++ anywhere knows which is which.
    // ------------------------------------------------------------------
    Object button, plainRock;
    button.setObjectID("the-button");
    plainRock.setObjectID("a-rock");
    world = {&button, &plainRock};
    {
        check(makeControl(button, Control::kCategoryButton, cats),
              "a being joins a control category through an edge, not a type");
        check(!makeControl(plainRock, "category.control.nonesuch", cats),
              "and cannot join a category nobody authored");

        g_activated.clear();
        click(button);
        check(g_activated.size() == 1 && g_activated[0] == "the-button",
              "clicking a button activates it");

        g_activated.clear();
        click(plainRock);
        check(g_activated.empty(),
              "clicking an ordinary being does nothing — membership is the whole difference");
    }

    // ------------------------------------------------------------------
    // 3. The toggle pair. Each law fires only on its own side.
    // ------------------------------------------------------------------
    Object toggle;
    toggle.setObjectID("the-toggle");
    world.push_back(&toggle);
    {
        makeControl(toggle, Control::kCategoryToggle, cats);
        check(!onOf(toggle), "a toggle starts off");

        click(toggle);
        check(onOf(toggle), "a click turns it on");
        click(toggle);
        check(!onOf(toggle), "and the next click turns it off");
        click(toggle);
        check(onOf(toggle), "and it keeps alternating");

        // THE REGRESSION THIS CASE EXISTS FOR. Authored first as a PAIR of
        // laws ("if off, turn on" / "if on, turn off"), the toggle never
        // moved: the on-law's write marked the Rete state fact dirty, the
        // dirty fact re-activated the off-law's Compare terminal in the next
        // chain round, and both fired within one tick. One click, no net
        // change. The flip lives in the mathematics now, where it cannot
        // re-trigger anything, and this loop is what would catch a return to
        // the pair form.
        bool alternates = true;
        bool expect = onOf(toggle);
        for (int i = 0; i < 6; ++i) {
            expect = !expect;
            click(toggle);
            if (onOf(toggle) != expect) alternates = false;
        }
        check(alternates, "six clicks, six flips — the cascade does not eat any of them");

        // And a Person can set the whole thing down.
        laws.find("control-toggle-law")->setEnabled(false);
        const bool held = onOf(toggle);
        click(toggle);
        check(onOf(toggle) == held, "disabled, the toggle does not move");
        laws.find("control-toggle-law")->setEnabled(true);
        click(toggle);
        check(onOf(toggle) != held, "re-enabled, it moves again");
    }

    // ------------------------------------------------------------------
    // 4. The tuner. Authored mathematics, not a hard-coded increment.
    // ------------------------------------------------------------------
    Object tuner;
    tuner.setObjectID("the-tuner");
    world.push_back(&tuner);
    {
        makeControl(tuner, Control::kCategoryTuner, cats, 10.0, 0.25);
        check(nearf(valueOf(tuner), 10.0), "the tuner starts where it was seeded");

        scroll(tuner, 1.0f);
        check(nearf(valueOf(tuner), 10.25), "one notch moves it by one step");
        scroll(tuner, 4.0f);
        check(nearf(valueOf(tuner), 11.25), "four notches move it by four");
        scroll(tuner, -5.0f);
        check(nearf(valueOf(tuner), 10.0), "and the wheel's sign is the direction");

        // The step is the INSTANCE's, not the law's: two tuners under one law
        // move at different rates because the number lives on the being.
        tuner.setDynamicProperty(Control::kStep, PropertyValue(2.0));
        scroll(tuner, 1.0f);
        check(nearf(valueOf(tuner), 12.0), "changing one instance's step changes only it");
    }

    // ------------------------------------------------------------------
    // 4b. The key command. Its subject is whoever holds focus, and the
    //     archetype is a FACTORY rather than a boot registration: which
    //     key, on which control, is an authored choice with no default
    //     worth the engine picking.
    // ------------------------------------------------------------------
    Object keyControl;
    keyControl.setObjectID("the-key-control");
    world.push_back(&keyControl);
    {
        auto keyLaw = createKeyCommandLaw(author, "e");
        laws.add(keyLaw);
        laws.bindTrigger(keyLaw->getIdentifier(), "key-pressed");
        makeControl(keyControl, Control::kCategoryKey, cats);

        const auto pressKey = [&](const std::string& key, Object* focused) {
            channel.lastKey = key;
            Core::EventBus::instance().publish(
                ECA::Event{"key-pressed", focused, nullptr, std::time(nullptr)});
            laws.tick();
        };

        g_activated.clear();
        pressKey("e", &keyControl);
        check(g_activated.size() == 1 && g_activated[0] == "the-key-control",
              "the bound key activates the focused control");

        g_activated.clear();
        pressKey("q", &keyControl);
        check(g_activated.empty(), "another key does not");

        g_activated.clear();
        pressKey("e", nullptr);
        check(g_activated.empty(),
              "and with nothing focused the key reaches nobody, rather than everybody");
    }

    // ------------------------------------------------------------------
    // 5. ONE law, MANY instances. The point of category membership.
    // ------------------------------------------------------------------
    std::vector<std::unique_ptr<Object>> crowd;
    {
        for (int i = 0; i < 5; ++i) {
            auto b = std::make_unique<Object>();
            b->setObjectID("crowd-button-" + std::to_string(i));
            makeControl(*b, Control::kCategoryButton, cats);
            world.push_back(b.get());
            crowd.push_back(std::move(b));
        }

        g_activated.clear();
        for (const auto& b : crowd) click(*b);
        check(g_activated.size() == 5,
              "five buttons, one law, no law authored per button");

        int distinct = 0;
        for (int i = 0; i < 5; ++i) {
            const std::string want = "crowd-button-" + std::to_string(i);
            for (const std::string& got : g_activated) {
                if (got == want) { ++distinct; break; }
            }
        }
        check(distinct == 5, "and each announced ITSELF, not the law's idea of a subject");
    }

    // ------------------------------------------------------------------
    // 6. THE ONE THAT MATTERS. Capture a button as a concept; instantiate
    //    it; the newborn is a member of its category and the SAME law
    //    reaches it, with nothing re-authored.
    // ------------------------------------------------------------------
    {
        std::vector<Object*> source{&button};
        auto concept = ObjectConcept::captureFrom(source, "Button", &author);
        check(concept != nullptr, "a control captures as an ordinary concept");

        bool anchored = false;
        for (const auto& t : concept->relationTemplates()) {
            if (t.isAnchored() && t.type == Control::kInstanceOf &&
                t.bAnchorId == Control::kCategoryButton) {
                anchored = true;
            }
        }
        check(anchored,
              "capture remembers the edge to the CATEGORY — an anchor outside the set");

        // …and survives the round trip, because a concept that only remembers
        // membership in RAM is a concept that forgets it on save.
        auto reborn = ObjectConcept::fromJson(concept->toJson());
        bool anchoredAfterJson = false;
        for (const auto& t : reborn->relationTemplates()) {
            if (t.isAnchored() && t.bAnchorId == Control::kCategoryButton) anchoredAfterJson = true;
        }
        check(anchoredAfterJson, "and the anchor survives serialization");

        auto born = concept->instantiate(glm::mat4(1.0f));
        check(born.size() == 1, "the concept instantiates");
        for (auto& b : born) newborns.push_back(std::move(b));
        Object* newborn = newborns.back().get();

        bool joined = false;
        for (const auto& rel : graph.getAll()) {
            if (rel && rel->type == Control::kInstanceOf &&
                rel->aId() == newborn->getIdentifier() &&
                rel->bId() == Control::kCategoryButton) {
                joined = true;
            }
        }
        check(joined, "the newborn is a member of the category, by its own fresh edge");

        g_activated.clear();
        click(*newborn);
        check(g_activated.size() == 1 && g_activated[0] == newborn->getIdentifier(),
              "and the button law reaches it with NOTHING re-authored");
        check(newborn->getIdentifier() != button.getIdentifier(),
              "while remaining a distinct being — a slot is not an identity");
    }

    // ------------------------------------------------------------------
    // 7. An anchor that has left the world is skipped, never guessed.
    //    A control whose category is gone is simply not in it, which is
    //    the truth and is legible as such.
    // ------------------------------------------------------------------
    {
        Object orphanSource;
        orphanSource.setObjectID("orphan-source");
        world.push_back(&orphanSource);
        makeControl(orphanSource, Control::kCategoryButton, cats);

        std::vector<Object*> source{&orphanSource};
        auto concept = ObjectConcept::captureFrom(source, "Orphan", &author);

        const std::size_t edgesBefore = graph.getAll().size();
        cats.remove(Control::kCategoryButton);       // the category leaves the world

        auto born = concept->instantiate(glm::mat4(1.0f));
        check(born.size() == 1, "the newborn is still born");
        std::size_t instanceEdges = 0;
        for (std::size_t i = edgesBefore; i < graph.getAll().size(); ++i) {
            const auto& rel = graph.getAll()[i];
            if (rel && rel->type == Control::kInstanceOf) ++instanceEdges;
        }
        check(instanceEdges == 0, "but joins no category, rather than joining a guessed one");

        for (auto& b : born) newborns.push_back(std::move(b));
        g_activated.clear();
        click(*newborns.back());
        check(g_activated.empty(), "and the button law correctly does not reach it");
    }

    Universe::instance().setProvider({});
    Universe::instance().setRelationProvider({});
    Universe::instance().setRelationRegistrar({});

    glfwDestroyWindow(window);
    glfwTerminate();

    if (g_failures) {
        std::printf("control_patterns_test: %d FAILURES\n", g_failures);
        return 1;
    }
    std::printf("control_patterns_test: all checks passed\n");
    return 0;
}
