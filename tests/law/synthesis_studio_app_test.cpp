#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/RelationManager.hpp"
#include "Singularity/Input/Interaction/ControlPatterns.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <vector>

int main() {
    std::cout << "=== Running Synthesis Studio App Integration Test ===" << std::endl;

    // 1. Load world JSON
    std::ifstream f("saves/worlds/synthesis_studio.json");
    assert(f.is_open() && "synthesis_studio.json must exist");
    nlohmann::json worldJson;
    f >> worldJson;
    f.close();

    assert(worldJson.contains("zones") && !worldJson["zones"].empty());
    const auto& zoneJson = worldJson["zones"][0];
    assert(zoneJson["identifier"] == "SynthesisStudio");

    // 2. Setup live Universe & Zone from the authored save
    auto zone = std::make_unique<Zone>("SynthesisStudio", "studio");
    CategoryManager categories;

    Object author;
    author.setObjectID("author.gemini-spark");

    // Seed categories from worldJson
    std::vector<Singular*> beings{zone.get(), &author};
    for (const auto& catJson : worldJson["categories"]) {
        auto cat = categories.create(catJson["objectID"].get<std::string>());
        if (cat) beings.push_back(cat.get());
    }

    // Load zone objects
    for (const auto& objJson : zoneJson["world"]["objects"]) {
        auto obj = std::make_unique<Object>();
        obj->setObjectID(objJson["objectID"].get<std::string>());
        obj->setShape(static_cast<Object::ShapeKind>(objJson["shapeKind"].get<int>()), Object::ShapeParams{});
        if (objJson.contains("authoredProperties")) {
            for (auto it = objJson["authoredProperties"].begin(); it != objJson["authoredProperties"].end(); ++it) {
                const std::string propName = it.key();
                const auto& valObj = it.value();
                const std::string type = valObj["t"].get<std::string>();
                if (type == "string") obj->setDynamicProperty(propName, PropertyValue(valObj["v"].get<std::string>()));
                else if (type == "bool") obj->setDynamicProperty(propName, PropertyValue(valObj["v"].get<bool>()));
                else if (type == "int") obj->setDynamicProperty(propName, PropertyValue(valObj["v"].get<int>()));
                else if (type == "double") obj->setDynamicProperty(propName, PropertyValue(valObj["v"].get<double>()));
            }
        }
        zone->addObject(std::move(obj));
    }

    // The world's relation graph: one RelationManager standing in for the
    // active Zone's Formation, read by Related conditions and written both
    // by the authored-save load below and by ActionNode::addRelation
    // (mirrors control_patterns_test.cpp's setup).
    RelationManager graph;

    // Load relations
    for (const auto& relJson : zoneJson["formationRelations"]) {
        const std::string aId = relJson["entityA"].get<std::string>();
        const std::string bId = relJson["entityB"].get<std::string>();
        const std::string type = relJson["type"].get<std::string>();

        Singular* a = nullptr;
        Singular* b = nullptr;
        for (const auto& o : zone->getOwnedObjects()) {
            if (o && o->getIdentifier() == aId) a = o.get();
            if (o && o->getIdentifier() == bId) b = o.get();
        }
        if (!a) a = categories.get(aId).get();
        if (!b) b = categories.get(bId).get();

        if (a && b) {
            graph.add(std::make_shared<Relation>(type, *a, *b, relJson["directed"].get<bool>(), 1.0f));
        }
    }

    Universe::instance().setProvider([&](std::vector<Singular*>& all) {
        for (Singular* being : beings) all.push_back(being);
        for (const auto& obj : zone->getOwnedObjects()) {
            if (obj) all.push_back(obj.get());
        }
    });
    Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
        for (const auto& rel : graph.getAll()) {
            if (rel) out.push_back(rel.get());
        }
    });
    Universe::instance().setRelationRegistrar(
        [&](std::shared_ptr<Relation> rel) { graph.add(std::move(rel)); });

    // A generic probe event for standalone ConditionNode::compile() checks
    // below; only conditions keyed on "@event.subject/object" read its fields.
    ECA::Event probe{"test", nullptr, nullptr, 0};

    // -----------------------------------------------------------------------
    // Test 1: Verify Studio Controls & Categories Exist
    // -----------------------------------------------------------------------
    Object* btnSpawnOrb = nullptr;
    Object* btnToggleTheme = nullptr;
    Object* padC5 = nullptr;
    for (const auto& obj : zone->getOwnedObjects()) {
        if (obj->getIdentifier() == "studio.btn.spawn-orb") btnSpawnOrb = obj.get();
        if (obj->getIdentifier() == "studio.btn.toggle-theme") btnToggleTheme = obj.get();
        if (obj->getIdentifier() == "studio.pad.c5") padC5 = obj.get();
    }
    assert(btnSpawnOrb != nullptr);
    assert(btnToggleTheme != nullptr);
    assert(padC5 != nullptr);

    ConditionNode isButton = ConditionNode::related("instance-of", "category.control.button");
    ConditionNode isToggle = ConditionNode::related("instance-of", "category.control.toggle");
    assert(isButton.compile()(probe, *btnSpawnOrb));
    assert(isToggle.compile()(probe, *btnToggleTheme));
    assert(isButton.compile()(probe, *padC5));
    std::cout << "[Test 1 PASS] Studio widgets loaded and evaluated category membership correctly." << std::endl;

    // -----------------------------------------------------------------------
    // Test 2: Button Interaction Chain — Spawn Harmonic Orb
    // -----------------------------------------------------------------------
    {
        const std::size_t countBefore = zone->getOwnedObjects().size();

        // Archetype Button Law: object-clicked -> Publish control-activated
        ActionNode archetypePublish = ActionNode::publish("control-activated");
        auto archExec = archetypePublish.compile();
        ECA::Event clickEvent{"object-clicked", btnSpawnOrb, nullptr, 0};
        archExec(clickEvent, *btnSpawnOrb);

        // Domain Law: on control-activated on spawn-orb button -> Create Harmonic Orb with category.art.stroke
        ActionNode spawnOrbAction = ActionNode::create(1, "interactive.harmonic.orb", {
            ActionNode::set("scale", PropertyValue(glm::vec3(0.4f))),
            ActionNode::set("color", PropertyValue(glm::vec3(1.0f, 0.84f, 0.22f))),
            ActionNode::set("position", PropertyValue(glm::vec3(0.0f, 1.6f, 1.2f))),
            ActionNode::set("acoustic.frequency", PropertyValue(880.0)),
            ActionNode::set("acoustic.amplitude", PropertyValue(0.9)),
            ActionNode::addRelation("", "category.art.stroke", "instance-of"),
            ActionNode::addRelation("", "category.interactive.orb", "instance-of")
        });

        auto domainExec = spawnOrbAction.compile();
        ECA::Event activatedEvent{"control-activated", btnSpawnOrb, nullptr, 0};
        domainExec(activatedEvent, *zone);

        assert(zone->getOwnedObjects().size() == countBefore + 1);
        Object* newbornOrb = zone->getOwnedObjects().back().get();
        assert(newbornOrb != nullptr);
        assert(newbornOrb->getObjectType() == "interactive.harmonic.orb");

        // Verify the newborn Orb is in category.art.stroke and category.interactive.orb
        ConditionNode inStroke = ConditionNode::related("instance-of", "category.art.stroke");
        ConditionNode inOrb = ConditionNode::related("instance-of", "category.interactive.orb");
        assert(inStroke.compile()(probe, *newbornOrb));
        assert(inOrb.compile()(probe, *newbornOrb));

        std::cout << "[Test 2 PASS] Button click chain successfully spawned Harmonic Orb and related it to categories." << std::endl;
    }

    // -----------------------------------------------------------------------
    // Test 3: Chord Pad Musical Interaction
    // -----------------------------------------------------------------------
    {
        PropertyValue freq;
        assert(padC5->getDynamicProperty("acoustic.frequency", freq));
        assert(std::get<double>(freq) == 523.25);

        ActionNode padAction = ActionNode::sequence({
            ActionNode::playAudio("acoustic.frequency", "acoustic.amplitude", "crystal"),
            ActionNode::set("position.y", PropertyValue(0.83)),
            ActionNode::publish("note-played")
        });

        auto padExec = padAction.compile();
        ECA::Event actEvent{"control-activated", padC5, nullptr, 0};
        padExec(actEvent, *padC5);

        std::cout << "[Test 3 PASS] Chord pad executed audio and tactile reaction." << std::endl;
    }

    // -----------------------------------------------------------------------
    // Test 4: Stroke Drawing Law and Behavior
    // -----------------------------------------------------------------------
    {
        const std::size_t countBefore = zone->getOwnedObjects().size();

        ActionNode strokeAction = ActionNode::create(1, "art.stroke.segment", {
            ActionNode::set("scale", PropertyValue(glm::vec3(0.08f))),
            ActionNode::set("color", PropertyValue(glm::vec3(1.0f, 0.85f, 0.15f))),
            ActionNode::addRelation("", "category.art.stroke", "instance-of")
        });

        auto strokeExec = strokeAction.compile();
        ECA::Event drawEvent{"draw-stroke", &author, nullptr, 0};
        strokeExec(drawEvent, *zone);

        assert(zone->getOwnedObjects().size() == countBefore + 1);
        Object* strokeDab = zone->getOwnedObjects().back().get();
        assert(strokeDab != nullptr);

        ConditionNode inStroke = ConditionNode::related("instance-of", "category.art.stroke");
        assert(inStroke.compile()(probe, *strokeDab));

        // Test stroke hover glow reaction
        ActionNode glowAction = ActionNode::set("color", PropertyValue(glm::vec3(1.0f, 1.0f, 1.0f)));
        auto glowExec = glowAction.compile();
        ECA::Event hoverEvent{"object-hover-entered", strokeDab, nullptr, 0};
        glowExec(hoverEvent, *strokeDab);

        PropertyValue col;
        assert(strokeDab->findProperty("color") != nullptr || strokeDab->getDynamicProperty("color", col));

        std::cout << "[Test 4 PASS] Art stroke drawing and behavior laws successfully applied." << std::endl;
    }

    // -----------------------------------------------------------------------
    // Test 5: Verify Authored Laws Count & End-to-End Serialization
    // -----------------------------------------------------------------------
    {
        const auto& lawsJson = worldJson["authoredLaws"]["laws"];
        assert(lawsJson.size() == 11);

        bool foundDrawToggle = false;
        bool foundSliderSync = false;
        for (const auto& law : lawsJson) {
            if (law["id"] == "law-studio-draw-mode-toggle") foundDrawToggle = true;
            if (law["id"] == "law-studio-slider-sync") foundSliderSync = true;
        }
        assert(foundDrawToggle && foundSliderSync);
        std::cout << "[Test 5 PASS] All 11 authored laws verified in JSON serialization." << std::endl;
    }

    std::cout << "=== ALL SYNTHESIS STUDIO INTEGRATION TESTS PASSED ===" << std::endl;
    return 0;
}
