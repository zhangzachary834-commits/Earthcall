// Sonnet Bridge Builder — Quest 3 scratch adapter probe (CPU / codegen only).
//
// Claude Code · Claude Sonnet 5.5 (claude-sonnet-5-5) · session
// b2a4a59b-a34e-406b-9999-98f2292363f1 · 2026-10-27, commissioned by Sixth-One Sun
// (Codex / GPT-6.1 Sol). Record: docs/audits/SONNET_BRIDGE_ROUTED_MODE2_TO_SCREEN_2026-10-27.md
//
// This probe NEVER creates a WebGPU device. It links the engine only to call the
// REAL lowering APIs (readVolumeDensity -> lowerVolumeEvaluator -> composeVolumeSet,
// lowerVolumeRayChannels) and then answers, with executed checks, the questions the
// bridge depends on:
//   A. What the production Screen volume program actually exposes (bindings,
//      entry points, per-member evaluators, param layout, time slots).
//   B. Which values the executed 6el/6em Mode2 harness reads, and how they map onto
//      the production P buffer (contiguous-block search, common prefix, and the one
//      live per-field-order literal, noiseFreq = {0.08,0.09,0.07,0.10}[idx]).
//   G. The transport coefficients (alpha, colour) re-derived from the PRODUCTION
//      bindings with the same OntoMath office the harness uses
//      (factorCommonScalar / evaluateFactorCoefficients) at the Person's clock.
//   C. A ray/clip/tile ADAPTER that mirrors the production fs ray setup and opaque
//      clip (verification oracle; the proposed device pass does this on the GPU).
//   D. Publication + composition equivalence and the whole-ray Current fallback
//      contract, modelled on the CPU.
//   E. Anchor-uniqueness of the proposed structural fs patch in the real
//      composeVolumeSet text (text patch only; NOT compiled, NOT run).
//   F. Ordering: whether the scratch kernels' fixed-index composition agrees with
//      front-to-back entry order on the recorded native rays.
//
// Mathematical meaning is not decided here. Everything numeric is read from the
// lowering outputs; the only arithmetic mirrored is camera/clip geometry that the
// production fs already performs (SdfWgsl.cpp V5 fs).
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Singularity/OntoMath/CommonScalarFactor.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/VolumeRayTransport.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "json.hpp"

using J = nlohmann::json;

// ---- layout contract of the scratch routed harness (6dg/6el), kept verbatim ----
struct alignas(16) ScratchRay {            // == GpuRay in ray_transport_gpu_6dg_specialization.cpp
    glm::vec4 ro_clipA;                    // xyz eye, w = clip lower (t0)
    glm::vec4 rd_clipB;                    // xyz unit direction, w = clip upper (t1)
    glm::uvec4 counts;                     // scratch-private
};
static_assert(sizeof(ScratchRay) == 48, "scratch Ray stride must stay 48 B");

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { ++g_fail; std::cerr << "CHECK FAILED: " << msg << "\n"; } } while (0)

static std::string fnv(const std::string& s) {
    uint64_t h = 1469598103934665603ULL;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ULL; }
    std::ostringstream o; o << std::hex << std::setw(16) << std::setfill('0') << h; return o.str();
}
static size_t countOcc(const std::string& s, const std::string& needle) {
    size_t n = 0, p = 0; while ((p = s.find(needle, p)) != std::string::npos) { ++n; p += needle.size(); } return n;
}

// ---- C. production fs ray setup, mirrored for verification ---------------------
struct CameraFrame {
    glm::mat4 invViewProj; glm::vec3 eye; glm::vec2 viewport;
};
// worldAtDepth() of the V5 fs: pixel is in framebuffer coords (centre = +0.5).
static glm::vec3 worldAtDepth(const CameraFrame& c, glm::vec2 pixel, float depth) {
    glm::vec4 ndc((pixel.x / c.viewport.x) * 2.f - 1.f, (1.f - pixel.y / c.viewport.y) * 2.f - 1.f, depth, 1.f);
    glm::vec4 h = c.invViewProj * ndc; return glm::vec3(h) / h.w;
}
static glm::vec2 rayAabb(glm::vec3 ro, glm::vec3 rd, glm::vec3 bmin, glm::vec3 bmax) {
    glm::vec3 safe = rd;
    for (int i = 0; i < 3; ++i) if (std::fabs(rd[i]) < 1e-8f) safe[i] = 1e-8f;
    glm::vec3 a = (bmin - ro) / safe, b = (bmax - ro) / safe;
    glm::vec3 lo = glm::min(a, b), hi = glm::max(a, b);
    return {std::max(std::max(lo.x, lo.y), lo.z), std::min(std::min(hi.x, hi.y), hi.z)};
}
// One native pixel -> routed-harness Ray record. Returns false where the
// production fs would `discard` (no union-box chord, or opaque depth in front).
static bool nativeRay(const CameraFrame& c, glm::ivec2 px, const float* sceneDepth /*viewport-sized*/,
                      glm::vec3 unionMin, glm::vec3 unionMax, ScratchRay& out) {
    glm::vec2 fragPos(px.x + 0.5f, px.y + 0.5f);
    glm::vec3 farWorld = worldAtDepth(c, fragPos, 1.f);
    glm::vec3 rd = glm::normalize(farWorld - c.eye);
    glm::vec2 bounds = rayAabb(c.eye, rd, unionMin, unionMax);
    float t0 = std::max(bounds.x, 0.f), t1 = bounds.y;
    if (t1 <= t0) return false;
    const int maxX = (int)std::max(c.viewport.x, 1.f) - 1, maxY = (int)std::max(c.viewport.y, 1.f) - 1;
    glm::ivec2 q(std::clamp((int)std::floor(fragPos.x), 0, maxX), std::clamp((int)std::floor(fragPos.y), 0, maxY));
    float d = sceneDepth[q.y * (int)c.viewport.x + q.x];
    if (d < 0.999999f) {
        glm::vec3 w = worldAtDepth(c, fragPos, d);
        t1 = std::min(t1, std::max(glm::dot(w - c.eye, rd), 0.f));
    }
    if (t1 <= t0) return false;
    out.ro_clipA = glm::vec4(c.eye, t0); out.rd_clipB = glm::vec4(rd, t1); out.counts = glm::uvec4(0);
    return true;
}

struct TileTopology {                       // the scratch contract: 16 rays per tile, tileOffsets has N+1 entries
    std::vector<uint32_t> tileOffsets, tileRayIndices;
    uint32_t tiles = 0, tilesX = 0, tilesY = 0;
};
// 4x4 PIXEL tiles over the framebuffer, ray indices in compaction order. Scratch
// kernels assume lid>>4 == tile slot and lid&15 == ray in tile, so <=16 rays per tile.
static TileTopology buildTiles(uint32_t W, uint32_t H, const std::vector<int32_t>& rayOfPixel /*-1 = no ray*/) {
    TileTopology t; t.tilesX = (W + 3) / 4; t.tilesY = (H + 3) / 4; t.tiles = t.tilesX * t.tilesY;
    t.tileOffsets.assign(t.tiles + 1, 0);
    for (uint32_t ty = 0; ty < t.tilesY; ++ty) for (uint32_t tx = 0; tx < t.tilesX; ++tx) {
        uint32_t id = ty * t.tilesX + tx; t.tileOffsets[id] = (uint32_t)t.tileRayIndices.size();
        for (uint32_t y = ty * 4; y < std::min(H, ty * 4 + 4); ++y)
            for (uint32_t x = tx * 4; x < std::min(W, tx * 4 + 4); ++x) {
                int32_t r = rayOfPixel[y * W + x]; if (r >= 0) t.tileRayIndices.push_back((uint32_t)r);
            }
    }
    t.tileOffsets[t.tiles] = (uint32_t)t.tileRayIndices.size();
    return t;
}

// ---- D. publication / composition model ---------------------------------------
// Production fs returns premultiplied vec4(L, alpha=1-T) with blend src=One,
// dst=OneMinusSrcAlpha  =>  C_out = L + T*C_scene. A routed publication is the same
// vec4; "refused/unpublished" is encoded as alpha < 0 (impossible for a real answer).
static glm::vec3 blendOver(glm::vec4 src, glm::vec3 dst) { return glm::vec3(src) + (1.f - src.w) * dst; }
static bool fsDiscards(glm::vec4 v) {
    float m = std::max({std::fabs(v.x), std::fabs(v.y), std::fabs(v.z)});
    return v.w <= 1e-5f && m <= 1e-6f;
}

int main(int argc, char** argv) {
    const std::string zonePath = argc > 1 ? argv[1] : "saves/zones/Northern Veil/zone.ecform";
    const std::string raysPath = argc > 2 ? argv[2] : "";
    const std::string outPath = argc > 3 ? argv[3] : "sonnet_bridge_probe.json";
    const std::string patchedWgslPath = argc > 4 ? argv[4] : "";
    J report;

    // ---------------- A. real lowering ----------------
    std::ifstream in(zonePath, std::ios::binary);
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), {});
    J zone = J::parse(J::from_msgpack(bytes).at("MigrationRoot").get<std::string>());

    std::vector<std::shared_ptr<geom::FieldNode>> fields;
    std::vector<Rendering::VolumeDensityBinding> media;
    for (const auto& j : zone.at("spatialFields")) {
        auto f = geom::FieldNode::fromJson(j);
        Rendering::VolumeDensityBinding b;
        if (!f || !Rendering::readVolumeDensity(*f, /*clock*/ 7.25, 0.0, b) || !b.densityExpr) continue;
        fields.push_back(std::move(f)); media.push_back(b);
    }
    CHECK(!media.empty(), "no volume media found in zone");
    report["media_count"] = media.size();

    std::vector<sdfwgsl::VolumeProgramInput> inputs;
    std::vector<sdfwgsl::KernelVolumeEvaluator> evaluators;
    for (const auto& m : media) {
        sdfwgsl::VolumeProgramInput in_;
        in_.densityExpr = m.densityExpr; in_.extinctionExpr = m.extinctionExpr; in_.scatteringExpr = m.scatteringExpr;
        in_.volumeChromaExpr = m.volumeChromaExpr; in_.phaseExpr = m.phaseExpr; in_.emissionExpr = m.emissionExpr;
        in_.occluderSdf = m.occluderSdf;
        inputs.push_back(in_);
        evaluators.push_back(sdfwgsl::lowerVolumeEvaluator(in_));
    }
    for (size_t i = 0; i < evaluators.size(); ++i)
        CHECK(evaluators[i].program.ok, "evaluator " << i << " refused: " << evaluators[i].program.error);
    sdfwgsl::Program prog = sdfwgsl::composeVolumeSet(inputs, evaluators, /*trimProofSupport*/ true);
    CHECK(prog.ok, "composeVolumeSet refused: " << prog.error);
    const std::string& W = prog.wgsl;
    J& A = report["A_production_program"];
    A["ok"] = prog.ok; A["wgsl_bytes"] = W.size(); A["wgsl_fnv64"] = fnv(W);
    A["param_floats"] = prog.params.size();
    A["kernel_time_slots"] = prog.kernelVolumeTimeSlots.size();
    A["time_prepare_instance_base"] = prog.kernelVolumePreparationInstanceBase;
    {   J slots = J::array();
        for (auto& s : prog.kernelVolumeTimeSlots) slots.push_back({{"slot", s.slot}, {"expr", s.expression}});
        A["time_slots"] = slots; }
    {   std::regex re(R"(@group\((\d)\) @binding\((\d)\) var(<[a-z, _]+>)? (\w+))"); J b = J::array();
        for (std::sregex_iterator it(W.begin(), W.end(), re), e; it != e; ++it)
            b.push_back({{"group", std::stoi((*it)[1])}, {"binding", std::stoi((*it)[2])}, {"name", (*it)[4].str()}});
        A["bindings"] = b; }
    A["has_vertex_fs_entry"] = (W.find("@vertex") != std::string::npos) && (W.find("@fragment\nfn fs(") != std::string::npos);
    J evals = J::array();
    for (size_t i = 0; i < media.size(); ++i) {
        const std::string n = std::to_string(i);
        evals.push_back({{"member", i},
            {"density", W.find("fn volumeDensityEval_" + n + "(") != std::string::npos},
            {"extinction", W.find("fn volumeExtinctionEval_" + n + "(") != std::string::npos},
            {"emission", W.find("fn volumeEmissionEval_" + n + "(") != std::string::npos},
            {"producer", media[i].producerId}, {"origin", {media[i].origin.x, media[i].origin.y, media[i].origin.z}},
            {"half", {std::fabs(media[i].scale.x), std::fabs(media[i].scale.y), std::fabs(media[i].scale.z)}},
            {"density_revision", media[i].densityRevision}});
    }
    A["member_evaluators"] = evals;

    // OntoMath-owned per-ray derivations that a generic routed classifier would consume
    {   Rendering::VolumeRayFormCache cache; J forms = J::array();
        for (size_t i = 0; i < media.size(); ++i) {
            const auto& f = cache.formsFor(media[i]);
            forms.push_back({{"member", i}, {"density_present", f.density.present},
                {"density_polynomial_admitted", f.density.admitted()}, {"density_algebraic_admitted", f.density.algebraicAdmitted()},
                {"density_refusal", f.density.refusal}, {"emission_admitted", f.emission.admitted()}});
        }
        A["ray_channel_forms"] = forms; A["form_cache_lowerings"] = cache.stats().channelLowerings; }

    // ---------------- B. what the scratch kernels read vs. what P holds ----------------
    // The scratch harness reads collectVolumeParams(density).values (34 floats) POSITIONALLY:
    //   v[0] gain, v[2] Wx, v[5] ky-slope, v[6] Wy, v[15] wz, v[16..24] 3x(amp,freq,phase).
    // and bakes, in WGSL, per-curtain noise freq / alpha / colour (m==0..3 literals).
    const std::set<int> scratchSlots = {0, 2, 5, 6, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};
    J& B = report["B_scratch_vs_production_params"]; B["members"] = J::array();
    size_t cursor = 0;
    for (size_t i = 0; i < media.size(); ++i) {
        auto density = sdfwgsl::collectVolumeParams(media[i].densityExpr);
        auto full = sdfwgsl::collectVolumeParams(media[i].densityExpr, media[i].extinctionExpr, media[i].scatteringExpr,
                                                 media[i].volumeChromaExpr, media[i].phaseExpr, media[i].emissionExpr, media[i].occluderSdf);
        J m; m["member"] = i; m["density_block_floats"] = density.values.size(); m["all_channel_floats"] = full.values.size();
        // contiguous-block search of the scratch 34-float density block inside the production set params
        long found = -1;
        if (!density.values.empty() && prog.params.size() >= density.values.size())
            for (size_t o = 0; o + density.values.size() <= prog.params.size(); ++o)
                if (std::equal(density.values.begin(), density.values.end(), prog.params.begin() + o)) { found = (long)o; break; }
        m["density_block_offset_in_set_params"] = found;
        { size_t k = 0; while (k < density.values.size() && k < full.values.size() && density.values[k] == full.values[k]) ++k;
          m["density_vs_all_channel_common_prefix"] = k; }
        { J fv = J::array(); for (float v : full.values) fv.push_back(v); m["all_channel_values"] = fv; }
        { const std::string n = std::to_string(i);
          J clk = J::object();
          for (const char* ch : {"Density", "Extinction", "Scattering", "Chroma", "Phase", "Emission"}) {
              const std::string key = std::string("fn volume") + ch + "Eval_" + n + "(";
              size_t a = W.find(key); if (a == std::string::npos) { clk[ch] = "absent"; continue; }
              size_t b = W.find("\nfn ", a + 1); std::string body = W.substr(a, b == std::string::npos ? std::string::npos : b - a);
              clk[ch] = (body.find("volumeTime") != std::string::npos) ? "reads u.volumeTime" : "no clock read";
          }
          m["evaluator_clock_reads"] = clk; }
        // does the production member param segment start where a running cursor says?
        m["running_cursor"] = cursor; cursor += full.values.size();
        // The only live baked coefficient in the executed harness main: noiseFreq = freqs[idx] (field order).
        { static const float freqs[4] = {0.08f, 0.09f, 0.07f, 0.10f};
          m["harness_noise_freq_literal_by_field_order"] = i < 4 ? freqs[i] : -1.0;
          m["authored_slot35_all_channel"] = full.values.size() > 35 ? full.values[35] : -1.0;
          m["literal_matches_authored_slot35"] = i < 4 && full.values.size() > 35 && std::fabs(full.values[35] - freqs[i]) < 1e-7; }
        J dv = J::array(); for (size_t s = 0; s < density.values.size(); ++s) dv.push_back(density.values[s]);
        m["density_values"] = dv;
        // is any scratch-read slot a kernel time slot?
        J timed = J::array();
        for (auto& ts : prog.kernelVolumeTimeSlots) timed.push_back(ts.slot);
        m["kernel_time_slot_indices_set_wide"] = timed;
        B["members"].push_back(m);
    }
    B["scratch_read_slots"] = std::vector<int>(scratchSlots.begin(), scratchSlots.end());
    B["scratch_params_clock_used"] = 0.0;   // 6dg: readVolumeDensity(*field, 0, 0, bnd) — see audit
    B["rays_clock"] = 7.25;

    // ---------------- C. adapter self-checks (synthetic frame) ----------------
    {   const uint32_t Wd = 256, Hd = 144;
        glm::vec3 eye(0.f, 50.f, 75.f);
        glm::mat4 view = glm::lookAt(eye, glm::vec3(40.f, 62.f, 110.f), glm::vec3(0, 1, 0));
        glm::mat4 proj = glm::perspective(glm::radians(60.f), float(Wd) / Hd, 0.1f, 1000.f);
        CameraFrame cam{glm::inverse(proj * view), eye, glm::vec2(Wd, Hd)};
        glm::vec3 uMin(1e9f), uMax(-1e9f);
        for (auto& m : media) { uMin = glm::min(uMin, m.origin - glm::abs(m.scale)); uMax = glm::max(uMax, m.origin + glm::abs(m.scale)); }
        // opaque plane z = 120 (sceneDepth from the same projection): depth(px) of the plane
        std::vector<float> depth(Wd * Hd, 1.f);
        for (uint32_t y = 0; y < Hd; ++y) for (uint32_t x = 0; x < Wd; ++x) {
            glm::vec3 rd = glm::normalize(worldAtDepth(cam, {x + .5f, y + .5f}, 1.f) - eye);
            if (std::fabs(rd.z) < 1e-6f) continue;
            float t = (120.f - eye.z) / rd.z; if (t <= 0) continue;
            glm::vec4 clip = proj * view * glm::vec4(eye + rd * t, 1.f); depth[y * Wd + x] = clip.z / clip.w;
        }
        std::vector<ScratchRay> rays; std::vector<int32_t> rayOfPixel(Wd * Hd, -1);
        size_t discarded = 0, clipped = 0; double maxUnit = 0, maxPlaneErr = 0;
        for (uint32_t y = 0; y < Hd; ++y) for (uint32_t x = 0; x < Wd; ++x) {
            ScratchRay r;
            if (!nativeRay(cam, {x, y}, depth.data(), uMin, uMax, r)) { ++discarded; continue; }
            rayOfPixel[y * Wd + x] = (int32_t)rays.size(); rays.push_back(r);
            maxUnit = std::max(maxUnit, (double)std::fabs(glm::length(glm::vec3(r.rd_clipB)) - 1.f));
            glm::vec2 b = rayAabb(eye, glm::vec3(r.rd_clipB), uMin, uMax);
            if (r.rd_clipB.w < b.y - 1e-3f) {   // opaque clip active: must equal plane distance along the ray
                ++clipped; float tp = (120.f - eye.z) / r.rd_clipB.z; maxPlaneErr = std::max(maxPlaneErr, (double)std::fabs(r.rd_clipB.w - tp) / std::max(tp, 1.f));
            }
            CHECK(r.ro_clipA.w >= 0.f && r.rd_clipB.w > r.ro_clipA.w, "clip order violated");
        }
        TileTopology topo = buildTiles(Wd, Hd, rayOfPixel);
        size_t maxPerTile = 0, covered = topo.tileRayIndices.size(); std::set<uint32_t> seen;
        for (uint32_t t = 0; t < topo.tiles; ++t) {
            maxPerTile = std::max<size_t>(maxPerTile, topo.tileOffsets[t + 1] - topo.tileOffsets[t]);
            for (uint32_t k = topo.tileOffsets[t]; k < topo.tileOffsets[t + 1]; ++k) CHECK(seen.insert(topo.tileRayIndices[k]).second, "ray in two tiles");
        }
        CHECK(covered == rays.size() && seen.size() == rays.size(), "tile partition must cover every ray exactly once");
        CHECK(maxPerTile <= 16, "tile exceeds 16 rays (scratch kernels assume lid&15)");
        CHECK(maxUnit < 1e-5, "direction not unit");
        // f32 NDC depth at near=0.1/far=1000 quantizes distance; production fs reads the same texture, so the bound is relative.
        CHECK(maxPlaneErr < 1e-3, "opaque clip != analytic plane distance (relative): " << maxPlaneErr);
        J& C = report["C_ray_clip_tile_adapter"];
        C["frame"] = {Wd, Hd}; C["rays"] = rays.size(); C["discarded_by_fs_rule"] = discarded; C["opaque_clipped"] = clipped;
        C["max_unit_error"] = maxUnit; C["max_opaque_clip_relative_error_vs_plane"] = maxPlaneErr;
        C["tiles"] = topo.tiles; C["max_rays_per_tile"] = maxPerTile;
        // scratch compile-time capacity constants vs a native 2560x1440 frame
        const uint32_t nativeTiles = ((2560 + 3) / 4) * ((1440 + 3) / 4);
        C["native_2560x1440_tiles_needed"] = nativeTiles; C["scratch_hardcoded_tiles"] = 2304;
        C["native_2560x1440_rays_max"] = 2560u * 1440u; C["scratch_hardcoded_rays"] = 32784;
        C["scratch_ray_bytes_native_frame"] = (uint64_t)2560 * 1440 * sizeof(ScratchRay);
    }

    // ---------------- D. publication / composition model ----------------
    {   J& D = report["D_publication_and_composition"];
        glm::vec3 scene(0.3f, 0.5f, 0.7f); double worst = 0; size_t n = 0;
        for (float T : {1.0f, 0.9f, 0.5f, 0.01f, 0.0f}) for (float Lr : {0.f, 0.2f, 1.5f}) {
            glm::vec4 v(Lr, Lr * .5f, Lr * .25f, 1.f - T);
            glm::vec3 viaBlend = blendOver(v, scene), direct = glm::vec3(v) + T * scene;
            worst = std::max(worst, (double)glm::length(viaBlend - direct)); ++n;
        }
        CHECK(worst < 1e-6, "premultiplied blend identity broken");
        D["blend_identity_cases"] = n; D["blend_identity_max_err"] = worst;
        // discard threshold: a published near-zero answer that fs would discard differs from writing it by <= 1e-5
        glm::vec4 tiny(5e-7f, 0, 0, 9e-6f); CHECK(fsDiscards(tiny), "tiny must be fs-discarded");
        D["fs_discard_rule"] = "alpha<=1e-5 && max|L|<=1e-6"; D["discard_vs_blend_max_delta"] = glm::length(blendOver(tiny, scene) - scene);
        D["refusal_sentinel"] = "alpha < 0 (a real answer has alpha = 1 - T in [0, 1])";
        // ORDER TRAP: a refused sentinel (0,0,0,-1) satisfies the fs discard rule (alpha<=1e-5, |L|<=1e-6). The refusal
        // test (w<0 -> run Current) MUST precede the discard test, as in the proposed patch, or refused pixels vanish silently.
        glm::vec4 refused(0, 0, 0, -1.f);
        CHECK(refused.w < 0.f && fsDiscards(refused), "documenting: sentinel collides with discard rule if tested in the wrong order");
        D["sentinel_collides_with_discard_rule_if_discard_tested_first"] = fsDiscards(refused);
    }

    // ---------------- E. proposed structural fs patch: anchor uniqueness ----------------
    {   J& E = report["E_fs_patch_anchors"];
        const std::vector<std::pair<std::string, std::string>> anchors = {
            {"bindings_P", "@group(0) @binding(1) var<storage, read> P: Params;"},
            {"bindings_sceneDepth", "@group(0) @binding(2) var sceneDepthTex: texture_depth_2d;"},
            {"opaque_clip", "t1 = min(t1, max(opaqueT, 0.0));\n    }\n    if (t1 <= t0) { discard; }\n"},
            {"transport_state_init", "    var transmittance = 1.0;\n    var integratedRadiance = vec3<f32>(0.0);\n"},
            {"final_return", "    return vec4<f32>(integratedRadiance, alpha);\n}"},
            {"discard_rule", "if (alpha <= 1e-5 && integratedMagnitude <= 1e-6) { discard; }"}};
        bool all = true; for (auto& [k, a] : anchors) { size_t c = countOcc(W, a); E[k] = c; all = all && c == 1; }
        E["all_anchors_unique"] = all; CHECK(all, "fs patch anchors are not all unique in composeVolumeSet output");
        if (all) {
            std::string P = W;
            auto rep = [&](const std::string& a, const std::string& b) { size_t p = P.find(a); P.replace(p, a.size(), b); };
            rep("@group(0) @binding(2) var sceneDepthTex: texture_depth_2d;",
                "@group(0) @binding(2) var sceneDepthTex: texture_depth_2d;\n"
                "// PROPOSED (bridge lane): routed publication, one vec4 per framebuffer pixel, written by the routed\n"
                "// compute passes. rgb = L, w = alpha = 1 - T. w < 0 means refused/unpublished -> run Current below.\n"
                "@group(0) @binding(3) var<storage, read> routedPublication: array<vec4<f32>>;");
            rep("    var transmittance = 1.0;\n    var integratedRadiance = vec3<f32>(0.0);\n",
                "    {\n"
                "        let routed = routedPublication[u32(px.y) * u32(u.viewport.x) + u32(px.x)];\n"
                "        if (routed.w >= 0.0) {\n"
                "            if (routed.w <= 1e-5 && max(max(abs(routed.x), abs(routed.y)), abs(routed.z)) <= 1e-6) { discard; }\n"
                "            return routed;   // same premultiplied answer, same blend state\n"
                "        }\n"
                "    }\n"
                "    var transmittance = 1.0;\n    var integratedRadiance = vec3<f32>(0.0);\n");
            E["patched_wgsl_bytes"] = P.size(); E["patched_wgsl_fnv64"] = fnv(P);
            if (!patchedWgslPath.empty()) { std::ofstream(patchedWgslPath) << P; E["patched_wgsl_path"] = patchedWgslPath; }
        }
    }

    // ---------------- G. coefficients from production bindings via the OntoMath office ----------------
    {   J& G = report["G_transport_coefficients_from_production_bindings"]; G["clock"] = 7.25; G["members"] = J::array();
        auto single = [](const OntoMath::Piecewise* p) -> const OntoMath::MathNode* {
            if (!p || p->pieces.size() != 1) return nullptr;
            const auto& x = p->pieces[0];
            return !x.guard && !x.call && !x.fold && !x.hasLo && !x.hasHi && !x.whereLEZero ? x.mathNode.get() : nullptr; };
        static const float refAlpha[4] = {2.0f / 15.0f, 9.0f / 65.0f, 8.0f / 55.0f, 4.0f / 25.0f};   // dead table in harness main
        for (size_t i = 0; i < media.size(); ++i) {
            J m; m["member"] = i;
            const auto* d = single(media[i].densityExpr); const auto* sg = single(media[i].extinctionExpr); const auto* em = single(media[i].emissionExpr);
            if (!d || !sg || !em) { m["status"] = "refused: channel is not a single unguarded MathNode piece -> Current"; G["members"].push_back(m); continue; }
            OntoMath::TypeEnv env = {{"p", OntoMath::ValueKind::Vector}, {"x", OntoMath::ValueKind::Scalar}, {"y", OntoMath::ValueKind::Scalar},
                                     {"z", OntoMath::ValueKind::Scalar}, {OntoMath::kTimeVar, OntoMath::ValueKind::Scalar}};
            std::string why; auto recipe = OntoMath::factorCommonScalar({d, sg, em}, env, {OntoMath::kTimeVar}, &why);
            if (!recipe || recipe->factorChild != std::vector<unsigned>({1, 1, 0})) { m["status"] = "refused: " + why; G["members"].push_back(m); continue; }
            auto vals = OntoMath::evaluateFactorCoefficients(*recipe, {{OntoMath::kTimeVar, PropertyValue(7.25)}}, &why);
            if (!vals || vals->size() != 3 || !std::holds_alternative<glm::vec3>((*vals)[2])) { m["status"] = "coefficient preparation refused: " + why; G["members"].push_back(m); continue; }
            auto lit = [](const OntoMath::MathNode& n) { return (n.op == OntoMath::MathNode::Op::ScalarLeaf && n.scalarForm.terms.size() == 1 && n.scalarForm.terms[0].factors.empty())
                                                             ? n.scalarForm.terms[0].coefficient : -1.0; };
            double cD = lit(recipe->coefficients[0]), cX = lit(recipe->coefficients[1]); glm::vec3 v = std::get<glm::vec3>((*vals)[2]);
            m["status"] = (cD > 0 && cX > 0) ? "derived" : "refused: non-literal scalar coefficient";
            if (cD > 0 && cX > 0) {
                m["alpha"] = cX / cD; m["sigma_coefficient"] = cX; m["colour"] = {v.x / cX, v.y / cX, v.z / cX};
                if (i < 4) { m["alpha_vs_harness_dead_table"] = std::fabs(cX / cD - refAlpha[i]);
                             CHECK(std::fabs(cX / cD - refAlpha[i]) < 1e-6, "derived alpha disagrees with harness table for member " << i); }
            }
            G["members"].push_back(m);
        }
    }

    // ---------------- F. box-chord overlap on the recorded native rays ----------------
    if (!raysPath.empty()) {
        std::ifstream rin(raysPath); J rj; rin >> rj;
        const auto& R = rj.at("rays"); glm::vec3 ro(rj["origin"][0].get<float>(), rj["origin"][1].get<float>(), rj["origin"][2].get<float>());
        const size_t M = media.size(); size_t multi = 0, misordered = 0, overlapRays = 0;
        std::vector<std::array<double, 2>> chords(M);
        for (const auto& r : R) {
            glm::vec3 d(r[2].get<float>(), r[3].get<float>(), r[4].get<float>());
            float a = r[5].get<float>(), b = r[6].get<float>();
            std::vector<std::pair<float, size_t>> hits; std::vector<glm::vec2> iv(M, glm::vec2(0));
            for (size_t m = 0; m < M; ++m) {
                glm::vec2 e = rayAabb(ro, d, media[m].origin - glm::abs(media[m].scale), media[m].origin + glm::abs(media[m].scale));
                float lo = std::max(e.x, a), hi = std::min(e.y, b);
                if (hi > lo) { hits.push_back({lo, m}); iv[m] = {lo, hi}; }
            }
            if (hits.size() < 2) continue;
            ++multi;
            for (size_t i = 0; i + 1 < hits.size(); ++i) if (hits[i].second > hits[i + 1].second) { ++misordered; break; }
            bool ov = false;
            for (size_t i = 0; i < M && !ov; ++i) for (size_t j = i + 1; j < M; ++j)
                if (iv[i].y > iv[i].x && iv[j].y > iv[j].x && std::min(iv[i].y, iv[j].y) > std::max(iv[i].x, iv[j].x)) { ov = true; break; }
            if (ov) ++overlapRays;
        }
        J& F = report["F_order_and_overlap_on_recorded_rays"];
        F["rays"] = R.size(); F["rays_crossing_2plus_media"] = multi;
        F["rays_where_box_entry_order_differs_from_index_order"] = misordered; F["rays_with_overlapping_media_chords"] = overlapRays;
        F["note"] = "box chords overlap on every recorded ray, so the S1/S2 admission burden rests on the classifier's SUPPORT-interval proof (6el classifier: mask 1, or mask 3 with !tileCanOverlap && exit[0] <= enter[1]), not on box geometry";
    }

    report["failures"] = g_fail;
    std::ofstream(outPath) << report.dump(2) << "\n";
    std::cerr << (g_fail ? "FAIL" : "OK") << " — wrote " << outPath << "\n";
    return g_fail ? 1 : 0;
}
