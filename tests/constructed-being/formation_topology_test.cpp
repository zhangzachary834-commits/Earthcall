#include "Relation/Formation/Formation.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Relation.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>

namespace {
bool formationHas(const Formation& f, const Singular* s) {
    for (const auto* m : f.getMembers()) {
        if (m == s) return true;
    }
    return false;
}
} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "formation_topology_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "formation_topology_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "formation_topology_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    // Create objects
    std::vector<std::unique_ptr<Object>> beings;
    for (int i = 0; i < 6; ++i) {
        beings.push_back(std::make_unique<Object>());
    }

    Object* a = beings[0].get();
    Object* b = beings[1].get();
    Object* c = beings[2].get();
    Object* d = beings[3].get(); // peripheral connected via directed edge to C
    Object* e = beings[4].get(); // separate node
    Object* f = beings[5].get(); // separate node

    Formation form;
    form.addMember(a);
    form.addMember(b);
    form.addMember(c);
    form.addMember(d);
    form.addMember(e);
    form.addMember(f);

    // A-B and B-C and C-A (undirected triangle, size 3 valid core)
    auto ab = std::make_shared<Relation>("bond", *a, *b);
    ab->directed = false;
    auto bc = std::make_shared<Relation>("bond", *b, *c);
    bc->directed = false;
    auto ca = std::make_shared<Relation>("bond", *c, *a);
    ca->directed = false;

    // C->D (directed, peripheral)
    auto cd = std::make_shared<Relation>("bond", *c, *d);
    cd->directed = true;

    // E-F (undirected line, size 2 invalid core)
    auto ef = std::make_shared<Relation>("bond", *e, *f);
    ef->directed = false;

    form.addRelation(ab);
    form.addRelation(bc);
    form.addRelation(ca);
    form.addRelation(cd);
    form.addRelation(ef);

    // Initial resolution.
    //
    // Adjacency is direction-BLIND: direction is what a relation means, not a
    // statement about whether two beings are held together. So C->D joins D to
    // the A-B-C component, giving one component of four and one of two.
    Formation::Topology t1 = form.resolveTopology();

    // One valid core (size >= 3), so nothing is spawned off; {E,F} is too small
    // to stand on its own and comes back as orphaned rather than being erased.
    assert(t1.applied);
    assert(!t1.rooted);
    assert(t1.spawned.empty());
    assert(t1.orphaned.size() == 2);

    // Check members
    assert(formationHas(form, a));
    assert(formationHas(form, b));
    assert(formationHas(form, c));
    assert(formationHas(form, d)); // Peripheral is retained!
    assert(!formationHas(form, e)); // Released — reported in t1.orphaned
    assert(!formationHas(form, f)); // Released — reported in t1.orphaned

    // Core status still asks about UNDIRECTED edges: being held together at all
    // is what membership answers; being at the core is a stronger claim.
    assert(form.isCoreMember(a));
    assert(form.isCoreMember(b));
    assert(form.isCoreMember(c));
    assert(!form.isCoreMember(d)); // D is peripheral, no undirected edges
    assert(!form.isCoreMember(e)); // E was released

    // Break the cycle A-B-C into just A-B and B-C. Still one component of four.
    form.removeRelation(ca);
    Formation::Topology t2 = form.resolveTopology();
    assert(t2.applied);
    assert(t2.spawned.empty());
    assert(t2.orphaned.empty());
    assert(formationHas(form, a));
    assert(formationHas(form, b));
    assert(formationHas(form, c));

    // Remove B-C. Now A-B (size 2) and C-D (size 2): no component is a valid
    // core. The answer is "nothing here holds" — and it is REPORTED, not
    // enacted. Asking a Formation a question must not destroy it.
    form.removeRelation(bc);
    Formation::Topology t3 = form.resolveTopology();
    assert(!t3.applied);
    assert(t3.spawned.empty());
    assert(t3.orphaned.size() == 4);
    assert(form.getMembers().size() == 4);   // membership untouched by the asking

    beings.clear();
    glfwDestroyWindow(window);
    glfwTerminate();

    std::printf("formation_topology_test: OK\n");
    return 0;
}
