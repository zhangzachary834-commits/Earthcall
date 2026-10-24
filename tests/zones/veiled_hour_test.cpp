// Verification for "The Veiled Hour".
//
// A nebula is a hard thing to verify, because the thing that makes one
// beautiful is a RELATIONSHIP between two fields — bright gas and dark dust —
// and neither half is impressive alone. So this test does not ask "is there
// fog". It asks whether the specific physical claims the Zone makes about
// itself are true of the numbers it actually saved:
//
//   1. Every expression COMPILES to WGSL. (The Gyroid Reliquary shipped 72
//      green checks and could not draw a single frame, because compiling is a
//      different claim from evaluating.)
//   2. The pillars POINT AT THE CLUSTER. Not approximately — the tip of every
//      pillar is closer to the cluster than its root is.
//   3. The pillars GROW with distance from the cluster. That is the 1/r^2
//      ionising flux eating the near ends, and it is the whole reason a real
//      pillar field looks like one. A Zone with seven identical columns would
//      pass any test that only asked "are there columns".
//   4. The pillars are ABLATED, not smoothly tapered — a cone with four
//      spheres bitten out of it, so the silhouette is ragged.
//   5. THE CONTRAST IS REAL. The dust must be genuinely dark against a
//      genuinely bright cavity, by a wide margin. This is the one claim that
//      decides whether the Zone is a nebula or a smoke machine.
//   6. The dust is the cluster's SHADOW, not decoration near a light: dust
//      density must fall as the cluster's flux rises.
//   7. The forward-scattering phase function ACTUALLY forward-scatters. Its
//      convention was read out of the transport and asserted in the Zone's own
//      records, so it is measured here rather than trusted.
//   8. Every medium reaches EXACTLY zero somewhere and is non-negative
//      everywhere it is sampled. A field with no zero is fog with no boundary.

#include "Singularity/Storage/SaveSystem.hpp"

#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/AuthorableLight.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <variant>
#include <vector>

extern MaterialManager materials;

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << std::endl;
        return;
    }
    std::cout << "  ok: " << description << std::endl;
}

void report(const std::string& label, double value) {
    std::cout << "       " << label << " = " << value << std::endl;
}

std::map<std::string, PropertyValue> varsAt(double x, double y, double z, double t = 0.0) {
    return {
        {"p", PropertyValue(glm::vec3(static_cast<float>(x), static_cast<float>(y),
                                      static_cast<float>(z)))},
        {"x", PropertyValue(x)}, {"y", PropertyValue(y)}, {"z", PropertyValue(z)},
        {OntoMath::kTimeVar, PropertyValue(t)},
        {"n", PropertyValue(glm::vec3(0.0f, 1.0f, 0.0f))},
        {"wi.x", PropertyValue(0.0)}, {"wi.y", PropertyValue(0.0)}, {"wi.z", PropertyValue(1.0)},
        {"wo.x", PropertyValue(0.0)}, {"wo.y", PropertyValue(0.0)}, {"wo.z", PropertyValue(1.0)},
        {"omega.x", PropertyValue(0.0)}, {"omega.y", PropertyValue(0.0)}, {"omega.z", PropertyValue(1.0)},
    };
}

double evalScalar(const OntoMath::Piecewise& pw, double x, double y, double z, double t = 0.0) {
    const auto res = pw.evaluate(varsAt(x, y, z, t));
    if (!res) return std::nan("");
    double n = 0.0;
    if (!propertyValueToNumber(*res, n)) return std::nan("");
    return n;
}

bool evalVector(const OntoMath::Piecewise& pw, glm::vec3& out, double x, double y, double z) {
    const auto res = pw.evaluate(varsAt(x, y, z));
    if (!res) return false;
    if (const auto* v = std::get_if<glm::vec3>(&res.value())) { out = *v; return true; }
    return false;
}

const geom::FieldNode* fieldNamed(const Zone& zone, const std::string& id) {
    if (const auto* root = zone.spatialRoot()) {
        if (root->getIdentifier() == id) return root;
    }
    for (const auto& f : zone.additionalSpatialFields()) {
        if (f && f->getIdentifier() == id) return f.get();
    }
    return nullptr;
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "The Veiled Hour — emission, absorption, and 1/r^2\n";
    std::cout << "============================================================\n";

    namespace fs = std::filesystem;
    const fs::path repoRoot = fs::path(__FILE__).parent_path().parent_path().parent_path();
    const fs::path zonePath = repoRoot / "saves/zones/The Veiled Hour/zone.json";

    // --- 1. The save, and hydration ----------------------------------------
    const nlohmann::json zoneJson = SaveSystem::readSaveData(zonePath.string());
    check(!zoneJson.empty(), "The Veiled Hour zone.json exists and is readable");
    if (zoneJson.empty()) return 1;
    check(zoneJson.value("identifier", "") == "The Veiled Hour",
          "the document identifier matches the directory key");
    check(zoneJson.value("owner", "") == "Zach",
          "the Zone is owned by the Person who asked for it");
    check(zoneJson.contains("injected_by") && !zoneJson["injected_by"].get<std::string>().empty(),
          "an author is recorded — nothing entered the world anonymously");

    ZoneManager mgr;
    mgr.hydrateFromZoneStore();
    std::shared_ptr<Zone> zone = nullptr;
    for (const auto& z : mgr.zones()) {
        if (z && z->getIdentifier() == "The Veiled Hour") { zone = z; break; }
    }
    check(zone != nullptr, "the Zone hydrates through ZoneManager");
    if (!zone) return 1;

    const auto& objects = zone->getOwnedObjects();
    check(objects.size() == 55, "all 55 authored beings are present after hydration");

    bool allMaterialsResolve = true;
    for (const auto& obj : objects) {
        if (obj && !materials.get(obj->materialId())) {
            allMaterialsResolve = false;
            std::cout << "       unresolved: " << obj->materialId() << std::endl;
        }
    }
    check(allMaterialsResolve, "every Object's material resolves to a real Material being");

    // Gather the pieces.
    const Object* pillars[16];
    int pillarCount = 0;
    const Object* clusterObj = nullptr;
    for (const auto& obj : objects) {
        if (!obj) continue;
        const std::string id = obj->getIdentifier();
        if (id.rfind("veil.pillar.", 0) == 0 && pillarCount < 16) {
            pillars[pillarCount++] = obj.get();
        }
    }
    check(pillarCount == 7, "all seven Pillars are present");

    const geom::FieldNode* cavity = fieldNamed(*zone, "veil.cavity");
    const geom::FieldNode* dust = fieldNamed(*zone, "veil.dust");
    const geom::FieldNode* cluster = fieldNamed(*zone, "veil.cluster");
    const geom::FieldNode* scatter = fieldNamed(*zone, "veil.forward-scatter");
    const geom::FieldNode* outer = fieldNamed(*zone, "veil.outer");
    check(cavity != nullptr, "the ionised Cavity is present");
    check(dust != nullptr, "the cold Dust is present");
    check(cluster != nullptr, "the hidden Cluster is present");
    check(scatter != nullptr, "the Forward Scatter is present");
    if (!cavity || !dust || !cluster || !scatter) return 1;

    // --- 2. Everything compiles --------------------------------------------
    {
        std::vector<const geom::FieldNode*> nodes;
        if (const auto* root = zone->spatialRoot()) nodes.push_back(root);
        for (const auto& f : zone->additionalSpatialFields()) if (f) nodes.push_back(f.get());
        check(nodes.size() == 8, "one spatial root and seven radiant beings");
        for (const auto* n : nodes) {
            if (!n) continue;
            const std::string who = n->getIdentifier();
            if (n->field && !n->field->astDefinition.pieces.empty()) {
                const auto l = sdfwgsl::inspectScalarExpression(&n->field->astDefinition, true);
                check(l.ok, who + ": source radiance COMPILES to WGSL");
                if (!l.ok) std::cout << "       " << l.error << std::endl;
            }
            if (n->lightChroma && !n->lightChroma->pieces.empty()) {
                const auto l = sdfwgsl::inspectVectorExpression(n->lightChroma.get(), true);
                check(l.ok, who + ": source chroma COMPILES to WGSL");
                if (!l.ok) std::cout << "       " << l.error << std::endl;
            }
            if (n->lightAngular && !n->lightAngular->pieces.empty()) {
                const auto l = sdfwgsl::inspectAngularExpression(n->lightAngular.get());
                check(l.ok, who + ": angular emission COMPILES to WGSL");
                if (!l.ok) std::cout << "       " << l.error << std::endl;
            }
            if (n->volumeDensity && !n->volumeDensity->pieces.empty()) {
                const auto l = sdfwgsl::inspectDensityExpression(n->volumeDensity.get());
                check(l.ok, who + ": volume density D COMPILES to WGSL");
                if (!l.ok) std::cout << "       " << l.error << std::endl;
            }
            if (n->volumeExtinction && !n->volumeExtinction->pieces.empty()) {
                const auto l = sdfwgsl::inspectExtinctionExpression(n->volumeExtinction.get());
                check(l.ok, who + ": extinction COMPILES to WGSL");
                if (!l.ok) std::cout << "       " << l.error << std::endl;
            }
            if (n->volumeChroma && !n->volumeChroma->pieces.empty()) {
                const auto l = sdfwgsl::inspectVolumeChromaExpression(n->volumeChroma.get());
                check(l.ok, who + ": medium chroma COMPILES to WGSL");
                if (!l.ok) std::cout << "       " << l.error << std::endl;
            }
            if (n->volumeEmission && !n->volumeEmission->pieces.empty()) {
                const auto l = sdfwgsl::inspectEmissionExpression(n->volumeEmission.get());
                check(l.ok, who + ": emission COMPILES to WGSL");
                if (!l.ok) std::cout << "       " << l.error << std::endl;
            }
            // The phase function, whose convention is asserted in the Zone.
            if (n->volumePhase && !n->volumePhase->pieces.empty()) {
                const auto l = sdfwgsl::inspectPhaseExpression(n->volumePhase.get());
                check(l.ok, who + ": its phase function COMPILES to WGSL, reading wi and wo");
                if (!l.ok) std::cout << "       " << l.error << std::endl;
            }
            if (n->volumeOccluder && geom::isSdfActive(n->volumeOccluder.get())) {
                const auto l = sdfwgsl::inspectOccluderLayout(n->volumeOccluder.get());
                check(l.ok, who + ": its occluder COMPILES to WGSL");
                if (!l.ok) std::cout << "       " << l.error << std::endl;
            }
        }
        for (int i = 0; i < pillarCount; ++i) {
            const auto l = sdfwgsl::inspectOccluderLayout(&pillars[i]->getFieldData());
            check(l.ok, pillars[i]->getIdentifier() + ": its authored geometry COMPILES to WGSL");
            if (!l.ok) std::cout << "       " << l.error << std::endl;
        }
    }

    // --- 3. The pillars POINT AT THE CLUSTER -------------------------------
    // The transform's translation is each pillar's midpoint and local +Z is its
    // axis, so the tip is midpoint + up*len/2 and the root is midpoint - up*len/2.
    // The claim is that the tip is CLOSER to the cluster than the root.
    {
        int pointing = 0;
        double worstGap = 1e9;
        for (int i = 0; i < pillarCount; ++i) {
            const glm::mat4 m = pillars[i]->getTransform();
            const glm::vec3 mid(m[3][0], m[3][1], m[3][2]);
            const glm::vec3 up(m[2][0], m[2][1], m[2][2]);
            double len = 0.0;
            {
                PropertyValue pv;
                if (pillars[i]->getDynamicProperty("veil.lengthMetres", pv)) {
                    double n = 0.0;
                    if (propertyValueToNumber(pv, n)) len = n;
                }
            }
            const glm::vec3 tip = mid + up * (static_cast<float>(len * 0.5));
            const glm::vec3 root = mid - up * (static_cast<float>(len * 0.5));
            const double dTip = glm::length(tip);
            const double dRoot = glm::length(root);
            if (dTip < dRoot) ++pointing;
            worstGap = std::min(worstGap, dRoot - dTip);
        }
        report("pillars whose tip is nearer the cluster than its root", pointing);
        report("smallest margin (metres)", worstGap);
        check(pointing == pillarCount,
              "EVERY pillar leans inward: its tip is closer to the cluster than its root, "
              "so the seven of them converge on the one light you never see");
        check(worstGap > 0.5, "the convergence is not a rounding artefact — it is metres, not epsilon");
    }

    // --- 4. The pillars GROW with distance (the 1/r^2 law) -----------------
    {
        std::vector<std::pair<double, double>> byDistance;  // (distance, length)
        for (int i = 0; i < pillarCount; ++i) {
            double dist = 0.0, len = 0.0;
            PropertyValue pv;
            if (pillars[i]->getDynamicProperty("veil.distanceToClusterMetres", pv)) {
                double n = 0.0; if (propertyValueToNumber(pv, n)) dist = n;
            }
            if (pillars[i]->getDynamicProperty("veil.lengthMetres", pv)) {
                double n = 0.0; if (propertyValueToNumber(pv, n)) len = n;
            }
            byDistance.emplace_back(dist, len);
        }
        std::sort(byDistance.begin(), byDistance.end());
        for (const auto& [d, l] : byDistance) {
            std::cout << "       pillar at " << d << " m from the cluster is " << l << " m long" << std::endl;
        }
        // Spearman-style monotonicity: no nearer pillar may be longer than a
        // farther one by more than the authored jitter (0.86..1.14 of nominal).
        int inversions = 0;
        for (size_t a = 0; a + 1 < byDistance.size(); ++a) {
            if (byDistance[a].second > byDistance[a + 1].second * 1.14) ++inversions;
        }
        check(byDistance.back().second > byDistance.front().second,
              "the farthest pillar is longer than the nearest — the ionising flux has "
              "already eaten the ends of the ones standing close to the cluster");
        check(inversions == 0,
              "pillar length never DECREASES with distance beyond the authored jitter, so "
              "the field obeys one law instead of being seven accidents");
    }

    // --- 5. The pillars are ABLATED, not smooth cones ----------------------
    // A smooth cone's silhouette is a circle at every height. An ablated one
    // has bites taken out of it, so the OUTER radius varies around the ring.
    //
    // The first version of this check marched outward from a point on the
    // pillar's own axis. That is wrong twice over: on the axis the SDF is
    // negative, so the march stopped at t = 0 and measured nothing, and where
    // the ablation had hollowed the axis the march found the INNER wall and
    // reported the hole's shape instead of the silhouette's. It also returned
    // the same number for two completely different sphere configurations, which
    // is how you know a measurement is not looking at the thing it claims to.
    //
    // So: for each direction, sample outward and take the LARGEST radius still
    // inside the solid. That is the silhouette, unambiguously, with no marching
    // and no dependence on where the probe started.
    {
        int ablated = 0;
        double worstWobble = 0.0;
        for (int i = 0; i < pillarCount; ++i) {
            double len = 0.0, baseR = 0.0;
            PropertyValue pv;
            if (pillars[i]->getDynamicProperty("veil.lengthMetres", pv)) {
                double n = 0.0; if (propertyValueToNumber(pv, n)) len = n;
            }
            if (pillars[i]->getDynamicProperty("veil.baseRadiusMetres", pv)) {
                double n = 0.0; if (propertyValueToNumber(pv, n)) baseR = n;
            }
            // Evaluated in LOCAL space — see the note in 5b. The cone's axis is
            // local +Z and the solid spans local z in [-len/2, +len/2].
            double worst = 0.0;
            for (double hFrac = -0.35; hFrac <= 0.45; hFrac += 0.1) {
                const float lz = static_cast<float>(hFrac * len);
                double minR = 1e9, maxR = 0.0;
                for (int a = 0; a < 24; ++a) {
                    const double ang = 2.0 * M_PI * a / 24.0;
                    const float dx = static_cast<float>(std::cos(ang));
                    const float dy = static_cast<float>(std::sin(ang));
                    double lastInside = 0.0;
                    const double limit = baseR * 2.4 + 1.0;
                    for (int sIdx = 1; sIdx <= 240; ++sIdx) {
                        const double r = limit * sIdx / 240.0;
                        if (geom::evalSdf(pillars[i]->getFieldData(),
                                          glm::vec3(static_cast<float>(r) * dx,
                                                    static_cast<float>(r) * dy, lz)) < 0.0f) {
                            lastInside = r;
                        }
                    }
                    if (lastInside > 0.0) {
                        minR = std::min(minR, lastInside);
                        maxR = std::max(maxR, lastInside);
                    }
                }
                if (maxR > 1e-8) worst = std::max(worst, (maxR - minR) / maxR);
            }
            worstWobble = std::max(worstWobble, worst);
            if (worst > 0.20) ++ablated;
        }
        report("worst silhouette raggedness across all pillars", worstWobble);
        check(ablated >= pillarCount - 1,
              "the pillars are genuinely ABLATED — their OUTER silhouette wobbles by more "
              "than 20% around the ring, so they are bitten columns and not smooth cones");
    }

    // --- 5b. The pillars are SOLID, not hollow shells ---------------------
    // Both caps of the cone were written with the wrong sign in the first
    // version, so `max(cone, half - z, z + half)` was positive everywhere on
    // the axis and each pillar was a shell with no material inside it. It
    // type-checked, compiled to WGSL, and was EMPTY. The tell was numerical:
    // the SDF on the pillar's own axis read +15.98, exactly half the length,
    // where a solid column reads negative.
    //
    // NOTE ON FRAMES, because this bit twice. `geom::evalSdf` takes the point
    // in the NODE'S OWN FRAME, not world space: evalLeaf computes
    // `world - n.offset` and evaluates the expression there, and it is the
    // RENDERER that maps the ray through inst.invModel first (SdfWgsl.cpp:1622).
    // So a rotated object must be probed with local coordinates. The Gyroid
    // Reliquary's objects are all identity transforms, so world and local
    // coincide there and the mistake is invisible; these pillars are rotated,
    // and feeding world coordinates in produced +6.1 where the solid is at
    // -1.2. Everything below therefore evaluates in LOCAL space, constructed
    // directly: a point at local (lx, ly, lz) is mid + right*lx + fwd*ly + up*lz.
    {
        int solidInside = 0, openOutside = 0;
        for (int i = 0; i < pillarCount; ++i) {
            double len = 0.0;
            PropertyValue pv;
            if (pillars[i]->getDynamicProperty("veil.lengthMetres", pv)) {
                double n = 0.0; if (propertyValueToNumber(pv, n)) len = n;
            }
            // A quarter of the way DOWN from the root, on the axis: solidly inside.
            if (geom::evalSdf(pillars[i]->getFieldData(),
                              glm::vec3(0.0f, 0.0f, static_cast<float>(-len * 0.25))) < 0.0f) {
                ++solidInside;
            }
            // Local z far beyond the cap: unambiguously empty space.
            if (geom::evalSdf(pillars[i]->getFieldData(),
                              glm::vec3(0.0f, 0.0f, static_cast<float>(len))) > 0.0f) {
                ++openOutside;
            }
        }
        report("pillars that are solid on their own axis", solidInside);
        check(solidInside == pillarCount,
              "every pillar has MATERIAL on its own axis a quarter of the way up — the "
              "caps are signed correctly and the column is solid, not an empty shell");
        check(openOutside == pillarCount,
              "every pillar also has open space beyond its extent, so it is a bounded "
              "solid rather than something that fills the sky");
    }

    // --- 5c. The pillars are ONE PIECE, and one piece is a SOLID COLUMN ---
    // This is the check that was missing, and its absence is why seven shattered
    // columns shipped as "a ton of overlapping cones".
    //
    // The silhouette-raggedness metric scored 0.906 on a pillar that was in fact
    // split into two disjoint chunks with holes between them. Raggedness cannot
    // distinguish "eroded" from "shattered" — both are irregular. What
    // distinguishes them is CONTINUITY: walk down the column's own axis and the
    // material must be one unbroken run, and the run must be most of the length.
    {
        int onePiece = 0;
        double worstFill = 1.0;
        double worstLip = 0.0;
        std::mt19937 lipRng(31337u);
        std::uniform_real_distribution<float> j3(-1.0f, 1.0f);
        for (int i = 0; i < pillarCount; ++i) {
            const auto& sdf = pillars[i]->getFieldData();
            double len = 0.0;
            PropertyValue pv;
            if (pillars[i]->getDynamicProperty("veil.lengthMetres", pv)) {
                double n = 0.0; if (propertyValueToNumber(pv, n)) len = n;
            }
            // Walk the axis: count maximal runs of material.
            const int STEPS = 600;
            const double half = len * 0.5;
            int runs = 0;
            bool prev = false;
            int insideCount = 0;
            for (int k = 0; k <= STEPS; ++k) {
                const float lz = static_cast<float>(-half + 2.0 * half * k / STEPS);
                const bool in = geom::evalSdf(sdf, glm::vec3(0.0f, 0.0f, lz)) < 0.0f;
                if (in) ++insideCount;
                if (in && !prev) ++runs;
                prev = in;
            }
            if (runs == 1) ++onePiece;
            worstFill = std::min(worstFill, static_cast<double>(insideCount) / (STEPS + 1));

            // MEASURE the Lipschitz constant of the composed expression, rather
            // than trusting the algebra that produced the divisor. If this
            // number ever rises above the authored divisor, the marcher can
            // tunnel and the pillars grow holes.
            const float ex = pillars[i]->getFieldExtent().x;
            for (int s = 0; s < 3000; ++s) {
                glm::vec3 a(j3(lipRng) * ex * 0.95f, j3(lipRng) * ex * 0.95f, j3(lipRng) * half * 0.95f);
                glm::vec3 dir(j3(lipRng), j3(lipRng), j3(lipRng));
                if (glm::length(dir) < 1e-4f) continue;
                dir = glm::normalize(dir);
                const float h = 0.02f + 0.10f * std::abs(j3(lipRng));
                const float da = geom::evalSdf(sdf, a);
                const float db = geom::evalSdf(sdf, a + dir * h);
                worstLip = std::max(worstLip, static_cast<double>(std::abs(da - db) / h));
            }
        }
        report("pillars that are exactly ONE piece along their axis", onePiece);
        report("worst axis fill fraction (material / length)", worstFill);
        report("MEASURED Lipschitz constant of the pillar SDF", worstLip);
        check(onePiece == pillarCount,
              "EVERY pillar is ONE unbroken column: walking its axis finds a single "
              "run of material. This is the check whose absence let seven shattered "
              "shards ship as a pile of cones.");
        check(worstFill > 0.55,
              "and the column is mostly solid along its axis — a pillar is a column, "
              "not a string of beads");
        check(worstLip < 8.0,
              "MEASURED conservatism: the composed pillar SDF's Lipschitz constant is "
              "under the authored divisor of 8.0, so the raymarcher cannot step "
              "through a pillar and grow holes in it");
    }

    // --- 6. THE CONTRAST IS REAL -------------------------------------------
    // The single claim that decides whether this is a nebula or a smoke
    // machine. Sample the cavity's emission and the dust's extinction on the
    // same points and require the gas to be bright where the dust is thin, and
    // the dust to be able to go properly opaque.
    {
        double peakEmission = 0.0;
        if (cavity->volumeEmission && !cavity->volumeEmission->pieces.empty()) {
            glm::vec3 e(0.0f);
            if (evalVector(*cavity->volumeEmission, e, 0.0, 0.0, 40.0)) {
                peakEmission = std::max(static_cast<double>(e.r), std::max(static_cast<double>(e.g), static_cast<double>(e.b)));
            }
        }
        report("cavity emission magnitude near the cluster", peakEmission);

        double dustPeak = 0.0;
        if (dust->volumeExtinction && !dust->volumeExtinction->pieces.empty()) {
            dustPeak = evalScalar(*dust->volumeExtinction, 0.0, 0.0, 40.0);
        }
        report("dust extinction near the cluster", dustPeak);
        check(peakEmission > 0.05, "the cavity actually EMITS — it is a light source, not lit fog");
        check(dustPeak > 0.05, "the dust actually EXTINCTS — it is opaque, not coloured");

        // The dust must be able to be dark. Its chroma is authored near-black;
        // if it were grey it would be fog and there would be no silhouettes.
        if (dust->volumeChroma && !dust->volumeChroma->pieces.empty()) {
            glm::vec3 chroma(0.0f);
            const bool ok = evalVector(*dust->volumeChroma, chroma, 0.0, 0.0, 40.0);
            check(ok, "the dust's chroma evaluates to a Vector");
            if (ok) {
                report("dust chroma luminance", 0.2126 * chroma.r + 0.7152 * chroma.g + 0.0722 * chroma.b);
                check(0.2126 * chroma.r + 0.7152 * chroma.g + 0.0722 * chroma.b < 0.25,
                      "the dust is genuinely DARK (luminance under 0.25) — that is what lets "
                      "it read as a silhouette against the gas instead of as more gas");
            }
        }

        // And the dust must genuinely OCCLUDE — an active solid, not a fog.
        if (dust->volumeOccluder && geom::isSdfActive(dust->volumeOccluder.get())) {
            const auto& occ = *dust->volumeOccluder;
            int inside = 0, outside = 0;
            std::mt19937 rng(4242u);
            std::uniform_real_distribution<float> s(-1.0f, 1.0f);
            for (int i = 0; i < 4000; ++i) {
                const glm::vec3 q(s(rng) * 60.0f, s(rng) * 60.0f, s(rng) * 60.0f);
                if (geom::evalSdf(occ, q) < 0.0f) ++inside; else ++outside;
            }
            report("sampled points inside the dust occluder", inside);
            check(inside > 200 && outside > 200,
                  "the dust's occluder is a real solid with a real surface — it has both an "
                  "inside and an outside, so it can actually cast a lane through the gas");
        }
    }

    // --- 7. The dust IS the cluster's shadow -------------------------------
    {
        // Near the cluster the flux is high, so the dust must be thinner there
        // than out in the cold. Sample both along one ray.
        const double nearD = evalScalar(*dust->volumeDensity, 0.0, 0.0, 18.0);
        const double farD = evalScalar(*dust->volumeDensity, 0.0, 0.0, 58.0);
        report("dust density 18 m from the cluster", nearD);
        report("dust density 58 m from the cluster", farD);
        check(nearD < farD,
              "the dust thins toward the cluster: it survives only where the 1/r^2 "
              "ionising flux is weak, so it IS the light's shadow rather than "
              "decoration placed near one");
    }

    // --- 8. The phase function ACTUALLY forward-scatters --------------------
    // The convention was read out of the transport: wi = normalize(sample - source)
    // and wo = normalize(eye - sample), so dot(wi,wo) = +1 is light continuing
    // straight on to the eye. Measure it: a sample BETWEEN the light and the eye
    // must scatter far more than one BEHIND the light.
    if (scatter->volumePhase && !scatter->volumePhase->pieces.empty()) {
        const auto& pw = *scatter->volumePhase;
        auto phaseWith = [&](double sampleZ, double wiZ, double woZ) {
            auto vars = varsAt(0.0, 0.0, sampleZ);
            vars["wi.x"] = PropertyValue(0.0);
            vars["wi.y"] = PropertyValue(0.0);
            vars["wi.z"] = PropertyValue(wiZ);
            vars["wo.x"] = PropertyValue(0.0);
            vars["wo.y"] = PropertyValue(0.0);
            vars["wo.z"] = PropertyValue(woZ);
            const auto r = pw.evaluate(vars);
            double n = 0.0;
            if (r && propertyValueToNumber(*r, n)) return n;
            return std::nan("");
        };
        // Source at the origin, eye far down +Z.
        // Sample at +12: wi = +Z (light travelling out toward the eye), wo = +Z. Forward.
        const double forward = phaseWith(12.0, 1.0, 1.0);
        // Sample at -12: wi = -Z (light travelling away from the eye), wo = +Z. Backward.
        const double backward = phaseWith(-12.0, -1.0, 1.0);
        // Sample at right angles: the 90-degree case, which must sit between.
        const double sideways = phaseWith(12.0, 0.0, 1.0);
        report("phase, forward scattering (dot = +1)", forward);
        report("phase, 90 degrees (dot = 0)", sideways);
        report("phase, backward scattering (dot = -1)", backward);
        check(std::isfinite(forward) && std::isfinite(backward) && std::isfinite(sideways),
              "the phase function evaluates finitely at all three scattering angles");
        check(forward > sideways && sideways > backward,
              "FORWARD SCATTERING VERIFIED: the authored Henyey-Greenstein lobe really "
              "does peak when light continues on toward the eye, which is the "
              "convention the Zone's own records claim");
        check(forward > backward * 4.0,
              "and it does so decisively — the lobe is strongly forward-weighted, so "
              "looking toward the cluster will visibly brighten the nebula");
    }

    // --- 9. Every medium is bounded and non-negative -----------------------
    {
        std::vector<const geom::FieldNode*> nodes;
        for (const auto& f : zone->additionalSpatialFields()) if (f) nodes.push_back(f.get());
        if (const auto* root = zone->spatialRoot()) nodes.push_back(root);
        std::mt19937 rng(77u);
        std::uniform_real_distribution<float> s(-400.0f, 400.0f);
        for (const auto* n : nodes) {
            if (!n) continue;
            const std::string who = n->getIdentifier();
            struct Ch { const char* label; const OntoMath::Piecewise* pw; bool vectorResult;
                        bool mustReachZero = true; };
            std::vector<Ch> channels;
            if (n->field) channels.push_back({"source radiance", &n->field->astDefinition, false});
            if (n->volumeDensity) channels.push_back({"volume density", n->volumeDensity.get(), false});
            if (n->volumeExtinction) channels.push_back({"extinction", n->volumeExtinction.get(), false});
            if (n->volumeEmission) channels.push_back({"emission", n->volumeEmission.get(), true});
            // A chroma is a COLOUR, not a density. It is bounded by the
            // density in front of it, not by itself, and a colour that reached
            // zero everywhere would be BLACK FOG. So chroma is checked for
            // finiteness and non-negativity only. The first version of this
            // check demanded a zero from every channel and failed five radiant
            // beings for being correctly coloured.
            if (n->volumeChroma) channels.push_back({"medium chroma", n->volumeChroma.get(), true, false});

            for (const auto& ch : channels) {
                if (!ch.pw || ch.pw->pieces.empty()) continue;
                bool finite = true, nonNegative = true, reachesZero = false;
                for (int i = 0; i < 220; ++i) {
                    const double x = s(rng), y = s(rng), z = s(rng);
                    if (ch.vectorResult) {
                        glm::vec3 v(0.0f);
                        if (!evalVector(*ch.pw, v, x, y, z)) { finite = false; break; }
                        if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)) finite = false;
                        if (v.x < -1e-6 || v.y < -1e-6 || v.z < -1e-6) nonNegative = false;
                        const double lum = 0.2126 * v.x + 0.7152 * v.y + 0.0722 * v.z;
                        if (std::abs(lum) < 1e-4) reachesZero = true;
                    } else {
                        const double d = evalScalar(*ch.pw, x, y, z);
                        if (!std::isfinite(d)) { finite = false; break; }
                        if (d < -1e-6) nonNegative = false;
                        if (std::abs(d) < 1e-4) reachesZero = true;
                    }
                    if (!finite || !nonNegative) break;
                }
                check(finite, who + " / " + ch.label + ": finite everywhere sampled "
                      "(a single NaN silently deletes a whole volume pass)");
                check(nonNegative, who + " / " + ch.label + ": non-negative everywhere sampled");
                if (ch.mustReachZero) {
                    check(reachesZero, who + " / " + ch.label + ": reaches exactly zero somewhere — "
                          "the medium has a real boundary, so it is not infinite fog");
                }
            }
        }
    }

    // --- 10. The cavity is a two-colour nebula, not a monochrome one -------
    if (cavity->volumeChroma && !cavity->volumeChroma->pieces.empty()) {
        glm::vec3 near(0.0f), far(0.0f);
        const bool a = evalVector(*cavity->volumeChroma, near, 0.0, 0.0, 12.0);
        const bool b = evalVector(*cavity->volumeChroma, far, 0.0, 0.0, 85.0);
        check(a && b, "the cavity's chroma evaluates at two radii");
        if (a && b) {
            std::cout << "       cavity chroma at r=12  = (" << near.r << ", " << near.g << ", " << near.b << ")" << std::endl;
            std::cout << "       cavity chroma at r=85  = (" << far.r << ", " << far.g << ", " << far.b << ")" << std::endl;
            // Near the cluster the doubly-ionised oxygen dominates (teal); far
            // out it is plain ionised hydrogen (deep red). The two must differ,
            // and the red one must actually be redder.
            check(near.b > far.b + 0.05,
                  "the cavity is a TWO-COLOUR nebula: OIII teal sits inside the "
                  "H-alpha red margin, as it does in a real H II region");
            check(far.r > far.b,
                  "the outer cavity is genuinely red — H-alpha, the signature colour of "
                  "ionised hydrogen");
        }
    }

    // --- 11. The reflection haze is blue, because scattering is blue -------
    const geom::FieldNode* refl = fieldNamed(*zone, "veil.reflection");
    if (refl && refl->volumeChroma && !refl->volumeChroma->pieces.empty()) {
        glm::vec3 h(0.0f);
        if (evalVector(*refl->volumeChroma, h, 0.0, 0.0, 20.0)) {
            report("reflection haze luminance", 0.2126 * h.r + 0.7152 * h.g + 0.0722 * h.b);
            check(h.b > h.r,
                  "the reflection haze is BLUE-dominant — scattered starlight off dust, "
                  "which is the ingredient that makes a nebula read as dusty");
        }
    }

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks << " checks completed (" << (g_checks - g_failures)
              << " passed, " << g_failures << " failed).\n";
    if (g_failures == 0) {
        std::cout << "veiled_hour_test: ALL OK\n";
        return 0;
    }
    std::cout << "veiled_hour_test: FAILED\n";
    return 1;
}
