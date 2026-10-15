#include "ZonesOfEarth/AuthorsOfLaw/SecondNatureLawAuthoring.hpp"
#include "Person/Person.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include "Singularity/TransferPolicy.hpp"

#include <array>
#include <cassert>
#include <iostream>
#include <memory>
#include <vector>

static Identity::SingularId makeTestKey(uint8_t fill) {
    std::array<uint8_t, 32> pubKeyBytes;
    pubKeyBytes.fill(fill);
    return Identity::SingularId::fromPublicKey(pubKeyBytes);
}

int main() {
    std::cout << "[Test] Starting second_nature_law_authoring_ambiguity_test...\n";

    // Setup TransferPolicy gates
    auto& policy = TransferPolicy::instance();
    for (const auto& [name, tier] : policy.gates()) {
        if (tier != TransferPolicy::Tier::Kernel) policy.setOpen(name, true);
    }

    // Setup Universe and LawManager
    LawManager lawManager;
    Physics::setLawManager(&lawManager);

    // Create two distinct Persons sharing the same display name "Alice"
    Person alice1(Soul("soul.alice1"), Body("body.alice1"), "default");
    alice1.soul().bindPerson(&alice1);
    alice1.setDisplayName("Alice");

    Person alice2(Soul("soul.alice2"), Body("body.alice2"), "default");
    alice2.soul().bindPerson(&alice2);
    alice2.setDisplayName("Alice");

    // Key alice1 and alice2 with distinct cryptographic keys
    Identity::SingularId key1 = makeTestKey(0x01);
    bool setKey1 = alice1.setPersonId(key1);
    assert(setKey1);

    Identity::SingularId key2 = makeTestKey(0x02);
    bool setKey2 = alice2.setPersonId(key2);
    assert(setKey2);

    // Create a prototype Law
    auto prototype = std::make_shared<Law>("prototype-law");
    lawManager.add(prototype);

    // Create a request object naming prototype->getIdentifier() as prototype
    Object request("request-obj");
    request.setDynamicProperty(SecondNatureLawAuthoring::kPrototypeProperty, PropertyValue(prototype->getIdentifier()));

    // Register provider for Universe::instance().beings()
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&alice1);
        beings.push_back(&alice2);
        beings.push_back(&request);
        for (const auto& law : lawManager.getAll()) {
            if (law) beings.push_back(law.get());
        }
    });

    // Event naming author "Alice" (display name)
    ECA::Event event{SecondNatureLawAuthoring::kInvokeEvent, &request, nullptr, 0, "Alice"};

    bool instantiated = SecondNatureLawAuthoring::instantiate(event);

    // Should refuse due to ambiguous author resolution ("Alice" matches both alice1 and alice2 display names).
    assert(!instantiated);

    Property* statusProp = request.findProperty(SecondNatureLawAuthoring::kStatusProperty);
    assert(statusProp != nullptr);
    std::string status = std::get<std::string>(statusProp->value());
    assert(status.find("refused") != std::string::npos);

    // Test resolving by exact cryptographic ID (key1)
    ECA::Event eventExact{SecondNatureLawAuthoring::kInvokeEvent, &request, nullptr, 0, key1.toString()};
    bool instantiatedExact = SecondNatureLawAuthoring::instantiate(eventExact);
    if (!instantiatedExact) {
        Property* statusPropExact = request.findProperty(SecondNatureLawAuthoring::kStatusProperty);
        if (statusPropExact) {
            std::cout << "Exact refusal reason: " << std::get<std::string>(statusPropExact->value()) << std::endl;
        }
    }
    assert(instantiatedExact);

    Physics::setLawManager(nullptr);
    Universe::instance().setProvider({});

    std::cout << "[Test] second_nature_law_authoring_ambiguity_test passed successfully!" << std::endl;
    return 0;
}
