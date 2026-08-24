// Set-to-set grouping: merging subformations must absorb only the matches.
//
// A Formation groups its relations into subformations — one per (type, connected
// component) — so a set built from a set stays legible. When a new relation
// bridges several existing subformations, they merge into one.
//
// The merge used to walk `subformations` by index and erase as it went. Each
// erase shifted every later element down, so a subformation that never matched
// could slide into a slot the loop had yet to visit — and get absorbed into a
// group it has no relation to, while a true match was skipped. This test pins
// the invariant: matched sets merge, unmatched sets are left alone.

#include "ConstructedBeing/Singular/Object/Formation/Formation.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Relation.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

bool formationHas(const Formation& f, const Singular* s) {
    for (const auto* m : f.getMembers()) {
        if (m == s) return true;
    }
    return false;
}

} // namespace

int main() {
    // Object construction allocates GL textures, so the test needs a context.
    if (!glfwInit()) {
        std::fprintf(stderr, "formation_merge_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "formation_merge_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "formation_merge_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    Formation root;

    // Eight beings, all members of the root set so relation endpoints resolve.
    std::vector<std::unique_ptr<Object>> beings;
    for (int i = 0; i < 8; ++i) {
        beings.push_back(std::make_unique<Object>());
        root.addMember(beings.back().get());
    }
    Object* o1 = beings[0].get();
    Object* o2 = beings[1].get();
    Object* o3 = beings[2].get();
    Object* o4 = beings[3].get();
    Object* o5 = beings[4].get();

    // Four subformations, in this order:
    //   [0] "bond"  {o1,o2}      <- becomes the merge primary
    //   [1] "other" {o3,o4}      <- MUST survive untouched (different type)
    //   [2] "bond"  {o5,o6}
    //   [3] "bond"  {o7,o8}
    root.addRelation(std::make_shared<Relation>("bond", *o1, *o2));
    root.addRelation(std::make_shared<Relation>("other", *o3, *o4));
    root.addRelation(std::make_shared<Relation>("bond", *beings[4], *beings[5]));
    root.addRelation(std::make_shared<Relation>("bond", *beings[6], *beings[7]));
    assert(root.getSubformations().size() == 4);

    // Give the last "bond" set a member it shares with the first, so a single
    // relation can match three subformations at once: indices {0, 2, 3}. This
    // is the shape that exposed the shifting-index bug — the erase at index 2
    // used to drag index 1 ("other") into the merge.
    root.getSubformations()[3]->addMember(o1);

    root.addRelation(std::make_shared<Relation>("bond", *o1, *o5));

    // Three "bond" sets collapse into one; "other" is not a match and remains.
    const auto& subs = root.getSubformations();
    assert(subs.size() == 2);

    const Formation* bond = nullptr;
    const Formation* other = nullptr;
    for (const auto& sub : subs) {
        if (sub->getRelationTypeTag() == "bond") bond = sub.get();
        if (sub->getRelationTypeTag() == "other") other = sub.get();
    }
    assert(bond && "the merged bond set must survive");
    assert(other && "the unrelated 'other' set must not be absorbed");

    // Every bonded being landed in the primary.
    for (int i = 0; i < 8; ++i) {
        if (beings[i].get() == o3 || beings[i].get() == o4) continue;
        assert(formationHas(*bond, beings[i].get()) && "bonded being lost in merge");
    }

    // The "other" set kept its own members, and they did not leak into the bond.
    assert(formationHas(*other, o3) && formationHas(*other, o4));
    assert(!formationHas(*bond, o3) && !formationHas(*bond, o4) &&
           "unrelated set was wrongly absorbed");

    // Beings must die before the GL context they hold textures in.
    beings.clear();
    glfwDestroyWindow(window);
    glfwTerminate();

    std::printf("formation_merge_test: OK\n");
    return 0;
}
