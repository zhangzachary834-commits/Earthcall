#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/RelationManager.hpp"
#include "Singularity/Input/Interaction/ControlPatterns.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

int main() {
    std::cout << "=== Running AddRelation Action & 2D UI / Art Stroke Test ===" << std::endl;

    // Setup Universe, Zone, Author, and Categories
    Zone zone("test-zone", "default");
    Object author;
    author.setObjectID("author.person");
    CategoryManager categories;
    Singularity::Input::seedControlCategories(categories, author);
    Singularity::Input::seedArtCategories(categories, author);

    auto catButton = categories.get(Singularity::Input::Control::kCategoryButton);
    auto catStroke = categories.get(Singularity::Input::Control::kCategoryStroke);
    assert(catButton != nullptr);
    assert(catStroke != nullptr);

    std::vector<Singular*> beings{&zone, &author, catButton.get(), catStroke.get()};

    // The world's relation graph: one RelationManager standing in for the
    // active Zone's Formation, read by Related conditions and written by
    // ActionNode::addRelation (mirrors control_patterns_test.cpp's setup).
    RelationManager graph;
    Universe::instance().setProvider([&](std::vector<Singular*>& all) {
        for (Singular* b : beings) all.push_back(b);
        for (const auto& obj : zone.getOwnedObjects()) {
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
    // Test 1: Direct AddRelation ActionNode
    // -----------------------------------------------------------------------
    {
        auto obj = std::make_unique<Object>();
        obj->setObjectID("test-button-obj");
        Object* objPtr = obj.get();
        zone.addObject(std::move(obj));

        ActionNode addRel = ActionNode::addRelation("", Singularity::Input::Control::kCategoryButton, "instance-of");
        auto executor = addRel.compile();

        ECA::Event ev{"test-event", objPtr, nullptr, 0};
        executor(ev, *objPtr);

        assert(graph.getAll().size() >= 1);
        bool found = false;
        for (const auto& rel : graph.getAll()) {
            if (rel && rel->a() == objPtr && rel->b() == catButton.get() && rel->type == "instance-of") {
                found = true;
                break;
            }
        }
        assert(found);
        std::cout << "[Test 1 PASS] Direct AddRelation successfully wired instance-of relation." << std::endl;
    }

    // -----------------------------------------------------------------------
    // Test 2: Create ActionNode with Child AddRelation (2D UI Button Spawn)
    // -----------------------------------------------------------------------
    {
        const std::size_t initialObjCount = zone.getOwnedObjects().size();
        const std::size_t initialRelCount = graph.getAll().size();

        ActionNode createBtn = ActionNode::create(0, "ui.button", {
            ActionNode::set("scale", PropertyValue(glm::vec3(1.5f, 0.4f, 0.05f))),
            ActionNode::set("color", PropertyValue(glm::vec3(0.2f, 0.6f, 0.9f))),
            ActionNode::addProperty("", "controlLabel", PropertyValue(std::string("Submit"))),
            ActionNode::addRelation("", Singularity::Input::Control::kCategoryButton, "instance-of")
        });

        auto executor = createBtn.compile();
        ECA::Event ev{"spawn-ui-button", &author, nullptr, 0};
        executor(ev, zone);

        assert(zone.getOwnedObjects().size() == initialObjCount + 1);
        Object* newborn = zone.getOwnedObjects().back().get();
        assert(newborn != nullptr);
        assert(newborn->getObjectType() == "ui.button");

        // Verify label was set
        PropertyValue labelVal;
        assert(newborn->findProperty("controlLabel") != nullptr || newborn->getDynamicProperty("controlLabel", labelVal));

        // Verify instance-of relation exists in the graph
        bool foundRel = false;
        for (const auto& rel : graph.getAll()) {
            if (rel && rel->a() == newborn && rel->b() == catButton.get() && rel->type == "instance-of") {
                foundRel = true;
                break;
            }
        }
        assert(foundRel);

        // Verify ConditionNode::related evaluates true on the newborn
        ConditionNode inBtnCat = ConditionNode::related("instance-of", Singularity::Input::Control::kCategoryButton);
        assert(inBtnCat.compile()(probe, *newborn));

        std::cout << "[Test 2 PASS] Create action with child AddRelation successfully created button and wired category." << std::endl;
    }

    // -----------------------------------------------------------------------
    // Test 3: Stroke Drawing Creation with AddRelation (Art Stroke)
    // -----------------------------------------------------------------------
    {
        const std::size_t initialObjCount = zone.getOwnedObjects().size();

        ActionNode drawStroke = ActionNode::create(1, "art.stroke.segment", {
            ActionNode::set("scale", PropertyValue(glm::vec3(0.05f))),
            ActionNode::set("color", PropertyValue(glm::vec3(1.0f, 0.8f, 0.0f))),
            ActionNode::addRelation("", Singularity::Input::Control::kCategoryStroke, "instance-of")
        });

        auto executor = drawStroke.compile();
        ECA::Event ev{"draw-stroke", &author, nullptr, 0};
        executor(ev, zone);

        assert(zone.getOwnedObjects().size() == initialObjCount + 1);
        Object* strokeDab = zone.getOwnedObjects().back().get();
        assert(strokeDab != nullptr);
        assert(strokeDab->getObjectType() == "art.stroke.segment");

        // Verify instance-of category.art.stroke
        ConditionNode inStrokeCat = ConditionNode::related("instance-of", Singularity::Input::Control::kCategoryStroke);
        assert(inStrokeCat.compile()(probe, *strokeDab));

        // Test stroke hover acoustic law reaction
        auto soundLaw = Singularity::Input::createStrokeAcousticLaw(author);
        assert(soundLaw != nullptr);
        assert(soundLaw->couldApplyTo(*strokeDab));

        std::cout << "[Test 3 PASS] Stroke creation with AddRelation creates stroke Singulars reactive to art laws." << std::endl;
    }

    std::cout << "=== ALL ADD_RELATION AND UI/STROKE LAW TESTS PASSED ===" << std::endl;
    return 0;
}
