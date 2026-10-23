#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"

#include <iostream>
#include <string>
#include <cassert>

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
    std::cout << "=== Relation Test ===\n";

    Singularity::Language::Lexeme typeLexeme("TestSymbol", "TestType");
    DummySingular beingA("BeingA");
    DummySingular beingB("BeingB");

    Relation relation(typeLexeme, beingA, beingB, true, 2.5f);

    check(relation.typeLabel() == "TestSymbol", "Initial typeLabel");
    check(relation.aId() == "BeingA", "Initial aId");
    check(relation.bId() == "BeingB", "Initial bId");
    check(relation.directed == true, "Initial directed");
    check(relation.getWeight() == 2.5f, "Initial weight");

    Singularity::Language::Lexeme newTypeLexeme("NewTestSymbol", "NewTestType");
    relation.setTypeLexeme(&newTypeLexeme);
    check(relation.typeLabel() == "NewTestSymbol", "Updated typeLabel");

    check(relation.isBetween(beingA, beingB) == true, "isBetween true for BeingA and BeingB");
    check(relation.isBetween("BeingA", "BeingB") == true, "isBetween string true for BeingA and BeingB");
    check(relation.involves(&beingA) == true, "involves true for BeingA");
    check(relation.involves(&beingB) == true, "involves true for BeingB");
    check(relation.involves("BeingA") == true, "involves string true for BeingA");

    DummySingular beingC("BeingC");
    check(relation.involves(&beingC) == false, "involves false for BeingC");
}

} // namespace

int main() {
    run_tests();

    std::cout << "\nTest summary: " << (g_checks - g_failures) << "/" << g_checks << " passed.\n";
    return g_failures > 0 ? 1 : 0;
}
