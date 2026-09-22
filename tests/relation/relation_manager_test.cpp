#include "Relation/RelationManager.hpp"
#include "Relation/Relation.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "json.hpp"

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

void test_get_relations_of_type() {
    std::cout << "--- test_get_relations_of_type ---\n";
    RelationManager rm;
    DummySingular a("nodeA");
    DummySingular b("nodeB");
    rm.add(std::make_shared<Relation>("type1", a, b, true));
    rm.add(std::make_shared<Relation>("type2", a, b, true));

    auto rels = rm.getRelationsOfType("type1");
    check(rels.size() == 1, "Should find 1 relation of type1");
    if (rels.size() > 0) {
        check(rels[0]->type == "type1", "Found relation should be of type1");
    }
}

void test_find_adjacent_entities() {
    std::cout << "--- test_find_adjacent_entities ---\n";
    RelationManager rm;
    DummySingular a("nodeA");
    DummySingular b("nodeB");
    DummySingular c("nodeC");

    rm.add(std::make_shared<Relation>("link", a, b, true));
    rm.add(std::make_shared<Relation>("friend", b, c, false)); // undirected

    auto adjA = rm.findAdjacentEntities("nodeA");
    check(adjA.size() == 1 && adjA[0] == "nodeB", "nodeA should have nodeB as adjacent");

    auto adjB = rm.findAdjacentEntities("nodeB");
    // "link" is directed nodeA -> nodeB, so nodeB does not see nodeA via findAdjacentEntities since nodeB is not the source and relation is directed
    // "friend" is undirected nodeB <-> nodeC, so nodeB sees nodeC. Total = 1.
    check(adjB.size() == 1 && adjB[0] == "nodeC", "nodeB should have nodeC as adjacent");

    auto adjB_link = rm.findAdjacentEntities("nodeB", "link");
    check(adjB_link.size() == 0, "nodeB has no outgoing link relation");
}

void test_json_serialization() {
    std::cout << "--- test_json_serialization ---\n";
    RelationManager rm;
    DummySingular a("nodeA");
    DummySingular b("nodeB");
    rm.add(std::make_shared<Relation>("type1", a, b, true));

    nlohmann::json j = rm.toJson();
    RelationManager rm2;
    rm2.loadFromJson(j);

    check(rm2.getAll().size() == 1, "Loaded manager should have 1 relation");
    if (rm2.getAll().size() > 0) {
        check(rm2.getAll()[0]->type == "type1", "Loaded relation should have type1");
        check(rm2.getAll()[0]->aId() == "nodeA", "Loaded relation aId should match");
    }
}

void test_relations_involving() {
    std::cout << "--- test_relations_involving ---\n";
    RelationManager rm;
    DummySingular a("nodeA");
    DummySingular b("nodeB");
    auto rel = std::make_shared<Relation>("type1", a, b, true);
    rm.add(rel);

    std::vector<Relation*> out;
    rm.relationsInvolving(a, out);
    check(out.size() == 1, "relationsInvolving should find 1 relation for a");
    if (out.size() > 0) {
        check(out[0] == rel.get(), "relationsInvolving should return the correct relation pointer");
    }
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
    test_get_relations_of_type();
    test_find_adjacent_entities();
    test_json_serialization();
    test_relations_involving();

    std::cout << "============================================================\n";
    std::cout << "RelationManager test summary: "
              << g_checks << " checks, " << g_failures << " failures\n";
    std::cout << "============================================================\n";

    return (g_failures == 0) ? 0 : 1;
}
