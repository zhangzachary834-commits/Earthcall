// Scratch probe: does the Synthesis Studio's LAW-side click pipeline
// (InteractionChannel -> EventBus -> ReteNetwork -> LawManager::tick) leak or
// slow down over many more clicks than the regression test exercises?
//
// Zach's hypothesis, verbatim: not ImGui (WantCaptureMouse has never blocked
// world-space clicks for him before) but a second pipeline -- the Laws --
// getting "tangled" after enough clicks, possibly correlated with click count
// or time-in-zone rather than a fixed threshold.
//
// This hammers thousands of press/release cycles on a real 2D pad through
// InteractionChannel::observe() + LawManager::tick() -- the exact same path
// the app uses, no GLFW/ImGui involved at all -- and logs, every N clicks:
//   - ReteNetwork fact-list size (the working memory both event and state
//     facts live in; if this only grows, that IS the leak)
//   - drainAgenda() activity implicitly, via g_sounded growth
//   - wall-clock time per batch (a leak should show up as this climbing)
//   - the pad's y2D immediately after each release, to catch a click that
//     silently fails to spring back (a drift bug would surface here even if
//     rare/intermittent)
//
// Built temporarily through the tests/ CMake glob for convenience; removed
// after the run per docs/ENGINEERING_DISCIPLINE.md's Working Notes.

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

#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace {

int g_sounded = 0;

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

double readNumber(Singular& being, const std::string& path, double fallback = 1e30) {
    PropertyValue v;
    double n = 0.0;
    if (!lawGetValue(being, PropertyPath::parse(path), v)) return fallback;
    return propertyValueToNumber(v, n) ? n : fallback;
}

} // namespace

int main() {
    std::printf("=== Synthesis Studio click-stress probe ===\n");

    std::ifstream f("saves/worlds/synthesis_studio.json");
    if (!f.is_open()) {
        std::fprintf(stderr, "cannot open saves/worlds/synthesis_studio.json "
                             "(CWD must be the source root)\n");
        return 1;
    }
    nlohmann::json world;
    f >> world;
    f.close();

    const auto& zoneJson = world["zones"][0];

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

    laws.add(std::make_shared<FirstMoverLaw>("shape-generator-3d-law"));
    laws.add(std::make_shared<FirstMoverLaw>("tool-create-3d-law"));
    laws.loadFromJson(world["authoredLaws"]);
    laws.connectToEventBus();

    registerAudioSink([&](Singular&, double, double, const std::string&) { ++g_sounded; });

    Singularity::Input::InteractionChannel interaction;
    interaction.setEnabled(true);

    Singular* author = findBeing("Zach");
    if (author) {
        Singularity::Input::syncRegisterControlPatterns(laws, categories, *author);
    }

    Object* pad = findObject("hud.pad.c5");
    if (!pad) {
        std::fprintf(stderr, "hud.pad.c5 not found\n");
        return 1;
    }

    const int kTotalClicks = 6000;
    const int kBatchSize = 200;

    std::vector<Object*> reachable;
    for (const auto& o : zone->getOwnedObjects())
        if (o) reachable.push_back(o.get());

    int lockoutAt = -1;
    int firstFactGrowthAt = -1;
    std::size_t lastFactCount = laws.rete().facts().size();
    std::size_t lastAgendaCount = laws.rete().agenda().size();

    auto batchStart = std::chrono::steady_clock::now();

    for (int i = 0; i < kTotalClicks; ++i) {
        glm::vec4 r = pad->getRect2D();
        float cx = (r.x + r.z) * 0.5f;
        float cy = (r.y + r.w) * 0.5f;

        Singularity::Input::InteractionChannel::Sense s;
        s.pointerX = cx;
        s.pointerY = cy;
        s.left = true;
        interaction.observe(s, reachable);
        laws.tick();

        s.left = false;
        interaction.observe(s, reachable);
        laws.tick();

        // Did this click actually register at all? hoveredId should still be
        // the pad (pointer never moved) -- if picking itself goes dark, that
        // is the lockout, independent of the depress/spring elevation.
        if (interaction.hoveredId != "hud.pad.c5" && lockoutAt < 0) {
            lockoutAt = i;
            std::printf("  LOCKOUT at click %d: hoveredId is '%s' (expected hud.pad.c5)\n",
                        i, interaction.hoveredId.c_str());
        }

        const double y2D = readNumber(*pad, "y2D");
        if (std::fabs(y2D - readNumber(*pad, "restY2D")) > 0.01 && lockoutAt < 0) {
            std::printf("  SPRING FAILURE at click %d: y2D=%.3f did not return to restY2D\n",
                        i, y2D);
        }

        if ((i + 1) % kBatchSize == 0) {
            auto now = std::chrono::steady_clock::now();
            double ms = std::chrono::duration<double, std::milli>(now - batchStart).count();
            batchStart = now;

            const std::size_t factCount = laws.rete().facts().size();
            const std::size_t agendaCount = laws.rete().agenda().size();

            std::printf("  clicks=%6d  batch_ms=%8.2f  ms/click=%6.4f  "
                       "rete.facts=%4zu  rete.agenda=%4zu  sounded=%d\n",
                       i + 1, ms, ms / kBatchSize, factCount, agendaCount, g_sounded);
            std::fflush(stdout);

            if (factCount > lastFactCount && firstFactGrowthAt < 0 && i > kBatchSize) {
                // Only flag SUSTAINED growth across batches, not one-off jitter.
                firstFactGrowthAt = i;
            }
            lastFactCount = factCount;
            lastAgendaCount = agendaCount;
        }
    }

    std::printf("=== done: %d clicks, %d sounded, lockout=%s, fact-growth-flag=%s ===\n",
               kTotalClicks, g_sounded,
               lockoutAt < 0 ? "none" : std::to_string(lockoutAt).c_str(),
               firstFactGrowthAt < 0 ? "none" : std::to_string(firstFactGrowthAt).c_str());
    (void)lastAgendaCount;
    return 0;
}
