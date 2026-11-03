// A Zone authors how far its sight reaches through `view.farPlane` on its spatial root.
//
// The app hard-coded 100 m, which clipped The Merry Dancers' 960 m sky while offscreen
// stills at another far plane looked complete. Zach chose an authored far plane on
// 2026-11-03. What must hold:
//   - a Zone that says nothing keeps exactly the historical answer;
//   - the value is live: a write (as a Law's would be) changes the next read;
//   - it persists through the FieldNode's save form;
//   - anything that would make an invalid projection is ignored, because Engine::render
//     returns with nothing drawn when the projection refuses.
// This pins the read, not pixels. Whether the sky appears in the live window is on the
// Person Verification List.
// Claude Code / Claude Opus 5.5 / session ed9d515b-d6ac-4098-a7f9-ce71d02406b3 / 2026-11-03.

#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/OntoMath/LinearAlgebra.hpp"
#include "Singularity/Screen/AuthoredView.hpp"

#include <cmath>
#include <iostream>
#include <limits>
#include <memory>
#include <string>

namespace {
int g_checks = 0, g_failures = 0;
void check(bool ok, const std::string& what) {
    ++g_checks;
    if (!ok) ++g_failures;
    std::cout << (ok ? "  ok: " : "  FAILED: ") << what << std::endl;
}
constexpr float kFallback = 100.0f, kNear = 0.1f;

float readFrom(const PropertyValue& v) {
    geom::FieldNode root;
    root.setDynamicProperty("view.farPlane", v);
    return Rendering::authoredFarPlane(&root, kFallback, kNear);
}
}  // namespace

int main() {
    std::cout << "view.farPlane: a Zone authors its own reach\n";

    check(Rendering::authoredFarPlane(nullptr, kFallback, kNear) == kFallback, "no root: the historical far plane");
    {
        geom::FieldNode silent;
        check(Rendering::authoredFarPlane(&silent, kFallback, kNear) == kFallback,
              "a Zone that says nothing keeps the historical far plane");
    }

    check(readFrom(PropertyValue(2000.0f)) == 2000.0f, "an authored float is honoured");
    check(readFrom(PropertyValue(40.0f)) == 40.0f, "a shorter reach is honoured too: it is authored, not a floor");

    check(readFrom(PropertyValue(std::string("far"))) == kFallback, "a wrongly typed value is ignored");
    check(readFrom(PropertyValue(0.0f)) == kFallback, "zero is ignored");
    check(readFrom(PropertyValue(-5.0f)) == kFallback, "a negative value is ignored");
    check(readFrom(PropertyValue(kNear)) == kFallback, "a far plane at the near plane is ignored");
    check(readFrom(PropertyValue(std::numeric_limits<float>::quiet_NaN())) == kFallback, "NaN is ignored");
    check(readFrom(PropertyValue(std::numeric_limits<float>::infinity())) == kFallback, "infinity is ignored");

    {
        geom::FieldNode root;
        root.setDynamicProperty("view.farPlane", PropertyValue(2000.0f));
        root.setDynamicProperty("view.farPlane", PropertyValue(500.0f));
        check(Rendering::authoredFarPlane(&root, kFallback, kNear) == 500.0f, "a later write is what the next frame reads");

        PropertyValue kept;
        check(root.getDynamicProperty("view.farPlane", kept), "the property stays visible to Law");
        double number = 0.0;
        check(propertyValueToNumber(kept, number) && number == 500.0, "and reads back its value");
    }

    // The save form the Zone store actually uses, not a mock of it.
    {
        const nlohmann::json saved = {
            {"id", "far-plane-root"},
            {"origin", {0.0, 0.0, 0.0}},
            {"scale", {1.0, 1.0, 1.0}},
            {"authoredProperties", {{"view.farPlane", {{"t", "float"}, {"v", 2000.0}}}}},
        };
        const auto loaded = geom::FieldNode::fromJson(saved);
        check(loaded != nullptr, "a root carrying the property loads");
        check(loaded && Rendering::authoredFarPlane(loaded.get(), kFallback, kNear) == 2000.0f, "and answers from the save");
        if (loaded) {
            const auto again = geom::FieldNode::fromJson(loaded->toJson());
            check(again && Rendering::authoredFarPlane(again.get(), kFallback, kNear) == 2000.0f,
                  "and survives a save and reload");
        }
    }

    // Everything authoredFarPlane admits must be a projection the renderer accepts.
    for (const float far : {0.2f, 40.0f, 2000.0f, 1.0e6f}) {
        const float admitted = readFrom(PropertyValue(far));
        check(OntoMath::cameraPerspective(0.78, 1.7, kNear, admitted, true).has_value(),
              "far " + std::to_string(far) + " is a valid projection");
    }

    std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures ? 1 : 0;
}
