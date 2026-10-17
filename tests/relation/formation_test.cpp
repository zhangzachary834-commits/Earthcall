#include "Relation/Formation/Formation.hpp"
#include "Relation/Relation.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include <iostream>
#include <string>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << "\n";
        return;
    }
    std::cout << "  ok: " << description << "\n";
}

class DummySingular : public Singular {
public:
    std::string id;
    explicit DummySingular(std::string identifier) : id(std::move(identifier)) {}
    std::string getIdentifier() const override { return id; }
    void buildProperties() override {}
};

void run_tests() {
    std::cout << "=== Formation Test ===\n";

    Formation formation;
    check(formation.hasRoot() == false, "Initial formation has no root");

    DummySingular beingA("BeingA");
    DummySingular beingB("BeingB");

    formation.addElement(beingA);
    formation.addElement(beingB);

    check(formation.getMembers().size() == 2, "Formation has 2 members");

    formation.setRoot(&beingA);
    check(formation.hasRoot() == true, "Formation has root");
    check(formation.isRoot(&beingA) == true, "BeingA is root");

    formation.removeElement(beingB);
    check(formation.getMembers().size() == 1, "Formation has 1 member after removal");

    Singularity::Language::Lexeme typeLexeme("TestSymbol", "TestType");
    auto relation = std::make_shared<Relation>(typeLexeme, beingA, beingB, true, 1.0f);

    // BeingB must be a member to add a relation involving it, let's add it back
    formation.addElement(beingB);
    bool added = formation.addRelation(relation);
    check(added == true, "Relation added successfully");
}

} // namespace

int main() {
    run_tests();
    std::cout << "\nTest summary: " << (g_checks - g_failures) << "/" << g_checks << " passed.\n";
    return g_failures > 0 ? 1 : 0;
}
