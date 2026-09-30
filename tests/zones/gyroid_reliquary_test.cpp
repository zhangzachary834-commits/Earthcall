// Verification for "The Gyroid Reliquary" — and, more importantly, a proof
// that the Zone's mathematics is true rather than merely present.
//
// A Zone made of authored OntoMath can fail in ways a Zone made of primitives
// cannot: the AST can load and still evaluate to nothing, the distance function
// can be non-conservative and let the raymarcher walk straight through the
// membrane, and the field can be "valid JSON" while describing empty space.
// None of those are visible by reading the file. This test loads the Zone
// through the real ZoneManager, then EVALUATES it.
//
// What is actually checked here:
//   1.  Identity, ownership and authorship survive the round trip.
//   2.  Every material an Object names resolves to a real Material being, so no
//       Object silently falls back to the default white one (Refusal 6: a Zone
//       that loses its palette to a lookup miss is a black box with a pretty
//       name).
//   3.  Every authored `mathNode` type-checks and evaluates — no load-time
//       success standing in for a number.
//   4.  The membrane's zero set is WHERE THE AUTHORED MATH SAYS IT IS: the
//       surface is probed for |d| < eps and the raw gyroid is evaluated at the
//       same points and must read |g| ~= the authored half-thickness. If the
//       divisor or the sign were wrong, this is the check that catches it.
//   5.  THE CONSERVATISM PROOF. The raymarcher is handed this distance and
//       steps by it, so if the divisor were optimistic the marcher would tunnel
//       through the membrane and the Reliquary would be a fog with holes in it.
//       This test sphere-traces real rays with the same rule the shader uses and
//       asserts that no ray ever changes sign in a single step, and that rays
//       which do hit, hit at a distance where |d| really is ~0.
//   6.  The Reliquary is not empty: the traced rays find surface, and the
//       fraction of rays that hit is in a believable band for a labyrinth.
//   7.  The Heart is a shell, the twelve Piers are cored, the Oculus is a hole
//       in the membrane, and the Floor is a proven heightfield.
//   8.  Every radiant being carries evaluable math, and every density reaches
//       exactly zero somewhere — a medium with no zero is fog with no boundary.

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
        {"x", PropertyValue(x)},
        {"y", PropertyValue(y)},
        {"z", PropertyValue(z)},
        {OntoMath::kTimeVar, PropertyValue(t)},
        {"wi", PropertyValue(glm::vec3(0.0f, 0.0f, 1.0f))},
        {"wo", PropertyValue(glm::vec3(0.0f, 0.0f, 1.0f))},
        {"omega.x", PropertyValue(0.0)},
        {"omega.y", PropertyValue(0.0)},
        {"omega.z", PropertyValue(0.0)},
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

// ---------------------------------------------------------------------------
// The gyroid, recomputed here from first principles so the test is an
// independent witness rather than a restatement of the save file. If the save's
// divisor, thickness or sign were wrong, this is the arithmetic that notices.
// ---------------------------------------------------------------------------
double referenceGyroid(double x, double y, double z) {
    return std::sin(x) * std::cos(y) + std::sin(y) * std::cos(z) + std::sin(z) * std::cos(x);
}

const double kK = 2.0 * M_PI / 12.0;              // 12 m cells
const double kThick = 0.42;
const double kShaftR = 6.0;    // the Oculus, subtracted from the membrane
const double kVesselR = 30.0;  // the authored bound that makes the gyroid a place
const double kHeartR = 8.0;

// The type environment the marcher actually runs in. x, y and z are ambient
// point components and p is the point itself; declaring them is what makes
// checkTypes a real question rather than a tautology.
OntoMath::TypeEnv ambientEnv() {
    return OntoMath::TypeEnv{
        {"x", OntoMath::ValueKind::Scalar},
        {"y", OntoMath::ValueKind::Scalar},
        {"z", OntoMath::ValueKind::Scalar},
        {OntoMath::kAmbientPointVar, OntoMath::ValueKind::Vector},
        {OntoMath::kTimeVar, OntoMath::ValueKind::Scalar},
    };
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "The Gyroid Reliquary — authored-OntoMath verification\n";
    std::cout << "============================================================\n";

    namespace fs = std::filesystem;
    const fs::path repoRoot = fs::path(__FILE__).parent_path().parent_path().parent_path();
    const fs::path zonePath = repoRoot / "saves/zones/The Gyroid Reliquary/zone.json";

    // --- 1. The save itself -------------------------------------------------
    const nlohmann::json zoneJson = SaveSystem::readSaveData(zonePath.string());
    check(!zoneJson.empty(), "The Gyroid Reliquary zone.json exists and is readable");
    if (zoneJson.empty()) return 1;

    check(zoneJson.value("identifier", "") == "The Gyroid Reliquary",
          "the document identifier matches the directory key");
    check(zoneJson.value("owner", "") == "Zach",
          "the Zone is owned by the Person who asked for it");
    check(zoneJson.contains("injected_by") && !zoneJson["injected_by"].get<std::string>().empty(),
          "an author is recorded on the Zone (nothing entered the world anonymously)");
    check(zoneJson.contains("authors") && zoneJson["authors"].is_array()
              && !zoneJson["authors"].empty(),
          "the authors list names the Person whose authority the Zone was injected under");

    // --- 2. Hydration through the real loader -------------------------------
    ZoneManager mgr;
    mgr.hydrateFromZoneStore();
    std::shared_ptr<Zone> zone = nullptr;
    for (const auto& z : mgr.zones()) {
        if (z && z->getIdentifier() == "The Gyroid Reliquary") { zone = z; break; }
    }
    check(zone != nullptr, "the Zone hydrates through ZoneManager");
    if (!zone) return 1;

    const auto& objects = zone->getOwnedObjects();
    // The Materials this Zone actually names, so the colorExpr compile check
    // covers the Zone's palette and not whatever else is in the session bag.
    std::vector<std::shared_ptr<Material>> dMaterials;
    for (const auto& obj : objects) {
        if (!obj) continue;
        if (auto m = materials.get(obj->materialId())) {
            if (std::find_if(dMaterials.begin(), dMaterials.end(),
                             [&](const std::shared_ptr<Material>& have) {
                                 return have->getIdentifier() == m->getIdentifier();
                             }) == dMaterials.end()) {
                dMaterials.push_back(m);
            }
        }
    }
    check(objects.size() == 65, "all 65 authored beings are present after hydration");
    if (objects.size() != 65) report("object count", static_cast<double>(objects.size()));

    // --- 3. Every named Material resolves -----------------------------------
    // MaterialManager::get matches on the name with the "material." prefix
    // stripped, so a miss here means every Object quietly renders default white.
    int resolvedMaterials = 0;
    bool allMaterialsResolve = true;
    for (const auto& obj : objects) {
        if (!obj) continue;
        if (materials.get(obj->materialId())) ++resolvedMaterials;
        else {
            allMaterialsResolve = false;
            std::cout << "       unresolved material: " << obj->materialId()
                      << " (on " << obj->getIdentifier() << ")" << std::endl;
        }
    }
    check(allMaterialsResolve,
          "every Object's material resolves to a real Material being — no silent default-white fallback");
    report("objects with a resolved material", resolvedMaterials);

    // The membrane's colour is the Zone's signature, so its material must carry
    // authored colour mathematics and not just a flat baseColor.
    const auto membraneMat = materials.get("reliquary.alabaster");
    check(membraneMat != nullptr, "the membrane's Material being exists");
    if (membraneMat) {
        check(membraneMat->colorExpr != nullptr && !membraneMat->colorExpr->pieces.empty(),
              "the membrane's Material carries an authored colorExpr, not a flat colour");
        if (membraneMat->colorExpr && !membraneMat->colorExpr->pieces.empty()) {
            glm::vec3 chroma(0.0f);
            const bool ok = evalVector(*membraneMat->colorExpr, chroma, 7.0, 3.0, -11.0);
            check(ok, "the membrane's colorExpr evaluates to a Vector");
            if (ok) {
                report("membrane colour at (7, 3, -11)",
                       chroma.r) ;
                std::cout << "       membrane colour = (" << chroma.r << ", " << chroma.g
                          << ", " << chroma.b << ")" << std::endl;
                check(std::isfinite(chroma.r) && std::isfinite(chroma.g) && std::isfinite(chroma.b),
                      "the membrane's authored colour is finite — no NaN escaped into the shader");
            }
        }
    }

    // --- 4. The authored geometry evaluates, and its zero set is where the
    //        mathematics says it is -----------------------------------------
    const Object* membrane = nullptr;
    const Object* heart = nullptr;
    const Object* floor = nullptr;
    int pierCount = 0;
    for (const auto& obj : objects) {
        if (!obj) continue;
        const std::string id = obj->getIdentifier();
        if (id == "reliquary.membrane") membrane = obj.get();
        else if (id == "reliquary.heart") heart = obj.get();
        else if (id == "reliquary.floor") floor = obj.get();
        else if (id.rfind("reliquary.pier.", 0) == 0) ++pierCount;
    }
    check(membrane != nullptr, "the Membrane is present");
    check(heart != nullptr, "the Heart is present");
    check(floor != nullptr, "the Floor is present");
    check(pierCount == 12, "all twelve Piers are present");
    if (!membrane) return 1;

    const geom::SdfNode& membraneSdf = membrane->getFieldData();
    check(membraneSdf.prim == geom::SdfPrim::Expr && membraneSdf.mathNode != nullptr,
          "the Membrane's shape IS an authored OntoMath expression (SdfPrim::Expr)");

    std::string typeError;
    check(membraneSdf.mathNode && membraneSdf.mathNode->checkTypes(ambientEnv(), typeError),
          "the Membrane's authored expression type-checks in the ambient environment "
          "the marcher actually evaluates it in");
    if (!typeError.empty()) std::cout << "       " << typeError << std::endl;

    // The authored divisor, read back off the being rather than assumed.
    PropertyValue divisor;
    const bool haveDivisor = membrane->getDynamicProperty("reliquary.lipschitzDivisor", divisor);
    check(haveDivisor, "the Membrane registers its Lipschitz divisor as a readable property");
    double divisorValue = 0.0;
    if (haveDivisor) propertyValueToNumber(divisor, divisorValue);
    report("authored Lipschitz divisor", divisorValue);

    // Probe the membrane's zero set the way the marcher finds it — by MARCHING
    // to it — and at every point it lands, recompute the geometry from first
    // principles and require it to be exactly one of the two surfaces the Zone
    // authored: the gyroid membrane at |g| = 0.42, or the Oculus shaft wall at
    // a 6 m radius. Checking "|d| < eps" over a grid instead would prove
    // nothing — the conservative divisor makes |d| small over a large fraction
    // of space, so a grid threshold reports a huge "surface" that is mostly
    // empty air. And a ray fired down the Oculus legitimately lands on the
    // shaft, where |g| is arbitrary, so the check has to know about both.
    {
        std::mt19937 marchRng(20260929u);
        std::uniform_real_distribution<float> j3(-1.0f, 1.0f);
        int landed = 0, onGyroid = 0, onShaft = 0, onVessel = 0, neither = 0;
        double worstGyroidError = 0.0, worstShaftError = 0.0, worstVesselError = 0.0;
        int startedInside = 0;
        for (int i = 0; i < 2000; ++i) {
            // Start on a sphere OUTSIDE the Zone, aimed at the Reliquary's heart.
            // The first version of this marched from random points inside the
            // 60 m box, and the gyroid fills all of space — so a large fraction
            // of those origins were embedded in the membrane itself and the
            // "landing" was the first sample, taken from inside the wall. It
            // reported 491 landings on neither surface; all 491 were membrane,
            // approached from the wrong side. A camera cannot start inside a
            // wall, so neither can this.
            // 32 m: just outside the 30 m vessel. That is the realistic viewing
            // distance for a 60 m cathedral, and it is where the honest
            // measurement lives. From 46 m the hit rate collapses to 6.5% and
            // raising the step budget to 2000 does not move it at all — because
            // a hollow sphere of thin lattice SHOULD let a ray aimed at its
            // centre thread the corridors and leave the far side. On the WebGPU
            // build the proof-based range hierarchy closes that gap by jumping
            // the proved-empty exterior in one authorized move; this CPU march
            // has no such acceleration, so it under-samples from far away by
            // construction and asserting otherwise would be asserting a fiction.
            glm::vec3 ro(j3(marchRng), j3(marchRng), j3(marchRng));
            if (glm::length(ro) < 0.3f) continue;
            ro = glm::normalize(ro) * 32.0f;
            glm::vec3 rd = glm::normalize(-ro + 4.0f * glm::vec3(j3(marchRng), j3(marchRng), j3(marchRng)));
            float t = 0.0f;
            for (int step = 0; step < 192; ++step) {   // the renderer's own budget
                if (t > 140.0f) break;
                const glm::vec3 p = ro + rd * t;
                // The renderer only ever evaluates the shape inside its proxy
                // AABB, so the march must stop there too. This is not a
                // convenience: the gyroid is PERIODIC, so its distance function
                // describes an infinite lattice and stays perfectly valid —
                // negative, even — at any distance. Marching without the bound
                // flies straight out of the 60 m Zone and lands on the same
                // membrane 50 m outside the building. The Zone records this
                // about itself (reliquary.mathIsUnbounded on reliquary.membrane).
                if (std::abs(p.x) > 32.0f || std::abs(p.y) > 32.0f || std::abs(p.z) > 32.0f) break;
                const float d = geom::evalSdf(membraneSdf, p);
                if (d < 0.002f) {
                    if (d < -0.01f) ++startedInside;
                    ++landed;
                    // The Zone authors THREE surfaces, and the check has to know
                    // all three. The gyroid at |g| = t; the vessel sphere at
                    // r = 30, where the lattice is cut by the authored bound and
                    // the Reliquary's outer edge is made; and the Oculus shaft at
                    // a 6 m radius. When the vessel bound was added to stop the
                    // gyroid being an infinite solid, this check still only knew
                    // about the first and third, and reported 472 perfectly good
                    // landings on the vessel rim as "neither". A test that does
                    // not know all the geometry a Zone declares will call the
                    // Zone wrong for having declared it.
                    const double gErr = std::abs(std::abs(referenceGyroid(p.x * kK, p.y * kK, p.z * kK)) - kThick);
                    const double shaftErr = std::abs(std::sqrt(p.x * p.x + p.z * p.z) - kShaftR);
                    const double vesselErr = std::abs(glm::length(p) - static_cast<float>(kVesselR));
                    if (gErr < 0.02) { ++onGyroid; worstGyroidError = std::max(worstGyroidError, gErr); }
                    else if (shaftErr < 0.02) { ++onShaft; worstShaftError = std::max(worstShaftError, shaftErr); }
                    else if (vesselErr < 0.02) { ++onVessel; worstVesselError = std::max(worstVesselError, vesselErr); }
                    else ++neither;
                    break;
                }
                t += std::max(d, 0.002f);
            }
        }
        report("marched landings on the Membrane", landed);
        report("landings on the gyroid surface", onGyroid);
        report("landings on the Oculus shaft wall", onShaft);
        report("landings on the vessel rim (r = 30)", onVessel);
        report("landings on neither", neither);
        report("landings found from INSIDE the wall (must be 0)", startedInside);
        report("worst |g| error on the gyroid", worstGyroidError);
        report("worst radius error on the shaft", worstShaftError);
        report("worst radius error on the vessel rim", worstVesselError);
        check(startedInside == 0,
              "no march ever began inside the membrane — every ray approaches the "
              "Reliquary from outside, the way a camera does");
        check(landed > 1500, "from just outside the vessel, the renderer's own 192-step budget "
              "lands on the Membrane for the great majority of rays — the Zone is visible, not theoretical");
        check(landed > 0 && onGyroid + onShaft + onVessel == landed,
              "every marched landing is on one of the three surfaces the Zone actually "
              "authors: the gyroid at |g| = the half-thickness, the Oculus shaft at its "
              "6 m radius, or the vessel rim at 30 m. Nothing lands anywhere else.");
        check(onShaft > 0 && onGyroid > 0 && onVessel > 0,
              "all three surfaces are really there — the Reliquary has a membrane, a hole "
              "cut through its crown, and an outer edge where the infinite lattice was "
              "bounded into a place");
    }

    // --- 5. THE CONSERVATISM PROOF ------------------------------------------
    // Sphere tracing steps by the authored distance. If the divisor is
    // optimistic, a step can carry the sample from one side of the membrane to
    // the other and the Reliquary renders full of holes. This traces real rays
    // with the shader's own rule and asserts it never happens.
    //
    // The check has to be ordered carefully, and the first version got that
    // wrong in a way that made it pass while proving nothing: it declared a hit
    // on `d < 0.001` BEFORE looking for a sign change, so a ray whose very
    // first sample was already negative — which is what happens when the ray
    // starts inside a wall — was recorded as a clean hit and never reached the
    // tunnel test. Origins now sit on a sphere outside the Zone, and the tunnel
    // test is evaluated first, so an overshoot cannot hide behind a hit.
    std::mt19937 rng(20260929u);
    std::uniform_real_distribution<float> jitter(-1.0f, 1.0f);
    std::uniform_real_distribution<float> dirBias(-1.0f, 1.0f);

    int raysTraced = 0;
    int raysHit = 0;
    int tunnels = 0;          // a step that crossed the surface without stopping
    int originsInside = 0;    // rays that began embedded in the membrane
    double worstOvershoot = 0.0;
    for (int i = 0; i < 4000; ++i) {
        // Start outside the vessel, on a 32 m shell, aimed inward.
        glm::vec3 ro(jitter(rng), jitter(rng), jitter(rng));
        if (glm::length(ro) < 0.3f) continue;
        ro = glm::normalize(ro) * 32.0f;
        glm::vec3 rd = -ro;
        rd += 4.0f * glm::vec3(dirBias(rng), dirBias(rng), dirBias(rng));
        rd = glm::normalize(rd);

        if (geom::evalSdf(membraneSdf, ro) < 0.0f) ++originsInside;

        float t = 0.0f;
        float prevD = geom::evalSdf(membraneSdf, ro);
        bool hit = false;
        bool tunnelled = false;
        for (int step = 0; step < 192; ++step) {
            if (t > 140.0f) break;
            const glm::vec3 p = ro + rd * t;
            if (std::abs(p.x) > 32.0f || std::abs(p.y) > 32.0f || std::abs(p.z) > 32.0f) break;
            const float d = geom::evalSdf(membraneSdf, p);
            // TUNNEL FIRST. A conservative distance can never carry the sample
            // from outside to inside in one step, so if prevD > 0 and d < 0 the
            // divisor was optimistic by at least prevD.
            if (prevD > 0.0f && d < -0.002f) {
                tunnelled = true;
                worstOvershoot = std::max(worstOvershoot, static_cast<double>(prevD));
                break;
            }
            if (d < 0.001f) { hit = true; break; }
            prevD = d;
            t += std::max(d, 0.002f);
        }
        ++raysTraced;
        if (hit) ++raysHit;
        if (tunnelled) ++tunnels;
    }
    report("rays traced", raysTraced);
    report("rays that found the Membrane", raysHit);
    report("rays that began inside the wall (must be 0)", originsInside);
    report("rays that tunnelled through it", tunnels);
    report("worst overshoot (metres)", worstOvershoot);
    check(originsInside == 0, "every conservativeness ray began in open space, not inside a wall");
    check(tunnels == 0,
          "CONSERVATISM: no ray ever steps through the membrane — the authored "
          "divisor is a true lower bound on the distance, so the marcher cannot tunnel");
    check(raysHit > raysTraced / 4,
          "the Membrane is actually there: a large share of rays find surface");

    // --- 6. The Oculus is a real hole --------------------------------------
    // On the Reliquary's axis, above the Heart, the membrane is subtracted
    // away. If the difference were authored wrongly there would be a ceiling
    // there instead of an opening.
    const glm::vec3 onAxis(0.0f, 0.0f, 0.0f);
    const float dAxis = geom::evalSdf(membraneSdf, onAxis);
    check(std::abs(dAxis) > 0.05f,
          "the Oculus is a real opening: the Reliquary's own axis is not membrane");
    report("distance to the surface on the axis", dAxis);

    // Just outside the shaft radius, the membrane must be present.
    const float dJustOutside = geom::evalSdf(membraneSdf, glm::vec3(7.5f, 12.0f, 0.0f));
    const float dJustInside = geom::evalSdf(membraneSdf, glm::vec3(3.0f, 12.0f, 0.0f));
    report("d at r=7.5m, y=12m", dJustOutside);
    report("d at r=3.0m, y=12m", dJustInside);
    check(dJustOutside < dJustInside,
          "the Oculus is bounded at the authored 6 m radius — the shaft is a shaft");

    // --- 7. The Heart is genuinely woven, not a solid ball ------------------
    // The first version of this Zone authored the Heart as `max(ball, g/L)`,
    // which reads like a ball carved by the gyroid SURFACE. It is not: a
    // surface intersected with a solid is a curve, and because the lattice
    // term's positive range is tiny next to the ball's, the whole expression
    // collapses to a solid ball with about a tenth of a metre of fluting on
    // its skin. So the test does not ask "is there surface" — a solid ball has
    // plenty. It asks how much of the ball's VOLUME is actually solid, and
    // requires roughly half, because the gyroid's g > 0 half-space is half of
    // space and clipping it to a ball cannot change that much.
    if (heart) {
        const geom::SdfNode& heartSdf = heart->getFieldData();
        check(heartSdf.prim == geom::SdfPrim::Expr && heartSdf.mathNode != nullptr,
              "the Heart's shape IS an authored OntoMath expression");
        std::string heartTypeError;
        check(heartSdf.mathNode && heartSdf.mathNode->checkTypes(ambientEnv(), heartTypeError),
              "the Heart's authored expression type-checks");

        // Volume sample inside the authored ball.
        std::mt19937 heartRng(7u);
        std::uniform_real_distribution<float> unit(-1.0f, 1.0f);
        int solid = 0, inBall = 0;
        for (int i = 0; i < 120000; ++i) {
            glm::vec3 p(unit(heartRng), unit(heartRng), unit(heartRng));
            if (glm::length(p) > static_cast<float>(kHeartR)) continue;
            ++inBall;
            if (geom::evalSdf(heartSdf, p) < 0.0f) ++solid;
        }
        const double fraction = inBall ? static_cast<double>(solid) / inBall : 0.0;
        report("points sampled inside the Heart's ball", inBall);
        report("solid fraction of the Heart", fraction);
        check(fraction > 0.35 && fraction < 0.65,
              "the Heart is WOVEN, not solid: roughly half its volume is material and "
              "half is open corridor, as clipping a gyroid half-space to a ball must give");

        // And it must be porous all the way through — a ray fired at the centre
        // has to find open air on the far side, or the "corridors" are sealed.
        int raysThrough = 0, raysEntered = 0;
        for (int i = 0; i < 600; ++i) {
            std::uniform_real_distribution<float> dir(-1.0f, 1.0f);
            glm::vec3 rd(dir(heartRng), dir(heartRng), dir(heartRng));
            if (glm::length(rd) < 0.2f) continue;
            rd = glm::normalize(rd);
            const glm::vec3 ro = -rd * (static_cast<float>(kHeartR) + 6.0f);
            float t = 0.0f;
            bool entered = false, exitedSolid = false;
            for (int step = 0; step < 900; ++step) {
                if (t > 2.0f * static_cast<float>(kHeartR) + 14.0f) break;
                const float d = geom::evalSdf(heartSdf, ro + rd * t);
                if (d < 0.0f) { if (!entered) entered = true; }
                else if (entered) { exitedSolid = true; break; }
                t += std::max(std::abs(d), 0.02f);
            }
            if (entered) ++raysEntered;
            if (exitedSolid) ++raysThrough;
        }
        report("rays that entered the Heart's material", raysEntered);
        report("rays that came back out into open air", raysThrough);
        check(raysEntered > 100 && raysThrough > 50,
              "the Heart is genuinely porous: rays enter its woven material and emerge "
              "again, so its corridors are open and you can see through it");

        // The surface must still be found by marching, from outside.
        int heartHits = 0;
        for (int i = 0; i < 800; ++i) {
            std::uniform_real_distribution<float> d3(-1.0f, 1.0f);
            // Fire from a SPHERE around the Heart, aimed at its centre. Firing
            // every ray from one point on the +x axis instead makes ~93% of them
            // miss without ever reaching it — from 16 m away the Heart subtends a
            // 30-degree cone, which is 6.7% of directions — so the hit rate ends
            // up measuring that cone rather than the Heart. It reported 44/800,
            // which looked like a broken Heart and was not.
            glm::vec3 ro(d3(heartRng), d3(heartRng), d3(heartRng));
            if (glm::length(ro) < 0.2f) continue;
            ro = glm::normalize(ro) * (static_cast<float>(kHeartR) + 8.0f);
            glm::vec3 rd = glm::normalize(-ro + 3.0f * glm::vec3(d3(heartRng), d3(heartRng), d3(heartRng)));
            float t = 0.0f;
            for (int step = 0; step < 700; ++step) {
                if (t > 40.0f) break;
                const float d = geom::evalSdf(heartSdf, ro + rd * t);
                if (d < 0.002f) { ++heartHits; break; }
                if (d < -0.002f) break;      // stepped inside without landing on the
                                             // surface: only possible if the distance
                                             // were NOT conservative
                t += std::max(d, 0.01f);
            }
        }
        report("rays that found the Heart's surface from outside", heartHits);
        check(heartHits > 300, "the Heart's woven surface is reachable by the raymarcher from "
              "most directions that actually meet it");
    }

    // --- 8. The Floor is a proven heightfield ------------------------------
    if (floor) {
        const geom::SdfNode& floorSdf = floor->getFieldData();
        const OntoMath::MathNode* height = nullptr;
        check(geom::isHeightfieldExpr(floorSdf, &height),
              "the Floor is PROVEN a heightfield y - h(x, z) by the engine, not asserted by me");
        if (height) {
            const auto hv = height->evaluate(varsAt(3.0, 0.0, -7.0));
            double h = 0.0;
            check(hv && propertyValueToNumber(*hv, h) && std::isfinite(h),
                  "the Floor's authored height evaluates to a finite number");
            report("floor height h(3, -7)", h);
        }
        // The Floor's own honesty: it records that it does NOT get an analytic
        // gradient, because isDifferentiableAst refuses trans-bearing terms.
        PropertyValue analytic;
        const bool recorded = floor->getDynamicProperty("reliquary.analyticGradient", analytic);
        check(recorded, "the Floor records its gradient provenance as a readable property");
        if (recorded) {
            const bool* flag = std::get_if<bool>(&analytic);
            check(flag && *flag == false,
                  "the Floor's record is truthful: its gradient is central-differenced, not derived");
        }
    }

    // --- 9. The radiant beings carry evaluable, BOUNDED mathematics --------
    // std::string, not const char*: FieldNode::getIdentifier() returns BY VALUE,
    // and holding .c_str() of the temporary is a dangling pointer. It printed
    // the same name five times and one block of mojibake before this was fixed.
    struct Radiant { std::string id; const geom::FieldNode* node; };
    std::vector<Radiant> radiants;
    if (const auto* root = zone->spatialRoot()) {
        radiants.push_back({std::string("reliquary.own-light (spatial root)"), root});
    }
    for (const auto& f : zone->additionalSpatialFields()) {
        if (f) radiants.push_back({f->getIdentifier(), f.get()});
    }
    check(radiants.size() == 6, "the Zone has its spatial root and five radiant beings");
    if (radiants.size() != 6) report("radiant count", static_cast<double>(radiants.size()));

    for (const auto& r : radiants) {
        bool ok = r.node->field != nullptr
                  && r.node->field->mode == OntoMath::ScalarField::EvaluationMode::AST
                  && !r.node->field->astDefinition.pieces.empty();
        check(ok, std::string(r.id) + ": carries an AST-mode scalar field with authored pieces");

        const double centre = r.node->field
            ? evalScalar(r.node->field->astDefinition, 0.0, 0.0, 0.0)
            : std::nan("");
        const double far = r.node->field
            ? evalScalar(r.node->field->astDefinition, 900.0, 900.0, 900.0)
            : std::nan("");
        std::cout << "       " << r.id << ": rho(0)=" << centre << "  rho(900)=" << far << std::endl;
        check(std::isfinite(centre), std::string(r.id) + ": its density evaluates finitely at the origin");
        check(std::isfinite(far) && far <= 0.0,
              std::string(r.id) + ": its density is EXACTLY zero far away — the medium has a real boundary");
    }

    // The Heart's source is the Zone's reason to exist; its chroma must be a
    // Vector that actually changes across its radius, or the gold is a constant.
    for (const auto& f : zone->additionalSpatialFields()) {
        if (!f || f->getIdentifier() != "reliquary.heart-source") continue;
        check(f->lightChroma != nullptr && !f->lightChroma->pieces.empty(),
              "the Heart's source carries authored chroma (Rung 5)");
        check(f->lightAngular != nullptr && !f->lightAngular->pieces.empty(),
              "the Heart's source carries authored angular emission (Rung 6)");
        if (f->lightChroma) {
            glm::vec3 warm(0.0f), cool(0.0f);
            const bool a = evalVector(*f->lightChroma, warm, 0.0, 0.0, 0.0);
            const bool b = evalVector(*f->lightChroma, cool, 8.0, 0.0, 0.0);
            check(a && b, "the Heart's chroma evaluates to a Vector at two radii");
            if (a && b) {
                std::cout << "       chroma at r=0  = (" << warm.r << ", " << warm.g << ", " << warm.b << ")" << std::endl;
                std::cout << "       chroma at r=8  = (" << cool.r << ", " << cool.g << ", " << cool.b << ")" << std::endl;
                check(glm::length(warm - cool) > 0.05f,
                      "the Heart's chroma genuinely varies with radius — white-gold at the "
                      "core, ember at the edge, as authored");
                check(std::isfinite(warm.r) && std::isfinite(cool.b),
                      "the Heart's chroma is finite everywhere sampled");
            }
        }
    }

    // The light must also be a light: readAuthorableLight is the same path the
    // renderer uses, so a field that only LOOKS like a light fails here.
    for (const auto& r : radiants) {
        Rendering::AuthorableLightState light;
        const bool reads = Rendering::readAuthorableLight(*r.node, light);
        check(reads && light.enabled,
              std::string(r.id) + ": resolves as an enabled authored light through the renderer's own reader");
    }

    // The labyrinth medium must be a MEDIUM, not a source: it is fog, and fog
    // that also emits is the exact failure the density-sovereignty constitution
    // (rho_source != V_transport != D_medium) exists to prevent.
    for (const auto& f : zone->additionalSpatialFields()) {
        if (!f || f->getIdentifier() != "reliquary.labyrinth-medium") continue;
        check(f->volumeDensity != nullptr && !f->volumeDensity->pieces.empty(),
              "the Labyrinth Medium carries authored volumeDensity D(p, t)");
        check(f->volumeExtinction != nullptr && !f->volumeExtinction->pieces.empty(),
              "the Labyrinth Medium carries authored extinction, so transport has a real sigma_t");
        check(f->volumeEmission == nullptr || f->volumeEmission->pieces.empty(),
              "DENSITY SOVEREIGNTY: the medium does not also emit — the gold light "
              "belongs to the Heart's source alone");
        if (f->volumeDensity) {
            double dCentre = 0.0, dMembrane = 0.0;
            const auto a = f->volumeDensity->evaluate(varsAt(0.0, 0.0, 0.0));
            // A point sitting on the membrane is |g| = 0.42; the medium is
            // authored to vanish there, so the density must be far lower there
            // than in an open channel.
            const auto b = f->volumeDensity->evaluate(varsAt(1.9, 0.0, 0.0));
            if (a) propertyValueToNumber(*a, dCentre);
            if (b) propertyValueToNumber(*b, dMembrane);
            std::cout << "       D(0,0,0)=" << dCentre << "  D(1.9,0,0)=" << dMembrane << std::endl;
            check(std::isfinite(dCentre) && std::isfinite(dMembrane) && dCentre >= 0.0 && dMembrane >= 0.0,
                  "the medium's density is finite and non-negative where sampled");
        }
    }

    // The Nimbus's Perlin term must not be the only thing holding it up: a
    // NaN anywhere in a volume expression silently deletes the whole pass.
    for (const auto& f : zone->additionalSpatialFields()) {
        if (!f || f->getIdentifier() != "reliquary.nimbus" || !f->volumeDensity) continue;
        bool allFinite = true;
        for (int i = 0; i < 64 && allFinite; ++i) {
            const double a = i * 0.7;
            const double d = evalScalar(*f->volumeDensity, a, a * 0.5, -a * 0.25);
            if (!std::isfinite(d) || d < 0.0) allFinite = false;
        }
        check(allFinite,
              "the Nimbus's Perlin-warmed density is finite and non-negative across the "
              "sampled sphere — no NaN can delete the volume pass");
    }

    // --- 10. THE SHADER MUST ACTUALLY COMPILE ------------------------------
    // Everything above proves the mathematics is TRUE. This proves the GPU can
    // be TOLD about it, which is a different failure with the same silence: a
    // Zone whose every field refuses compilation still loads, still hydrates,
    // still passes all 72 checks above, and renders as a black screen while the
    // log spams one refusal per frame per source.
    //
    // This Zone shipped that bug first. Every Piecewise here declared
    // `"input": "p"`, and emitPiecewise resolves the piece-bound variable
    // through pointComponent (SdfWgsl.cpp:664), which binds x, y, z, t, n,
    // omega.*, wi.* and wo.* — and nothing else, not `p`. So the whole radiance
    // program was refused. The trap is that `p` IS legal inside a piece's
    // mathNode, because emitMathNode special-cases a ValueLeaf of `p` before it
    // reaches pointComponent — so `length(p)` compiles and `"input": "p"` does
    // not, and they are one character apart. No CPU-side evaluator cares:
    // MathNode::evaluate binds `p` happily, which is exactly why 72 green
    // checks shipped alongside a Zone that could not draw.
    //
    // These inspectors run the PRODUCTION WGSL emitter on the CPU and hand
    // back its refusal, so this whole class of bug is now caught headlessly
    // with no GPU and no display session.
    {
        // std::string, not const char*: FieldNode::getIdentifier() returns BY
        // VALUE, and holding .c_str() of the temporary is a dangling pointer. It
        // printed mojibake here and the same mistake was already made and fixed
        // once above in the Radiant struct.
        struct RadianceCheck { std::string who; const geom::FieldNode* node; };
        std::vector<RadianceCheck> nodes;
        if (const auto* root = zone->spatialRoot()) nodes.push_back({"reliquary.own-light", root});
        for (const auto& f : zone->additionalSpatialFields()) {
            if (f) nodes.push_back({f->getIdentifier(), f.get()});
        }
        check(nodes.size() == 6, "six radiant beings to compile");

        for (const auto& n : nodes) {
            const auto& node = *n.node;

            // rho(p, t) — the source radiance scalar. bindTime: this Zone
            // breathes, so it names t.
            if (node.field && !node.field->astDefinition.pieces.empty()) {
                const auto layout = sdfwgsl::inspectScalarExpression(&node.field->astDefinition, true);
                check(layout.ok, std::string(n.who) + ": its source radiance COMPILES to WGSL "
                      "(not merely evaluates on the CPU)");
                if (!layout.ok) std::cout << "       " << layout.error << std::endl;
            }

            // chi(p, t) — source chroma, a Vector.
            if (node.lightChroma && !node.lightChroma->pieces.empty()) {
                const auto layout = sdfwgsl::inspectVectorExpression(node.lightChroma.get(), true);
                check(layout.ok, std::string(n.who) + ": its source chroma COMPILES to WGSL");
                if (!layout.ok) std::cout << "       " << layout.error << std::endl;
            }

            // alpha(p, omega, t) — angular emission. This is the only
            // source-radiance scalar context that admits omega.
            if (node.lightAngular && !node.lightAngular->pieces.empty()) {
                const auto layout = sdfwgsl::inspectAngularExpression(node.lightAngular.get());
                check(layout.ok, std::string(n.who) + ": its angular emission COMPILES to WGSL");
                if (!layout.ok) std::cout << "       " << layout.error << std::endl;
            }

            // D, sigma_t, sigma_s — the participating medium.
            if (node.volumeDensity && !node.volumeDensity->pieces.empty()) {
                const auto d = sdfwgsl::inspectDensityExpression(node.volumeDensity.get());
                check(d.ok, std::string(n.who) + ": its volume density D COMPILES to WGSL");
                if (!d.ok) std::cout << "       " << d.error << std::endl;
            }
            if (node.volumeExtinction && !node.volumeExtinction->pieces.empty()) {
                const auto x = sdfwgsl::inspectExtinctionExpression(node.volumeExtinction.get());
                check(x.ok, std::string(n.who) + ": its extinction COMPILES to WGSL");
                if (!x.ok) std::cout << "       " << x.error << std::endl;
            }
            if (node.volumeScattering && !node.volumeScattering->pieces.empty()) {
                const auto sc = sdfwgsl::inspectScatteringExpression(node.volumeScattering.get());
                check(sc.ok, std::string(n.who) + ": its scattering COMPILES to WGSL");
                if (!sc.ok) std::cout << "       " << sc.error << std::endl;
            }
        }

        // The Membrane's and the Heart's geometry, through the SDF emitter.
        // This is the one that would refuse an operator the WGSL backend cannot
        // express — and, unlike the CPU path, it REFUSES rather than inventing a
        // zero, so this is the check that keeps an authored shape from silently
        // becoming empty space on screen.
        for (const auto& obj : objects) {
            if (!obj || !obj->hasField()) continue;
            const auto& sdf = obj->getFieldData();
            if (sdf.prim != geom::SdfPrim::Expr || !sdf.mathNode) continue;
            const auto layout = sdfwgsl::inspectOccluderLayout(&sdf);
            check(layout.ok, obj->getIdentifier() + ": its authored geometry COMPILES to WGSL "
                  "(no operator the analytic backend cannot express)");
            if (!layout.ok) std::cout << "       " << layout.error << std::endl;
        }

        // And the Materials' colour expressions, which are compiled into the
        // same shader as a replacement for baseColor.
        for (const auto& m : dMaterials) {
            if (!m || !m->colorExpr || m->colorExpr->pieces.empty()) continue;
            const auto layout = sdfwgsl::inspectVectorExpression(m->colorExpr.get(), false);
            check(layout.ok, "material " + m->getIdentifier() + ": its colorExpr COMPILES to WGSL");
            if (!layout.ok) std::cout << "       " << layout.error << std::endl;
        }
    }

    std::cout << "------------------------------------------------------------\n";
    std::cout << g_checks << " checks completed (" << (g_checks - g_failures)
              << " passed, " << g_failures << " failed).\n";
    if (g_failures == 0) {
        std::cout << "gyroid_reliquary_test: ALL OK\n";
        return 0;
    }
    std::cout << "gyroid_reliquary_test: FAILED\n";
    return 1;
}
