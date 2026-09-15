// The relation endpoint index must answer exactly what a full scan answers.
//
// FORMATION_RETE.md §8 rung 4. `Related` used to find a subject's edges by walking
// every relation in the world, once per candidate per tick; measured, a law scoped to
// a category cost 3.1x an identical law reading a plain property at 400 beings, the gap
// widening with the world. RelationManager::relationsInvolving now answers from an index.
//
// An index is allowed to change HOW MANY edges the condition examines. It is never
// allowed to change WHICH ones can match. So the oracle here is not a hand-computed
// answer alone: every case is evaluated twice — once with the index installed, once
// scanning every relation — and both must equal the expected value. A disagreement is a
// law that would fire in a test harness (which usually scans) and stay silent in the app
// (which always indexes), or the reverse.
//
// The cases are the ways an index goes wrong:
//   A. bound edges: direction, far end, relation type
//   B. undirected edges hold of both ends
//   C. an edge ADDED after the index was built
//   D. each of the four removal paths
//   E. an edge LOADED with unbound endpoints (kept identifiers only)
//   F. a being freed through forgetBeingEverywhere and REBORN under its stable name
//   G. an endpoint forgotten DIRECTLY, with the manager never told
//   H. a bound being RENAMED after the index was built
//   I. a COPIED manager
//   J. a far end named by the application event
//
// FOR FUTURE AGENTS (Jules especially): if you add any write to RelationManager's
// `relations` vector, call touch() and add a case here that goes through it. If you
// change how Relation endpoints bind or forget, add a case here. The failure this guards
// is silent: the Law stays enabled, the Relation stays in the graph, and the Law never
// sees it.
// To-do: docs/Agenda/Tasks/Specific Tasks/Formation_Rete/Formation_Rete.md

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Relation/Relation.hpp"
#include "Relation/RelationManager.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "json.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

RelationManager* g_active = nullptr;

void installScan() {
    Universe::instance().setRelationProvider([](std::vector<Relation*>& out) {
        for (const auto& r : g_active->getAll()) if (r) out.push_back(r.get());
    });   // also clears any index provider
}

void installIndexed() {
    installScan();
    Universe::instance().setRelationsInvolvingProvider(
        [](const Singular& being, std::vector<Relation*>& out) {
            g_active->relationsInvolving(being, out);
        });
}

// Evaluate one Related condition both ways and demand both give `expected`.
void expect(const char* what, const char* type, const char* other,
            const Singular& subject, bool expected) {
    ECA::Event probe;
    const auto pred = ConditionNode::related(type, other).compile();
    installScan();
    const bool scanned = pred(probe, subject);
    installIndexed();
    const bool indexed = pred(probe, subject);
    if (scanned != expected || indexed != expected) {
        std::fprintf(stderr, "FAILED %s: expected %d, scan gave %d, index gave %d\n",
                     what, (int)expected, (int)scanned, (int)indexed);
        assert(false && "the index and the full scan must both give the expected answer");
    }
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "relation_endpoint_index_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "relation_endpoint_index_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "relation_endpoint_index_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        Object piece;     piece.setObjectID("piece-1");
        Object category;  category.setObjectID("category.chess.piece");
        Object stranger;  stranger.setObjectID("stranger");
        Object left;      left.setObjectID("left");
        Object right;     right.setObjectID("right");
        std::vector<Singular*> population{&piece, &category, &stranger, &left, &right};
        Universe::instance().setProvider([&](std::vector<Singular*>& b) {
            for (Singular* s : population) b.push_back(s);
        });

        RelationManager g;
        g_active = &g;

        // ---------------- A. bound, directed ----------------
        g.add(std::make_shared<Relation>("instance-of", piece, category, true));
        expect("A source holds", "instance-of", "", piece, true);
        expect("A directed: not of target", "instance-of", "", category, false);
        expect("A unrelated being", "instance-of", "", stranger, false);
        expect("A far end by name", "instance-of", "category.chess.piece", piece, true);
        expect("A wrong far end", "instance-of", "stranger", piece, false);
        expect("A wrong type", "touching", "", piece, false);
        expect("A any type", "", "", piece, true);

        // ---------------- B. undirected ----------------
        g.add(std::make_shared<Relation>("touching", left, right, false));
        expect("B left end", "touching", "", left, true);
        expect("B right end", "touching", "right", left, true);
        expect("B right end holds too", "touching", "left", right, true);

        // ---------------- C. added after the index was built ----------------
        expect("C before add", "owns", "", stranger, false);   // builds the index
        g.add(std::make_shared<Relation>("owns", stranger, piece, true));
        expect("C after add", "owns", "", stranger, true);

        // ---------------- D. every removal path ----------------
        auto owns = g.getAll().back();
        assert(g.remove(owns));
        expect("D remove", "owns", "", stranger, false);

        g.add(std::make_shared<Relation>("owns", stranger, piece, true));
        expect("D re-added", "owns", "", stranger, true);
        assert(g.removeBetween(stranger, piece, "owns"));
        expect("D removeBetween(Singular)", "owns", "", stranger, false);

        g.add(std::make_shared<Relation>("owns", stranger, piece, true));
        expect("D re-added again", "owns", "", stranger, true);
        assert(g.removeBetween(std::string("stranger"), std::string("piece-1"), "owns"));
        expect("D removeBetween(string)", "owns", "", stranger, false);

        g.add(std::make_shared<Relation>("owns", stranger, piece, true));
        expect("D re-added a third time", "owns", "", stranger, true);
        assert(g.removeInvolving(&stranger));
        expect("D removeInvolving", "owns", "", stranger, false);
        expect("D removeInvolving spared others", "instance-of", "", piece, true);

        // ---------------- E. loaded unbound ----------------
        RelationManager loaded;
        loaded.loadFromJson(nlohmann::json::array({
            {{"type", "instance-of"}, {"entityA", "piece-1"},
             {"entityB", "category.chess.piece"}, {"directed", true}}}));
        g_active = &loaded;
        assert(loaded.getAll()[0]->a() == nullptr && "precondition: unbound");
        expect("E unbound source by name", "instance-of", "", piece, true);
        expect("E unbound far end by name", "instance-of", "category.chess.piece", piece, true);
        expect("E unbound: still directed", "instance-of", "", category, false);
        expect("E unbound: not a stranger", "instance-of", "", stranger, false);
        g_active = &g;

        // ---------------- F. freed, then reborn under the same stable name ----------------
        {
            RelationManager zoneGraph;
            g_active = &zoneGraph;
            auto dying = std::make_unique<Object>();
            dying->setObjectID("piece-9");
            zoneGraph.add(std::make_shared<Relation>("instance-of", *dying, category, true));
            population.push_back(dying.get());
            expect("F before death", "instance-of", "", *dying, true);   // engine indexes first
            population.pop_back();
            RelationManager::forgetBeingEverywhere(dying.get());   // engine order: before free
            dying.reset();
            Object reborn;  reborn.setObjectID("piece-9");
            population.push_back(&reborn);
            expect("F reborn under its stable identifier", "instance-of", "", reborn, true);
            expect("F reborn, far end", "instance-of", "category.chess.piece", reborn, true);
            population.pop_back();
            g_active = &g;
        }

        // ---------------- G. forgotten directly, manager never told ----------------
        {
            RelationManager zoneGraph;
            g_active = &zoneGraph;
            Object original;  original.setObjectID("piece-5");
            auto edge = std::make_shared<Relation>("instance-of", original, category, true);
            zoneGraph.add(edge);
            expect("G while bound", "instance-of", "", original, true);   // builds the index
            edge->forgetEndpoint(&original);                              // no touch()
            Object reborn;  reborn.setObjectID("piece-5");
            expect("G reborn after an untold forget", "instance-of", "", reborn, true);
            g_active = &g;
        }

        // ---------------- H. renamed after the index was built ----------------
        {
            RelationManager zoneGraph;
            g_active = &zoneGraph;
            Object renamed;   renamed.setObjectID("old-name");
            zoneGraph.add(std::make_shared<Relation>("instance-of", renamed, category, true));
            expect("H before rename", "instance-of", "", renamed, true);   // builds the index
            renamed.setObjectID("new-name");                               // no touch()
            expect("H renamed being is still itself", "instance-of", "", renamed, true);
            Object impostor;  impostor.setObjectID("old-name");
            expect("H a new being with the OLD name is not it", "instance-of", "", impostor, false);
            g_active = &g;
        }

        // ---------------- I. copied manager ----------------
        {
            expect("I original", "instance-of", "", piece, true);   // original index built
            RelationManager copy(g);
            g_active = &copy;
            expect("I copy sees the edge", "instance-of", "", piece, true);
            copy.add(std::make_shared<Relation>("owns", left, right, true));
            expect("I copy sees its own addition", "owns", "", left, true);
            g_active = &g;
            expect("I original does not", "owns", "", left, false);
        }

        // ---------------- J. far end named by the application event ----------------
        Universe::instance().setApplicationEvent(&category, nullptr);
        expect("J far end is the event subject", "instance-of", "@event.subject", piece, true);
        Universe::instance().setApplicationEvent(&stranger, nullptr);
        expect("J far end is not the event subject", "instance-of", "@event.subject", piece, false);
        Universe::instance().clearApplicationEvent();

        Universe::instance().setRelationProvider(nullptr);
        Universe::instance().setProvider(nullptr);
        g_active = nullptr;
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("relation_endpoint_index_test: OK\n");
    return 0;
}
