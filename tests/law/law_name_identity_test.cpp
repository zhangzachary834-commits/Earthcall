// Regression test for Law name vs identity separation.
// Verifies that Laws sharing identical display names (e.g., "Gravity") remain distinct beings,
// and that resolution by identifier does not collide or collapse based on display name spelling.

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
    std::cout << "Running Law Name Identity Test...\n";

    Object author;
    author.setName("TestAuthor");

    LawManager lm;

    // 1. Create two distinct Law instances with identical display names but different identifiers
    std::cout << "\n[1] Creating distinct Laws with identical display names\n";
    auto law1 = lm.createLaw("Gravity", "law-gravity-zone-a", {&author});
    auto law2 = lm.createLaw("Gravity", "law-gravity-zone-b", {&author});

    check(law1 != nullptr, "law1 created");
    check(law2 != nullptr, "law2 created");
    check(law1.get() != law2.get(), "law1 and law2 are distinct Law objects");
    check(law1->name() == law2->name(), "law1 and law2 share identical display name 'Gravity'");
    check(law1->getIdentifier() == "law-gravity-zone-a", "law1 has identifier 'law-gravity-zone-a'");
    check(law2->getIdentifier() == "law-gravity-zone-b", "law2 has identifier 'law-gravity-zone-b'");

    // 2. Lookup by unique identifier strictly distinguishes the two Laws
    std::cout << "\n[2] Resolution by unique identifier\n";
    check(lm.find("law-gravity-zone-a") == law1.get(), "find('law-gravity-zone-a') returns law1");
    check(lm.find("law-gravity-zone-b") == law2.get(), "find('law-gravity-zone-b') returns law2");

    // 3. Lookup by display name alone does not resolve a Law (a name points, does not constitute identity)
    std::cout << "\n[3] Resolution by display name alone returns nullptr\n";
    check(lm.find("Gravity") == nullptr, "find('Gravity') returns nullptr");

    std::cout << "\nALL OK — " << g_checks << " checks passed.\n";
    return 0;
}
