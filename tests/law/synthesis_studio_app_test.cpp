// The Synthesis Studio, held to its own save file.
//
// The previous version of this test passed 5/5 and proved almost nothing. It
// hand-built fresh ActionNodes in C++ that RESEMBLED the authored ones,
// compiled those, and asserted the compiled C++ behaved — so it would have
// passed unchanged with every law deleted from the save, and it had already
// drifted from what the save actually said (position 1.2 against the save's
// 1.25, scale 0.4 against 0.45). Test 3 compiled a playAudio node and asserted
// nothing at all about the result, which is how ActionNode::PlayAudio came to
// be a stub that published an event with no subscriber, reported SUCCESS, and
// stayed green for weeks. See docs/audits/SYNTHESIS_STUDIO_AUDIT_2026-09-02.md.
//
// So this one never writes a law. It LOADS the save, binds the triggers the
// save names, publishes real events through the EventBus, ticks the
// LawManager, and asks the world what happened. If a law text is wrong, this
// fails; if a law text is deleted, this fails; if an ActionNode silently does
// nothing, this fails — which is the only arrangement under which the test is
// a regression net for the studio rather than a second copy of it.

#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/RelationManager.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

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

bool nearly(double a, double b, double eps = 1e-3) { return std::fabs(a - b) < eps; }

// What the audio channel would have been asked to sound. The studio's whole
// musical half runs through here, and the point of recording it is that the
// old stub could not be caught any other way.
struct SoundedNote {
    std::string subject;
    double frequency = 0.0;
    double amplitude = 0.0;
    std::string timbre;
};
std::vector<SoundedNote> g_sounded;

double readNumber(Singular& being, const std::string& path, double fallback = 1e30) {
    PropertyValue v;
    double n = 0.0;
    if (!lawGetValue(being, PropertyPath::parse(path), v)) return fallback;
    return propertyValueToNumber(v, n) ? n : fallback;
}

bool readBool(Singular& being, const std::string& path) {
    PropertyValue v;
    if (!lawGetValue(being, PropertyPath::parse(path), v)) return false;
    if (const bool* b = std::get_if<bool>(&v)) return *b;
    double n = 0.0;
    return propertyValueToNumber(v, n) && n != 0.0;
}

void applyAuthoredProperties(Object& obj, const nlohmann::json& props) {
    for (auto it = props.begin(); it != props.end(); ++it) {
        const std::string name = it.key();
        const auto& val = it.value();
        const std::string type = val.value("t", std::string{});
        if (type == "string") obj.setDynamicProperty(name, PropertyValue(val["v"].get<std::string>()));
        else if (type == "bool") obj.setDynamicProperty(name, PropertyValue(val["v"].get<bool>()));
        else if (type == "int") obj.setDynamicProperty(name, PropertyValue(val["v"].get<int>()));
        else if (type == "double") obj.setDynamicProperty(name, PropertyValue(val["v"].get<double>()));
    }
}

} // namespace

int main() {
    std::printf("=== Synthesis Studio: the authored save, executed ===\n");

    std::ifstream f("saves/worlds/synthesis_studio.json");
    if (!f.is_open()) {
        std::fprintf(stderr, "synthesis_studio_app_test: cannot open "
                             "saves/worlds/synthesis_studio.json (CWD must be the source root; "
                             "CMakeLists sets WORKING_DIRECTORY for this test)\n");
        return 1;
    }
    nlohmann::json world;
    f >> world;
    f.close();

    const auto& zoneJson = world["zones"][0];

    // ------------------------------------------------------------------
    // The world the save describes, stood up for real.
    // ------------------------------------------------------------------
    auto zone = std::make_unique<Zone>("SynthesisStudio", "studio");
    CategoryManager categories;
    RelationManager graph;

    std::vector<Singular*> extras{zone.get()};
    for (const auto& catJson : world["categories"]) {
        auto cat = categories.create(catJson["objectID"].get<std::string>());
        if (cat) cat->setPhysicalObject(0);
    }

    for (const auto& objJson : zoneJson["world"]["objects"]) {
        auto obj = std::make_unique<Object>();
        obj->setObjectID(objJson["objectID"].get<std::string>());
        obj->setShape(static_cast<Object::ShapeKind>(objJson["shapeKind"].get<int>()),
                      Object::ShapeParams{});
        if (objJson.contains("transform")) {
            glm::mat4 m(1.0f);
            const auto& t = objJson["transform"];
            for (int c = 0; c < 4; ++c)
                for (int r = 0; r < 4; ++r) m[c][r] = t[c * 4 + r].get<float>();
            obj->setTransform(m);
        }
        if (objJson.contains("x2D")) obj->setX2D(objJson["x2D"].get<float>());
        if (objJson.contains("y2D")) obj->setY2D(objJson["y2D"].get<float>());
        if (objJson.contains("authoredProperties"))
            applyAuthoredProperties(*obj, objJson["authoredProperties"]);
        zone->addObject(std::move(obj));
    }

    const auto findBeing = [&](const std::string& id) -> Singular* {
        for (const auto& o : zone->getOwnedObjects())
            if (o && o->getIdentifier() == id) return o.get();
        if (auto cat = categories.get(id)) return cat.get();
        return nullptr;
    };
    const auto findObject = [&](const std::string& id) -> Object* {
        for (const auto& o : zone->getOwnedObjects())
            if (o && o->getIdentifier() == id) return o.get();
        return nullptr;
    };

    for (const auto& relJson : zoneJson["formationRelations"]) {
        Singular* a = findBeing(relJson["entityA"].get<std::string>());
        Singular* b = findBeing(relJson["entityB"].get<std::string>());
        if (a && b) {
            graph.add(std::make_shared<Relation>(relJson["type"].get<std::string>(), *a, *b,
                                                 relJson["directed"].get<bool>(), 1.0f));
        }
    }

    LawManager laws;
    Universe::instance().setProvider([&](std::vector<Singular*>& all) {
        for (Singular* being : extras) all.push_back(being);
        for (const auto& obj : zone->getOwnedObjects())
            if (obj) all.push_back(obj.get());
        for (const auto& cat : categories.getAll())
            if (cat) all.push_back(cat.get());
        for (const auto& law : laws.getAll())
            if (law) all.push_back(law.get());
    });
    Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
        for (const auto& rel : graph.getAll())
            if (rel) out.push_back(rel.get());
    });
    Universe::instance().setRelationRegistrar(
        [&](std::shared_ptr<Relation> rel) { graph.add(std::move(rel)); });
    Universe::instance().setClock(0.0, 1.0 / 60.0);

    // ------------------------------------------------------------------
    // 1. The save's laws load, and load AUTHORED.
    //
    //    Authorship reattaches by identifier against Universe::beings(), so a
    //    world whose author being is missing loads every law Unauthored and
    //    Law::applyTo refuses all of them — silently, as far as any assertion
    //    about object counts is concerned. Check it directly.
    // ------------------------------------------------------------------
    laws.loadFromJson(world["authoredLaws"]);
    laws.connectToEventBus();

    {
        const std::size_t authored = world["authoredLaws"]["laws"].size();
        check(laws.getAll().size() == authored,
              "every authored law in the save is in the register (" +
                  std::to_string(authored) + ")");

        bool allAuthored = true, allText = true;
        for (const auto& law : laws.getAll()) {
            if (!law) continue;
            if (!law->isAuthored()) {
                std::printf("    unauthored: %s\n", law->getIdentifier().c_str());
                allAuthored = false;
            }
            if (!law->hasConditionModel()) {
                std::printf("    opaque: %s\n", law->getIdentifier().c_str());
                allText = false;
            }
        }
        check(allAuthored, "every law reattached to author.gemini-spark — the structural gate");
        check(allText, "every law carries a readable condition tree");

        // The archetypes belong to the engine (ControlPatterns.cpp) and were
        // duplicated here, so both fired on every click and the Law Graph
        // showed a Person two of each. The save must not carry them.
        check(laws.find("law-control-button-archetype") == nullptr &&
                  laws.find("law-control-toggle-archetype") == nullptr,
              "the save does not re-author the engine's control archetypes");
    }

    // The studio's own archetype stand-in, so the test does not depend on
    // EngineInit having run: object-clicked on a button publishes
    // control-activated, exactly as control-button-law does.
    Object* btnSpawn = findObject("studio.btn.spawn-orb");
    Object* btnTheme = findObject("studio.btn.toggle-theme");
    Object* btnDraw = findObject("hud.btn.draw-stroke");
    Object* padC5 = findObject("studio.pad.c5");
    Object* handle = findObject("studio.slider.handle");
    Object* stateStudio = findObject("state.studio");
    Object* floorObj = findObject("studio.platform.floor");
    Object* crystal = findObject("studio.pedestal.crystal");
    if (!btnSpawn || !btnTheme || !btnDraw || !padC5 || !handle || !stateStudio ||
        !floorObj || !crystal) {
        std::fprintf(stderr, "synthesis_studio_app_test: the save is missing a control\n");
        return 1;
    }

    std::time_t clock = std::time(nullptr);
    const bool debug = std::getenv("STUDIO_DEBUG") != nullptr;
    const auto publish = [&](const char* type, Object* subject) {
        Core::EventBus::instance().publish(ECA::Event{type, subject, nullptr, ++clock});
        auto recs = laws.tick();
        if (debug) {
            std::printf("  [%s on %s] %zu record(s)\n", type,
                        subject ? subject->getIdentifier().c_str() : "null", recs.size());
            for (const auto& r : recs)
            {
                std::printf("      %s / %s -> result %d changed=%d\n", r.lawId.c_str(),
                            r.targetId.c_str(), (int)r.result, (int)r.changedSomething());
                for (const auto& c : r.conditionDescriptions)
                    std::printf("           cond: %s\n", c.c_str());
            }
        }
    };
    const auto activate = [&](Object& being) { publish("control-activated", &being); };

    if (debug) {
        auto recs = laws.tick();
        std::printf("--- bare tick: %zu record(s)\n", recs.size());
        for (const auto& r : recs)
            std::printf("      %s / %s -> result %d\n", r.lawId.c_str(), r.targetId.c_str(),
                        (int)r.result);
    }

    // ------------------------------------------------------------------
    // 2. PlayAudio actually sounds, and says so honestly when it cannot.
    //
    //    With no sink registered the node must FAIL, not report success —
    //    that inversion is the whole of audit §A1. With a sink, the authored
    //    frequency must arrive: the pad sounds 523.25 Hz because that is what
    //    the pad carries, not because anything here said so.
    // ------------------------------------------------------------------
    {
        registerAudioSink(nullptr);
        g_sounded.clear();
        if (debug) {
            PropertyValue v;
            std::printf("  probe pre : isChordPad read=%d val=%d\n",
                        (int)lawGetValue(*padC5, PropertyPath::parse("isChordPad"), v),
                        (int)(std::holds_alternative<bool>(v) && std::get<bool>(v)));
        }
        activate(*padC5);
        if (debug) {
            PropertyValue v;
            std::printf("  probe post: isChordPad read=%d val=%d  freq read=%d\n",
                        (int)lawGetValue(*padC5, PropertyPath::parse("isChordPad"), v),
                        (int)(std::holds_alternative<bool>(v) && std::get<bool>(v)),
                        (int)lawGetValue(*padC5, PropertyPath::parse("acoustic.frequency"), v));
        }
        check(g_sounded.empty(), "with no audio channel bound, nothing is sounded");

        registerAudioSink([](Singular& subject, double frequency, double amplitude,
                             const std::string& timbre) {
            g_sounded.push_back({subject.getIdentifier(), frequency, amplitude, timbre});
        });
        g_sounded.clear();
        activate(*padC5);
        check(g_sounded.size() == 1, "clicking the C5 pad sounds exactly one note");
        if (g_sounded.size() == 1) {
            check(nearly(g_sounded[0].frequency, 523.25),
                  "the note is the pad's own authored 523.25 Hz");
            check(g_sounded[0].subject == "studio.pad.c5",
                  "the note is sounded by the pad, not by some shared emitter");
            check(g_sounded[0].timbre == "triangle",
                  "the timbre is one the synthesizer has (\"crystal\" was not)");
        }

        // …and the tactile half of the same law.
        check(nearly(readNumber(*padC5, "position.y"), 0.88 - 0.04),
              "the pad dips 4 cm under the press");
        publish("object-released", padC5);
        check(nearly(readNumber(*padC5, "position.y"), 0.88),
              "and springs back to its authored rest height on release");
    }

    // ------------------------------------------------------------------
    // 3. Spawning an orb — the whole chain, from the save's own law text.
    // ------------------------------------------------------------------
    {
        const std::size_t before = zone->getOwnedObjects().size();
        g_sounded.clear();
        activate(*btnSpawn);

        check(zone->getOwnedObjects().size() == before + 1,
              "law-studio-spawn-orb mints exactly one being per activation");
        check(nearly(readNumber(*stateStudio, "spawnCount"), 1.0),
              "@state.studio.spawnCount reached the state being through a rooted path");
        check(g_sounded.size() == 1 && nearly(g_sounded[0].frequency, 587.33),
              "the spawn button sounds its own voice, not a shared 440 Hz constant");

        Object* orb = zone->getOwnedObjects().back().get();
        if (orb) {
            check(orb->getObjectType() == "interactive.harmonic.orb",
                  "the newborn carries the authored create type");
            ConditionNode inOrbs = ConditionNode::related("instance-of", "category.interactive.orb");
            ECA::Event probe{"test", nullptr, nullptr, 0};
            check(inOrbs.compile()(probe, *orb),
                  "and its instance-of edge to category.interactive.orb was made");
            check(nearly(readNumber(*orb, "shape.r"), 0.22),
                  "shape.r took the authored radius (`scale` is not a property and never was)");
        }

        // A second orb must not land on the first. spawnCount parameterises a
        // ring; before the repair every orb spawned on one point.
        const double firstX = orb ? readNumber(*orb, "position.x") : 0.0;
        activate(*btnSpawn);
        Object* second = zone->getOwnedObjects().back().get();
        check(second && !nearly(readNumber(*second, "position.x"), firstX, 1e-2),
              "the second orb lands somewhere else — spawnCount is read, not just written");
    }

    // ------------------------------------------------------------------
    // 4. The theme toggle goes BOTH ways, and something reads it.
    //
    //    `themeNight := themeNight + 1` on a bool latches on and never
    //    returns (audit §B1), and before the repair nothing read the flag at
    //    all, so a Person saw no difference either way (audit §B2).
    // ------------------------------------------------------------------
    {
        check(!readBool(*stateStudio, "themeNight"), "the studio starts in the day theme");

        activate(*btnTheme);
        check(readBool(*stateStudio, "themeNight"), "one click turns night on");
        laws.tick();
        const double nightR = readNumber(*floorObj, "color.r");
        if (debug) {
            std::printf("  floor color.r = %f ; handle cv=%f px=%f ; pulse=%f\n", nightR,
                        readNumber(*handle, "controlValue"),
                        readNumber(*handle, "position.x"),
                        readNumber(*stateStudio, "pulseRate"));
            auto recs = laws.tick();
            std::printf("  continuous tick: %zu record(s)\n", recs.size());
            for (const auto& r : recs)
                std::printf("      %s / %s -> %d changed=%d\n", r.lawId.c_str(),
                            r.targetId.c_str(), (int)r.result, (int)r.changedSomething());
        }
        check(nearly(nightR, 0.07, 0.02), "and the studio's surfaces take the night ambient");

        activate(*btnTheme);
        check(!readBool(*stateStudio, "themeNight"),
              "a second click turns it back off — the flip is in the mathematics");
        laws.tick();
        check(readNumber(*floorObj, "color.r") > nightR + 0.1,
              "and the surfaces return to day");
    }

    // ------------------------------------------------------------------
    // 5. Draw mode toggles, shows its own state, and gates drawing.
    // ------------------------------------------------------------------
    {
        check(!readBool(*stateStudio, "drawMode"), "draw mode starts off");
        activate(*btnDraw);
        check(readBool(*stateStudio, "drawMode"), "the draw button turns it on");
        laws.tick();
        PropertyValue label;
        btnDraw->getDynamicProperty("controlLabel", label);
        check(std::holds_alternative<std::string>(label) &&
                  std::get<std::string>(label) == "DRAW: ON",
              "and the button says so — a mode a Person cannot see is a mode they fight");

        activate(*btnDraw);
        check(!readBool(*stateStudio, "drawMode"), "and it turns back off");
    }

    // ------------------------------------------------------------------
    // 6. The slider stops at the bounds it always carried.
    //
    //    controlMin 0.2 / controlMax 3.0 were authored and read by nothing, so
    //    the handle slid off its track in both directions and pulseRate went
    //    negative (audit §B3). The clamp is a three-piece Piecewise; the
    //    outer pieces are what make it a clamp rather than a hole in the
    //    domain, and a hole would FREEZE the control instead of stopping it.
    // ------------------------------------------------------------------
    {
        check(nearly(readNumber(*handle, "position.x"), 1.0 - 1.6, 1e-2),
              "the handle is authored where its own law puts it — no jump on load");

        handle->setDynamicProperty("controlValue", PropertyValue(9.5));
        laws.tick();
        check(nearly(readNumber(*handle, "controlValue"), 3.0),
              "a value past controlMax is clamped back to it");
        check(nearly(readNumber(*handle, "position.x"), 3.0 - 1.6, 1e-2),
              "and the handle stops at the end of the track");
        check(nearly(readNumber(*stateStudio, "pulseRate"), 3.0),
              "pulseRate follows the clamped value, not the runaway one");

        handle->setDynamicProperty("controlValue", PropertyValue(-4.0));
        laws.tick();
        check(nearly(readNumber(*handle, "controlValue"), 0.2),
              "a value below controlMin is clamped up to it");

        // …and pulseRate is read by something. The crystal's colour is an
        // authored sinusoid whose amplitude the slider moves, so the two ends
        // of the track must not look the same.
        handle->setDynamicProperty("controlValue", PropertyValue(3.0));
        Universe::instance().setClock(0.4, 1.0 / 60.0);
        laws.tick();
        const double loud = readNumber(*crystal, "color.r");
        handle->setDynamicProperty("controlValue", PropertyValue(0.2));
        laws.tick();
        const double quiet = readNumber(*crystal, "color.r");
        check(!nearly(loud, quiet, 1e-3),
              "the crystal's pulse depth follows pulseRate — the slider means something");
    }

    // ------------------------------------------------------------------
    // 7. Strokes are drawn on the canvas, and nowhere else.
    //
    //    `isCanvas` was authored on the easel from the first draft and read by
    //    NO law, so strokes could be laid on the sky — where pointerWorld is
    //    the origin and every segment piled up on one point (audit §B6).
    // ------------------------------------------------------------------
    {
        // The stroke law is WhileTrue over the interaction channel's levels.
        // Standing in for the channel here keeps the test about the law text.
        Object channel;
        channel.setObjectID("interaction-channel");
        channel.setDynamicProperty("leftDown", PropertyValue(true));
        channel.setDynamicProperty("dragging", PropertyValue(true));
        channel.setDynamicProperty("pointerWorld", PropertyValue(glm::vec3(0.4f, 2.1f, 2.3f)));
        extras.push_back(&channel);
        registerWorldReading("@world.pointerOver", [](Singular& subject, PropertyValue& out) {
            out = PropertyValue(subject.getIdentifier() == "studio.easel.canvas");
            return true;
        });
        stateStudio->setDynamicProperty("drawMode", PropertyValue(true));

        const std::size_t before = zone->getOwnedObjects().size();
        laws.tick();
        const std::size_t drawn = zone->getOwnedObjects().size() - before;
        check(drawn == 1, "one tick with the pointer on the canvas lays exactly one segment");

        if (drawn >= 1) {
            Object* dab = zone->getOwnedObjects().back().get();
            check(nearly(readNumber(*dab, "acoustic.frequency"), 1046.5),
                  "the segment carries the acoustics law-stroke-hover-sound reads");

            g_sounded.clear();
            publish("object-hover-entered", dab);
            check(g_sounded.size() >= 1 && nearly(g_sounded[0].frequency, 1046.5),
                  "so hovering a stroke actually chimes");
        }

        // Off the canvas, nothing is drawn — the constraint constrains.
        registerWorldReading("@world.pointerOver", [](Singular&, PropertyValue& out) {
            out = PropertyValue(false);
            return true;
        });
        const std::size_t quiet = zone->getOwnedObjects().size();
        laws.tick();
        laws.tick();
        check(zone->getOwnedObjects().size() == quiet,
              "with the pointer off the canvas, no segments are laid at the origin");

        stateStudio->setDynamicProperty("drawMode", PropertyValue(false));
        registerWorldReading("@world.pointerOver", nullptr);
        extras.pop_back();
    }

    registerAudioSink(nullptr);

    if (g_failures == 0) {
        std::printf("=== Synthesis Studio: all checks passed ===\n");
        return 0;
    }
    std::printf("=== Synthesis Studio: %d check(s) FAILED ===\n", g_failures);
    return 1;
}
