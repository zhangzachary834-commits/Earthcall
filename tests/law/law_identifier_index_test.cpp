// Focused seam-level regression for Law identity and LawManager indexing lifecycle.
// WebSocketServer delegates identifier matching to this LawManager contract; this is not a socket E2E test.
//
// Verifies:
// 1. LawManager::createLaw(name, identifier, authors) sets the requested ID before indexing,
//    so LawManager::find(identifier) succeeds immediately.
// 2. Multiple authored Laws sharing the SAME display name (e.g. "Gravity") remain distinct beings
//    when created with DIFFERENT unique identifiers.
// 3. Repeated lookup by the SAME identifier targets the existing Law without changing the register.
// 4. Display name alone is NEVER treated as a unique identity key by LawManager::find.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include <cassert>
#include <ctime>
#include <vector>
#include <iostream>
#include <string>

namespace {

int g_checks = 0;
void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        std::cout << "  FAILED: " << description << std::endl;
        assert(false);
    }
    std::cout << "  ok: " << description << std::endl;
}

} // namespace

int main() {
    std::cout << "Running Law Identifier Index Test...\n";

    Object author;
    author.setName("Author1");

    LawManager lm;

    // ------------------------------------------------------------------
    // 1. Immediate index population on creation
    // ------------------------------------------------------------------
    std::cout << "\n[1] Immediate index population on creation\n";
    {
        auto law = lm.createLaw("Authored Law", "law-custom-101", {&author});
        check(law != nullptr, "Law created successfully");
        check(law->getIdentifier() == "law-custom-101", "Law has requested identifier");
        check(lm.find("law-custom-101") == law.get(), "lm.find(requestedId) resolves the newborn Law immediately");
    }

    // ------------------------------------------------------------------
    // 2. Same display name, different identifiers -> distinct Laws
    // ------------------------------------------------------------------
    std::cout << "\n[2] Same display name, different identifiers -> distinct Laws\n";
    {
        auto lawA = lm.createLaw("Gravity", "law-gravity-custom", {&author});
        auto lawB = lm.createLaw("Gravity", "law-gravity-preset", {&author});

        check(lawA != nullptr && lawB != nullptr, "Both Laws created");
        check(lawA.get() != lawB.get(), "lawA and lawB are distinct pointers");
        check(lawA->name() == lawB->name(), "lawA and lawB share display name 'Gravity'");
        check(lawA->getIdentifier() == "law-gravity-custom", "lawA retains unique ID 'law-gravity-custom'");
        check(lawB->getIdentifier() == "law-gravity-preset", "lawB retains unique ID 'law-gravity-preset'");
        check(lm.find("law-gravity-custom") == lawA.get(), "find('law-gravity-custom') resolves lawA");
        check(lm.find("law-gravity-preset") == lawB.get(), "find('law-gravity-preset') resolves lawB");

        const std::size_t countBeforeDuplicate = lm.getAll().size();
        auto duplicate = lm.createLaw("Different Display Name", "law-gravity-custom", {&author});
        check(duplicate == nullptr, "explicit create refuses an already-registered identifier");
        check(lm.getAll().size() == countBeforeDuplicate, "duplicate identifier does not change the Law register");
        check(lm.find("law-gravity-custom") == lawA.get(), "duplicate create leaves the original identity binding intact");
    }

    // ------------------------------------------------------------------
    // 3. Same identifier lookup targets the existing Law
    // ------------------------------------------------------------------
    std::cout << "\n[3] Re-authoring existing identifier finds existing Law\n";
    {
        std::size_t countBefore = lm.getAll().size();
        Law* existing = lm.find("law-gravity-custom");
        check(existing != nullptr, "Found existing law by identifier");

        existing->setEnabled(false);
        check(!existing->isEnabled(), "Successfully mutated existing law state");
        check(lm.getAll().size() == countBefore, "Identifier lookup did not change the Law register");
    }

    // ------------------------------------------------------------------
    // 4. Display name alone is NOT an identity key
    // ------------------------------------------------------------------
    std::cout << "\n[4] Display name alone is NOT an identity key\n";
    {
        check(lm.find("Gravity") == nullptr, "lm.find('Gravity') by display name returns nullptr");
        check(lm.find("Authored Law") == nullptr, "lm.find('Authored Law') by display name returns nullptr");

        // WebSocket property writes use LawManager::find for both the literal
        // identifier and the optional '@'-stripped identifier. Display names
        // must not resolve even when two Laws share the same visible name.
        const auto resolveSocketLaw = [&](const std::string& target) -> Law* {
            const std::string normalized = (!target.empty() && target[0] == '@')
                ? target.substr(1) : target;
            if (Law* law = lm.find(target)) return law;
            return lm.find(normalized);
        };
        check(resolveSocketLaw("Gravity") == nullptr,
              "WebSocket-style Law lookup refuses a shared display name");
        check(resolveSocketLaw("@Gravity") == nullptr,
              "WebSocket-style Law lookup refuses prefixed display names");
        check(resolveSocketLaw("law-gravity-custom") == lm.find("law-gravity-custom"),
              "WebSocket-style Law lookup resolves an exact unique identifier");
        check(resolveSocketLaw("@law-gravity-preset") == lm.find("law-gravity-preset"),
              "WebSocket-style Law lookup resolves an @-prefixed unique identifier");
    }


    // ------------------------------------------------------------------
    // 5. A drive resolves its subject and retained event participant on
    //    later event-less ticks. Missing participants must not be invented.
    // ------------------------------------------------------------------
    std::cout << "\n[5] Drive-session indexed subject and event-object lookup\n";
    {
        Object subject, other;
        bool subjectPresent = true, otherPresent = true;
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            if (subjectPresent) beings.push_back(&subject);
            if (otherPresent) beings.push_back(&other);
        });

        LawManager drives;
        drives.connectToEventBus();
        auto law = drives.createLaw("indexed-event-object-drive", {&author});
        law->setDrives(true);
        law->setActionModel(ActionNode::set(
            "@event.object.position.y", PropertyValue(19.0)));
        const std::size_t alpha = drives.rete().addAlphaNode(
            "type == indexed-drive", [](const FactPtr& fact) {
                return fact->type == "indexed-drive";
            });
        drives.rete().bindLawToAlpha(law->getIdentifier(), alpha);

        Universe::instance().setClock(1000.0, 0.1);
        Core::EventBus::instance().publish(
            ECA::Event{"indexed-drive", &subject, &other, std::time(nullptr)});
        drives.tick();
        check(drives.driveSessions().size() == 1, "event starts exactly one drive");
        check(other.getPosition().y == 19.0f, "event action writes its object");

        other.setPosition(glm::vec3(0.0f));
        Universe::instance().setClock(1001.0, 0.1);
        drives.tick();
        check(other.getPosition().y == 19.0f,
              "event object remains addressable on the next event-less tick");

        otherPresent = false;
        other.setPosition(glm::vec3(0.0f));
        Universe::instance().setClock(1002.0, 0.1);
        drives.tick();
        check(other.getPosition().y == 0.0f,
              "missing event object is not resolved from stale identity");

        otherPresent = true;
        Universe::instance().setClock(1003.0, 0.1);
        drives.tick();
        check(other.getPosition().y == 19.0f,
              "restored event object resolves again by its identifier");

        subjectPresent = false;
        Universe::instance().setClock(1004.0, 0.1);
        drives.tick();
        check(drives.driveSessions().empty(),
              "missing subject ends its drive session");
        Universe::instance().setProvider(nullptr);
    }

    // ------------------------------------------------------------------
    // 6. Duplicate identifiers are not a uniqueness guarantee. The first
    //    being in the Universe snapshot wins, even for a drive originally
    //    launched by a later being with the same identifier.
    // ------------------------------------------------------------------
    std::cout << "\n[6] Drive-session duplicate identifier first-match order\n";
    {
        Object first, second;
        first.setObjectID("duplicate-drive-subject");
        second.setObjectID("duplicate-drive-subject");
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&first);
            beings.push_back(&second);
        });

        LawManager drives;
        drives.connectToEventBus();
        auto law = drives.createLaw("duplicate-id-drive", {&author});
        law->setDrives(true);
        law->setActionModel(ActionNode::set("position.y", PropertyValue(7.0)));
        const std::size_t alpha = drives.rete().addAlphaNode(
            "type == duplicate-drive", [](const FactPtr& fact) {
                return fact->type == "duplicate-drive";
            });
        drives.rete().bindLawToAlpha(law->getIdentifier(), alpha);

        Universe::instance().setClock(2000.0, 0.1);
        Core::EventBus::instance().publish(
            ECA::Event{"duplicate-drive", &second, nullptr, std::time(nullptr)});
        drives.tick();
        check(drives.driveSessions().size() == 1,
              "duplicate-ID subject starts one drive");

        first.setPosition(glm::vec3(0.0f));
        second.setPosition(glm::vec3(0.0f));
        Universe::instance().setClock(2001.0, 0.1);
        drives.tick();
        check(first.getPosition().y == 7.0f,
              "first matching snapshot being receives the later drive tick");
        check(second.getPosition().y == 0.0f,
              "later duplicate does not override the first match");
        Universe::instance().setProvider(nullptr);
    }

    std::cout << "\nSUCCESS — " << g_checks << " checks passed.\n";
    return 0;
}
