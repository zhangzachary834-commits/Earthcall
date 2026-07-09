// Governance milestone test (LAW_AND_CREATION_SYSTEM.md, commit 8 — final):
//   metalaws with zero new machinery, the authority ceiling, and both
//   synthesis paths.
//
// The manifesto's claim, in asserts: "Lower scopes may govern Laws within
// their jurisdiction, but cannot override higher-order Metalaws... This
// keeps Earthcall's civic order from collapsing into either chaos or
// tyranny."

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/LawSynthesis.hpp"
#include "Form/Object/Object.hpp"

#include <GLFW/glfw3.h>
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {

bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "metalaw_test: glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "metalaw_test", nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "metalaw_test: no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    {
        Object author;

        // ------------------------------------------------------------------
        // 1. A Law is a legible Singular: its state addresses by path.
        //    (authorityLevel deliberately does NOT — Singularity-granted.)
        // ------------------------------------------------------------------
        Law citizen("ground-rest");
        citizen.addAuthor(author);
        citizen.setConditionModel(ConditionNode::compare(
            "position.y", ConditionNode::Op::Lt, PropertyValue(0.0)));
        citizen.setActionModel(ActionNode::set("position.y", PropertyValue(0.0)));

        PropertyValue v;
        assert(PropertyPath::parse("enabled").getValue(citizen, v));
        assert(std::get<bool>(v) == true);
        assert(PropertyPath::parse("name").getValue(citizen, v));
        assert(std::get<std::string>(v) == "ground-rest");
        assert(citizen.findProperty("authorityLevel") == nullptr);   // the ceiling is not legible

        // ------------------------------------------------------------------
        // 2. A metalaw is just a law whose target is a law.
        // ------------------------------------------------------------------
        Law metalaw("sabbath");   // disable the citizen law
        metalaw.addAuthor(author);
        metalaw.setActionModel(ActionNode::set("enabled", PropertyValue(false)));

        assert(metalaw.applyTo(citizen) == Law::ApplicationResult::Applied);
        assert(!citizen.isEnabled());                       // disabled BY LAW

        Object fallen;
        fallen.setPosition(glm::vec3(0.0f, -4.0f, 0.0f));
        assert(citizen.applyTo(fallen) == Law::ApplicationResult::Disabled);
        assert(nearf(fallen.getPosition().y, -4.0f));       // the disabled law is silent

        // Re-enable by law, too.
        Law dawn("dawn");
        dawn.addAuthor(author);
        dawn.setActionModel(ActionNode::set("enabled", PropertyValue(true)));
        assert(dawn.applyTo(citizen) == Law::ApplicationResult::Applied);
        assert(citizen.isEnabled());

        // ------------------------------------------------------------------
        // 3. The ceiling: lower authority may not govern higher.
        // ------------------------------------------------------------------
        Law kernel("personhood-integrity");
        kernel.addAuthor(author);
        kernel.setAuthorityLevel(10);                       // Singularity-granted
        kernel.setActionModel(ActionNode::set("position.y", PropertyValue(0.0)));

        assert(metalaw.applyTo(kernel) == Law::ApplicationResult::AuthorityDenied);
        assert(kernel.isEnabled());                         // untouched — tyranny refused

        // A peer (or higher) authority may govern.
        Law council("council");
        council.addAuthor(author);
        council.setAuthorityLevel(10);
        council.setActionModel(ActionNode::set("enabled", PropertyValue(false)));
        assert(council.applyTo(kernel) == Law::ApplicationResult::Applied);
        assert(!kernel.isEnabled());

        // The refusal is logged like every application — audit, not silence.
        assert(metalaw.applicationLog().back().result ==
               Law::ApplicationResult::AuthorityDenied);

        // ------------------------------------------------------------------
        // 4. Interpretive synthesis: tree algebra, structure kept visible.
        // ------------------------------------------------------------------
        Law gild("gild");
        gild.addAuthor(author);
        gild.setConditionModel(ConditionNode::compare(
            "position.y", ConditionNode::Op::Lt, PropertyValue(10.0)));
        gild.setActionModel(ActionNode::set("shape.fillet", PropertyValue(0.9f)));

        auto higher = LawSynthesis::compose("rest-and-gild", citizen, gild, {&author});
        assert(higher->hasConditionModel() && higher->hasActionModel());
        assert(higher->conditionModel()->kind == ConditionNode::Kind::All);
        assert(higher->actionModel()->kind == ActionNode::Kind::Sequence);

        Object subject;
        subject.setPosition(glm::vec3(0.0f, -2.0f, 0.0f));
        assert(higher->applyTo(subject) == Law::ApplicationResult::Applied);
        assert(nearf(subject.getPosition().y, 0.0f));       // citizen's effect
        assert(PropertyPath::parse("shape.fillet").getValue(subject, v));
        assert(nearf(std::get<float>(v), 0.9f));            // gild's effect

        // The higher law survives serialization like any law.
        auto reborn = Law::fromJson(higher->toJson());
        reborn->addAuthor(author);
        Object subject2;
        subject2.setPosition(glm::vec3(0.0f, -1.0f, 0.0f));
        assert(reborn->applyTo(subject2) == Law::ApplicationResult::Applied);
        assert(nearf(subject2.getPosition().y, 0.0f));

        // ------------------------------------------------------------------
        // 5. Native synthesis: the recorder refits the JOINT process.
        //    Two additive drifts demonstrate together; the higher law owns
        //    one fused linear model of what they jointly did.
        // ------------------------------------------------------------------
        Law driftUp("drift-up");
        driftUp.addAuthor(author);
        driftUp.setActionModel(ActionNode::add("position.y", 0.3));
        Law driftUpMore("drift-up-more");
        driftUpMore.addAuthor(author);
        driftUpMore.setActionModel(ActionNode::add("position.y", 0.7));

        Object demonstrator;                                 // starts at y = 0
        auto fused = LawSynthesis::synthesizeByDemonstration(
            "joint-drift", driftUp, driftUpMore, demonstrator,
            {"position.y"}, /*steps=*/120, /*dt=*/1.0f / 60.0f, {&author});

        assert(fused->hasActionModel());
        assert(fused->actionModel()->kind == ActionNode::Kind::Drive);
        // Joint slope: (0.3 + 0.7) per step at 60 steps/sec = 60 units/sec.
        assert(fused->actionModel()->curve.form == CurveModel::Form::Polynomial);
        assert(std::fabs(fused->actionModel()->curve.coeffs[1] - 60.0) < 1.0);

        // Provenance names both constituents.
        const std::string prov = fused->provenance().toJson().dump();
        assert(prov.find(driftUp.getIdentifier()) != std::string::npos);
        assert(prov.find(driftUpMore.getIdentifier()) != std::string::npos);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    std::puts("metalaw_test: ALL OK");
    return 0;
}
