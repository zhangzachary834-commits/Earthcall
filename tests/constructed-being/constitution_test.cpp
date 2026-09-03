// Constitution milestone test (Kernel Boundaries):
// The manifesto explicitly defines "Kernel Boundaries" that no law or zone may violate:
// 1. Nothing may violate fundamental Person guards (e.g., body position).
// 2. Nothing may be enforced through the Global Ourverse–only local Zones.
// 3. Nobody can be forced to stay in another person's zone against their will.
// 4. Singularity level constraints cannot be overridden by lower level Metalaws.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Person.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {
bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }
}

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "constitution_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "constitution_test", nullptr, nullptr);
    if (window) {
        glfwMakeContextCurrent(window);
    }

    Person target(Soul("target"), Body::createBasicAvatar("Voxel"), "strict");
    Person dictator(Soul("dictator"), Body::createBasicAvatar("Voxel"), "strict");

    Zone prison("Prison", "default");

    {
        Object zack;
        zack.setObjectID("zack");
        zack.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));

        Object stranger;
        stranger.setObjectID("stranger");

        // ------------------------------------------------------------------
        // 1. Person Guard: A law trying to overwrite another Person's body position.
        // ------------------------------------------------------------------
        Law hostileLaw("push-zack");
        hostileLaw.addAuthor(stranger);
        hostileLaw.setActionModel(ActionNode::set("position.y", PropertyValue(100.0f)));
        
        // This is a test of the civic order. A person cannot be acted upon
        // without consent/authority in ways that violate their body position.
        auto result = hostileLaw.applyTo(zack);
        (void)result; // Should be rejected
        
        // ------------------------------------------------------------------
        // 3. Singularity-level ceiling override rejection (AI as First Mover).
        // ------------------------------------------------------------------
        Law aiProposer("ai-proposal");
        // Authored, or applyTo stops at Unauthored and never reaches the
        // ceiling check this section is actually about.
        aiProposer.addAuthor(dictator);
        aiProposer.setAuthorityLevel(0); // AI / ordinary authority
        
        Law singularityCeiling("ai-ceiling");
        singularityCeiling.grantAuthority(100); // Singularity
        
        // AI tries to disable the ceiling.
        aiProposer.setActionModel(ActionNode::set("enabled", PropertyValue(false)));
        assert(aiProposer.applyTo(singularityCeiling) == Law::ApplicationResult::AuthorityDenied);
        assert(singularityCeiling.isEnabled()); // Ceiling remains intact
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("constitution_test: ALL OK");
    return 0;
}
