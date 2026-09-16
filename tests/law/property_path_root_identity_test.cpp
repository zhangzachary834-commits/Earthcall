// PropertyPath root identity: lexical addressability must never choose a being
// merely because it was the last one with the same spelling.
//
// PropertyPath's StringIds are deliberately lexical: they make a Person's
// authored path cheap to compare.  The ontological boundary is resolveLawRoot:
// once a path says `@someone...`, that spelling must resolve to ONE being or
// to nobody.  It must never silently retarget because two distinct Singulars
// happen to be called by the same root string.
//
// This test carries both paths required by ENGINEERING_DISCIPLINE.md:
//   1. isolated root resolution (`resolveLawRoot`), and
//   2. the real law-facing read funnel (`lawGetValue`).

#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>

namespace {

double number(const PropertyValue& value) {
    double out = 0.0;
    assert(propertyValueToNumber(value, out));
    return out;
}

} // namespace

int main() {
    Object subject;
    subject.setObjectID("subject");

    Object unique;
    unique.setObjectID("unique-root");
    unique.setDynamicProperty("value", PropertyValue(7.0));

    // Distinct beings, same textual root.  Before this regression guard,
    // resolveLawRoot's unordered_map assignment made the later pointer win.
    Object duplicateA;
    duplicateA.setObjectID("duplicate-root");
    duplicateA.setDynamicProperty("value", PropertyValue(1.0));

    Object duplicateB;
    duplicateB.setObjectID("duplicate-root");
    duplicateB.setDynamicProperty("value", PropertyValue(2.0));

    // Dotted identifiers exercise the longest-root rule too.  Ambiguity in the
    // most-specific root must fail closed; it must not become insertion-order
    // identity simply because the path itself contains dots.
    Object dottedA;
    dottedA.setObjectID("material.clay");
    dottedA.setPosition(glm::vec3(1.0f, 0.0f, 0.0f));

    Object dottedB;
    dottedB.setObjectID("material.clay");
    dottedB.setPosition(glm::vec3(2.0f, 0.0f, 0.0f));

    std::vector<Singular*> population{
        &subject, &unique, &duplicateA, &duplicateB, &dottedA, &dottedB};
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        for (Singular* being : population) beings.push_back(being);
    });
    // The root cache is revision-keyed; this is the same invalidation contract
    // Zone add/remove/switch uses when the live population changes.
    Universe::instance().bumpStructuralRevision();

    // Control: lexical interning remains useful when the spelling designates a
    // single live being.
    {
        const PropertyPath path = PropertyPath::parse("@unique-root.value");
        std::size_t startIndex = 0;
        assert(resolveLawRoot(subject, path, startIndex) == &unique);
        assert(startIndex == 1);

        PropertyValue value;
        assert(lawGetValue(subject, path, value));
        assert(std::fabs(number(value) - 7.0) < 1e-9);
    }

    // Identity witness: equal spelling is not equal being.  There is no
    // principled referent, so both the root resolver and the law read fail.
    {
        const PropertyPath path = PropertyPath::parse("@duplicate-root.value");
        std::size_t startIndex = 0;
        assert(resolveLawRoot(subject, path, startIndex) == nullptr);

        PropertyValue value;
        assert(!lawGetValue(subject, path, value));
    }

    // Same invariant when the identifier itself contains dots and root
    // resolution must search longest-first.
    {
        const PropertyPath path = PropertyPath::parse("@material.clay.position.x");
        std::size_t startIndex = 0;
        assert(resolveLawRoot(subject, path, startIndex) == nullptr);

        PropertyValue value;
        assert(!lawGetValue(subject, path, value));
    }

    Universe::instance().setProvider(nullptr);
    Universe::instance().bumpStructuralRevision();

    std::cout << "property_path_root_identity_test: OK\n";
    return 0;
}
