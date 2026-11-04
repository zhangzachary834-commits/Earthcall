// Witness for "The Merry Dancers", Zach's aurora Zone (2026-11-03).
//
// Beauty is Zach's verdict, not this test's: see the Person Verification List. This
// asks whether the Zone's factual claims about itself are true of the saved numbers,
// through the same store hydration, type checks and shader emitter the app uses.
//
//   1. It hydrates from a temporary SaveRoot; every being and material resolves.
//   2. Every channel and every surface COMPILES to WGSL. Compiling is a different claim
//      from evaluating, and a refused shader draws nothing.
//   3. LIGHT NEVER LEAVES THE GATE. Density gates where emission can fire, so emission
//      that is nonzero where density is zero would be silently clipped sheet.
//   4. The sheet is THIN and fires somewhere real, so it is a sheet and not fog.
//   5. COLOUR IS ALTITUDE: green low in the sheet, red-violet high. This is the claim
//      the Zone is built on.
//   6. The reflection is the band's exact mirror, at the authored gain.
//   7. The sheets MOVE: emission at a point changes with t.
//   8. The sky's ridge is only computed in its elevation band, and the far pieces agree
//      with it where they meet.
//   9. Save Zone keeps it: nothing authored is lost through a round trip.
//
// The source Zone is copied and never written. argv[1] may name a different SaveRoot.
// Claude Code / Claude Opus 5.5 / session ed9d515b-d6ac-4098-a7f9-ce71d02406b3 / 2026-11-03.

#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/AuthoredView.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

extern MaterialManager materials;
namespace fs = std::filesystem;

namespace {
const char* kZoneId = "The Merry Dancers";
int g_checks = 0;
int g_failures = 0;

void check(bool ok, const std::string& what) {
    ++g_checks;
    if (!ok) ++g_failures;
    std::cout << (ok ? "  ok: " : "  FAILED: ") << what << std::endl;
}

std::map<std::string, PropertyValue> varsAt(const glm::vec3& p, double t, const glm::vec3& omega) {
    return {{"p", PropertyValue(p)},
            {"x", PropertyValue(p.x)}, {"y", PropertyValue(p.y)}, {"z", PropertyValue(p.z)},
            {"t", PropertyValue(static_cast<float>(t))},
            {"omega.x", PropertyValue(omega.x)}, {"omega.y", PropertyValue(omega.y)},
            {"omega.z", PropertyValue(omega.z)}};
}

bool emissionAt(const geom::FieldNode& n, const glm::vec3& p, double t, glm::vec3& out) {
    const auto r = n.volumeEmission->evaluate(varsAt(p, t, glm::normalize(glm::vec3(0.1f, -0.6f, -0.8f))));
    if (!r) return false;
    if (const auto* v = std::get_if<glm::vec3>(&r.value())) { out = *v; return true; }
    return false;
}

double densityAt(const geom::FieldNode& n, const glm::vec3& p) {
    const auto r = n.volumeDensity->evaluate(varsAt(p, 0.0, glm::vec3(0, 0, 1)));
    double d = std::nan("");
    if (r) propertyValueToNumber(*r, d);
    return d;
}

// Walk across the sheet at (x, y) and return the z of peak total emission.
double peakZ(const geom::FieldNode& n, float x, float y, double t, glm::vec3* best = nullptr) {
    const float hz = std::abs(n.scale.z);
    double bestZ = 0.0, bestE = -1.0;
    for (float z = -hz; z <= hz; z += 0.5f) {
        glm::vec3 e(0.f);
        if (!emissionAt(n, glm::vec3(x, y, z), t, e)) continue;
        const double sum = e.x + e.y + e.z;
        if (sum > bestE) {
            bestE = sum;
            bestZ = z;
            if (best) *best = e;
        }
    }
    return bestZ;
}
}  // namespace

int main(int argc, char** argv) {
    std::cout << "The Merry Dancers: the Zone's claims about itself\n";
    const fs::path repoRoot = fs::path(__FILE__).parent_path().parent_path().parent_path();
    const fs::path source = argc > 1 ? fs::path(argv[1]) : repoRoot / "saves";
    check(fs::exists(source / "zones" / kZoneId / "zone.json"), "the Zone's first seed exists");

    const fs::path scratch = fs::temp_directory_path() /
        ("earthcall-merry-dancers-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    fs::create_directories(scratch / "zones");
    fs::copy(source / "zones" / kZoneId, scratch / "zones" / kZoneId, fs::copy_options::recursive);
    const nlohmann::json original = SaveSystem::readSaveData((scratch / "zones" / kZoneId / "zone.json").string());
    SaveSystem::setSaveRoot(scratch.string());

    // --- 1. Hydration ------------------------------------------------------------
    const auto loadStart = std::chrono::steady_clock::now();
    ZoneManager mgr;
    mgr.hydrateFromZoneStore();
    const double loadSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - loadStart).count();
    std::cout << "       hydrate seconds = " << loadSeconds << std::endl;
    std::shared_ptr<Zone> zone;
    size_t zoneIndex = 0;
    for (size_t i = 0; i < mgr.zones().size(); ++i) {
        if (mgr.zones()[i] && mgr.zones()[i]->getIdentifier() == kZoneId) { zone = mgr.zones()[i]; zoneIndex = i; }
    }
    check(zone != nullptr, "the Zone hydrates through ZoneManager");
    if (!zone) return 1;
    check(original.value("owner", "") == "Zach", "owned by the Person who asked for it");
    check(!original.value("injected_by", "").empty(), "an author is recorded");

    const auto& objects = zone->getOwnedObjects();
    check(objects.size() == 28, "sky, shore and 26 spruces hydrate");
    bool materialsResolve = true, colourAuthored = true;
    for (const auto& o : objects) {
        auto m = o ? materials.get(o->materialId()) : nullptr;
        if (!m) { materialsResolve = false; continue; }
        if (!m->colorExpr || m->colorExpr->pieces.empty()) colourAuthored = false;
    }
    check(materialsResolve, "every Object's material resolves");
    check(colourAuthored, "every surface carries an authored colour field");

    // The sky is only visible if the Zone's sight reaches it: the app's historical far
    // plane clipped it, invisibly to every other check in this file.
    {
        const float reach = Rendering::authoredFarPlane(zone->spatialRoot(), 100.0f, 0.1f);
        std::cout << "       authored far plane = " << reach << std::endl;
        float farthestSky = 0.f;
        for (const auto& o : objects)
            if (o && o->getIdentifier() == "merry.sky") farthestSky = o->getFieldExtent().x;
        check(farthestSky > 0.f, "the sky surface is present");
        check(reach > farthestSky * 1.5f, "the Zone's far plane reaches the sky, and past it from anywhere nearby");
    }

    // --- 2. Everything compiles ---------------------------------------------------
    for (const auto& o : objects) {
        if (!o) continue;
        auto m = materials.get(o->materialId());
        const auto program = sdfwgsl::compile(o->getFieldData(), nullptr, m ? m->colorExpr.get() : nullptr, nullptr);
        if (!program.ok) std::cout << "       " << o->getIdentifier() << ": " << program.error << std::endl;
        check(program.ok, o->getIdentifier() + " compiles to WGSL");
    }
    std::vector<geom::FieldNode*> media;
    for (const auto& f : zone->additionalSpatialFields()) if (f) media.push_back(f.get());
    check(media.size() == 3, "two sheets and one reflection");
    geom::FieldNode* arc = nullptr;
    geom::FieldNode* band = nullptr;
    geom::FieldNode* mirror = nullptr;
    for (auto* n : media) {
        const std::string id = n->getIdentifier();
        if (id == "merry.aurora.overhead-arc") arc = n;
        if (id == "merry.aurora.far-band") band = n;
        if (id == "merry.aurora.far-band.reflection") mirror = n;
        const auto d = sdfwgsl::inspectDensityExpression(n->volumeDensity.get());
        const auto e = sdfwgsl::inspectEmissionExpression(n->volumeEmission.get());
        if (!d.ok) std::cout << "       " << d.error << std::endl;
        if (!e.ok) std::cout << "       " << e.error << std::endl;
        check(d.ok, id + ": density compiles");
        check(e.ok, id + ": emission compiles");
    }
    check(arc && band && mirror, "the three media are present by stable identifier");
    if (!arc || !band || !mirror) return 1;

    // --- 3. Light never leaves the gate -------------------------------------------
    for (auto* n : media) {
        int fired = 0, escaped = 0;
        const glm::vec3 h = glm::abs(n->scale);
        const float ySign = n == mirror ? -1.f : 1.f;
        for (float x = -h.x + 5.f; x < h.x; x += 37.f) {
            for (float yy = -0.9f; yy < 1.0f; yy += 0.2f) {
                const float y = yy * h.y * ySign;
                const float centre = static_cast<float>(peakZ(*n, x, y, 20.0));
                for (float dz = -40.f; dz <= 40.f; dz += 2.f) {
                    const glm::vec3 p(x, y, centre + dz);
                    if (std::abs(p.z) > h.z) continue;
                    glm::vec3 e(0.f);
                    if (!emissionAt(*n, p, 20.0, e)) continue;
                    if (e.x + e.y + e.z > 1e-5f) {
                        ++fired;
                        if (!(densityAt(*n, p) > 0.0)) ++escaped;
                    }
                }
            }
        }
        std::cout << "       " << n->getIdentifier() << ": fired=" << fired << " escaped=" << escaped << std::endl;
        check(fired > 200, n->getIdentifier() + " actually shines");
        check(escaped == 0, n->getIdentifier() + ": no emission escapes its density gate");
    }

    // --- 4. A sheet, not fog ------------------------------------------------------
    {
        const float y = -band->scale.y + 180.f;
        glm::vec3 peak(0.f);
        const double z0 = peakZ(*band, 40.f, y, 20.0, &peak);
        glm::vec3 off(0.f);
        emissionAt(*band, glm::vec3(40.f, y, static_cast<float>(z0) + 30.f), 20.0, off);
        check(peak.y > 0.0f, "the far band has a bright core");
        check(off.x + off.y + off.z == 0.0f, "30 m off the sheet, emission is exactly zero");
    }

    // --- 5. Colour is altitude ----------------------------------------------------
    {
        // Sum across x, so one dark ray cannot decide the claim. World altitudes.
        auto spectrumAt = [&](float worldY) {
            glm::vec3 total(0.f);
            const float y = worldY - band->origin.y;
            for (float x = -420.f; x <= 420.f; x += 12.f) {
                glm::vec3 e(0.f);
                peakZ(*band, x, y, 20.0, &e);
                total += e;
            }
            return total;
        };
        const glm::vec3 low = spectrumAt(175.f);
        const glm::vec3 high = spectrumAt(460.f);
        std::cout << "       low rgb " << low.x << " " << low.y << " " << low.z
                  << "  high rgb " << high.x << " " << high.y << " " << high.z << std::endl;
        check(low.y > low.x && low.y > low.z, "low in the sheet, green dominates");
        check(high.x > high.y || high.z > high.y, "high in the sheet, red and violet exceed green");
    }

    // --- 6. The reflection is the mirror ------------------------------------------
    {
        const float gain = 0.70f;
        double worst = 0.0;
        int compared = 0;
        for (float x = -300.f; x <= 300.f; x += 50.f) {
            const float y = -band->scale.y + 170.f;
            glm::vec3 e(0.f);
            const float z = static_cast<float>(peakZ(*band, x, y, 20.0, &e));
            // World y maps to -y, and box-local y flips about each box's own origin.
            glm::vec3 m(0.f);
            if (!emissionAt(*mirror, glm::vec3(x, -y, z), 20.0, m)) continue;
            if (e.y < 0.004f) continue;
            ++compared;
            worst = std::max(worst, static_cast<double>(std::abs(m.y / e.y - gain) / gain));
        }
        std::cout << "       worst green-ratio error " << worst << " over " << compared << " rays" << std::endl;
        check(compared >= 5, "the reflection was compared at real emission");
        // Ripples soften the two finest ray layers, so the ratio is close, not exact.
        check(worst < 0.6, "the reflection follows the band at about its authored gain");
    }

    // --- 7. The sheets move -------------------------------------------------------
    {
        const float y = -arc->scale.y + 90.f;
        int moved = 0, samples = 0;
        for (float x = -300.f; x <= 300.f; x += 30.f) {
            glm::vec3 a(0.f), b(0.f);
            const float z = static_cast<float>(peakZ(*arc, x, y, 20.0));
            emissionAt(*arc, glm::vec3(x, y, z), 20.0, a);
            emissionAt(*arc, glm::vec3(x, y, z), 40.0, b);
            ++samples;
            if (glm::length(a - b) > 1e-3f) ++moved;
        }
        check(moved > samples / 2, "emission changes with t across the arc");
    }

    // --- 8. The sky's pieces meet -------------------------------------------------
    {
        auto sky = materials.get("material.merry.sky");
        check(sky && sky->colorExpr && sky->colorExpr->pieces.size() == 3, "the sky has its band and two sky-only pieces");
        if (sky && sky->colorExpr) {
            auto colourAt = [&](const glm::vec3& p, glm::vec3& out) {
                const auto r = sky->colorExpr->evaluate({{"p", PropertyValue(p)}, {"x", PropertyValue(p.x)},
                                                         {"y", PropertyValue(p.y)}, {"z", PropertyValue(p.z)}});
                if (!r) return false;
                if (const auto* v = std::get_if<glm::vec3>(&r.value())) { out = *v; return true; }
                return false;
            };
            // Across the seam the ridge must be absent, so the two sides agree closely.
            double worstSeam = 0.0;
            bool evaluated = true;
            for (float a = 0.f; a < 6.28f; a += 0.3f) {
                const float rh = std::sqrt(960.f * 960.f - 190.f * 190.f);
                glm::vec3 below(0.f), above(0.f);
                evaluated = colourAt(glm::vec3(rh * std::cos(a), 189.99f, rh * std::sin(a)), below) &&
                            colourAt(glm::vec3(rh * std::cos(a), 190.01f, rh * std::sin(a)), above) && evaluated;
                worstSeam = std::max(worstSeam, static_cast<double>(glm::length(below - above)));
            }
            std::cout << "       worst sky seam = " << worstSeam << std::endl;
            check(evaluated, "the sky evaluates on the host at the seam");
            check(worstSeam < 0.02, "no visible seam where the ridge pieces meet");
            // The ridge exists: the north horizon is land, and the sky above it is not.
            glm::vec3 land(0.f), air(0.f);
            colourAt(glm::vec3(0.f, 40.f, 959.f), land);
            colourAt(glm::vec3(0.f, 700.f, 656.f), air);
            std::cout << "       low-north rgb " << land.x << " " << land.y << " " << land.z << std::endl;
            check(glm::length(land) != glm::length(air), "sky colour varies with direction");

            // Stars are points. OntoMath's Div guards small denominators, and a kernel that
            // divided by radius^2 ~ 7e-7 got 0 back and lit every faint star's whole cell,
            // which covered the sky in blocks. A sample is star-lit if it stands above the mean
            // of its neighbours 3 m either side: smooth aurora cancels in that difference, and
            // a point does not. Whole lit cells make it true along every cell edge.
            int starSamples = 0, starLit = 0;
            for (float el = 0.30f; el < 1.45f; el += 0.0421f) {
                const float dAz = 3.f / (960.f * std::cos(el));
                for (float az = 0.f; az < 6.28f; az += 0.1481f) {
                    const auto onDome = [&](float azimuth) {
                        return glm::vec3(960.f * std::cos(el) * std::cos(azimuth), 960.f * std::sin(el),
                                         960.f * std::cos(el) * std::sin(azimuth));
                    };
                    glm::vec3 centre(0.f), before(0.f), after(0.f);
                    if (!colourAt(onDome(az), centre) || !colourAt(onDome(az - dAz), before) ||
                        !colourAt(onDome(az + dAz), after)) continue;
                    ++starSamples;
                    if (std::abs(centre.y - 0.5f * (before.y + after.y)) > 0.01f) ++starLit;
                }
            }
            const double starShare = starSamples ? static_cast<double>(starLit) / starSamples : 1.0;
            std::cout << "       star share of the sky = " << starShare << " of " << starSamples << std::endl;
            check(starSamples > 800, "the star survey evaluated");
            check(starShare > 0.0005, "stars are present");
            // Measured with the bug restored: 43%. Correct stars: about 3%.
            check(starShare < 0.10, "stars are points, not cells of light");
        }
    }

    // --- 9. Save Zone keeps it ----------------------------------------------------
    {
        check(mgr.persistZone(zoneIndex), "Save Zone succeeds inside the temporary SaveRoot");
        const auto saved = SaveSystem::readZoneIdentity(kZoneId);
        check(saved.value("identifier", "") == kZoneId, "the saved identity is the same Zone");
        const auto savedFields = saved.value("spatialFields", nlohmann::json::array());
        check(savedFields.size() == original["spatialFields"].size(), "every medium survives Save Zone");
        for (size_t i = 0; i < savedFields.size() && i < original["spatialFields"].size(); ++i) {
            const std::string id = savedFields[i].value("id", "");
            check(savedFields[i].contains("volumeEmission") && savedFields[i].contains("volumeDensity"),
                  id + ": channels survive Save Zone");
            auto reloaded = geom::FieldNode::fromJson(savedFields[i]);
            glm::vec3 a(0.f), b(0.f);
            auto* live = media[i];
            const glm::vec3 p(40.f, live->scale.y * (live == mirror ? 0.4f : -0.4f), static_cast<float>(peakZ(*live, 40.f, live->scale.y * (live == mirror ? 0.4f : -0.4f), 20.0)));
            const bool oka = emissionAt(*live, p, 20.0, a);
            const bool okb = reloaded && reloaded->volumeEmission && emissionAt(*reloaded, p, 20.0, b);
            check(oka && okb && glm::length(a - b) < 1e-4f, id + ": reloaded emission agrees numerically");
        }
        std::size_t savedObjects = 0;
        if (saved.contains("world") && saved["world"].contains("objects")) savedObjects = saved["world"]["objects"].size();
        check(savedObjects == original["world"]["objects"].size(), "every Object survives Save Zone");
        // Provenance lives on the beings: Save Zone does not keep a top-level injected_by
        // (rule 5 of First Mover discipline: a field that does not round-trip was never real).
        bool provenance = !savedFields.empty();
        for (const auto& f : savedFields) {
            const auto ap = f.value("authoredProperties", nlohmann::json::object());
            if (!ap.contains("injected_by") || !ap.contains("authors")) provenance = false;
        }
        for (const auto& o : saved["world"]["objects"]) {
            const auto ap = o.value("authoredProperties", nlohmann::json::object());
            if (!ap.contains("injected_by") || !ap.contains("authors")) provenance = false;
        }
        check(provenance, "every medium and Object keeps its authors and injected_by through Save Zone");
        check(saved.value("owner", "") == "Zach", "ownership survives Save Zone");
        check(saved["spatialRoot"]["authoredProperties"].contains("view.farPlane"), "the authored far plane survives Save Zone");
    }

    SaveSystem::setSaveRoot("");
    std::error_code ec;
    fs::remove_all(scratch, ec);

    std::cout << "\n" << (g_checks - g_failures) << "/" << g_checks << " checks passed\n";
    return g_failures ? 1 : 0;
}
