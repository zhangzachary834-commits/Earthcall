#include "Form/Object/Formation/Formation.hpp"
#include "Form/Object/Object.hpp"
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

    // Initial resolution
    auto spawned = form.resolveTopology();
    
    // Core 1 (A,B,C) + peripheral (D) is size 4. This remains in the primary formation.
    // Core 2 (E,F) is size 2, meaning it is dissolved (cleared out of the formation).
    // Spawned should be empty because there's only 1 valid core.
    assert(spawned.empty());

    // Check members
    assert(formationHas(form, a));
    assert(formationHas(form, b));
    assert(formationHas(form, c));
    assert(formationHas(form, d)); // Peripheral is retained!
    assert(!formationHas(form, e)); // Dissolved
    assert(!formationHas(form, f)); // Dissolved

    // Check core status
    assert(form.isCoreMember(a));
    assert(form.isCoreMember(b));
    assert(form.isCoreMember(c));
    assert(!form.isCoreMember(d)); // D is peripheral, no undirected edges
    assert(!form.isCoreMember(e)); // E is dissolved entirely

    // Break the cycle A-B-C into just A-B and B-C. (still size 3 valid core!)
    form.removeRelation(ca);
    auto spawned2 = form.resolveTopology();
    assert(spawned2.empty());
    assert(formationHas(form, a));
    assert(formationHas(form, b));
    assert(formationHas(form, c));

    // Remove B-C. Now we have A-B (size 2) and C (size 1).
    form.removeRelation(bc);
    auto spawned3 = form.resolveTopology();
    assert(spawned3.empty());
    // Since all cores are < 3, the entire formation dissolves!
    assert(form.getMembers().empty());

    beings.clear();
    glfwDestroyWindow(window);
    glfwTerminate();

    std::printf("formation_topology_test: OK\n");
    return 0;
}
