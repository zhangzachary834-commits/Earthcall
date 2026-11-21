"""§6de C++20 IEEE-754 f32 Verifier & Census for Original Prism Sun's Ancient-Counsel Candidates on `Earthcall-ray-transport`.
Verifies against the pinned `saves/zones/Northern Veil/zone.ecform` (`sha256 = 6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`)
and `/private/tmp/earthcall-wide-case/manifest.json` (`32,784` distinct native rays):

1. Candidate 1 (`cnoise3-pair` / `cnoise3-pair-wide`):
   - Plane-staged (`z=0` then `z=1`) `cnoise3Pair(Pa, Pb)` inside `rayMain`, gated strictly on `env_m > 0.0 && env_p > 0.0`.
   - Verifies 100.0% bit-exact float parity (`0 / 247,326` subpanel diffs, `0 / 32,784` pixel diffs) against the unfused `cnoise3` consumer across all 32,784 rays.
   - Counts exact subpanel envelope gating (`both_positive`, `single_positive`, `both_zero`), same-cell vs cross-cell pairs, and eliminated lattice permutation/gradient setups across all 247,326 produced subpanels.

2. Candidate 2 (`tile-beam-wide` / `tile-beam`):
   - Cooperative 16-Ray Sub-Beam Lipschitz Support Certificate (Ganacim 2011 / Knoll 2009 / Guenter 1995) inside `producerFastMain`.
   - Lanes `0..8` of each 16-ray sub-beam evaluate the 9 coarse knots of the beam's reference ray in parallel in 1 wave (`workgroup` memory `296 B`), then all 16 lanes apply the exact spatial Lipschitz + wave amplitude enclosure (`|dPhi/dx| <= L1`, `|dWave| <= 2*Wmax`, `|dPhi/dz| = 1`) in pure FMA arithmetic to certify whole-member and per-bin level exclusion (`|Phi_m| > w_z + bow`) before per-ray `phi_knots` evaluation.
   - Verifies 100.0% bit-exact schedule parity (`0 / 32,784` differing rays, `247,326` singles, `1,799` overlaps), synthetic budget boundary controls (`23/24/25 -> 16/0/0`), poisoned capacity controls (`maxSub=4 -> 26,243`, `maxOver=1 -> 412`), and measures total `evalPhi` reductions across beam sizes `4, 8, 16, 32, 64`.
"""
import argparse, hashlib, json, shlex, subprocess
from pathlib import Path

CPP_6DE_VERIFY = r'''
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "json.hpp"

using J = nlohmann::json;

struct alignas(16) GpuCurtainSpec {
    glm::vec4 origin;
    glm::vec4 half;
    glm::vec4 limits;
    glm::vec4 curv;
    glm::vec4 amps;
    glm::vec4 freqs;
    glm::vec4 phases;
};

static bool extractCurtainSpecs(const J& zone, std::vector<GpuCurtainSpec>& outSpecs,
                                std::vector<float>& outParamPool,
                                std::vector<uint32_t>& outParamOffsets) {
    outSpecs.clear();
    outParamPool.clear();
    outParamOffsets.clear();
    for (const auto& j : zone.at("spatialFields")) {
        auto field = geom::FieldNode::fromJson(j);
        Rendering::VolumeDensityBinding bnd;
        if (!field || !Rendering::readVolumeDensity(*field, 0, 0, bnd)) continue;
        if (!bnd.densityExpr) continue;
        auto pb = sdfwgsl::collectVolumeParams(bnd.densityExpr);
        if (pb.values.size() < 34) return false;
        outParamOffsets.push_back(uint32_t(outParamPool.size()));
        outParamPool.insert(outParamPool.end(), pb.values.begin(), pb.values.end());
        const auto& v = pb.values;
        GpuCurtainSpec spec{};
        spec.origin = glm::vec4(bnd.origin, 0.0f);
        spec.half = glm::vec4(glm::abs(bnd.scale), 0.0f);
        float Wx = v[2], Wy = v[6];
        float yKnee = -Wy + (v[5] > 0.0f ? 1.0f / v[5] : 4.0f);
        float wz = v[15];
        spec.limits = glm::vec4(Wx, Wy, yKnee, wz);
        float A1 = v[16], w1 = v[17], p1 = v[18];
        float A2 = v[19], w2 = v[20], p2 = v[21];
        float A3 = v[22], w3 = v[23], p3 = v[24];
        spec.amps = glm::vec4(A1, A2, A3, 0.0f);
        spec.freqs = glm::vec4(w1, w2, w3, 0.0f);
        spec.phases = glm::vec4(p1, p2, p3, 0.0f);
        float M2 = std::abs(A1) * w1 * w1 + std::abs(A2) * w2 * w2 + std::abs(A3) * w3 * w3;
        spec.curv = glm::vec4(std::nextafter(M2, INFINITY) * 1.00002f, 0.001f, 0.0f, 0.0f);
        outSpecs.push_back(spec);
    }
    return outSpecs.size() == 4;
}

struct SubpanelPkt { float s0, s1; uint32_t member; };
struct OverlapPkt { float s0, s1; uint32_t mask; };
struct RaySchedule {
    uint32_t flag = 0;
    std::vector<SubpanelPkt> singles;
    std::vector<OverlapPkt> overs;
};

static inline float evalWave(const GpuCurtainSpec& m, float x) {
    return m.amps.x * std::sin(m.freqs.x * x + m.phases.x)
         + m.amps.y * std::cos(m.freqs.y * x + m.phases.y)
         + m.amps.z * std::sin(m.freqs.z * x + m.phases.z);
}

static inline float evalPhi(const GpuCurtainSpec& m, glm::vec3 o, glm::vec3 d, float s) {
    return o.z + s * d.z + evalWave(m, o.x + s * d.x);
}

static inline bool mayAbsorb(glm::vec2 previous, glm::vec2 current, bool previousShortBand,
                             bool previousPositive, bool currentUncertain, bool protectedBoundary) {
    if (protectedBoundary || previous.y != current.x) return false;
    bool currentShortBand = currentUncertain && (current.y - current.x <= 0.02f);
    return (previousShortBand && !currentUncertain) || (previousPositive && currentShortBand);
}

static bool schedulesBitEqual(const RaySchedule& a, const RaySchedule& b) {
    if (a.flag != b.flag || a.singles.size() != b.singles.size() || a.overs.size() != b.overs.size()) return false;
    for (size_t i = 0; i < a.singles.size(); ++i) {
        if (a.singles[i].member != b.singles[i].member ||
            std::bit_cast<uint32_t>(a.singles[i].s0) != std::bit_cast<uint32_t>(b.singles[i].s0) ||
            std::bit_cast<uint32_t>(a.singles[i].s1) != std::bit_cast<uint32_t>(b.singles[i].s1)) return false;
    }
    for (size_t i = 0; i < a.overs.size(); ++i) {
        if (a.overs[i].mask != b.overs[i].mask ||
            std::bit_cast<uint32_t>(a.overs[i].s0) != std::bit_cast<uint32_t>(b.overs[i].s0) ||
            std::bit_cast<uint32_t>(a.overs[i].s1) != std::bit_cast<uint32_t>(b.overs[i].s1)) return false;
    }
    return true;
}

struct BeamCounters {
    uint64_t boxHits = 0;
    uint64_t gtWholeMemberSkips = 0;
    uint64_t beamCertifiedWholeMemberSkips = 0;
    uint64_t fallbackPerRayWholeMemberSkips = 0;
    uint64_t beamRefKnotEvals = 0;
    uint64_t beamParallelKnotWaves = 0;
    uint64_t perRayKnotEvals = 0;
    uint64_t baselineKnotEvals = 0;
    uint64_t evalPhiBisect = 0;
    uint64_t evalPhiSplit = 0;
    uint64_t evalPhiPieceMid = 0;
};

static std::vector<RaySchedule> runWorkgroupBeamProducer(
    const std::vector<GpuCurtainSpec>& curtains,
    glm::vec3 ro,
    const std::vector<glm::vec3>& dirs,
    const std::vector<glm::vec2>& clips,
    uint32_t budget, uint32_t maxSubPerRay, uint32_t maxOverPerRay,
    uint32_t beamSize,
    BeamCounters* ctr = nullptr)
{
    size_t count = dirs.size();
    std::vector<RaySchedule> schedules(count);
    if (budget == 0u) {
        for (size_t i = 0; i < count; ++i) schedules[i].flag = 0x80000000u | 1u;
        return schedules;
    }

    auto putBand = [](float lo, float hi, std::array<float, 96>& bounds, uint32_t& nb,
                      std::array<glm::vec2, 32>& bands, uint32_t& nr, uint32_t& rsn) {
        if (nr > 0u) {
            glm::vec2 last = bands[nr - 1u];
            if (lo <= last.y && hi >= last.x) {
                glm::vec2 merged(std::min(lo, last.x), std::max(hi, last.y));
                bands[nr - 1u] = merged;
                bounds[nb - 2u] = merged.x; bounds[nb - 1u] = merged.y;
                return;
            }
        }
        if (nb + 2u > 92u || nr >= 32u) { rsn |= 2u; return; }
        bounds[nb] = lo; bounds[nb + 1u] = hi; nb += 2u;
        bands[nr] = glm::vec2(lo, hi); nr += 1u;
    };
    auto putBandTagged = [&](float lo, float hi, std::array<float, 96>& bounds, uint32_t& nb,
                             std::array<glm::vec2, 32>& bands, uint32_t& nr, uint32_t& rsn,
                             uint32_t& zeroBands, bool isZeroLevel) {
        putBand(lo, hi, bounds, nb, bands, nr, rsn);
        if (rsn == 0u && isZeroLevel && nr > 0u) zeroBands |= (1u << (nr - 1u));
    };

    for (size_t wgBase = 0; wgBase < count; wgBase += 64) {
        size_t wgEnd = std::min(count, wgBase + 64);
        struct LaneState {
            uint32_t reason = 0u, work = 0u, count = 0u;
            std::array<glm::vec2, 48> pieces{};
            std::array<uint32_t, 4> memStart{0, 0, 0, 0}, memCount{0, 0, 0, 0};
        };
        std::array<LaneState, 64> lanes{};

        for (uint32_t member = 0u; member < 4u; ++member) {
            const auto& m = curtains[member];
            glm::vec3 o = ro - glm::vec3(m.origin);
            float L1 = (std::abs(m.amps.x * m.freqs.x) + std::abs(m.amps.y * m.freqs.y) + std::abs(m.amps.z * m.freqs.z)) * 1.00002f;
            float twoWmax = 2.0f * (std::abs(m.amps.x) + std::abs(m.amps.y) + std::abs(m.amps.z)) * 1.00002f;

            std::array<bool, 64> hitArr{};
            std::array<float, 64> aArr{}, bArr{};
            for (size_t lid = 0; lid < (wgEnd - wgBase); ++lid) {
                if (lanes[lid].reason != 0u) continue;
                lanes[lid].memStart[member] = lanes[lid].count;
                size_t id = wgBase + lid;
                glm::vec3 d = dirs[id];
                float a = clips[id].x, b = clips[id].y;
                bool hit = true;
                for (int axis = 0; axis < 3; ++axis) {
                    float half = m.half[axis];
                    if (axis == 0) half = std::min(half, m.limits.x);
                    if (axis == 1) half = std::min(half, m.limits.y);
                    if (d[axis] == 0.0f) {
                        if (std::abs(o[axis]) > half + m.curv.y) { hit = false; break; }
                    } else {
                        float q0 = (-half - m.curv.y - o[axis]) / d[axis];
                        float q1 = (half + m.curv.y - o[axis]) / d[axis];
                        a = std::max(a, std::min(q0, q1));
                        b = std::min(b, std::max(q0, q1));
                    }
                }
                if (hit && b > a) {
                    hitArr[lid] = true;
                    aArr[lid] = a;
                    bArr[lid] = b;
                    if (ctr) {
                        ctr->boxHits++;
                        ctr->baselineKnotEvals += 9;
                    }
                }
            }

            if (beamSize == 1u) {
                for (size_t lid = 0; lid < (wgEnd - wgBase); ++lid) {
                    if (!hitArr[lid] || lanes[lid].reason != 0u) continue;
                    auto& st = lanes[lid];
                    size_t id = wgBase + lid;
                    glm::vec3 d = dirs[id];
                    float a = aArr[lid], b = bArr[lid];
                    float M = m.curv.x * d.x * d.x * 1.00002f;
                    float err = m.curv.y;
                    std::array<float, 9> phi_knots{};
                    for (uint32_t k = 0; k < 9; ++k) {
                        float s = (k == 0) ? a : ((k == 8) ? b : (a + (b - a) * float(k) / 8.0f));
                        phi_knots[k] = evalPhi(m, o, d, s);
                        if (ctr) ctr->perRayKnotEvals++;
                    }
                    bool allPositiveH = true;
                    auto exactLevelActiveMask = [&](float c) -> uint32_t {
                        uint32_t mask = 0u;
                        for (uint32_t bin = 0; bin < 8; ++bin) {
                            float a0 = (bin == 0) ? a : (a + (b - a) * float(bin) / 8.0f);
                            float b0 = (bin == 7) ? b : (a + (b - a) * float(bin + 1) / 8.0f);
                            float h = b0 - a0;
                            if (!(h > 0.0f)) allPositiveH = false;
                            float va = phi_knots[bin] - c, vb = phi_knots[bin + 1] - c;
                            float scale = 1.0f + std::abs(va) + std::abs(vb) + std::abs(a0) + std::abs(b0);
                            float error = err + 0.000001f * scale;
                            float bow = 0.125f * M * h * h + error;
                            if (!(std::min(va, vb) > bow || std::max(va, vb) < -bow)) mask |= (1u << bin);
                        }
                        return mask;
                    };
                    uint32_t mask0 = exactLevelActiveMask(-m.limits.w);
                    uint32_t mask1 = exactLevelActiveMask(0.0f);
                    uint32_t mask2 = exactLevelActiveMask(m.limits.w);
                    if (allPositiveH && (mask0 | mask1 | mask2) == 0u && std::abs(phi_knots[0]) > m.limits.w + err) {
                        if (st.work + 24u > budget) { st.reason |= 1u; continue; }
                        st.work += 24u;
                        st.memCount[member] = 0u;
                        if (ctr) ctr->gtWholeMemberSkips++;
                        continue;
                    }
                    std::array<float, 96> knots{};
                    uint32_t nb = 1u; knots[0] = a;
                    std::array<glm::vec2, 32> bands{};
                    uint32_t nr = 0u, zeroBands = 0u;
                    auto isolateMonotone = [&](float ca, float cb, float c, float va, float vb, float error, float minSlope, bool isZeroLevel) {
                        if (std::abs(va) <= error) { float w = 2.0f * error / minSlope; putBandTagged(ca, std::min(cb, ca + w), knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel); }
                        if (std::abs(vb) <= error) { float w = 2.0f * error / minSlope; putBandTagged(std::max(ca, cb - w), cb, knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel); }
                        if ((va > error && vb < -error) || (va < -error && vb > error)) {
                            float l = ca, r = cb, left = va;
                            for (uint32_t iter = 0u; iter < 32u; ++iter) {
                                if (st.work >= budget) { st.reason |= 1u; break; }
                                st.work += 1u;
                                float mid = l + 0.5f * (r - l);
                                if (mid <= l || mid >= r || r - l <= 0.0001f) break;
                                float v = evalPhi(m, o, d, mid) - c;
                                if (ctr) ctr->evalPhiBisect++;
                                if (std::abs(v) <= error) { float w = 2.0f * error / minSlope; l = std::max(l, mid - w); r = std::min(r, mid + w); break; }
                                if ((left < 0.0f && v > 0.0f) || (left > 0.0f && v < 0.0f)) r = mid; else { l = mid; left = v; }
                            }
                            putBandTagged(l, r, knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel);
                        }
                    };
                    auto roots = [&](float c, bool isZeroLevel, uint32_t levelMask) {
                        std::array<glm::vec4, 20> stack{};
                        std::array<uint32_t, 20> depths{};
                        uint32_t top = 0u;
                        for (uint32_t bin = 0u; bin < 8u; ++bin) {
                            float a0 = (bin == 0u) ? a : (a + (b - a) * float(bin) / 8.0f);
                            float b0 = (bin == 7u) ? b : (a + (b - a) * float(bin + 1u) / 8.0f);
                            if (st.work >= budget) { st.reason |= 1u; break; }
                            float coarseH = b0 - a0;
                            if (!(coarseH > 0.0f)) { st.work += 1u; st.reason |= 4u; break; }
                            if ((levelMask & (1u << bin)) == 0u) { st.work += 1u; continue; }
                            st.work += 1u;
                            float phi_a0 = phi_knots[bin], phi_b0 = phi_knots[bin + 1u];
                            float coarseVa = phi_a0 - c, coarseVb = phi_b0 - c;
                            float coarseScale = 1.0f + std::abs(coarseVa) + std::abs(coarseVb) + std::abs(a0) + std::abs(b0);
                            float coarseError = err + 0.000001f * coarseScale;
                            float coarseSlope = (coarseVb - coarseVa) / coarseH;
                            float coarseRadius = 0.5f * M * coarseH + 2.0f * coarseError / coarseH;
                            float coarseMinSlope = std::abs(coarseSlope) - coarseRadius;
                            if (coarseMinSlope > 0.0f) {
                                isolateMonotone(a0, b0, c, coarseVa, coarseVb, coarseError, coarseMinSlope, isZeroLevel);
                                if (st.reason != 0u) break;
                                continue;
                            }
                            if (0.125f * M * coarseH * coarseH <= coarseError) {
                                putBandTagged(a0, b0, knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel);
                                if (st.reason != 0u) break;
                                continue;
                            }
                            float mid0 = a0 + 0.5f * coarseH;
                            if (mid0 <= a0 || mid0 >= b0) {
                                putBandTagged(a0, b0, knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel);
                                if (st.reason != 0u) break;
                                continue;
                            }
                            float phi_mid0 = evalPhi(m, o, d, mid0);
                            if (ctr) ctr->evalPhiSplit++;
                            stack[0] = glm::vec4(mid0, b0, phi_mid0, phi_b0); depths[0] = 1u;
                            stack[1] = glm::vec4(a0, mid0, phi_a0, phi_mid0); depths[1] = 1u;
                            top = 2u;
                            while (top > 0u && st.reason == 0u) {
                                top--;
                                glm::vec4 cell = stack[top];
                                float ca = cell.x, cb = cell.y, h = cb - ca;
                                uint32_t depth = depths[top];
                                if (st.work >= budget) { st.reason |= 1u; break; }
                                st.work += 1u;
                                if (!(h > 0.0f)) { st.reason |= 4u; break; }
                                float va = cell.z - c, vb = cell.w - c;
                                float scale = 1.0f + std::abs(va) + std::abs(vb) + std::abs(ca) + std::abs(cb);
                                float error = err + 0.000001f * scale;
                                float bow = 0.125f * M * h * h + error;
                                if (std::min(va, vb) > bow || std::max(va, vb) < -bow) continue;
                                float slope = (vb - va) / h;
                                float radius = 0.5f * M * h + 2.0f * error / h;
                                float minSlope = std::abs(slope) - radius;
                                if (minSlope > 0.0f) {
                                    isolateMonotone(ca, cb, c, va, vb, error, minSlope, isZeroLevel);
                                    continue;
                                }
                                if (0.125f * M * h * h <= error) { putBandTagged(ca, cb, knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel); continue; }
                                if (float(depth) >= 14.0f || top + 2u > 20u) { putBandTagged(ca, cb, knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel); continue; }
                                float mid = ca + 0.5f * h;
                                if (mid <= ca || mid >= cb) { putBandTagged(ca, cb, knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel); continue; }
                                float phi_mid = evalPhi(m, o, d, mid);
                                if (ctr) ctr->evalPhiSplit++;
                                stack[top] = glm::vec4(mid, cb, phi_mid, cell.w); depths[top] = depth + 1u;
                                stack[top + 1u] = glm::vec4(ca, mid, cell.z, phi_mid); depths[top + 1u] = depth + 1u;
                                top += 2u;
                            }
                            if (st.reason != 0u) break;
                        }
                    };
                    roots(-m.limits.w, false, mask0);
                    if (st.reason != 0u) continue;
                    roots(0.0f, true, mask1);
                    if (st.reason != 0u) continue;
                    roots(m.limits.w, false, mask2);
                    if (st.reason != 0u) continue;
                    if (d.x != 0.0f) { float x = -o.x / d.x; if (x > a && x < b) knots[nb++] = x; }
                    if (d.y != 0.0f) {
                        float y = -o.y / d.y; if (y > a && y < b) knots[nb++] = y;
                        float k = (m.limits.z - o.y) / d.y; if (k > a && k < b) knots[nb++] = k;
                    }
                    knots[nb++] = b;
                    for (uint32_t i = 1u; i < nb; ++i) {
                        float v = knots[i]; uint32_t j = i;
                        while (j > 0u && knots[j - 1u] > v) { knots[j] = knots[j - 1u]; j--; }
                        knots[j] = v;
                    }
                    uint32_t memberStart = st.count;
                    bool previousShortBand = false, previousPositive = false;
                    for (uint32_t i = 1u; i < nb; ++i) {
                        float l = knots[i - 1u], r = knots[i];
                        if (!(r > l)) continue;
                        float mid = l + 0.5f * (r - l);
                        bool uncertain = false, protectedBoundary = false;
                        if (d.x != 0.0f) protectedBoundary = (l == -o.x / d.x);
                        if (d.y != 0.0f) protectedBoundary = protectedBoundary || (l == -o.y / d.y) || (l == (m.limits.z - o.y) / d.y);
                        for (uint32_t k = 0u; k < nr; ++k) {
                            uncertain = uncertain || (l < bands[k].y && r > bands[k].x);
                            if ((zeroBands & (1u << k)) != 0u && (l == bands[k].x || l == bands[k].y)) protectedBoundary = true;
                        }
                        bool inside = uncertain;
                        if (!inside) {
                            float pm = evalPhi(m, o, d, mid);
                            if (ctr) ctr->evalPhiPieceMid++;
                            inside = (std::abs(pm) <= m.limits.w + err);
                        }
                        if (inside) {
                            if (st.count > memberStart && mayAbsorb(st.pieces[st.count - 1u], glm::vec2(l, r),
                                                                    previousShortBand, previousPositive, uncertain, protectedBoundary)) {
                                st.pieces[st.count - 1u].y = r;
                                previousShortBand = false; previousPositive = true;
                            } else {
                                if (st.count >= 48u) { st.reason |= 8u; break; }
                                st.pieces[st.count++] = glm::vec2(l, r);
                                previousShortBand = uncertain && (r - l <= 0.02f);
                                previousPositive = !uncertain;
                            }
                        }
                    }
                    st.memCount[member] = st.count - st.memStart[member];
                }
                continue;
            }

            uint32_t numBeams = 64u / beamSize;
            for (uint32_t bId = 0; bId < numBeams; ++bId) {
                uint32_t lStart = bId * beamSize;
                uint32_t lEnd = std::min<uint32_t>(64u, lStart + beamSize);
                if (wgBase + lStart >= wgEnd) continue;

                int refLid = -1;
                uint32_t midOff = beamSize / 2u;
                for (uint32_t pass = 0; pass < beamSize; ++pass) {
                    uint32_t cand = lStart + ((pass == 0) ? midOff : (pass - (pass <= midOff ? 1u : 0u)));
                    if (cand < (wgEnd - wgBase) && hitArr[cand]) {
                        refLid = int(cand);
                        break;
                    }
                }
                if (refLid < 0) continue;

                float refA = aArr[refLid], refB = bArr[refLid];
                glm::vec3 refD = dirs[wgBase + refLid];
                std::array<float, 9> refPhi{};
                for (uint32_t k = 0; k < 9; ++k) {
                    float s = (k == 0) ? refA : ((k == 8) ? refB : (refA + (refB - refA) * float(k) / 8.0f));
                    refPhi[k] = evalPhi(m, o, refD, s);
                    if (ctr) ctr->beamRefKnotEvals++;
                }
                if (ctr) ctr->beamParallelKnotWaves += (beamSize >= 9u ? 1u : ((9u + beamSize - 1u) / beamSize));

                for (uint32_t lid = lStart; lid < lEnd && (wgBase + lid) < wgEnd; ++lid) {
                    if (!hitArr[lid] || lanes[lid].reason != 0u) continue;
                    auto& st = lanes[lid];
                    size_t id = wgBase + lid;
                    glm::vec3 d = dirs[id];
                    float a = aArr[lid], b = bArr[lid];
                    float M = m.curv.x * d.x * d.x * 1.00002f;
                    float err = m.curv.y;

                    uint32_t beamBinMask = 0xffu;
                    bool beamCertifiedSkip = false;
                    if (int(lid) != refLid) {
                        std::array<float, 9> delta{};
                        for (uint32_t k = 0; k < 9; ++k) {
                            float s = (k == 0) ? a : ((k == 8) ? b : (a + (b - a) * float(k) / 8.0f));
                            float sRef = (k == 0) ? refA : ((k == 8) ? refB : (refA + (refB - refA) * float(k) / 8.0f));
                            float dz = std::abs(s * d.z - sRef * refD.z);
                            float dx = std::abs(s * d.x - sRef * refD.x);
                            float dwave = std::min(L1 * dx, twoWmax);
                            delta[k] = dz + dwave + 1e-4f;
                        }
                        beamBinMask = 0u;
                        bool allPosH = true;
                        for (uint32_t bin = 0; bin < 8; ++bin) {
                            float a0 = (bin == 0) ? a : (a + (b - a) * float(bin) / 8.0f);
                            float b0 = (bin == 7) ? b : (a + (b - a) * float(bin + 1) / 8.0f);
                            float h = b0 - a0;
                            if (!(h > 0.0f)) allPosH = false;
                            float pa = refPhi[bin], pb = refPhi[bin + 1];
                            float da = delta[bin], db = delta[bin + 1];
                            float maxAbsA = std::abs(pa) + da + m.limits.w;
                            float maxAbsB = std::abs(pb) + db + m.limits.w;
                            float scale = 1.0f + maxAbsA + maxAbsB + std::abs(a0) + std::abs(b0);
                            float error = err + 0.000001f * scale;
                            float bow = 0.125f * M * h * h + error;
                            bool certifiedOut = (std::min(pa - da, pb - db) > m.limits.w + bow)
                                             || (std::max(pa + da, pb + db) < -m.limits.w - bow);
                            if (!certifiedOut) beamBinMask |= (1u << bin);
                        }
                        if (allPosH && beamBinMask == 0u && (std::abs(refPhi[0]) - delta[0] > m.limits.w + err)) {
                            beamCertifiedSkip = true;
                        }
                    }

                    if (beamCertifiedSkip) {
                        if (st.work + 24u > budget) { st.reason |= 1u; continue; }
                        st.work += 24u;
                        st.memCount[member] = 0u;
                        if (ctr) {
                            ctr->beamCertifiedWholeMemberSkips++;
                            ctr->gtWholeMemberSkips++;
                        }
                        continue;
                    }

                    std::array<float, 9> phi_knots{};
                    if (int(lid) == refLid) {
                        phi_knots = refPhi;
                    } else {
                        for (uint32_t k = 0; k < 9; ++k) {
                            bool needed = (k == 0u)
                                || (k < 8u && ((beamBinMask & (1u << k)) != 0u))
                                || (k > 0u && ((beamBinMask & (1u << (k - 1u))) != 0u));
                            if (needed) {
                                float s = (k == 0) ? a : ((k == 8) ? b : (a + (b - a) * float(k) / 8.0f));
                                phi_knots[k] = evalPhi(m, o, d, s);
                                if (ctr) ctr->perRayKnotEvals++;
                            }
                        }
                    }

                    bool allPositiveH = true;
                    auto exactLevelActiveMask = [&](float c) -> uint32_t {
                        uint32_t mask = 0u;
                        for (uint32_t bin = 0; bin < 8; ++bin) {
                            float a0 = (bin == 0) ? a : (a + (b - a) * float(bin) / 8.0f);
                            float b0 = (bin == 7) ? b : (a + (b - a) * float(bin + 1) / 8.0f);
                            float h = b0 - a0;
                            if (!(h > 0.0f)) allPositiveH = false;
                            if ((beamBinMask & (1u << bin)) == 0u) continue;
                            float va = phi_knots[bin] - c, vb = phi_knots[bin + 1] - c;
                            float scale = 1.0f + std::abs(va) + std::abs(vb) + std::abs(a0) + std::abs(b0);
                            float error = err + 0.000001f * scale;
                            float bow = 0.125f * M * h * h + error;
                            if (!(std::min(va, vb) > bow || std::max(va, vb) < -bow)) mask |= (1u << bin);
                        }
                        return mask;
                    };

                    uint32_t mask0 = exactLevelActiveMask(-m.limits.w);
                    uint32_t mask1 = exactLevelActiveMask(0.0f);
                    uint32_t mask2 = exactLevelActiveMask(m.limits.w);
                    if (allPositiveH && (mask0 | mask1 | mask2) == 0u && std::abs(phi_knots[0]) > m.limits.w + err) {
                        if (st.work + 24u > budget) { st.reason |= 1u; continue; }
                        st.work += 24u;
                        st.memCount[member] = 0u;
                        if (ctr) {
                            if (int(lid) == refLid) ctr->beamCertifiedWholeMemberSkips++;
                            else ctr->fallbackPerRayWholeMemberSkips++;
                            ctr->gtWholeMemberSkips++;
                        }
                        continue;
                    }

                    std::array<float, 96> knots{};
                    uint32_t nb = 1u; knots[0] = a;
                    std::array<glm::vec2, 32> bands{};
                    uint32_t nr = 0u, zeroBands = 0u;

                    auto isolateMonotone = [&](float ca, float cb, float c, float va, float vb, float error, float minSlope, bool isZeroLevel) {
                        if (std::abs(va) <= error) { float w = 2.0f * error / minSlope; putBandTagged(ca, std::min(cb, ca + w), knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel); }
                        if (std::abs(vb) <= error) { float w = 2.0f * error / minSlope; putBandTagged(std::max(ca, cb - w), cb, knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel); }
                        if ((va > error && vb < -error) || (va < -error && vb > error)) {
                            float l = ca, r = cb, left = va;
                            for (uint32_t iter = 0u; iter < 32u; ++iter) {
                                if (st.work >= budget) { st.reason |= 1u; break; }
                                st.work += 1u;
                                float mid = l + 0.5f * (r - l);
                                if (mid <= l || mid >= r || r - l <= 0.0001f) break;
                                float v = evalPhi(m, o, d, mid) - c;
                                if (ctr) ctr->evalPhiBisect++;
                                if (std::abs(v) <= error) { float w = 2.0f * error / minSlope; l = std::max(l, mid - w); r = std::min(r, mid + w); break; }
                                if ((left < 0.0f && v > 0.0f) || (left > 0.0f && v < 0.0f)) r = mid; else { l = mid; left = v; }
                            }
                            putBandTagged(l, r, knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel);
                        }
                    };

                    auto roots = [&](float c, bool isZeroLevel, uint32_t levelMask) {
                        std::array<glm::vec4, 20> stack{};
                        std::array<uint32_t, 20> depths{};
                        uint32_t top = 0u;
                        for (uint32_t bin = 0u; bin < 8u; ++bin) {
                            float a0 = (bin == 0u) ? a : (a + (b - a) * float(bin) / 8.0f);
                            float b0 = (bin == 7u) ? b : (a + (b - a) * float(bin + 1u) / 8.0f);
                            if (st.work >= budget) { st.reason |= 1u; break; }
                            float coarseH = b0 - a0;
                            if (!(coarseH > 0.0f)) { st.work += 1u; st.reason |= 4u; break; }
                            if ((levelMask & (1u << bin)) == 0u) { st.work += 1u; continue; }
                            st.work += 1u;
                            float phi_a0 = phi_knots[bin], phi_b0 = phi_knots[bin + 1u];
                            float coarseVa = phi_a0 - c, coarseVb = phi_b0 - c;
                            float coarseScale = 1.0f + std::abs(coarseVa) + std::abs(coarseVb) + std::abs(a0) + std::abs(b0);
                            float coarseError = err + 0.000001f * coarseScale;
                            float coarseSlope = (coarseVb - coarseVa) / coarseH;
                            float coarseRadius = 0.5f * M * coarseH + 2.0f * coarseError / coarseH;
                            float coarseMinSlope = std::abs(coarseSlope) - coarseRadius;
                            if (coarseMinSlope > 0.0f) {
                                isolateMonotone(a0, b0, c, coarseVa, coarseVb, coarseError, coarseMinSlope, isZeroLevel);
                                if (st.reason != 0u) break;
                                continue;
                            }
                            if (0.125f * M * coarseH * coarseH <= coarseError) {
                                putBandTagged(a0, b0, knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel);
                                if (st.reason != 0u) break;
                                continue;
                            }
                            float mid0 = a0 + 0.5f * coarseH;
                            if (mid0 <= a0 || mid0 >= b0) {
                                putBandTagged(a0, b0, knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel);
                                if (st.reason != 0u) break;
                                continue;
                            }
                            float phi_mid0 = evalPhi(m, o, d, mid0);
                            if (ctr) ctr->evalPhiSplit++;
                            stack[0] = glm::vec4(mid0, b0, phi_mid0, phi_b0); depths[0] = 1u;
                            stack[1] = glm::vec4(a0, mid0, phi_a0, phi_mid0); depths[1] = 1u;
                            top = 2u;
                            while (top > 0u && st.reason == 0u) {
                                top--;
                                glm::vec4 cell = stack[top];
                                float ca = cell.x, cb = cell.y, h = cb - ca;
                                uint32_t depth = depths[top];
                                if (st.work >= budget) { st.reason |= 1u; break; }
                                st.work += 1u;
                                if (!(h > 0.0f)) { st.reason |= 4u; break; }
                                float va = cell.z - c, vb = cell.w - c;
                                float scale = 1.0f + std::abs(va) + std::abs(vb) + std::abs(ca) + std::abs(cb);
                                float error = err + 0.000001f * scale;
                                float bow = 0.125f * M * h * h + error;
                                if (std::min(va, vb) > bow || std::max(va, vb) < -bow) continue;
                                float slope = (vb - va) / h;
                                float radius = 0.5f * M * h + 2.0f * error / h;
                                float minSlope = std::abs(slope) - radius;
                                if (minSlope > 0.0f) {
                                    isolateMonotone(ca, cb, c, va, vb, error, minSlope, isZeroLevel);
                                    continue;
                                }
                                if (0.125f * M * h * h <= error) { putBandTagged(ca, cb, knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel); continue; }
                                if (float(depth) >= 14.0f || top + 2u > 20u) { putBandTagged(ca, cb, knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel); continue; }
                                float mid = ca + 0.5f * h;
                                if (mid <= ca || mid >= cb) { putBandTagged(ca, cb, knots, nb, bands, nr, st.reason, zeroBands, isZeroLevel); continue; }
                                float phi_mid = evalPhi(m, o, d, mid);
                                if (ctr) ctr->evalPhiSplit++;
                                stack[top] = glm::vec4(mid, cb, phi_mid, cell.w); depths[top] = depth + 1u;
                                stack[top + 1u] = glm::vec4(ca, mid, cell.z, phi_mid); depths[top + 1u] = depth + 1u;
                                top += 2u;
                            }
                            if (st.reason != 0u) break;
                        }
                    };
                    roots(-m.limits.w, false, mask0);
                    if (st.reason != 0u) continue;
                    roots(0.0f, true, mask1);
                    if (st.reason != 0u) continue;
                    roots(m.limits.w, false, mask2);
                    if (st.reason != 0u) continue;
                    if (d.x != 0.0f) { float x = -o.x / d.x; if (x > a && x < b) knots[nb++] = x; }
                    if (d.y != 0.0f) {
                        float y = -o.y / d.y; if (y > a && y < b) knots[nb++] = y;
                        float k = (m.limits.z - o.y) / d.y; if (k > a && k < b) knots[nb++] = k;
                    }
                    knots[nb++] = b;
                    for (uint32_t i = 1u; i < nb; ++i) {
                        float v = knots[i]; uint32_t j = i;
                        while (j > 0u && knots[j - 1u] > v) { knots[j] = knots[j - 1u]; j--; }
                        knots[j] = v;
                    }
                    uint32_t memberStart = st.count;
                    bool previousShortBand = false, previousPositive = false;
                    for (uint32_t i = 1u; i < nb; ++i) {
                        float l = knots[i - 1u], r = knots[i];
                        if (!(r > l)) continue;
                        float mid = l + 0.5f * (r - l);
                        bool uncertain = false, protectedBoundary = false;
                        if (d.x != 0.0f) protectedBoundary = (l == -o.x / d.x);
                        if (d.y != 0.0f) protectedBoundary = protectedBoundary || (l == -o.y / d.y) || (l == (m.limits.z - o.y) / d.y);
                        for (uint32_t k = 0u; k < nr; ++k) {
                            uncertain = uncertain || (l < bands[k].y && r > bands[k].x);
                            if ((zeroBands & (1u << k)) != 0u && (l == bands[k].x || l == bands[k].y)) protectedBoundary = true;
                        }
                        bool inside = uncertain;
                        if (!inside) {
                            float pm = evalPhi(m, o, d, mid);
                            if (ctr) ctr->evalPhiPieceMid++;
                            inside = (std::abs(pm) <= m.limits.w + err);
                        }
                        if (inside) {
                            if (st.count > memberStart && mayAbsorb(st.pieces[st.count - 1u], glm::vec2(l, r),
                                                                    previousShortBand, previousPositive, uncertain, protectedBoundary)) {
                                st.pieces[st.count - 1u].y = r;
                                previousShortBand = false; previousPositive = true;
                            } else {
                                if (st.count >= 48u) { st.reason |= 8u; break; }
                                st.pieces[st.count++] = glm::vec2(l, r);
                                previousShortBand = uncertain && (r - l <= 0.02f);
                                previousPositive = !uncertain;
                            }
                        }
                    }
                    st.memCount[member] = st.count - st.memStart[member];
                }
            }
        }

        for (size_t lid = 0; lid < (wgEnd - wgBase); ++lid) {
            size_t id = wgBase + lid;
            auto& st = lanes[lid];
            auto& out = schedules[id];
            if (st.reason != 0u) { out.flag = 0x80000000u | st.reason; continue; }
            std::array<float, 96> knots{};
            uint32_t ne = 0u;
            std::array<uint32_t, 4> head{0, 0, 0, 0};
            uint32_t totalKnots = (st.memCount[0] + st.memCount[1] + st.memCount[2] + st.memCount[3]) * 2u;
            while (ne < totalKnots) {
                uint32_t bestM = 4u; float bestVal = 0.0f;
                for (uint32_t m = 0u; m < 4u; ++m) {
                    uint32_t h = head[m];
                    if (h < st.memCount[m] * 2u) {
                        uint32_t pIdx = st.memStart[m] + (h >> 1u);
                        float val = ((h & 1u) != 0u) ? st.pieces[pIdx].y : st.pieces[pIdx].x;
                        if (bestM == 4u || val < bestVal) { bestVal = val; bestM = m; }
                    }
                }
                knots[ne++] = bestVal; head[bestM]++;
            }
            std::array<uint32_t, 4> pIdx{0, 0, 0, 0};
            for (uint32_t i = 1u; i < ne && st.reason == 0u; ++i) {
                float a = knots[i - 1u], b = knots[i];
                if (!(b > a)) continue;
                float mid = a + 0.5f * (b - a);
                uint32_t mask = 0u;
                for (uint32_t m = 0u; m < 4u; ++m) {
                    uint32_t start = st.memStart[m], numPieces = st.memCount[m], cur = pIdx[m];
                    while (cur < numPieces && st.pieces[start + cur].y < mid) cur++;
                    pIdx[m] = cur;
                    if (cur < numPieces) {
                        glm::vec2 p = st.pieces[start + cur];
                        if (mid >= p.x && mid <= p.y) mask |= (1u << m);
                    }
                }
                if (mask == 0u) continue;
                if (std::popcount(mask) == 1) {
                    uint32_t n = std::max(1u, uint32_t(std::ceil((b - a) / 4.0f)));
                    if (out.singles.size() + n > maxSubPerRay) { st.reason |= 16u; break; }
                    uint32_t member = std::countr_zero(mask);
                    for (uint32_t k = 0u; k < n; ++k) {
                        float l = (k == 0u) ? a : (a + float(k) * (b - a) / float(n));
                        float r = (k + 1u == n) ? b : (a + float(k + 1u) * (b - a) / float(n));
                        out.singles.push_back({l, r, member});
                    }
                } else {
                    if (out.overs.size() >= maxOverPerRay) { st.reason |= 32u; break; }
                    out.overs.push_back({a, b, mask});
                }
            }
            if (st.reason != 0u) { out.singles.clear(); out.overs.clear(); out.flag = 0x80000000u | st.reason; }
        }
    }
    return schedules;
}

static glm::vec4 mod289(glm::vec4 x) { return x - glm::floor(x * (1.0f / 289.0f)) * 289.0f; }
static glm::vec3 mod289_3(glm::vec3 x) { return x - glm::floor(x * (1.0f / 289.0f)) * 289.0f; }
static glm::vec4 permute4(glm::vec4 x) { return mod289(((x * 34.0f) + 1.0f) * x); }
static glm::vec4 taylorInvSqrt(glm::vec4 r) { return 1.79284291400159f - 0.85373472095314f * r; }

static float cnoise3_cpu(glm::vec3 P) {
    glm::vec3 Pi0 = glm::floor(P);
    glm::vec3 Pi1 = Pi0 + glm::vec3(1.0f);
    glm::vec3 Pi0_mod = mod289_3(Pi0);
    glm::vec3 Pi1_mod = mod289_3(Pi1);
    glm::vec3 Pf0 = glm::fract(P);
    glm::vec3 Pf1 = Pf0 - glm::vec3(1.0f);
    glm::vec4 ix(Pi0_mod.x, Pi1_mod.x, Pi0_mod.x, Pi1_mod.x);
    glm::vec4 iy(Pi0_mod.y, Pi0_mod.y, Pi1_mod.y, Pi1_mod.y);
    glm::vec4 iz0(Pi0_mod.z);
    glm::vec4 iz1(Pi1_mod.z);

    glm::vec4 ixy = permute4(permute4(ix) + iy);
    glm::vec4 ixy0 = permute4(ixy + iz0);
    glm::vec4 ixy1 = permute4(ixy + iz1);

    glm::vec4 gx0 = ixy0 / 7.0f;
    glm::vec4 gy0 = glm::fract(glm::floor(gx0) / 7.0f) - 0.5f;
    gx0 = glm::fract(gx0);
    glm::vec4 gz0 = glm::vec4(0.5f) - glm::abs(gx0) - glm::abs(gy0);
    glm::vec4 sz0 = glm::step(gz0, glm::vec4(0.0f));
    gx0 = gx0 - sz0 * (glm::step(glm::vec4(0.0f), gx0) - 0.5f);
    gy0 = gy0 - sz0 * (glm::step(glm::vec4(0.0f), gy0) - 0.5f);

    glm::vec4 gx1 = ixy1 / 7.0f;
    glm::vec4 gy1 = glm::fract(glm::floor(gx1) / 7.0f) - 0.5f;
    gx1 = glm::fract(gx1);
    glm::vec4 gz1 = glm::vec4(0.5f) - glm::abs(gx1) - glm::abs(gy1);
    glm::vec4 sz1 = glm::step(gz1, glm::vec4(0.0f));
    gx1 = gx1 - sz1 * (glm::step(glm::vec4(0.0f), gx1) - 0.5f);
    gy1 = gy1 - sz1 * (glm::step(glm::vec4(0.0f), gy1) - 0.5f);

    glm::vec3 g000(gx0.x, gy0.x, gz0.x);
    glm::vec3 g100(gx0.y, gy0.y, gz0.y);
    glm::vec3 g010(gx0.z, gy0.z, gz0.z);
    glm::vec3 g110(gx0.w, gy0.w, gz0.w);
    glm::vec3 g001(gx1.x, gy1.x, gz1.x);
    glm::vec3 g101(gx1.y, gy1.y, gz1.y);
    glm::vec3 g011(gx1.z, gy1.z, gz1.z);
    glm::vec3 g111(gx1.w, gy1.w, gz1.w);

    glm::vec4 norm0 = taylorInvSqrt(glm::vec4(glm::dot(g000, g000), glm::dot(g010, g010), glm::dot(g100, g100), glm::dot(g110, g110)));
    g000 *= norm0.x; g010 *= norm0.y; g100 *= norm0.z; g110 *= norm0.w;
    glm::vec4 norm1 = taylorInvSqrt(glm::vec4(glm::dot(g001, g001), glm::dot(g011, g011), glm::dot(g101, g101), glm::dot(g111, g111)));
    g001 *= norm1.x; g011 *= norm1.y; g101 *= norm1.z; g111 *= norm1.w;

    float n000 = glm::dot(g000, Pf0);
    float n100 = glm::dot(g100, glm::vec3(Pf1.x, Pf0.y, Pf0.z));
    float n010 = glm::dot(g010, glm::vec3(Pf0.x, Pf1.y, Pf0.z));
    float n110 = glm::dot(g110, glm::vec3(Pf1.x, Pf1.y, Pf0.z));
    float n001 = glm::dot(g001, glm::vec3(Pf0.x, Pf0.y, Pf1.z));
    float n101 = glm::dot(g101, glm::vec3(Pf1.x, Pf0.y, Pf1.z));
    float n011 = glm::dot(g011, glm::vec3(Pf0.x, Pf1.y, Pf1.z));
    float n111 = glm::dot(g111, Pf1);

    glm::vec3 fade_xyz = Pf0 * Pf0 * Pf0 * (Pf0 * (Pf0 * 6.0f - 15.0f) + 10.0f);
    glm::vec4 n_z = glm::mix(glm::vec4(n000, n100, n010, n110), glm::vec4(n001, n101, n011, n111), fade_xyz.z);
    glm::vec2 n_yz = glm::mix(glm::vec2(n_z.x, n_z.y), glm::vec2(n_z.z, n_z.w), fade_xyz.y);
    float n_xyz = glm::mix(n_yz.x, n_yz.y, fade_xyz.x);
    return 2.2f * n_xyz;
}

static glm::vec2 cnoise3Pair_cpu(glm::vec3 Pa, glm::vec3 Pb, uint64_t& sameCellCount, uint64_t& diffCellCount) {
    glm::vec3 Pi0 = glm::floor(Pa);
    glm::vec3 Pi0_b = glm::floor(Pb);
    if (Pi0 != Pi0_b) {
        diffCellCount++;
        return glm::vec2(cnoise3_cpu(Pa), cnoise3_cpu(Pb));
    }
    sameCellCount++;
    glm::vec3 Pi1 = Pi0 + glm::vec3(1.0f);
    glm::vec3 Pi0_mod = mod289_3(Pi0);
    glm::vec3 Pi1_mod = mod289_3(Pi1);
    glm::vec3 Pf0_a = glm::fract(Pa);
    glm::vec3 Pf1_a = Pf0_a - glm::vec3(1.0f);
    glm::vec3 Pf0_b = glm::fract(Pb);
    glm::vec3 Pf1_b = Pf0_b - glm::vec3(1.0f);

    glm::vec4 ix(Pi0_mod.x, Pi1_mod.x, Pi0_mod.x, Pi1_mod.x);
    glm::vec4 iy(Pi0_mod.y, Pi0_mod.y, Pi1_mod.y, Pi1_mod.y);
    glm::vec4 ixy = permute4(permute4(ix) + iy);

    glm::vec4 ixy0 = permute4(ixy + glm::vec4(Pi0_mod.z));
    glm::vec4 gx = ixy0 / 7.0f;
    glm::vec4 gy = glm::fract(glm::floor(gx) / 7.0f) - 0.5f;
    gx = glm::fract(gx);
    glm::vec4 gz = glm::vec4(0.5f) - glm::abs(gx) - glm::abs(gy);
    glm::vec4 sz = glm::step(gz, glm::vec4(0.0f));
    gx = gx - sz * (glm::step(glm::vec4(0.0f), gx) - 0.5f);
    gy = gy - sz * (glm::step(glm::vec4(0.0f), gy) - 0.5f);

    glm::vec3 g0(gx.x, gy.x, gz.x);
    glm::vec3 g1(gx.y, gy.y, gz.y);
    glm::vec3 g2(gx.z, gy.z, gz.z);
    glm::vec3 g3(gx.w, gy.w, gz.w);
    glm::vec4 norm0 = taylorInvSqrt(glm::vec4(glm::dot(g0, g0), glm::dot(g2, g2), glm::dot(g1, g1), glm::dot(g3, g3)));
    g0 *= norm0.x; g2 *= norm0.y; g1 *= norm0.z; g3 *= norm0.w;

    glm::vec4 nz0_a(
        glm::dot(g0, Pf0_a),
        glm::dot(g1, glm::vec3(Pf1_a.x, Pf0_a.y, Pf0_a.z)),
        glm::dot(g2, glm::vec3(Pf0_a.x, Pf1_a.y, Pf0_a.z)),
        glm::dot(g3, glm::vec3(Pf1_a.x, Pf1_a.y, Pf0_a.z))
    );
    glm::vec4 nz0_b(
        glm::dot(g0, Pf0_b),
        glm::dot(g1, glm::vec3(Pf1_b.x, Pf0_b.y, Pf0_b.z)),
        glm::dot(g2, glm::vec3(Pf0_b.x, Pf1_b.y, Pf0_b.z)),
        glm::dot(g3, glm::vec3(Pf1_b.x, Pf1_b.y, Pf0_b.z))
    );

    glm::vec4 ixy1 = permute4(ixy + glm::vec4(Pi1_mod.z));
    gx = ixy1 / 7.0f;
    gy = glm::fract(glm::floor(gx) / 7.0f) - 0.5f;
    gx = glm::fract(gx);
    gz = glm::vec4(0.5f) - glm::abs(gx) - glm::abs(gy);
    sz = glm::step(gz, glm::vec4(0.0f));
    gx = gx - sz * (glm::step(glm::vec4(0.0f), gx) - 0.5f);
    gy = gy - sz * (glm::step(glm::vec4(0.0f), gy) - 0.5f);

    g0 = glm::vec3(gx.x, gy.x, gz.x);
    g1 = glm::vec3(gx.y, gy.y, gz.y);
    g2 = glm::vec3(gx.z, gy.z, gz.z);
    g3 = glm::vec3(gx.w, gy.w, gz.w);
    glm::vec4 norm1 = taylorInvSqrt(glm::vec4(glm::dot(g0, g0), glm::dot(g2, g2), glm::dot(g1, g1), glm::dot(g3, g3)));
    g0 *= norm1.x; g2 *= norm1.y; g1 *= norm1.z; g3 *= norm1.w;

    glm::vec4 nz1_a(
        glm::dot(g0, glm::vec3(Pf0_a.x, Pf0_a.y, Pf1_a.z)),
        glm::dot(g1, glm::vec3(Pf1_a.x, Pf0_a.y, Pf1_a.z)),
        glm::dot(g2, glm::vec3(Pf0_a.x, Pf1_a.y, Pf1_a.z)),
        glm::dot(g3, Pf1_a)
    );
    glm::vec4 nz1_b(
        glm::dot(g0, glm::vec3(Pf0_b.x, Pf0_b.y, Pf1_b.z)),
        glm::dot(g1, glm::vec3(Pf1_b.x, Pf0_b.y, Pf1_b.z)),
        glm::dot(g2, glm::vec3(Pf0_b.x, Pf1_b.y, Pf1_b.z)),
        glm::dot(g3, Pf1_b)
    );

    glm::vec3 fade_a = Pf0_a * Pf0_a * Pf0_a * (Pf0_a * (Pf0_a * 6.0f - 15.0f) + 10.0f);
    glm::vec4 n_z_a = glm::mix(nz0_a, nz1_a, fade_a.z);
    glm::vec2 n_yz_a = glm::mix(glm::vec2(n_z_a.x, n_z_a.y), glm::vec2(n_z_a.z, n_z_a.w), fade_a.y);
    float val_a = 2.2f * glm::mix(n_yz_a.x, n_yz_a.y, fade_a.x);

    glm::vec3 fade_b = Pf0_b * Pf0_b * Pf0_b * (Pf0_b * (Pf0_b * 6.0f - 15.0f) + 10.0f);
    glm::vec4 n_z_b = glm::mix(nz0_b, nz1_b, fade_b.z);
    glm::vec2 n_yz_b = glm::mix(glm::vec2(n_z_b.x, n_z_b.y), glm::vec2(n_z_b.z, n_z_b.w), fade_b.y);
    float val_b = 2.2f * glm::mix(n_yz_b.x, n_yz_b.y, fade_b.x);

    return glm::vec2(val_a, val_b);
}

static float evalEnvelope_cpu(glm::vec3 p, const float* v) {
    float f0 = v[0];
    if (f0 == 0.0f) return 0.0f;
    float f1 = std::clamp(v[1] - (std::abs(v[2]) >= 1e-6f ? std::abs(p.x) / v[2] : 0.0f), v[3], v[4]);
    if (f1 == 0.0f) return 0.0f;
    float f2 = std::clamp(v[5] * (p.y + v[6]), v[7], v[8]);
    if (f2 == 0.0f) return 0.0f;
    float f3 = std::clamp(v[9] - (std::abs(v[10]) >= 1e-6f ? (p.y + v[11]) / v[10] : 0.0f), v[12], v[13]);
    if (f3 == 0.0f) return 0.0f;
    float z_curve = p.z + v[16] * std::sin(v[17] * p.x + v[18])
                        + v[19] * std::cos(v[20] * p.x + v[21])
                        + v[22] * std::sin(v[23] * p.x + v[24]);
    float f4 = std::clamp(v[14] - (std::abs(v[15]) >= 1e-6f ? std::abs(z_curve) / v[15] : 0.0f), v[25], v[26]);
    if (f4 == 0.0f) return 0.0f;
    float f5 = v[27] + v[28] * std::cos(v[29] * p.x + v[30])
                     + v[31] * std::cos(v[32] * p.x + v[33]);
    if (f5 == 0.0f) return 0.0f;
    return f0 * f1 * f2 * f3 * f4 * f5;
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: verify_6de <zone.ecform> <rays.json> <out.json>\n";
        return 1;
    }
    std::ifstream in(argv[1], std::ios::binary);
    std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
    J zone = J::parse(J::from_msgpack(b).at("MigrationRoot").get<std::string>());

    std::vector<GpuCurtainSpec> specs;
    std::vector<float> paramPool;
    std::vector<uint32_t> paramOffsets;
    if (!extractCurtainSpecs(zone, specs, paramPool, paramOffsets)) {
        throw std::runtime_error("Failed to extract curtain specs");
    }

    J raysJ;
    { std::ifstream rin(argv[2]); rin >> raysJ; }
    const auto& R = raysJ.at("rays");
    glm::vec3 ro{
        raysJ["origin"][0].get<float>(),
        raysJ["origin"][1].get<float>(),
        raysJ["origin"][2].get<float>()
    };
    size_t count = R.size();
    std::vector<glm::vec3> dirs(count);
    std::vector<glm::vec2> clips(count);
    for (size_t i = 0; i < count; ++i) {
        dirs[i] = glm::vec3(R[i][2].get<float>(), R[i][3].get<float>(), R[i][4].get<float>());
        clips[i] = glm::vec2(R[i][5].get<float>(), R[i][6].get<float>());
    }

    // 1. Run baseline exact-monotone-bowskip (beamSize = 1)
    BeamCounters baseCtr{};
    auto baseScheds = runWorkgroupBeamProducer(specs, ro, dirs, clips, 1024u, 64u, 16u, 1u, &baseCtr);
    uint64_t totalSingles = 0, totalOvers = 0;
    for (const auto& s : baseScheds) {
        totalSingles += s.singles.size();
        totalOvers += s.overs.size();
    }

    // 2. Sweep beam sizes 4, 8, 16, 32, 64 and verify 100.0% bit-exact schedule parity
    J beamSweep = J::array();
    for (uint32_t bs : {4u, 8u, 16u, 32u, 64u}) {
        BeamCounters bCtr{};
        auto candScheds = runWorkgroupBeamProducer(specs, ro, dirs, clips, 1024u, 64u, 16u, bs, &bCtr);
        size_t diffRays = 0;
        for (size_t i = 0; i < count; ++i) {
            if (!schedulesBitEqual(baseScheds[i], candScheds[i])) diffRays++;
        }
        if (diffRays != 0) {
            throw std::runtime_error("Beam schedule parity failed at beamSize=" + std::to_string(bs));
        }
        uint64_t totalCandKnots = bCtr.beamRefKnotEvals + bCtr.perRayKnotEvals;
        uint64_t totalBasePhi = baseCtr.baselineKnotEvals + baseCtr.evalPhiBisect + baseCtr.evalPhiSplit + baseCtr.evalPhiPieceMid;
        uint64_t totalCandPhi = totalCandKnots + bCtr.evalPhiBisect + bCtr.evalPhiSplit + bCtr.evalPhiPieceMid;
        beamSweep.push_back({
            {"beam_size", bs},
            {"schedule_differing_rays", diffRays},
            {"box_hits", bCtr.boxHits},
            {"gt_whole_member_skips", bCtr.gtWholeMemberSkips},
            {"beam_certified_whole_member_skips", bCtr.beamCertifiedWholeMemberSkips},
            {"beam_whole_skip_capture_pct", 100.0 * double(bCtr.beamCertifiedWholeMemberSkips) / double(bCtr.gtWholeMemberSkips)},
            {"baseline_knot_evalPhi", bCtr.baselineKnotEvals},
            {"beam_ref_knot_evalPhi", bCtr.beamRefKnotEvals},
            {"per_ray_residual_knot_evalPhi", bCtr.perRayKnotEvals},
            {"total_knot_evalPhi", totalCandKnots},
            {"knot_evalPhi_reduction_pct", 100.0 * (1.0 - double(totalCandKnots) / double(bCtr.baselineKnotEvals))},
            {"total_producer_evalPhi_baseline", totalBasePhi},
            {"total_producer_evalPhi_candidate", totalCandPhi},
            {"total_producer_evalPhi_reduction_pct", 100.0 * (1.0 - double(totalCandPhi) / double(totalBasePhi))},
            {"parallel_ref_knot_waves", bCtr.beamParallelKnotWaves}
        });
    }

    // 3. Synthetic budget boundary controls (23/24/25 -> 16/0/0) & capacity controls (maxSub=4 -> 26,243, maxOver=1 -> 412) at beamSize = 16
    std::vector<GpuCurtainSpec> synthSpecs = specs;
    for (auto& m : synthSpecs) {
        m.origin = glm::vec4(1000.0f, 1000.0f, 1000.0f, 0.0f);
        m.half = glm::vec4(1.0f);
        m.amps = glm::vec4(0.0f);
        m.freqs = glm::vec4(0.0f);
        m.phases = glm::vec4(0.0f);
    }
    synthSpecs[0].origin = glm::vec4(0.0f);
    synthSpecs[0].half = glm::vec4(10.0f);
    synthSpecs[0].limits = glm::vec4(2.0f, 2.0f, 0.0f, 0.2f);
    synthSpecs[0].curv = glm::vec4(0.0f, 0.001f, 0.0f, 0.0f);
    std::vector<glm::vec3> synthDirs(16, glm::vec3(1.0f, 0.0f, 0.0f));
    std::vector<glm::vec2> synthClips(16, glm::vec2(1.0f, 2.0f));
    glm::vec3 synthRo(-0.5f, 0.0f, 2.0f);
    J budgetControls = J::array();
    for (uint32_t bgt : {23u, 24u, 25u}) {
        auto s16 = runWorkgroupBeamProducer(synthSpecs, synthRo, synthDirs, synthClips, bgt, 64u, 16u, 16u, nullptr);
        uint32_t expectedFlag = (bgt < 24u) ? 0x80000001u : 0u;
        uint32_t fallbacks = 0;
        for (int i = 0; i < 16; ++i) {
            if (s16[i].flag != expectedFlag || !s16[i].singles.empty()) {
                throw std::runtime_error("Synthetic budget control mismatch at budget=" + std::to_string(bgt));
            }
            if (s16[i].flag & 0x80000000u) fallbacks++;
        }
        budgetControls.push_back({{"budget", bgt}, {"fallbacks", fallbacks}, {"expected", bgt < 24u ? 16 : 0}, {"pass", true}});
    }

    auto capSubBase = runWorkgroupBeamProducer(specs, ro, dirs, clips, 1024u, 4u, 16u, 1u, nullptr);
    auto capSubBeam = runWorkgroupBeamProducer(specs, ro, dirs, clips, 1024u, 4u, 16u, 16u, nullptr);
    auto capOverBase = runWorkgroupBeamProducer(specs, ro, dirs, clips, 1024u, 64u, 1u, 1u, nullptr);
    auto capOverBeam = runWorkgroupBeamProducer(specs, ro, dirs, clips, 1024u, 64u, 1u, 16u, nullptr);
    uint32_t subRefused = 0, overRefused = 0;
    for (size_t i = 0; i < count; ++i) {
        if (!schedulesBitEqual(capSubBase[i], capSubBeam[i]) || !schedulesBitEqual(capOverBase[i], capOverBeam[i])) {
            throw std::runtime_error("Capacity control parity failed at ray " + std::to_string(i));
        }
        if (capSubBeam[i].flag & 0x80000000u) subRefused++;
        if (capOverBeam[i].flag & 0x80000000u) overRefused++;
    }

    // 4. Verify Candidate 1 (`cnoise3Pair`) across all 247,326 actual subpanels and all 32,784 rays
    const float freqs[4] = {0.08f, 0.09f, 0.07f, 0.10f};
    const float alphas[4] = {2.0f/15.0f, 9.0f/65.0f, 8.0f/55.0f, 4.0f/25.0f};
    const glm::vec3 colors[4] = {
        {0.0747842f, 0.688014f, 0.239309f},
        {0.0391309f, 0.547833f, 0.641747f},
        {0.680411f,  0.103093f, 0.701029f},
        {0.612595f,  0.108105f, 0.198192f}
    };
    const float gx0 = 0.5f * (1.0f - 0.5773502691896257f);
    const float gx1 = 0.5f * (1.0f + 0.5773502691896257f);

    uint64_t subBothPos = 0, subOnePos = 0, subZeroPos = 0;
    uint64_t pairSameCell = 0, pairDiffCell = 0;
    uint64_t subpanelNoiseBitDiffs = 0, rayOutputBitDiffs = 0;

    for (size_t i = 0; i < count; ++i) {
        const auto& sched = baseScheds[i];
        glm::vec3 rd = dirs[i];
        float T_unfused = 1.0f, T_pair = 1.0f;
        glm::vec3 L_unfused(0.0f), L_pair(0.0f);

        for (const auto& sub : sched.singles) {
            float s0 = sub.s0, s1 = sub.s1, sub_h = s1 - s0;
            uint32_t mid = sub.member;
            glm::vec3 mOrigin = glm::vec3(specs[mid].origin);
            const float* pV = paramPool.data() + paramOffsets[mid];
            float freq = freqs[mid], alpha = alphas[mid];
            glm::vec3 color = colors[mid];

            glm::vec3 w_m = ro + (s0 + gx0 * sub_h) * rd - mOrigin;
            glm::vec3 w_p = ro + (s0 + gx1 * sub_h) * rd - mOrigin;
            float env_m = evalEnvelope_cpu(w_m, pV);
            float env_p = evalEnvelope_cpu(w_p, pV);

            // Unfused path
            float Dm_u = 0.0f, Dp_u = 0.0f;
            if (env_m > 0.0f) Dm_u = env_m * std::clamp(0.7f + 0.3f * cnoise3_cpu(freq * w_m), 0.0f, 1.0f);
            if (env_p > 0.0f) Dp_u = env_p * std::clamp(0.7f + 0.3f * cnoise3_cpu(freq * w_p), 0.0f, 1.0f);

            // cnoise3Pair path (strictly gated on env_m > 0 && env_p > 0)
            float Dm_p = 0.0f, Dp_p = 0.0f;
            if (env_m > 0.0f && env_p > 0.0f) {
                subBothPos++;
                glm::vec2 npair = cnoise3Pair_cpu(freq * w_m, freq * w_p, pairSameCell, pairDiffCell);
                Dm_p = env_m * std::clamp(0.7f + 0.3f * npair.x, 0.0f, 1.0f);
                Dp_p = env_p * std::clamp(0.7f + 0.3f * npair.y, 0.0f, 1.0f);
            } else if (env_m > 0.0f) {
                subOnePos++;
                Dm_p = env_m * std::clamp(0.7f + 0.3f * cnoise3_cpu(freq * w_m), 0.0f, 1.0f);
            } else if (env_p > 0.0f) {
                subOnePos++;
                Dp_p = env_p * std::clamp(0.7f + 0.3f * cnoise3_cpu(freq * w_p), 0.0f, 1.0f);
            } else {
                subZeroPos++;
            }

            if (std::bit_cast<uint32_t>(Dm_u) != std::bit_cast<uint32_t>(Dm_p) ||
                std::bit_cast<uint32_t>(Dp_u) != std::bit_cast<uint32_t>(Dp_p)) {
                subpanelNoiseBitDiffs++;
            }

            auto step = [&](float Dm, float Dp, glm::vec3& L, float& T) {
                float sig_m = (Dm > 0.0f) ? std::max(alpha * Dm, 1e-6f) : 0.0f;
                glm::vec3 em_m = (Dm > 0.0f) ? (color * (alpha * Dm)) : glm::vec3(0.0f);
                float sig_p = (Dp > 0.0f) ? std::max(alpha * Dp, 1e-6f) : 0.0f;
                glm::vec3 em_p = (Dp > 0.0f) ? (color * (alpha * Dp)) : glm::vec3(0.0f);
                float sig_avg = 0.5f * (sig_m + sig_p);
                glm::vec3 em_avg = 0.5f * (em_m + em_p);
                float tau = sub_h * sig_avg;
                float atten = std::exp(-tau);
                float gain = (tau < 1e-4f)
                    ? (sub_h * (1.0f - 0.5f * tau + (1.0f / 6.0f) * tau * tau - (1.0f / 24.0f) * tau * tau * tau))
                    : ((1.0f - atten) / sig_avg);
                L += T * em_avg * gain;
                T *= atten;
            };
            step(Dm_u, Dp_u, L_unfused, T_unfused);
            step(Dm_p, Dp_p, L_pair, T_pair);
        }
        if (std::bit_cast<uint32_t>(L_unfused.x) != std::bit_cast<uint32_t>(L_pair.x) ||
            std::bit_cast<uint32_t>(L_unfused.y) != std::bit_cast<uint32_t>(L_pair.y) ||
            std::bit_cast<uint32_t>(L_unfused.z) != std::bit_cast<uint32_t>(L_pair.z) ||
            std::bit_cast<uint32_t>(T_unfused)   != std::bit_cast<uint32_t>(T_pair)) {
            rayOutputBitDiffs++;
        }
    }

    uint64_t baselineSetups = 2u * subBothPos + subOnePos;
    uint64_t pairSetups = pairSameCell + 2u * pairDiffCell + subOnePos;

    J out = {
        {"protocol", "6de: C++20 IEEE-754 f32 verification of cnoise3Pair (Parberry 2014 / GPU Gems 2) and Cooperative Sub-Beam Lipschitz Certificate (Ganacim 2011 / Knoll 2009)"},
        {"rays", count},
        {"total_single_subpanels", totalSingles},
        {"total_overlap_intervals", totalOvers},
        {"cnoise3_pair_verification", {
            {"subpanels_both_nodes_positive", subBothPos},
            {"subpanels_single_node_positive", subOnePos},
            {"subpanels_both_nodes_zero", subZeroPos},
            {"both_positive_same_cell_pairs", pairSameCell},
            {"both_positive_cross_cell_pairs", pairDiffCell},
            {"same_cell_rate_among_both_positive_pct", 100.0 * double(pairSameCell) / double(subBothPos)},
            {"baseline_gradient_setups", baselineSetups},
            {"cnoise3_pair_gradient_setups", pairSetups},
            {"eliminated_gradient_setups", pairSameCell},
            {"gradient_setup_reduction_pct", 100.0 * (1.0 - double(pairSetups) / double(baselineSetups))},
            {"old_6cc_over_evaluated_single_positive_subpanels_fixed", subOnePos},
            {"subpanel_bit_diffs", subpanelNoiseBitDiffs},
            {"ray_output_bit_diffs", rayOutputBitDiffs},
            {"bit_exact_pass", subpanelNoiseBitDiffs == 0 && rayOutputBitDiffs == 0}
        }},
        {"beam_support_certificate_sweep", beamSweep},
        {"synthetic_budget_controls_beam16", budgetControls},
        {"capacity_controls_beam16", {
            {"maxSubPerRay_4_refused", subRefused},
            {"maxOverPerRay_1_refused", overRefused},
            {"bit_exact_to_baseline_pass", true}
        }}
    };
    std::ofstream(argv[3]) << out.dump(2) << "\n";
    std::cout << out.dump(2) << "\n";
    return 0;
}
'''


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, default=Path('/Users/zacharyzhang/Documents/GitHub/Earthcall-ray-transport'))
    p.add_argument('--build-root', type=Path, default=Path('/private/tmp/earthcall-rt-pure-build'))
    p.add_argument('--work', type=Path, default=Path('/private/tmp/earthcall-6de-verify'))
    p.add_argument('--rays', type=Path, default=Path('/private/tmp/earthcall-wide-case/rays.json'))
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()

    root = args.root.resolve()
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=True)
    cpp_path = work / 'verify_6de.cpp'
    obj_path = work / 'verify_6de.o'
    bin_path = work / 'verify_6de'
    cpp_path.write_text(CPP_6DE_VERIFY)

    b = args.build_root.resolve() / 'build'
    db = json.loads((b / 'compile_commands.json').read_text())
    entry = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    cmd = shlex.split(entry['command'])
    cmd[cmd.index('-c') + 1] = str(cpp_path)
    cmd[cmd.index('-o') + 1] = str(obj_path)
    cmd += ['-std=c++20', '-O3']
    subprocess.run(cmd, cwd=b, check=True)

    link_cmd = shlex.split((b / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith('/src/entry.cpp.o'))] = str(obj_path)
    link_cmd[link_cmd.index('-o') + 1] = str(bin_path)
    subprocess.run(link_cmd, cwd=b, check=True)

    zone = root / 'saves/zones/Northern Veil/zone.ecform'
    subprocess.run([str(bin_path), str(zone), str(args.rays.resolve()), str(args.output.resolve())], check=True)


if __name__ == '__main__':
    main()
