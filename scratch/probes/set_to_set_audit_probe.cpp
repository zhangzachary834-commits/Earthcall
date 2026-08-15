// Audit probe for Singular set-to-set creation (LAW_AND_CREATION_SYSTEM.md §7).
// Executes the claims the audit rested on rather than asserting them from
// inspection. Every one of these FAILED on 2026-08-11 before the set-to-set
// unification; they are re-run here as the regression witness.
//
// Build: copy into tests/, re-run cmake (sources are globbed), build the
// set_to_set_audit_probe target, then delete the tests/ copy.

#include "ConstructedBeing/Object/Creation/ObjectConcept.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/World/World.hpp"

#include <GLFW/glfw3.h>
#include <cmath>
#include <cstdio>

static int failures = 0;
static void check(const char* what, bool ok) {
    std::printf("%-72s %s\n", what, ok ? "holds" : "DEFECT");
    if (!ok) ++failures;
}

int main() {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* w = glfwCreateWindow(64, 64, "probe", nullptr, nullptr);
    if (!w) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(w);

    // 1. A concept's provenance survives a save/load round trip.
    {
        Object author, src;
        src.setPosition(glm::vec3(1.0f, 2.0f, 3.0f));
        auto concept = ObjectConcept::captureFrom({&src}, "probe-concept", &author);
        const std::size_t before = concept->provenance().getAll().size();
        auto reborn = ObjectConcept::fromJson(concept->toJson());
        const std::size_t after = reborn->provenance().getAll().size();
        std::printf("   provenance: captured=%zu, after round trip=%zu\n", before, after);
        check("1. concept provenance survives save/load", before > 0 && after == before);
    }

    // 2. A bindings-only mapping (multivariable, no legacy `source`) transfers.
    {
        Object src;
        src.setPosition(glm::vec3(4.0f, 0.0f, 0.0f));
        auto concept = ObjectConcept::captureFrom({&src}, "bindings-only", nullptr);

        PropertyMapping m;                       // m.source deliberately empty
        m.bindings["a"] = PropertyPath::parse("position.x");
        m.target = PropertyPath::parse("position.y");
        m.hasExact = true;
        m.exact = OntoMath::Piecewise::continuous(
            OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::variable("a")));
        concept->addMapping(m);

        std::vector<Object*> sources{&src};
        auto newborns = concept->instantiate(glm::mat4(1.0f), &sources);
        const float y = newborns.empty() ? -999.0f : newborns[0]->getPosition().y;
        std::printf("   bindings-only mapping wrote position.y = %.3f (expected 4.0)\n", y);
        check("2. bindings-only PropertyMapping transfers", std::fabs(y - 4.0f) < 1e-4f);
    }

    // 3. A concept the author NAMES is filed under that name — the identity
    //    law text uses. (The retired universal Concept minted a fresh uuid per
    //    construction with no way to set a stable slug.)
    {
        auto concept = std::make_shared<ObjectConcept>("sound-emitter");
        concept->setConceptId("concept-sound-emitter");
        ConceptRegistry::instance().add(concept);
        auto found = ConceptRegistry::instance().find("concept-sound-emitter");
        std::printf("   concept id: %s\n", concept->getIdentifier().c_str());
        check("3. a named concept resolves by the name law text uses", found == concept);
    }

    // 4 & 5. Synthesize births reach the world, and carry provenance.
    {
        Object prototype;
        auto concept = ObjectConcept::captureFrom({&prototype}, "synth-probe", nullptr);
        ConceptRegistry::instance().add(concept);

        World world;
        Object subject;
        ActionNode node;
        node.kind = ActionNode::Kind::Synthesize;
        node.conceptId = concept->getIdentifier();
        auto executor = node.compile();

        const std::size_t before = world.getOwnedObjects().size();
        executor(ECA::Event{"beings-met", &subject, nullptr, 0}, world);
        const std::size_t after = world.getOwnedObjects().size();
        std::printf("   world objects %zu -> %zu\n", before, after);
        check("4. a synthesized being reaches the World", after > before);

        const std::string prov = concept->provenance().toJson().dump();
        check("5. synthesis records provenance for its newborns",
              prov.find("generated-from") != std::string::npos);
    }

    std::printf("\n%d claim(s) failed.\n", failures);
    glfwDestroyWindow(w);
    glfwTerminate();
    return failures == 0 ? 0 : 1;
}
