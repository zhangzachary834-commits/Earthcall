// Hierarchy of Joys — first rung.
//
// Telos is a Lexeme. The hierarchy is a rooted Formation of Lexemes with
// directed `grounds` Relations. Formations and Relations of other beings
// are ordered by those ranks. There is no HierarchyOfJoys class.
// See docs/architecture/ontology/HIERARCHY_OF_JOYS.md.

#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Object/Formation/Formation.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "Singularity/Language/JoyHierarchy.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

#include <GLFW/glfw3.h>
#include <algorithm>
#include <cassert>
#include <cstdio>

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "hierarchy_of_joys_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "hierarchy_of_joys_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "hierarchy_of_joys_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    using Singularity::Language::LanguageSystem;
    using Singularity::Language::seedJoyHierarchy;

    // 1. The foundation is a Lexeme with a stable slug, not a uuid and not
    //    a skinned Object.
    auto christ = LanguageSystem::instance().foundation();
    assert(christ);
    assert(christ->getIdentifier() == LanguageSystem::kFoundationId);
    assert(christ->getSymbol() == LanguageSystem::kFoundationSymbol);
    assert(LanguageSystem::instance().resolve("Christ") == christ);

    // 2. Person("default") gets a rooted joy hierarchy. The string is a
    //    seed, not stored state.
    Person person(Soul("Zach"), Body::createBasicAvatar("Voxel"), "default");
    assert(person.satisfiesJoyBounds());
    assert(person.joys().isJoyHierarchy());
    assert(person.joys().root() == christ.get());
    assert(person.telosId() == LanguageSystem::kFoundationId);
    assert(person.getDisplayName() == "Zach");
    // Soul has no second name. After bind, it answers as this Person.
    assert(person.soul().getIdentifier() == person.getIdentifier());
    assert(person.soul().constructionName().empty());
    assert(person.soul().person() == &person);
    Soul unbound("hint");
    assert(unbound.getIdentifier().empty());
    assert(unbound.constructionName() == "hint");

    // 3. Empty foundation refuses a root. The being exists; the bound fails.
    Person unrooted(Soul("Guest"), Body::createBasicAvatar("Voxel"), "");
    assert(!unrooted.satisfiesJoyBounds());
    assert(unrooted.telosId().empty());

    // 4. Zone matches Person: string seeds a Formation.
    Zone home("Home", "default");
    assert(home.satisfiesJoyBounds());
    assert(home.joys().root() == christ.get());

    // 5. A richer authored ladder: Christ grounds Love, Love grounds Knowledge.
    //    Objects take those Lexemes as telos; a Formation of the objects
    //    orders by rank; a cycle of `grounds` is refused.
    auto love = LanguageSystem::instance().resolve("Love");
    auto knowledge = LanguageSystem::instance().resolve("Knowledge");
    Formation ladder;
    ladder.markJoyHierarchy();
    ladder.addMember(christ.get());
    ladder.addMember(love.get());
    ladder.addMember(knowledge.get());
    assert(ladder.setRoot(christ.get()));
    assert(ladder.addRelation(std::make_shared<Relation>(
        Formation::kGroundsType, *christ, *love, true)));
    assert(ladder.addRelation(std::make_shared<Relation>(
        Formation::kGroundsType, *love, *knowledge, true)));
    // Cycle: Knowledge grounds Christ would close the loop.
    assert(!ladder.addRelation(std::make_shared<Relation>(
        Formation::kGroundsType, *knowledge, *christ, true)));
    assert(ladder.satisfiesJoyBounds());
    assert(ladder.getRelationTypeTag() == Formation::kJoyHierarchyTag);
    PropertyValue tag, rootId, count;
    assert(PropertyPath::parse("relationTypeTag").getValue(ladder, tag)
           == PropertyPath::PathResult::Ok);
    assert(std::get<std::string>(tag) == Formation::kJoyHierarchyTag);
    assert(PropertyPath::parse("root").getValue(ladder, rootId)
           == PropertyPath::PathResult::Ok);
    assert(std::get<std::string>(rootId) == christ->getIdentifier());
    assert(PropertyPath::parse("memberCount").getValue(ladder, count)
           == PropertyPath::PathResult::Ok);
    assert(std::get<int>(count) == 3);
    assert(ladder.rankOf(*christ) == 0);
    assert(ladder.rankOf(*love) == 1);
    assert(ladder.rankOf(*knowledge) == 2);

    Object altar, gift, treatise;
    altar.setTelosId(christ->getIdentifier());
    gift.setTelosId(love->getIdentifier());
    treatise.setTelosId(knowledge->getIdentifier());
    Object stray;
    stray.setTelosId("lexeme.unknown");

    assert(ladder.rankOf(altar) == 0);
    assert(ladder.rankOf(gift) == 1);
    assert(ladder.rankOf(treatise) == 2);
    assert(ladder.rankOf(stray) == -1);

    Formation ofBeings;
    ofBeings.addMember(&treatise);
    ofBeings.addMember(&altar);
    ofBeings.addMember(&gift);
    ofBeings.addMember(&stray);
    const auto byTelos = ofBeings.orderMembersBy(ladder);
    assert(byTelos.size() == 4);
    assert(byTelos[0] == &altar);
    assert(byTelos[1] == &gift);
    assert(byTelos[2] == &treatise);
    assert(byTelos[3] == &stray);

    // 6. Relations among those beings order by the more-foundational end.
    auto near = std::make_shared<Relation>("near", altar, gift, false);
    auto far  = std::make_shared<Relation>("cites", treatise, altar, true);
    Formation bonds;
    bonds.addMember(&altar);
    bonds.addMember(&gift);
    bonds.addMember(&treatise);
    assert(bonds.addRelation(far));
    assert(bonds.addRelation(near));
    const auto rels = bonds.orderRelationsBy(ladder);
    assert(rels.size() == 2);
    assert(rels[0]->type == "near");   // altar(0)–gift(1) before treatise(2)–altar(0)
    assert(rels[1]->type == "cites");

    LanguageSystem::instance().clear();
    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("hierarchy_of_joys_test: ALL OK");
    return 0;
}
