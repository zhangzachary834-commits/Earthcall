// A Relation is about a being's IDENTITY, not only about where it sits in memory.
//
// CLAUDE.md non-negotiable, Stable Identifiers: "Law text addresses beings by name."
// A Relation endpoint keeps that name even when it has no pointer:
//
//   Relation::Endpoint::id()     returns `savedId` when `ptr` is null;
//   Relation::Endpoint::forget() nulls `ptr` but KEEPS the identifier;
//   RelationManager::loadFromJson keeps unbound edges "for a later bind".
//
// THE REGRESSION (introduced 2026-09-10 by Claude Opus 5, Formation Rete rung 4).
// To stop the `Related` predicate from calling aId()/bId() on every relation in the
// world, it was rewritten to recognise the subject by POINTER (`rel->a() == self`).
// That was recorded as "kept for safety, not speed" and as dereferencing fewer far
// ends — true — and as preserving meaning — false. A null pointer never equals a
// live subject, so `Related` stopped matching every edge whose endpoint is unbound or
// forgotten, even though its kept identifier names the subject exactly. Measured
// 2026-09-14: all three cases below returned 0 where the id-based predicate returned 1.
//
// When that happens in the real engine: a save loaded without a resolver; and — more
// commonly — a being is freed (forgetBeingEverywhere nulls the pointer, keeps the name)
// and a being with the SAME stable identifier returns, as it does whenever a Zone
// reload recreates its objects. The Law stays enabled, the Relation stays in the
// graph, and the Law silently stops seeing it.
//
// The tests that were cited as guarding the rewrite (continuous_law_test §8,
// add_relation_action_test, rete_relation_state_test) all use BOUND endpoints, so none
// could fail. This one exists for the case they cannot see.
//
// FOR FUTURE AGENTS (Jules especially): identify a Relation's endpoint by pointer when
// it has one, and by its kept identifier when it does not. Never by pointer alone. Any
// index over relations (Formation Rete rung 4 is about to build one) must be keyed so
// that an unbound endpoint is still findable by the being's identifier, or it will
// inherit this bug at index speed.
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
#include <vector>

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "related_identity_endpoint_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "related_identity_endpoint_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "related_identity_endpoint_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    ECA::Event probe;
    const auto related = [&](const char* type, const char* other, const Singular& subject) {
        return ConditionNode::related(type, other).compile()(probe, subject);
    };

    {
        Object piece;       piece.setObjectID("piece-7");
        Object category;    category.setObjectID("category.chess.piece");
        Object stranger;    stranger.setObjectID("piece-8");
        std::vector<Singular*> population{&piece, &category, &stranger};
        Universe::instance().setProvider([&](std::vector<Singular*>& b) {
            for (Singular* s : population) b.push_back(s);
        });

        // --------------------------------------------------------------
        // A. LOADED UNBOUND. A save read without a resolver keeps the edge and
        //    both identifiers, with null pointers.
        // --------------------------------------------------------------
        RelationManager loaded;
        loaded.loadFromJson(nlohmann::json::array({
            {{"type", "instance-of"}, {"entityA", "piece-7"},
             {"entityB", "category.chess.piece"}, {"directed", true}}}));
        Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
            for (const auto& r : loaded.getAll()) if (r) out.push_back(r.get());
        });
        assert(!loaded.getAll().empty() && loaded.getAll()[0]->a() == nullptr &&
               "precondition: the loaded edge really is unbound");

        assert(related("instance-of", "", piece) &&
               "an unbound edge still names its source by identifier");
        assert(related("instance-of", "category.chess.piece", piece) &&
               "an unbound edge still names its far end by identifier");
        assert(!related("instance-of", "", stranger) &&
               "identifier matching must not match a being the edge does not name");
        assert(!related("instance-of", "", category) &&
               "a directed edge still holds only OF its source, bound or not");

        // --------------------------------------------------------------
        // B. FORGOTTEN, THEN REBORN UNDER THE SAME NAME — the Zone-reload case.
        // --------------------------------------------------------------
        RelationManager live;
        auto oldPiece = std::make_unique<Object>();
        oldPiece->setObjectID("piece-9");
        auto edge = std::make_shared<Relation>("instance-of", *oldPiece, category, true);
        live.add(edge);
        edge->forgetEndpoint(oldPiece.get());           // what forgetBeingEverywhere does
        Object reborn;  reborn.setObjectID("piece-9");  // same stable identifier
        population.push_back(&reborn);
        Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
            for (const auto& r : live.getAll()) if (r) out.push_back(r.get());
        });
        assert(edge->a() == nullptr && edge->aId() == "piece-9" &&
               "precondition: the pointer is gone, the identifier is kept");
        assert(related("instance-of", "", reborn) &&
               "a being reborn under a stable identifier is still the one the edge names");
        assert(related("instance-of", "category.chess.piece", reborn));

        // --------------------------------------------------------------
        // C. BOUND EDGES KEEP THEIR FAST, EXACT MEANING.
        // --------------------------------------------------------------
        RelationManager bound;
        bound.add(std::make_shared<Relation>("instance-of", piece, category, true));
        Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
            for (const auto& r : bound.getAll()) if (r) out.push_back(r.get());
        });
        assert(related("instance-of", "category.chess.piece", piece));
        assert(!related("instance-of", "", stranger));
        assert(!related("instance-of", "", category) && "direction honoured for bound edges");

        Universe::instance().setRelationProvider(nullptr);
        Universe::instance().setProvider(nullptr);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::printf("related_identity_endpoint_test: OK\n");
    return 0;
}
