// One being's facts being retracted must not cost every OTHER being its
// duplicate-prevention.
//
// FORMATION_RETE.md §8 rung 0 added `ReteNetwork::hasRelationStateFact` because
// three paths assert edge facts (the first-tick seed, the relation-formed
// handler, and the back-seed when a relation type first enters play) and
// `assertFact` does not deduplicate. Its own comment says why that matters:
// duplicates stack "into every alpha memory that matches, which is a standing
// per-tick propagation tax and, over a session of relations forming and
// dissolving, unbounded."
//
// That guard reads `_relationStateIndex` — and `retractStateFactsBySubject`
// CLEARED THAT INDEX WHOLESALE (found 2026-09-14, fixed 2026-09-16). So the
// moment any being's state facts were retracted — every object-destroyed, every
// relation-destroyed — the network forgot which edge facts it already held for
// everyone, and the next seed re-asserted them. The facts were not wrong; there
// were simply more and more of them, and every propagation walked them all.
//
// This test is about COUNTING, therefore, not about matching: the law fires
// correctly either way, which is exactly why nothing caught it.
//
// FOR FUTURE AGENTS (Jules especially): `_relationStateIndex` is keyed on the
// being POINTER, and the retraction path is given an IDENTIFIER, so the two are
// matched by asking each live subject its name. If you key it differently, keep
// the scope: retracting one being's facts may only forget that being.
// To-do: docs/Agenda/Tasks/Specific Tasks/Formation_Rete/Formation_Rete.md

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    if (!ok) { ++g_failures; std::printf("  FAILED: %s\n", what.c_str()); }
    else std::printf("  ok: %s\n", what.c_str());
}

std::size_t edgeFactsFor(const LawManager& mgr, const std::string& subjectId,
                         const std::string& kind) {
    std::size_t count = 0;
    for (const auto& fact : mgr.rete().facts()) {
        if (!fact) continue;
        if (fact->type == "relation-state" && fact->subjectId == subjectId &&
            fact->attribute == kind) {
            ++count;
        }
    }
    return count;
}

} // namespace

int main() {
    if (!glfwInit()) { std::fprintf(stderr, "relation_state_index_test: glfwInit failed\n"); return 1; }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "relation_state_index_test", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);

    {
        Object author;
        Object target;    target.setObjectID("category.target");
        Object bystander; bystander.setObjectID("bystander");
        std::vector<Singular*> population{&author, &target, &bystander};
        Universe::instance().setProvider([&](std::vector<Singular*>& out) {
            for (Singular* s : population) out.push_back(s);
        });
        RelationManager graph;
        Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
            for (const auto& r : graph.getAll()) if (r) out.push_back(r.get());
        });
        Universe::instance().setRelationsInvolvingProvider(
            [&](const Singular& being, std::vector<Relation*>& out) {
                graph.relationsInvolving(being, out);
            });
        Universe::instance().setClock(100.0, 0.1);

        LawManager mgr;
        mgr.connectToEventBus();

        auto law = mgr.createLaw("member-law", {&author});
        law->setActivation(Law::Activation::WhileTrue);
        law->setConditionModel(ConditionNode::related("instance-of", "category.target"));
        law->setActionModel(ActionNode::add("position.z", 1.0));

        graph.add(std::make_shared<Relation>("instance-of", bystander, target, true));
        mgr.tick();
        check(edgeFactsFor(mgr, "bystander", "instance-of") == 1,
              "the bystander has exactly one edge fact to begin with");

        // Someone ELSE lives and dies, over and over — and each round the
        // bystander gains ANOTHER edge of the same kind, to a different
        // category. One fact per (being, relation kind) is the whole shape of
        // `hasRelationStateFact`, so the bystander must still have exactly one
        // however many `instance-of` edges it carries.
        //
        // The death is what used to break it: retracting the passerby's state
        // facts cleared the index for EVERYONE, so the guard no longer knew the
        // bystander already had an `instance-of` fact, and the next
        // relation-formed handler asserted a second one. Then a third, a fourth.
        std::vector<std::unique_ptr<Object>> categories;
        for (int round = 0; round < 4; ++round) {
            auto passerby = std::make_unique<Object>();
            passerby->setObjectID("passerby-" + std::to_string(round));
            population.push_back(passerby.get());
            graph.add(std::make_shared<Relation>("instance-of", *passerby, target, true));
            Universe::instance().bumpStructuralRevision();
            mgr.tick();

            population.pop_back();
            graph.removeInvolving(passerby.get());
            RelationManager::forgetBeingEverywhere(passerby.get());
            passerby.reset();
            Universe::instance().bumpStructuralRevision();
            mgr.tick();

            auto category = std::make_unique<Object>();
            category->setObjectID("category.extra-" + std::to_string(round));
            population.push_back(category.get());
            graph.add(std::make_shared<Relation>("instance-of", bystander, *category, true));
            categories.push_back(std::move(category));
            Universe::instance().bumpStructuralRevision();
            mgr.tick();
        }

        const std::size_t bystanderFacts = edgeFactsFor(mgr, "bystander", "instance-of");
        std::printf("  bystander edge facts after four lives and deaths: %zu\n", bystanderFacts);
        check(bystanderFacts == 1,
              "a being uninvolved in any of it still has exactly one edge fact");

        // The guard must still work for the being whose facts WERE retracted:
        // re-seeding it is right, duplicating it is not.
        Object returning;  returning.setObjectID("passerby-0");
        population.push_back(&returning);
        graph.add(std::make_shared<Relation>("instance-of", returning, target, true));
        Universe::instance().bumpStructuralRevision();
        mgr.tick();
        mgr.tick();
        check(edgeFactsFor(mgr, "passerby-0", "instance-of") == 1,
              "and a being that comes back gets one fact, not two");

        Universe::instance().setRelationProvider(nullptr);
        Universe::instance().setProvider(nullptr);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("%s\n", g_failures ? "relation_state_index_test: FAILURES"
                                   : "relation_state_index_test: OK");
    return g_failures ? 1 : 0;
}
