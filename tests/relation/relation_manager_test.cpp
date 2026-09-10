#include "Relation/RelationManager.hpp"
#include "Relation/Relation.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"

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

protected:
    void buildProperties() override {}
};

void test_add_and_retrieve() {
    std::cout << "--- test_add_and_retrieve ---\n";
    RelationManager rm;
    DummySingular a("nodeA");
    DummySingular b("nodeB");

    auto r1 = std::make_shared<Relation>("connects_to", a, b, true);
    rm.add(r1);

    auto relsA = rm.getRelationsOf(a);
    check(relsA.size() == 1, "A should have 1 relation");

    auto relsBetween = rm.getRelationsBetween(a, b);
    check(relsBetween.size() == 1, "Should find 1 relation between A and B");
    if (relsBetween.size() > 0) {
        check(relsBetween[0]->type == "connects_to", "Relation type matches");
    }

    auto relsBetweenId = rm.getRelationsBetween("nodeA", "nodeB");
    check(relsBetweenId.size() == 1, "Should find 1 relation by string IDs");
}

void test_remove() {
    std::cout << "--- test_remove ---\n";
    RelationManager rm;
    DummySingular a("nodeA");
    DummySingular b("nodeB");

    auto r1 = std::make_shared<Relation>("friend", a, b, false);
    rm.add(r1);

    check(rm.getAll().size() == 1, "Manager has 1 relation initially");

    bool removed = rm.remove(r1);
    check(removed, "remove() returned true");
    check(rm.getAll().size() == 0, "Manager should be empty after removal");

    // Add again to test removeBetween
    auto r2 = std::make_shared<Relation>("enemy", a, b, false);
    rm.add(r2);
    check(rm.getAll().size() == 1, "Manager has 1 relation again");

    bool removedBetween = rm.removeBetween(a, b, "enemy");
    check(removedBetween, "removeBetween() returned true");
    check(rm.getAll().size() == 0, "Manager should be empty after removeBetween");
}

void test_remove_involving() {
    std::cout << "--- test_remove_involving ---\n";
    RelationManager rm;
    DummySingular a("nodeA");
    DummySingular b("nodeB");
    DummySingular c("nodeC");

    rm.add(std::make_shared<Relation>("link", a, b, false));
    rm.add(std::make_shared<Relation>("link", b, c, false));

    check(rm.getAll().size() == 2, "Manager has 2 relations initially");

    bool removedInvolving = rm.removeInvolving(&b);
    check(removedInvolving, "removeInvolving() returned true");
    check(rm.getAll().size() == 0, "Manager should be empty as both involved B");
}

void test_cycle_detection() {
    std::cout << "--- test_cycle_detection ---\n";
    RelationManager rm;
    DummySingular a("nodeA");
    DummySingular b("nodeB");
    DummySingular c("nodeC");

    // A -> B
    rm.add(std::make_shared<Relation>("parent_of", a, b, true));
    // B -> C
    rm.add(std::make_shared<Relation>("parent_of", b, c, true));

    bool cycles = rm.wouldFormCycle(&c, &a, "parent_of");
    check(cycles, "Adding C -> A should form a cycle for 'parent_of'");

    bool noCycle = rm.wouldFormCycle(&a, &c, "parent_of");
    check(!noCycle, "Adding A -> C should not form a cycle");
}

void test_forget_being() {
    std::cout << "--- test_forget_being ---\n";
    // Using a separate scope for the manager to ensure liveManagers behavior
    RelationManager rm;
    DummySingular a("nodeA");
    DummySingular* b = new DummySingular("nodeB");

    auto r = std::make_shared<Relation>("test_bond", a, *b, false);
    rm.add(r);

    check(r->b() == b, "Endpoint B should initially point to b");

    // Simulate being leaving the world
    RelationManager::forgetBeingEverywhere(b);

    check(r->b() == nullptr, "Endpoint B should be null after forgetBeingEverywhere");
    check(r->bId() == "nodeB", "Saved ID for endpoint B should be preserved");

    delete b;
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Running RelationManager tests...\n";
    std::cout << "============================================================\n";

    test_add_and_retrieve();
    test_remove();
    test_remove_involving();
    test_cycle_detection();
    test_forget_being();

    std::cout << "============================================================\n";
    std::cout << "RelationManager test summary: "
              << g_checks << " checks, " << g_failures << " failures\n";
    std::cout << "============================================================\n";

    return (g_failures == 0) ? 0 : 1;
}
