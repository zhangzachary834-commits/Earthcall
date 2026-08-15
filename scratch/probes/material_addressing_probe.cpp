// Probe: after providing Materials to the Universe, is Material.hpp's claim
// ("the Law system can address material.clay.baseColor") actually true?
//
// Two separate questions:
//   A. Are materials REACHABLE as beings?  (quantifiers, folds, Related)
//   B. Are they ADDRESSABLE by @-path?     (@material.clay.baseColor)
// The provider fixes A. B depends on how resolveLawRoot splits the path.

#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Object/Object.hpp"

#include <cassert>
#include <cstdio>
#include <memory>
#include <cmath>

int main() {
    Material clay("clay");
    clay.baseColor = glm::vec3(0.7f, 0.4f, 0.2f);
    Object subject; subject.setObjectID("probe-subject");

    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&clay);
        beings.push_back(&subject);
    });

    // ---- A. reachable as a being --------------------------------------
    bool found = false;
    for (Singular* b : Universe::instance().beings()) {
        if (b && b->getIdentifier() == "material.clay") found = true;
    }
    assert(found && "the provider should make materials reachable");
    std::printf("  A. materials ARE reachable as beings (quantifiers/folds/Related)\n");

    // Direct property read on the being works (this is what buildProperties gave).
    PropertyValue direct;
    assert(PropertyPath::parse("baseColor").getValue(clay, direct) ==
           PropertyPath::PathResult::Ok);
    std::printf("     direct read of baseColor on the being: OK\n");

    // ---- B. addressable by @-path? ------------------------------------
    // "material.clay" contains a dot; PropertyPath splits on dots.
    const auto path = PropertyPath::parse("@material.clay.baseColor");
    std::printf("     @material.clay.baseColor parses to %zu segments:",
                path.segments.size());
    for (const auto& s : path.segments) std::printf(" [%s]", s.c_str());
    std::printf("\n");

    PropertyPath remainder;
    Singular* root = resolveLawRoot(subject, path, remainder);
    assert(root == &clay && "@material.clay.* must resolve to the material");
    assert(remainder.segments.size() == 1 && remainder.segments[0] == "baseColor" &&
           "the consumed id segments must not reach the property lookup");
    std::printf("  B. @material.clay.baseColor -> being [%s], remainder [%s]\n",
                root->getIdentifier().c_str(), remainder.toString().c_str());

    // And the whole point: a law can now READ it through the qualified path.
    PropertyValue v;
    assert(lawGetValue(subject, path, v) && "qualified read should succeed");
    const glm::vec3* rgb = std::get_if<glm::vec3>(&v);
    assert(rgb && std::fabs(rgb->x - 0.7f) < 1e-6f);
    std::printf("     law read of @material.clay.baseColor -> (%.2f, %.2f, %.2f)\n",
                rgb->x, rgb->y, rgb->z);

    // ---- Regressions: the change must not disturb existing addressing ----
    // 1. A dotless identifier still resolves, consuming exactly one segment.
    PropertyPath rem2;
    Singular* root2 = resolveLawRoot(subject, PropertyPath::parse("@probe-subject.position"), rem2);
    assert(root2 == &subject && rem2.segments.size() == 1 && rem2.segments[0] == "position");

    // 2. Most specific wins: with BOTH "material" and "material.clay" present,
    //    @material.clay.baseColor must pick the longer one.
    Object generic; generic.setObjectID("material");
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&generic);      // deliberately first
        beings.push_back(&clay);
        beings.push_back(&subject);
    });
    PropertyPath rem3;
    Singular* root3 = resolveLawRoot(subject, path, rem3);
    assert(root3 == &clay && "longest match must beat the shorter, order-independently");
    std::printf("     specificity: \"material\" + \"material.clay\" -> picked [%s]\n",
                root3->getIdentifier().c_str());

    // 3. An unknown being is still nullptr (unproven referent, never a guess).
    PropertyPath rem4;
    assert(resolveLawRoot(subject, PropertyPath::parse("@nobody.here"), rem4) == nullptr);
    std::printf("     unknown being still resolves to nothing\n");

    Universe::instance().setProvider(nullptr);
    std::printf("OK  materials are reachable AND addressable; longest-match root resolution holds\n");
    return 0;
}
