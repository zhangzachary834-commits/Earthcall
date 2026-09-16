#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ConditionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/SecondNatureLawAuthoring.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"

#include <cassert>
#include <cmath>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

double readNumber(Singular& being, const std::string& property) {
    Property* p = being.findProperty(property);
    assert(p);
    PropertyValue value = p->value();
    if (const auto* d = std::get_if<double>(&value)) return *d;
    if (const auto* f = std::get_if<float>(&value)) return *f;
    if (const auto* i = std::get_if<int>(&value)) return *i;
    assert(false && "expected numeric property");
    return 0.0;
}

std::string readString(Singular& being, const std::string& property) {
    Property* p = being.findProperty(property);
    assert(p);
    PropertyValue value = p->value();
    const auto* text = std::get_if<std::string>(&value);
    assert(text);
    return *text;
}

} // namespace

int main() {
    LawManager laws;
    Physics::setLawManager(&laws);

    Object author;
    author.setObjectID("forge-author");

    Object target;
    target.setObjectID("forge-target");
    target.setDynamicProperty("forgeValue", PropertyValue(1.0));

    Object request;
    request.setObjectID("law-forge-state");
    request.setDynamicProperty(SecondNatureLawAuthoring::kTemplateProperty,
                               PropertyValue(std::string("law-template-click-set")));
    request.setDynamicProperty(SecondNatureLawAuthoring::kTargetProperty,
                               PropertyValue(std::string("forge-target")));
    request.setDynamicProperty(SecondNatureLawAuthoring::kNameProperty,
                               PropertyValue(std::string("When $TARGET is clicked, set its forge value")));

    auto prototype = std::make_shared<Law>("Concept: click -> set", std::vector<Singular*>{&author});
    prototype->setLawIdentifier("law-template-click-set");
    prototype->setEnabled(false); // a concept is inert until a Person invokes it
    prototype->setActivation(Law::Activation::OnEvent);
    prototype->setScope(Law::Scope::Subject);
    prototype->setConditionModel(ConditionNode::identity("$TARGET"));
    prototype->setActionModel(ActionNode::set("forgeValue", PropertyValue(7.0)));
    laws.add(prototype);
    laws.bindTrigger(prototype->getIdentifier(), "object-clicked");

    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&author);
        beings.push_back(&target);
        beings.push_back(&request);
        for (const auto& law : laws.getAll()) {
            if (law) beings.push_back(law.get());
        }
    });

    int authoredEvents = 0;
    Core::EventBus::instance().subscribe<ECA::Event>([&](const ECA::Event& event) {
        if (event.type == SecondNatureLawAuthoring::kAuthoredEvent) ++authoredEvents;
    });

    ECA::Event invoke{SecondNatureLawAuthoring::kInvokeEvent,
                      &request, nullptr, std::time(nullptr), author.getIdentifier()};
    assert(SecondNatureLawAuthoring::instantiate(invoke));

    Law* first = laws.find("law-template-click-set.birth-1");
    assert(first);
    assert(first != prototype.get());
    assert(first->isEnabled());
    assert(!prototype->isEnabled());
    assert(first->authors().getMembers().size() == 1);
    assert(first->authors().getMembers().front() == &author);
    assert(first->targets().getMembers().size() == 1);
    assert(first->targets().getMembers().front() == &target);
    assert(first->hasConditionModel());
    assert(first->conditionModel()->toJson().dump().find("forge-target") != std::string::npos);
    assert(first->conditionModel()->toJson().dump().find("$TARGET") == std::string::npos);
    assert(laws.triggersOf(first->getIdentifier()).size() == 1);
    assert(laws.triggersOf(first->getIdentifier()).front() == "object-clicked");
    assert(readString(request, SecondNatureLawAuthoring::kLastCreatedProperty) == first->getIdentifier());
    assert(readString(request, SecondNatureLawAuthoring::kStatusProperty).find("authored:") == 0);
    assert(authoredEvents == 1);

    // The newborn is ordinary executable Law text, not a UI-only token.
    assert(first->applyTo(target) == Law::ApplicationResult::Applied);
    assert(std::abs(readNumber(target, "forgeValue") - 7.0) < 1e-9);

    // Reusing the same instrument mints another identity without overwriting
    // the first result or the prototype.
    assert(SecondNatureLawAuthoring::instantiate(invoke));
    assert(laws.find("law-template-click-set.birth-2"));
    assert(laws.find("law-template-click-set.birth-1") == first);
    assert(authoredEvents == 2);

    // A declared target parameter is never guessed.
    request.setDynamicProperty(SecondNatureLawAuthoring::kTargetProperty,
                               PropertyValue(std::string("no-such-being")));
    assert(!SecondNatureLawAuthoring::instantiate(invoke));
    assert(readString(request, SecondNatureLawAuthoring::kStatusProperty) ==
           "refused: template requires a selected target");

    Physics::setLawManager(nullptr);
    Universe::instance().setProvider({});

    std::cout << "second_nature_law_authoring_test: all checks passed\n";
    return 0;
}
