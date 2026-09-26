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
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include <cassert>
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
    }

    std::cout << "\nSUCCESS — " << g_checks << " checks passed.\n";
    return 0;
}
