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
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/Input/Interaction/ControlPatterns.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

#include <cmath>
#include <cstdio>
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
        PropertyValue pv;
        if (type == "string") pv = PropertyValue(val["v"].get<std::string>());
        else if (type == "bool") pv = PropertyValue(val["v"].get<bool>());
        else if (type == "int") pv = PropertyValue(val["v"].get<int>());
        else if (type == "double") pv = PropertyValue(val["v"].get<double>());
        if (Property* p = obj.findProperty(name)) {
            p->setValue(pv);
        }
        obj.setDynamicProperty(name, pv);
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
        Object::ShapeParams sp{};
        if (objJson.contains("shapeParams") && objJson["shapeParams"].is_array()) {
            const auto& a = objJson["shapeParams"];
            if (a.size() >= 9) {
                sp.r = a[0]; sp.ry = a[1]; sp.rz = a[2]; sp.halfH = a[3]; sp.majorR = a[4];
                sp.minorR = a[5]; sp.paraboloidA = a[6]; sp.ovoidAsym = a[7]; sp.fillet = a[8];
            }
            if (a.size() >= 11) {
                sp.width2D = a[9].get<float>();
                sp.height2D = a[10].get<float>();
            }
        }
        obj->setShape(static_cast<Object::ShapeKind>(objJson["shapeKind"].get<int>()), sp);
        if (objJson.contains("transform")) {
            glm::mat4 m(1.0f);
            const auto& t = objJson["transform"];
            for (int c = 0; c < 4; ++c)
                for (int r = 0; r < 4; ++r) m[c][r] = t[c * 4 + r].get<float>();
            obj->setTransform(m);
        }
        if (objJson.contains("center")) {
            obj->setCenter(glm::vec3(objJson["center"][0].get<float>(),
                                     objJson["center"][1].get<float>(),
                                     objJson["center"][2].get<float>()));
        }
        if (objJson.contains("faceColors")) {
            int face = 0;
            for (const auto& c : objJson["faceColors"]) {
                obj->setFaceColor(face++, c[0].get<float>(), c[1].get<float>(), c[2].get<float>());
            }
        }
        if (objJson.contains("x2D")) obj->setX2D(objJson["x2D"].get<float>());
        if (objJson.contains("y2D")) obj->setY2D(objJson["y2D"].get<float>());
        if (objJson.contains("zOrder2D")) obj->setZOrder2D(objJson["zOrder2D"].get<int>());
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
    const auto publish = [&](const char* type, Object* subject) {
        Core::EventBus::instance().publish(ECA::Event{type, subject, nullptr, ++clock});
        laws.tick();
    };
    const auto activate = [&](Object& being) { publish("control-activated", &being); };

    // ------------------------------------------------------------------
    // 2. PlayAudio actually sounds, and says so honestly when it cannot.
    //
    //    With no sink registered the node must FAIL, not report success —
    //    that inversion is the whole of audit §A1. With a sink, the authored
    //    frequency must arrive: each pad sounds its own authored frequency
    //    across all seven notes of the major scale (C5 to B5).
    // ------------------------------------------------------------------
    {
        registerAudioSink(nullptr);
        g_sounded.clear();
        activate(*padC5);
        check(g_sounded.empty(), "with no audio channel bound, nothing is sounded");

        registerAudioSink([](Singular& subject, double frequency, double amplitude,
                             const std::string& timbre) {
            g_sounded.push_back({subject.getIdentifier(), frequency, amplitude, timbre});
        });

        struct ScaleNote {
            const char* pad3d;
            const char* pad2d;
            double freq;
            const char* noteName;
        };
        const ScaleNote majorScale[] = {
            {"studio.pad.c5", "hud.pad.c5", 523.25, "C5"},
            {"studio.pad.d5", "hud.pad.d5", 587.33, "D5"},
            {"studio.pad.e5", "hud.pad.e5", 659.25, "E5"},
            {"studio.pad.f5", "hud.pad.f5", 698.46, "F5"},
            {"studio.pad.g5", "hud.pad.g5", 783.99, "G5"},
            {"studio.pad.a5", "hud.pad.a5", 880.00, "A5"},
            {"studio.pad.b5", "hud.pad.b5", 987.77, "B5"},
        };

        for (const auto& note : majorScale) {
            Object* p3d = findObject(note.pad3d);
            Object* p2d = findObject(note.pad2d);
            check(p3d != nullptr, std::string("3D major scale pad exists: ") + note.pad3d);
            check(p2d != nullptr, std::string("2D HUD major scale pad exists: ") + note.pad2d);

            if (p3d) {
                g_sounded.clear();
                activate(*p3d);
                check(g_sounded.size() == 1,
                      std::string("clicking 3D pad ") + note.noteName + " sounds exactly one note");
                if (!g_sounded.empty()) {
                    check(nearly(g_sounded[0].frequency, note.freq),
                          std::string("3D pad ") + note.noteName + " plays " + std::to_string(note.freq) + " Hz");
                    check(g_sounded[0].subject == note.pad3d,
                          std::string("note sounded by ") + note.pad3d);
                    check(g_sounded[0].timbre == "triangle",
                          "the timbre is triangle");
                }
                check(nearly(readNumber(*p3d, "position.y"), 0.88 - 0.04),
                      std::string("3D pad ") + note.noteName + " dips under press");
                publish("object-released", p3d);
                check(nearly(readNumber(*p3d, "position.y"), 0.88),
                      std::string("3D pad ") + note.noteName + " springs back on release");
            }

            if (p2d) {
                g_sounded.clear();
                activate(*p2d);
                check(g_sounded.size() == 1,
                      std::string("clicking 2D pad ") + note.noteName + " sounds exactly one note");
                if (!g_sounded.empty()) {
                    check(nearly(g_sounded[0].frequency, note.freq),
                          std::string("2D pad ") + note.noteName + " plays " + std::to_string(note.freq) + " Hz");
                    check(g_sounded[0].subject == note.pad2d,
                          std::string("note sounded by ") + note.pad2d);
                }
                const double restY2D = readNumber(*p2d, "restY2D");
                check(nearly(readNumber(*p2d, "y2D"), restY2D + 3.0),
                      std::string("2D pad ") + note.noteName + " shifts y2D under press");
                publish("object-released", p2d);
                check(nearly(readNumber(*p2d, "y2D"), restY2D),
                      std::string("2D pad ") + note.noteName + " springs back on release");
            }
        }
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
        channel.setDynamicProperty("dragX", PropertyValue(0.0));
        channel.setDynamicProperty("dragY", PropertyValue(0.0));
        extras.push_back(&channel);
        registerWorldReading("@world.pointerOver", [](Singular& subject, PropertyValue& out) {
            out = PropertyValue(subject.getIdentifier() == "studio.easel.canvas");
            return true;
        });
        stateStudio->setDynamicProperty("drawMode", PropertyValue(true));

        // A HELD BUTTON IS NOT A STROKE. Without a movement gate the law is a
        // level with no edge — WhileTrue fires every tick the button is down,
        // so resting on the canvas laid sixty spheres a second on one spot. At
        // ~0.2 ms of render per object that is a 7 fps slideshow inside ten
        // seconds, which is what "the 2D buttons stop working after a certain
        // point" turned out to be.
        const std::size_t still = zone->getOwnedObjects().size();
        laws.tick();
        laws.tick();
        check(zone->getOwnedObjects().size() == still,
              "a held pointer that is not moving lays no segments at all");

        channel.setDynamicProperty("dragX", PropertyValue(9.0));
        const std::size_t before = zone->getOwnedObjects().size();
        laws.tick();
        const std::size_t drawn = zone->getOwnedObjects().size() - before;
        check(drawn == 1, "and one tick of real travel lays exactly one segment");

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

    registerAudioSink([](Singular& subject, double frequency, double amplitude,
                         const std::string& timbre) {
        g_sounded.push_back({subject.getIdentifier(), frequency, amplitude, timbre});
    });

    // ------------------------------------------------------------------
    // 9. Real mouse picking and click registration via InteractionChannel.
    //
    //    Guards the 4 root causes found in the click registration investigation:
    //    (1) shape.width2D/height2D correctly loaded from authoredProperties;
    //    (2) 2D controls pick accurately across their full width without overlap;
    //    (3) captions/text with negative pickPriority never swallow clicks;
    //    (4) real click (down + up) publishes object-clicked, fires
    //        control-button-law, and triggers the studio's action laws.
    // ------------------------------------------------------------------
    {
        Object* btnSpawn = findObject("hud.btn.spawn-orb");
        Object* dock = findObject("hud.dock.bg");
        Object* padC5 = findObject("hud.pad.c5");
        Object* padD5 = findObject("hud.pad.d5");
        Object* btnDraw = findObject("hud.btn.draw-stroke");
        Object* hudTitle = findObject("hud.title");

        check(btnSpawn && btnSpawn->getShapeParams().width2D == 132.0f,
              "hud.btn.spawn-orb loaded authored width2D (132)");
        check(dock && dock->getShapeParams().width2D == 820.0f,
              "hud.dock.bg loaded authored width2D (820)");
        check(padC5 && padD5 && padC5->getRect2D().z <= padD5->getRect2D().x,
              "2D HUD pads do not overlap (pad C5 ends before pad D5 begins)");
        check(hudTitle && hudTitle->pickPriority() < 0.0,
              "captions carry negative pickPriority and are transparent to picking");

        Singularity::Input::InteractionChannel interaction;
        interaction.setEnabled(true);

        std::vector<Object*> reachable;
        for (const auto& o : zone->getOwnedObjects()) {
            if (o) reachable.push_back(o.get());
        }

        auto pickAt = [&](float x, float y) -> std::string {
            Singularity::Input::InteractionChannel::Sense s;
            s.pointerX = x;
            s.pointerY = y;
            interaction.observe(s, reachable);
            return interaction.hoveredId;
        };

        check(pickAt(260.0f, 660.0f) == "hud.btn.spawn-orb",
              "picking left side of spawn button hits hud.btn.spawn-orb");
        check(pickAt(360.0f, 660.0f) == "hud.btn.spawn-orb",
              "picking right side of spawn button hits hud.btn.spawn-orb");
        check(pickAt(585.0f, 660.0f) == "hud.pad.d5",
              "picking center of pad D5 hits hud.pad.d5 (not stolen by C5)");
        check(pickAt(635.0f, 660.0f) == "hud.pad.e5",
              "picking center of pad E5 hits hud.pad.e5 (not stolen by D5)");
        check(pickAt(1000.0f, 660.0f) == "hud.btn.draw-stroke",
              "picking right side of draw button hits hud.btn.draw-stroke");
        check(pickAt(80.0f, 50.0f) == "",
              "caption text2d does not swallow clicks at (80, 50)");

        // Register control patterns (control-button-law, control-toggle-law)
        Singular* author = findBeing("Zach");
        if (author) {
            Singularity::Input::syncRegisterControlPatterns(laws, categories, *author);
        }

        // Real click on spawn button: mouse down + mouse up
        const size_t countBeforeClick = zone->getOwnedObjects().size();
        Singularity::Input::InteractionChannel::Sense sDown;
        sDown.pointerX = 300.0f;
        sDown.pointerY = 660.0f;
        sDown.left = true;
        interaction.observe(sDown, reachable);

        Singularity::Input::InteractionChannel::Sense sUp;
        sUp.pointerX = 300.0f;
        sUp.pointerY = 660.0f;
        sUp.left = false;
        interaction.observe(sUp, reachable);

        // Process triggered events
        laws.tick();

        const size_t countAfterClick = zone->getOwnedObjects().size();
        check(countAfterClick == countBeforeClick + 1,
              "real mouse click through InteractionChannel fires control-button-law and spawns an orb");

        // Repeat 20 clicks to test multi-click stability (user reported issue after ~15 clicks)
        bool multiClickOk = true;
        for (int i = 0; i < 20; ++i) {
            reachable.clear();
            for (const auto& o : zone->getOwnedObjects()) {
                if (o) reachable.push_back(o.get());
            }
            interaction.observe(sDown, reachable);
            interaction.observe(sUp, reachable);
            laws.tick();
            if (zone->getOwnedObjects().size() != countAfterClick + i + 1) {
                std::printf("  FAILED at click %d: count is %zu, expected %zu\n",
                            i + 1, zone->getOwnedObjects().size(), countAfterClick + i + 1);
                multiClickOk = false;
                break;
            }
        }
        check(multiClickOk, "20 successive clicks through InteractionChannel reliably spawn 20 orbs");

        // Test UI capture transitions (switching to ImGui and back)
        {
            reachable.clear();
            for (const auto& o : zone->getOwnedObjects()) {
                if (o) reachable.push_back(o.get());
            }

            // Scenario A: Click happened entirely inside ImGui
            Singularity::Input::InteractionChannel::Sense sImGuiDown = sDown;
            sImGuiDown.uiCaptured = true;
            interaction.observe(sImGuiDown, reachable);
            Singularity::Input::InteractionChannel::Sense sImGuiUp = sUp;
            sImGuiUp.uiCaptured = true;
            interaction.observe(sImGuiUp, reachable);
            laws.tick();

            // Scenario B: Mouse pressed inside ImGui, released outside in the world
            interaction.observe(sImGuiDown, reachable);
            interaction.observe(sUp, reachable);
            laws.tick();

            // Scenario C: Now regular click in the world on pad C5
            const size_t soundsBefore = g_sounded.size();
            Singularity::Input::InteractionChannel::Sense sPadDown;
            sPadDown.pointerX = 535.0f; // center of pad C5
            sPadDown.pointerY = 660.0f;
            sPadDown.left = true;
            interaction.observe(sPadDown, reachable);

            Singularity::Input::InteractionChannel::Sense sPadUp = sPadDown;
            sPadUp.left = false;
            interaction.observe(sPadUp, reachable);
            laws.tick();

            check(g_sounded.size() == soundsBefore + 1,
                  "click on pad C5 works after ImGui UI-captured events");
        }

        // Test click with small tremor (within clickSlopPixels)
        {
            reachable.clear();
            for (const auto& o : zone->getOwnedObjects()) {
                if (o) reachable.push_back(o.get());
            }
            const size_t orbsBefore = zone->getOwnedObjects().size();
            Singularity::Input::InteractionChannel::Sense sJitterDown;
            sJitterDown.pointerX = 260.0f; // on spawn button (x: 236..368)
            sJitterDown.pointerY = 660.0f;
            sJitterDown.left = true;
            interaction.observe(sJitterDown, reachable);

            // Move 4 pixels while held down (< 12px clickSlopPixels)
            Singularity::Input::InteractionChannel::Sense sJitterMove = sJitterDown;
            sJitterMove.pointerX = 264.0f;
            interaction.observe(sJitterMove, reachable);
            check(!interaction.dragging, "pointer moved 4px does not set dragging flag");

            // Release inside the button
            Singularity::Input::InteractionChannel::Sense sJitterUp = sJitterMove;
            sJitterUp.left = false;
            interaction.observe(sJitterUp, reachable);
            laws.tick();

            check(zone->getOwnedObjects().size() == orbsBefore + 1,
                  "click with natural hand tremor reliably fires object-clicked and spawns orb");
        }

        // Test window focus loss mid-press and clean recovery
        {
            reachable.clear();
            for (const auto& o : zone->getOwnedObjects()) {
                if (o) reachable.push_back(o.get());
            }

            // Mouse button pressed down
            Singularity::Input::InteractionChannel::Sense sDownOnBtn;
            sDownOnBtn.pointerX = 260.0f;
            sDownOnBtn.pointerY = 660.0f;
            sDownOnBtn.left = true;
            interaction.observe(sDownOnBtn, reachable);
            check(interaction.leftDown, "button is held down before focus loss");

            // Window loses focus (e.g. user Cmd-Tabs or switches tabs)
            interaction.onWindowFocus(false);
            check(!interaction.leftDown && interaction.pressedId.empty(),
                  "focus loss clears held button and in-flight pressedId");

            // Window regains focus
            interaction.onWindowFocus(true);

            // Fresh click on Pad D5 works cleanly without being blocked
            const size_t soundsBefore = g_sounded.size();
            Singularity::Input::InteractionChannel::Sense sPadD5;
            sPadD5.pointerX = 585.0f; // pad D5
            sPadD5.pointerY = 660.0f;
            sPadD5.left = true;
            interaction.observe(sPadD5, reachable);
            sPadD5.left = false;
            interaction.observe(sPadD5, reachable);
            laws.tick();

            check(g_sounded.size() == soundsBefore + 1,
                  "fresh click immediately succeeds after window focus loss and recovery");
        }

        // 3D picking from camera position
        glm::vec3 camPos(0.0f, 1.35f, -2.6f);
        auto rayTo = [&](const char* targetId) -> std::string {
            Object* target = findObject(targetId);
            if (!target) return "";
            glm::vec3 dir = glm::normalize(target->getCenter() - camPos);
            Singularity::Input::InteractionChannel::Sense s3D;
            s3D.rayOrigin = camPos;
            s3D.rayDirection = dir;
            s3D.pointerX = -999.0f;
            s3D.pointerY = -999.0f;
            interaction.observe(s3D, reachable);
            return interaction.hoveredId;
        };

        check(rayTo("studio.btn.spawn-orb") == "studio.btn.spawn-orb",
              "3D raycast from camera hits studio.btn.spawn-orb on desk");
        check(rayTo("studio.pad.c5") == "studio.pad.c5",
              "3D raycast from camera hits studio.pad.c5 on desk");

        // Sequential pad clicks across all 7 pads multiple times
        {
            const std::vector<std::string> padIds = {
                "hud.pad.c5", "hud.pad.d5", "hud.pad.e5", "hud.pad.f5",
                "hud.pad.g5", "hud.pad.a5", "hud.pad.b5"
            };
            size_t soundsStart = g_sounded.size();
            bool allPadsSucceeded = true;
            for (int pass = 0; pass < 3; ++pass) {
                for (size_t p = 0; p < padIds.size(); ++p) {
                    const auto& padId = padIds[p];
                    Object* pad = findObject(padId.c_str());
                    if (!pad) {
                        std::printf("  FAILED: pad %s not found\n", padId.c_str());
                        allPadsSucceeded = false;
                        break;
                    }
                    glm::vec4 r = pad->getRect2D();
                    float cx = (r.x + r.z) * 0.5f;
                    float cy = (r.y + r.w) * 0.5f;
                    
                    reachable.clear();
                    for (const auto& o : zone->getOwnedObjects()) if (o) reachable.push_back(o.get());

                    Singularity::Input::InteractionChannel::Sense sPad;
                    sPad.pointerX = cx;
                    sPad.pointerY = cy;
                    sPad.left = true;
                    interaction.observe(sPad, reachable);
                    sPad.left = false;
                    interaction.observe(sPad, reachable);
                    laws.tick();

                    if (interaction.hoveredId != padId) {
                        std::printf("  FAILED at pass %d pad %s: hoveredId is '%s', rect is [%.1f, %.1f, %.1f, %.1f], pointer at (%.1f, %.1f)\n",
                                    pass, padId.c_str(), interaction.hoveredId.c_str(), r.x, r.y, r.z, r.w, cx, cy);
                        allPadsSucceeded = false;
                    }
                }
            }
            std::printf("  After 21 pad clicks: sounded %zu notes (expected %zu)\n",
                        g_sounded.size() - soundsStart, (size_t)21);
            check(allPadsSucceeded && (g_sounded.size() - soundsStart == 21),
                  "clicking all 7 pads in sequence for 3 passes sounds all 21 notes");
        }
    }

    if (g_failures == 0) {
        std::printf("=== Synthesis Studio: all checks passed ===\n");
        return 0;
    }
    std::printf("=== Synthesis Studio: %d check(s) FAILED ===\n", g_failures);
    return 1;
}
