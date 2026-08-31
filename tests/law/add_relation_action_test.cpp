#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ConstructedBeing/CategoryManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/Input/Interaction/ControlPatterns.hpp"

#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

int main() {
    std::cout << "=== Running AddRelation Action & 2D UI / Art Stroke Test ===" << std::endl;

    // Setup Universe, Zone, Author, and Categories
    Zone zone("test-zone", "default");
    Singular author;
    author.setIdentifier("author.person");
    CategoryManager categories;
    Singularity::Input::seedControlCategories(categories, author);
    Singularity::Input::seedArtCategories(categories, author);

    auto* catButton = categories.get(Singularity::Input::Control::kCategoryButton);
    auto* catStroke = categories.get(Singularity::Input::Control::kCategoryStroke);
    assert(catButton != nullptr);
    assert(catStroke != nullptr);

    std::vector<Singular*> beings{&zone, &author, catButton, catStroke};

    Universe::instance().setProvider([&]() {
        std::vector<Singular*> all = beings;
        for (const auto& obj : zone.getOwnedObjects()) {
            if (obj) all.push_back(obj.get());
        }
        return all;
    });
    Universe::instance().setRelationProvider([&]() {
        return zone.getRelations();
    });

    // -----------------------------------------------------------------------
    // Test 1: Direct AddRelation ActionNode
    // -----------------------------------------------------------------------
    {
        auto obj = std::make_unique<Object>();
        obj->setIdentifier("test-button-obj");
        Object* objPtr = obj.get();
        zone.addObject(std::move(obj));

        ActionNode addRel = ActionNode::addRelation("", Singularity::Input::Control::kCategoryButton, "instance-of");
        auto executor = addRel.compile();

        ECA::Event ev{"test-event", objPtr, nullptr, 0};
        executor(ev, *objPtr);

        assert(zone.getRelations().size() >= 1);
        bool found = false;
        for (const auto& rel : zone.getRelations()) {
            if (rel && rel->a() == objPtr && rel->b() == catButton && rel->getType() == "instance-of") {
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
        const std::size_t initialRelCount = zone.getRelations().size();

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

        // Verify instance-of relation exists in zone
        bool foundRel = false;
        for (const auto& rel : zone.getRelations()) {
            if (rel && rel->a() == newborn && rel->b() == catButton && rel->getType() == "instance-of") {
                foundRel = true;
                break;
            }
        }
        assert(foundRel);

        // Verify ConditionNode::related evaluates true on the newborn
        ConditionNode inBtnCat = ConditionNode::related("instance-of", Singularity::Input::Control::kCategoryButton);
        assert(inBtnCat.evaluate(*newborn));

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
        assert(inStrokeCat.evaluate(*strokeDab));

        // Test stroke hover acoustic law reaction
        auto soundLaw = Singularity::Input::createStrokeAcousticLaw(author);
        assert(soundLaw != nullptr);
        assert(soundLaw->couldApplyTo(*strokeDab));

        std::cout << "[Test 3 PASS] Stroke creation with AddRelation creates stroke Singulars reactive to art laws." << std::endl;
    }

    std::cout << "=== ALL ADD_RELATION AND UI/STROKE LAW TESTS PASSED ===" << std::endl;
    return 0;
}
