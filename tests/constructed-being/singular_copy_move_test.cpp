// Singular copy/move field-preservation test (Bugs.md #8 guard).
//
// Singular::Singular(const Singular&) used to copy only `_telosId`, silently
// dropping designatedZones, _stakeholders, _dynamicProperties,
// _dataStructures, and name on every copy — and Zone, Relation, and Object
// all copy through it. Fixed in code 2026-08-24 (`ce5c1cbe`, filed under the
// misleading title "Attempt to fix chess lag"), but shipped with no
// regression test. This is that test: author a being with all five fields
// populated (dynamic properties spanning several PropertyValue kinds),
// copy-construct, copy-assign, move-construct, and move-assign, and assert
// every field survives each operation.

#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Property/DataStructure.hpp"

#include <glm/glm.hpp>
#include <cassert>
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

// Minimal concrete Singular. buildProperties() is pure virtual on the base
// and irrelevant to this test, so it is left empty rather than registering
// a vocabulary nobody here reads.
class TestSingular : public Singular {
public:
    std::string id;
    explicit TestSingular(std::string identifier) : id(std::move(identifier)) {}
    std::string getIdentifier() const override { return id; }
    void setName(const std::string& n) { name = n; }
    const std::string& getName() const { return name; }

protected:
    void buildProperties() override {}
};

void populate(TestSingular& s) {
    s.setName("Zach's Test Being");
    s.addZoneDesignation("Sanctum of Beginnings");
    s.addZoneDesignation("Home");
    s.addStakeholder("shape.r", "grok-4.6", "law-chess-click", 1700000000);
    s.setDynamicProperty("gridX", 4);
    s.setDynamicProperty("weight", 2.5f);
    s.setDynamicProperty("isSelected", true);
    s.setDynamicProperty("label", std::string("pawn"));
    s.setDynamicProperty("offset", glm::vec3(1.0f, 2.0f, 3.0f));
    s.addDataStructure(DataStructure("inventory", std::string("sword")));
    s.setTelosId("lexeme.play");
}

void assertPopulated(const TestSingular& s, const std::string& label) {
    check(s.getName() == "Zach's Test Being", label + ": name survived");
    check(s.getDesignatedZones().size() == 2 &&
              s.belongsToZone("Sanctum of Beginnings") && s.belongsToZone("Home"),
          label + ": designatedZones survived");
    check(s.stakeholders().size() == 1 &&
              s.stakeholders()[0].propertyPath == "shape.r" &&
              s.stakeholders()[0].authorId == "grok-4.6",
          label + ": stakeholders survived");

    PropertyValue v;
    check(s.getDynamicProperty("gridX", v) && std::get<int>(v) == 4,
          label + ": dynamic int property survived");
    check(s.getDynamicProperty("weight", v) && std::get<float>(v) == 2.5f,
          label + ": dynamic float property survived");
    check(s.getDynamicProperty("isSelected", v) && std::get<bool>(v) == true,
          label + ": dynamic bool property survived");
    check(s.getDynamicProperty("label", v) && std::get<std::string>(v) == "pawn",
          label + ": dynamic string property survived");
    check(s.getDynamicProperty("offset", v) && std::get<glm::vec3>(v) == glm::vec3(1.0f, 2.0f, 3.0f),
          label + ": dynamic glm::vec3 property survived");

    auto* ds = const_cast<TestSingular&>(s).getDataStructure("inventory");
    check(ds != nullptr && std::get<std::string>(ds->data) == "sword",
          label + ": authored data structure survived");
    check(s.telosId() == "lexeme.play", label + ": telosId survived");
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running Singular copy/move field preservation test (Bugs.md #8)...\n";
    std::cout << "============================================================\n";

    TestSingular original("test-being-1");
    populate(original);
    assertPopulated(original, "original");

    TestSingular copyConstructed(original);
    assertPopulated(copyConstructed, "copy-constructed");

    TestSingular copyAssigned("test-being-2");
    copyAssigned = original;
    assertPopulated(copyAssigned, "copy-assigned");

    TestSingular moveSource("test-being-3");
    populate(moveSource);
    TestSingular moveConstructed(std::move(moveSource));
    assertPopulated(moveConstructed, "move-constructed");

    TestSingular moveAssignSource("test-being-4");
    populate(moveAssignSource);
    TestSingular moveAssigned("test-being-5");
    moveAssigned = std::move(moveAssignSource);
    assertPopulated(moveAssigned, "move-assigned");

    std::cout << "============================================================\n";
    std::cout << "Singular copy/move test summary: "
              << g_checks << " checks, " << g_failures << " failures\n";
    std::cout << "============================================================\n";

    return (g_failures == 0) ? 0 : 1;
}
