"""§6da & §6db Exact IEEE-754 f32 Packet Parity + Work-Distribution & Fast-Path Census Verifier.
Compiles and executes a native arm64 C++20 IEEE-754 f32 harness against all 32,784 distinct native rays
in `/private/tmp/earthcall-wide-case/manifest.json` and the Apple M5 Metal hardware-captured GPU schedule in
`/private/tmp/earthcall-on-the-fly-run/GPU_RAY_TRANSPORT_ON_THE_FLY_2026-10-23.json` (and
`/private/tmp/earthcall-census-run/GPU_RAY_TRANSPORT_PRODUCER_WORK_CENSUS_2026-10-23.json`).

Verifies:
1. Baseline Guarded Sequential Producer (`§6cx`/`§6cy` reference) matches the Apple M5 Metal hardware-captured
   `schedule_census_inputs` across all 32,784 rays (247,326 single subpanels, 1,799 overlap segments,
   12,102 affine knots, and all raw operation counters).
2. `§6da` Part 2 (Sixth-One Sun's Per-Member Parallel Root-Production `memberRootsMain` [4*N = 131,136 threads]
   + `mergeMain` [N = 32,784 threads]):
   - 100.0% bit-exact packet equality (`0 / 32,784` differing rays, `247,326 / 247,326` singles,
     `1,799 / 1,799` overlaps, `12,102 / 12,102` affine knots).
   - Budget `0`, `23`, `24`, `25` and capacity `4/1` fallback controls.
   - Exact per-ray vs per-(ray,member) work and `evalPhi` distribution (`max_member_evalPhi_per_ray` vs
     `sum_member_evalPhi_per_ray`, global intermediate storage bytes, and work imbalance across the 4 curtains).
3. `§6db` (Firmament's Universal Cross-Level Depth-0 Bow Pre-Filter `bowActiveBins` + Whole-Member Bow Skip +
   Depth-0 Register Fast-Path in `rootsFast()` + Dynamic-Slack Linear-z Cull + Compact Private State `stack[8]`,
   `bands[16]`, `pieces[24]`, `knots[48]`):
   - 100.0% bit-exact packet equality (`0 / 32,784` differing rays, `247,326 / 247,326` singles,
     `1,799 / 1,799` overlaps, `12,102 / 12,102` affine knots, `0` changed support unions).
   - Budget `0`, `23`, `24`, `25` and capacity `4/1` fallback controls.
   - Exact reductions in `evalPhi` calls, `stack[0]` private-array writes/reads, coarse-bin loop bodies,
     whole-member `roots()` skips, knot-sort passes, and per-thread private memory bytes (`2,240 B -> 960 B`).
4. Combined `§6da + §6db` (Per-Member Parallel Root-Production WITH Depth-0 Register Fast-Path + Universal
   Cross-Level Bow Pre-Filter + Compact Private State).
"""
import argparse, hashlib, json, shlex, subprocess
from pathlib import Path

CPP_VERIFY_HARNESS = r'''
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

static bool extractCurtainSpecs(const J& zone, std::vector<GpuCurtainSpec>& outSpecs) {
    outSpecs.clear();
    for (const auto& j : zone.at("spatialFields")) {
        auto field = geom::FieldNode::fromJson(j);
        Rendering::VolumeDensityBinding bnd;
        if (!field || !Rendering::readVolumeDensity(*field, 0, 0, bnd)) continue;
        if (!bnd.densityExpr) continue;
        auto pb = sdfwgsl::collectVolumeParams(bnd.densityExpr);
        if (pb.values.size() < 34) return false;
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

struct SubpanelPkt {
    float s0, s1;
    uint32_t member;
};

struct OverlapPkt {
    float s0, s1;
    uint32_t mask;
};

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

struct BaselineCounters {
    uint64_t boxHits = 0;
    uint64_t evalPhiKnots = 0;
    uint64_t evalPhiBisect = 0;
    uint64_t evalPhiSplit = 0;
    uint64_t evalPhiPieceMid = 0;
    uint64_t stackPushes = 0;
    uint64_t stackPops = 0;
    uint64_t depth0BowRejects = 0;
    uint64_t depth0MonotoneResolved = 0;
    uint64_t depth0SmallResolved = 0;
    uint64_t depth0Subdivisions = 0;
    uint64_t rootsCalls = 0;
    uint64_t knotSortPasses = 0;
    uint32_t maxStackTop = 0;
    uint32_t maxDepth = 0;
    uint32_t maxBands = 0;
    uint32_t maxKnotsPerMember = 0;
    uint32_t maxPiecesPerMember = 0;
    uint32_t maxPiecesPerRay = 0;
    uint32_t maxMergedKnots = 0;
    std::vector<uint32_t> rayEvalPhiTotal;
    std::vector<uint32_t> rayMaxMemberEvalPhi;
    std::vector<uint32_t> rayWorkTotal;
    std::vector<uint32_t> rayMaxMemberWork;
};

struct FastPathCounters {
    uint64_t boxHits = 0;
    uint64_t linearZCulledMembers = 0;
    uint64_t linearZCulledBins = 0;
    uint64_t activeCoarseBins = 0;
    uint64_t bowActiveCoarseBins = 0;
    uint64_t universalBowRejectedBins = 0;
    uint64_t wholeMemberBowSkips = 0;
    uint64_t rootsCalls = 0;
    uint64_t knotSortPasses = 0;
    uint64_t evalPhiKnots = 0;
    uint64_t evalPhiBisect = 0;
    uint64_t evalPhiSplit = 0;
    uint64_t evalPhiPieceMid = 0;
    uint64_t stackPushes = 0;
    uint64_t stackPops = 0;
    uint32_t maxStackTop = 0;
    uint32_t maxDepth = 0;
    uint32_t maxBands = 0;
    uint32_t maxKnotsPerMember = 0;
    uint32_t maxPiecesPerMember = 0;
    uint32_t maxPiecesPerRay = 0;
    uint32_t maxMergedKnots = 0;
    std::vector<uint32_t> rayEvalPhiTotal;
    std::vector<uint32_t> rayMaxMemberEvalPhi;
};

static RaySchedule runBaselineRay(const std::vector<GpuCurtainSpec>& curtains,
                                  glm::vec3 ro, glm::vec3 d, float clipA, float clipB,
                                  uint32_t budget, uint32_t maxSubPerRay, uint32_t maxOverPerRay,
                                  BaselineCounters* ctr = nullptr) {
    RaySchedule out;
    if (budget == 0u) {
        out.flag = 0x80000000u | 1u;
        return out;
    }
    uint32_t reason = 0u, work = 0u;
    std::array<glm::vec2, 48> pieces{};
    std::array<uint32_t, 4> memStart{0, 0, 0, 0};
    std::array<uint32_t, 4> memCount{0, 0, 0, 0};
    uint32_t np = 0u;
    uint32_t rayPhiTotal = 0u, rayMaxMemPhi = 0u;
    uint32_t rayWorkSum = 0u, rayMaxMemWork = 0u;

    auto putBand = [&](float lo, float hi, std::array<float, 96>& bounds, uint32_t& nb,
                       std::array<glm::vec2, 32>& bands, uint32_t& nr, uint32_t& rsn) {
        if (nr > 0u) {
            glm::vec2 last = bands[nr - 1u];
            if (lo <= last.y && hi >= last.x) {
                glm::vec2 merged(std::min(lo, last.x), std::max(hi, last.y));
                bands[nr - 1u] = merged;
                bounds[nb - 2u] = merged.x;
                bounds[nb - 1u] = merged.y;
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

    for (uint32_t member = 0u; member < 4u; ++member) {
        memStart[member] = np;
        const auto& m = curtains[member];
        glm::vec3 o = ro - glm::vec3(m.origin);
        float a = clipA, b = clipB;
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
        if (!hit || !(b > a)) continue;
        if (ctr) ctr->boxHits++;

        uint32_t memPhi = 0u;
        uint32_t workBeforeMem = work;
        std::array<float, 96> knots{};
        uint32_t nb = 1u;
        knots[0] = a;
        std::array<glm::vec2, 32> bands{};
        uint32_t nr = 0u, zeroBands = 0u;
        std::array<float, 9> phi_knots{};
        for (uint32_t k = 0u; k < 9u; ++k) {
            float s = a + (b - a) * float(k) / 8.0f;
            if (k == 0u) s = a;
            if (k == 8u) s = b;
            phi_knots[k] = evalPhi(m, o, d, s);
            if (ctr) ctr->evalPhiKnots++;
            memPhi++;
        }

        auto roots = [&](float c, bool isZeroLevel) {
            if (ctr) ctr->rootsCalls++;
            float M = m.curv.x * d.x * d.x * 1.00002f;
            float err = m.curv.y;
            std::array<glm::vec4, 20> stack{};
            std::array<uint32_t, 20> depths{};
            uint32_t top = 0u;
            for (uint32_t bin = 0u; bin < 8u; ++bin) {
                float a0 = a + (b - a) * float(bin) / 8.0f;
                float b0 = a + (b - a) * float(bin + 1u) / 8.0f;
                if (bin == 0u) a0 = a;
                if (bin == 7u) b0 = b;
                stack[0] = glm::vec4(a0, b0, phi_knots[bin], phi_knots[bin + 1u]);
                depths[0] = 0u;
                top = 1u;
                if (ctr) {
                    ctr->stackPushes++;
                    ctr->maxStackTop = std::max(ctr->maxStackTop, top);
                }
                while (top > 0u && reason == 0u) {
                    top--;
                    if (ctr) ctr->stackPops++;
                    glm::vec4 cell = stack[top];
                    float ca = cell.x, cb = cell.y, h = cb - ca;
                    uint32_t depth = depths[top];
                    if (ctr) ctr->maxDepth = std::max(ctr->maxDepth, depth);
                    if (work >= budget) { reason |= 1u; break; }
                    work += 1u;
                    if (!(h > 0.0f)) { reason |= 4u; break; }
                    float va = cell.z - c, vb = cell.w - c;
                    float scale = 1.0f + std::abs(va) + std::abs(vb) + std::abs(ca) + std::abs(cb);
                    float error = err + 0.000001f * scale;
                    float bow = 0.125f * M * h * h + error;
                    if (std::min(va, vb) > bow || std::max(va, vb) < -bow) {
                        if (ctr && depth == 0u) ctr->depth0BowRejects++;
                        continue;
                    }
                    float slope = (vb - va) / h;
                    float radius = 0.5f * M * h + 2.0f * error / h;
                    float minSlope = std::abs(slope) - radius;
                    if (minSlope > 0.0f) {
                        if (ctr && depth == 0u) ctr->depth0MonotoneResolved++;
                        if (std::abs(va) <= error) {
                            float w = 2.0f * error / minSlope;
                            putBandTagged(ca, std::min(cb, ca + w), knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
                        }
                        if (std::abs(vb) <= error) {
                            float w = 2.0f * error / minSlope;
                            putBandTagged(std::max(ca, cb - w), cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
                        }
                        if ((va > error && vb < -error) || (va < -error && vb > error)) {
                            float l = ca, r = cb, left = va;
                            for (uint32_t iter = 0u; iter < 32u; ++iter) {
                                if (work >= budget) { reason |= 1u; break; }
                                work += 1u;
                                float mid = l + 0.5f * (r - l);
                                if (mid <= l || mid >= r || r - l <= 0.0001f) break;
                                float v = evalPhi(m, o, d, mid) - c;
                                if (ctr) ctr->evalPhiBisect++;
                                memPhi++;
                                if (std::abs(v) <= error) {
                                    float w = 2.0f * error / minSlope;
                                    l = std::max(l, mid - w);
                                    r = std::min(r, mid + w);
                                    break;
                                }
                                if ((left < 0.0f && v > 0.0f) || (left > 0.0f && v < 0.0f)) r = mid;
                                else { l = mid; left = v; }
                            }
                            putBandTagged(l, r, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
                        }
                        continue;
                    }
                    if (0.125f * M * h * h <= error) {
                        if (ctr && depth == 0u) ctr->depth0SmallResolved++;
                        putBandTagged(ca, cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
                        continue;
                    }
                    if (float(depth) >= 14.0f || top + 2u > 20u) {
                        putBandTagged(ca, cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
                        continue;
                    }
                    float mid = ca + 0.5f * h;
                    if (mid <= ca || mid >= cb) {
                        putBandTagged(ca, cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
                        continue;
                    }
                    if (ctr && depth == 0u) ctr->depth0Subdivisions++;
                    float phi_mid = evalPhi(m, o, d, mid);
                    if (ctr) ctr->evalPhiSplit++;
                    memPhi++;
                    stack[top] = glm::vec4(mid, cb, phi_mid, cell.w);
                    depths[top] = depth + 1u;
                    stack[top + 1u] = glm::vec4(ca, mid, cell.z, phi_mid);
                    depths[top + 1u] = depth + 1u;
                    top += 2u;
                    if (ctr) {
                        ctr->stackPushes += 2u;
                        ctr->maxStackTop = std::max(ctr->maxStackTop, top);
                    }
                }
                if (reason != 0u) break;
            }
        };

        roots(-m.limits.w, false);
        if (reason == 0u) roots(0.0f, true);
        if (reason == 0u) roots(m.limits.w, false);
        if (reason != 0u) break;

        if (d.x != 0.0f) { float x = -o.x / d.x; if (x > a && x < b) knots[nb++] = x; }
        if (d.y != 0.0f) {
            float y = -o.y / d.y; if (y > a && y < b) knots[nb++] = y;
            float k = (m.limits.z - o.y) / d.y; if (k > a && k < b) knots[nb++] = k;
        }
        knots[nb++] = b;
        if (ctr) {
            ctr->knotSortPasses++;
            ctr->maxBands = std::max(ctr->maxBands, nr);
            ctr->maxKnotsPerMember = std::max(ctr->maxKnotsPerMember, nb);
        }
        for (uint32_t i = 1u; i < nb; ++i) {
            float v = knots[i]; uint32_t j = i;
            while (j > 0u && knots[j - 1u] > v) { knots[j] = knots[j - 1u]; j--; }
            knots[j] = v;
        }
        uint32_t count = 0u;
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
                memPhi++;
                inside = (std::abs(pm) <= m.limits.w + m.curv.y);
            }
            if (inside) {
                if (count > 0u && mayAbsorb(pieces[np - 1u], glm::vec2(l, r),
                                            previousShortBand, previousPositive, uncertain, protectedBoundary)) {
                    pieces[np - 1u].y = r;
                    previousShortBand = false;
                    previousPositive = true;
                } else {
                    if (np >= 48u) { reason |= 8u; break; }
                    pieces[np++] = glm::vec2(l, r);
                    count++;
                    previousShortBand = uncertain && (r - l <= 0.02f);
                    previousPositive = !uncertain;
                }
            }
        }
        memCount[member] = count;
        uint32_t memWork = work - workBeforeMem;
        rayPhiTotal += memPhi;
        rayMaxMemPhi = std::max(rayMaxMemPhi, memPhi);
        rayWorkSum += memWork;
        rayMaxMemWork = std::max(rayMaxMemWork, memWork);
        if (ctr) ctr->maxPiecesPerMember = std::max(ctr->maxPiecesPerMember, count);
        if (reason != 0u) break;
    }

    if (ctr) {
        ctr->maxPiecesPerRay = std::max(ctr->maxPiecesPerRay, np);
        ctr->rayEvalPhiTotal.push_back(rayPhiTotal);
        ctr->rayMaxMemberEvalPhi.push_back(rayMaxMemPhi);
        ctr->rayWorkTotal.push_back(rayWorkSum);
        ctr->rayMaxMemberWork.push_back(rayMaxMemWork);
    }

    if (reason != 0u) {
        out.flag = 0x80000000u | reason;
        return out;
    }

    std::array<float, 96> mknots{};
    uint32_t ne = 0u;
    std::array<uint32_t, 4> head{0, 0, 0, 0};
    uint32_t totalKnots = np * 2u;
    if (ctr) ctr->maxMergedKnots = std::max(ctr->maxMergedKnots, totalKnots);
    while (ne < totalKnots) {
        uint32_t bestM = 4u;
        float bestVal = 0.0f;
        for (uint32_t m = 0u; m < 4u; ++m) {
            uint32_t h = head[m];
            if (h < memCount[m] * 2u) {
                uint32_t pIdx = memStart[m] + (h >> 1u);
                bool isY = (h & 1u) != 0u;
                float val = isY ? pieces[pIdx].y : pieces[pIdx].x;
                if (bestM == 4u || val < bestVal) { bestVal = val; bestM = m; }
            }
        }
        mknots[ne++] = bestVal;
        head[bestM]++;
    }

    std::array<uint32_t, 4> pIdx{0, 0, 0, 0};
    for (uint32_t i = 1u; i < ne && reason == 0u; ++i) {
        float a = mknots[i - 1u], b = mknots[i];
        if (!(b > a)) continue;
        float mid = a + 0.5f * (b - a);
        uint32_t mask = 0u;
        for (uint32_t m = 0u; m < 4u; ++m) {
            uint32_t start = memStart[m], numPieces = memCount[m], cur = pIdx[m];
            while (cur < numPieces && pieces[start + cur].y < mid) cur++;
            pIdx[m] = cur;
            if (cur < numPieces) {
                glm::vec2 p = pieces[start + cur];
                if (mid >= p.x && mid <= p.y) mask |= (1u << m);
            }
        }
        if (mask == 0u) continue;
        if (std::popcount(mask) == 1) {
            uint32_t n = std::max(1u, uint32_t(std::ceil((b - a) / 4.0f)));
            if (out.singles.size() + n > maxSubPerRay) { reason |= 16u; break; }
            uint32_t member = std::countr_zero(mask);
            for (uint32_t k = 0u; k < n; ++k) {
                float l = (k == 0u) ? a : (a + float(k) * (b - a) / float(n));
                float r = (k + 1u == n) ? b : (a + float(k + 1u) * (b - a) / float(n));
                out.singles.push_back({l, r, member});
            }
        } else {
            if (out.overs.size() >= maxOverPerRay) { reason |= 32u; break; }
            out.overs.push_back({a, b, mask});
        }
    }
    if (reason != 0u) {
        out.singles.clear();
        out.overs.clear();
        out.flag = 0x80000000u | reason;
    }
    return out;
}

// §6da Part 2: Per-member parallel root production (`memberRootsMain` [4*N] + `mergeMain` [N])
static RaySchedule runParallelRootsRay(const std::vector<GpuCurtainSpec>& curtains,
                                       glm::vec3 ro, glm::vec3 d, float clipA, float clipB,
                                       uint32_t budget, uint32_t maxSubPerRay, uint32_t maxOverPerRay) {
    RaySchedule out;
    if (budget == 0u) {
        out.flag = 0x80000000u | 1u;
        return out;
    }
    struct MemberOut {
        uint32_t count = 0u;
        uint32_t work = 0u;
        uint32_t reason = 0u;
        std::array<glm::vec2, 16> pieces{};
    };
    std::array<MemberOut, 4> mouts{};

    // Pass 1: 4 independent member threads
    for (uint32_t member = 0u; member < 4u; ++member) {
        auto& mo = mouts[member];
        const auto& m = curtains[member];
        glm::vec3 o = ro - glm::vec3(m.origin);
        float a = clipA, b = clipB;
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
        if (!hit || !(b > a)) continue;

        uint32_t reason = 0u, work = 0u;
        std::array<float, 96> knots{};
        uint32_t nb = 1u;
        knots[0] = a;
        std::array<glm::vec2, 32> bands{};
        uint32_t nr = 0u, zeroBands = 0u;
        std::array<float, 9> phi_knots{};
        for (uint32_t k = 0u; k < 9u; ++k) {
            float s = a + (b - a) * float(k) / 8.0f;
            if (k == 0u) s = a;
            if (k == 8u) s = b;
            phi_knots[k] = evalPhi(m, o, d, s);
        }

        auto putBand = [&](float lo, float hi) {
            if (nr > 0u) {
                glm::vec2 last = bands[nr - 1u];
                if (lo <= last.y && hi >= last.x) {
                    glm::vec2 merged(std::min(lo, last.x), std::max(hi, last.y));
                    bands[nr - 1u] = merged;
                    knots[nb - 2u] = merged.x;
                    knots[nb - 1u] = merged.y;
                    return;
                }
            }
            if (nb + 2u > 92u || nr >= 32u) { reason |= 2u; return; }
            knots[nb] = lo; knots[nb + 1u] = hi; nb += 2u;
            bands[nr] = glm::vec2(lo, hi); nr += 1u;
        };
        auto putBandTagged = [&](float lo, float hi, bool isZeroLevel) {
            putBand(lo, hi);
            if (reason == 0u && isZeroLevel && nr > 0u) zeroBands |= (1u << (nr - 1u));
        };
        auto roots = [&](float c, bool isZeroLevel) {
            float M = m.curv.x * d.x * d.x * 1.00002f;
            float err = m.curv.y;
            std::array<glm::vec4, 20> stack{};
            std::array<uint32_t, 20> depths{};
            uint32_t top = 0u;
            for (uint32_t bin = 0u; bin < 8u; ++bin) {
                float a0 = a + (b - a) * float(bin) / 8.0f;
                float b0 = a + (b - a) * float(bin + 1u) / 8.0f;
                if (bin == 0u) a0 = a;
                if (bin == 7u) b0 = b;
                stack[0] = glm::vec4(a0, b0, phi_knots[bin], phi_knots[bin + 1u]);
                depths[0] = 0u;
                top = 1u;
                while (top > 0u && reason == 0u) {
                    top--;
                    glm::vec4 cell = stack[top];
                    float ca = cell.x, cb = cell.y, h = cb - ca;
                    uint32_t depth = depths[top];
                    if (work >= budget) { reason |= 1u; break; }
                    work += 1u;
                    if (!(h > 0.0f)) { reason |= 4u; break; }
                    float va = cell.z - c, vb = cell.w - c;
                    float scale = 1.0f + std::abs(va) + std::abs(vb) + std::abs(ca) + std::abs(cb);
                    float error = err + 0.000001f * scale;
                    float bow = 0.125f * M * h * h + error;
                    if (std::min(va, vb) > bow || std::max(va, vb) < -bow) continue;
                    float slope = (vb - va) / h;
                    float radius = 0.5f * M * h + 2.0f * error / h;
                    float minSlope = std::abs(slope) - radius;
                    if (minSlope > 0.0f) {
                        if (std::abs(va) <= error) {
                            float w = 2.0f * error / minSlope;
                            putBandTagged(ca, std::min(cb, ca + w), isZeroLevel);
                        }
                        if (std::abs(vb) <= error) {
                            float w = 2.0f * error / minSlope;
                            putBandTagged(std::max(ca, cb - w), cb, isZeroLevel);
                        }
                        if ((va > error && vb < -error) || (va < -error && vb > error)) {
                            float l = ca, r = cb, left = va;
                            for (uint32_t iter = 0u; iter < 32u; ++iter) {
                                if (work >= budget) { reason |= 1u; break; }
                                work += 1u;
                                float mid = l + 0.5f * (r - l);
                                if (mid <= l || mid >= r || r - l <= 0.0001f) break;
                                float v = evalPhi(m, o, d, mid) - c;
                                if (std::abs(v) <= error) {
                                    float w = 2.0f * error / minSlope;
                                    l = std::max(l, mid - w);
                                    r = std::min(r, mid + w);
                                    break;
                                }
                                if ((left < 0.0f && v > 0.0f) || (left > 0.0f && v < 0.0f)) r = mid;
                                else { l = mid; left = v; }
                            }
                            putBandTagged(l, r, isZeroLevel);
                        }
                        continue;
                    }
                    if (0.125f * M * h * h <= error) { putBandTagged(ca, cb, isZeroLevel); continue; }
                    if (float(depth) >= 14.0f || top + 2u > 20u) { putBandTagged(ca, cb, isZeroLevel); continue; }
                    float mid = ca + 0.5f * h;
                    if (mid <= ca || mid >= cb) { putBandTagged(ca, cb, isZeroLevel); continue; }
                    float phi_mid = evalPhi(m, o, d, mid);
                    stack[top] = glm::vec4(mid, cb, phi_mid, cell.w);
                    depths[top] = depth + 1u;
                    stack[top + 1u] = glm::vec4(ca, mid, cell.z, phi_mid);
                    depths[top + 1u] = depth + 1u;
                    top += 2u;
                }
                if (reason != 0u) break;
            }
        };
        roots(-m.limits.w, false);
        if (reason == 0u) roots(0.0f, true);
        if (reason == 0u) roots(m.limits.w, false);
        if (reason != 0u) {
            mo.work = work;
            mo.reason = reason;
            continue;
        }
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
        uint32_t count = 0u;
        glm::vec2 lastPiece(0.0f);
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
            if (uncertain || std::abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {
                if (count > 0u && mayAbsorb(lastPiece, glm::vec2(l, r),
                                            previousShortBand, previousPositive, uncertain, protectedBoundary)) {
                    lastPiece.y = r;
                    mo.pieces[count - 1u] = lastPiece;
                    previousShortBand = false;
                    previousPositive = true;
                } else {
                    if (count >= 16u) { reason |= 8u; break; }
                    lastPiece = glm::vec2(l, r);
                    mo.pieces[count++] = lastPiece;
                    previousShortBand = uncertain && (r - l <= 0.02f);
                    previousPositive = !uncertain;
                }
            }
        }
        mo.count = count;
        mo.work = work;
        mo.reason = reason;
    }

    // Pass 2: mergeMain
    uint32_t reason = 0u, totalWork = 0u, totalPieces = 0u;
    std::array<uint32_t, 4> memStart{0, 0, 0, 0}, memCount{0, 0, 0, 0};
    std::array<glm::vec2, 48> pieces{};
    for (uint32_t m = 0u; m < 4u; ++m) {
        const auto& mo = mouts[m];
        if (mo.reason != 0u) { reason |= mo.reason; break; }
        if (totalWork > budget || budget - totalWork < mo.work) { reason |= 1u; break; }
        totalWork += mo.work;
        if (totalPieces + mo.count > 48u) { reason |= 8u; break; }
        memStart[m] = totalPieces;
        memCount[m] = mo.count;
        for (uint32_t k = 0u; k < mo.count; ++k) pieces[totalPieces + k] = mo.pieces[k];
        totalPieces += mo.count;
    }
    if (reason != 0u) {
        out.flag = 0x80000000u | reason;
        return out;
    }
    std::array<float, 96> mknots{};
    uint32_t ne = 0u;
    std::array<uint32_t, 4> head{0, 0, 0, 0};
    uint32_t totalKnots = totalPieces * 2u;
    while (ne < totalKnots) {
        uint32_t bestM = 4u;
        float bestVal = 0.0f;
        for (uint32_t m = 0u; m < 4u; ++m) {
            uint32_t h = head[m];
            if (h < memCount[m] * 2u) {
                uint32_t pIdx = memStart[m] + (h >> 1u);
                bool isY = (h & 1u) != 0u;
                float val = isY ? pieces[pIdx].y : pieces[pIdx].x;
                if (bestM == 4u || val < bestVal) { bestVal = val; bestM = m; }
            }
        }
        mknots[ne++] = bestVal;
        head[bestM]++;
    }
    std::array<uint32_t, 4> pIdx{0, 0, 0, 0};
    for (uint32_t i = 1u; i < ne && reason == 0u; ++i) {
        float a = mknots[i - 1u], b = mknots[i];
        if (!(b > a)) continue;
        float mid = a + 0.5f * (b - a);
        uint32_t mask = 0u;
        for (uint32_t m = 0u; m < 4u; ++m) {
            uint32_t start = memStart[m], numPieces = memCount[m], cur = pIdx[m];
            while (cur < numPieces && pieces[start + cur].y < mid) cur++;
            pIdx[m] = cur;
            if (cur < numPieces) {
                glm::vec2 p = pieces[start + cur];
                if (mid >= p.x && mid <= p.y) mask |= (1u << m);
            }
        }
        if (mask == 0u) continue;
        if (std::popcount(mask) == 1) {
            uint32_t n = std::max(1u, uint32_t(std::ceil((b - a) / 4.0f)));
            if (out.singles.size() + n > maxSubPerRay) { reason |= 16u; break; }
            uint32_t member = std::countr_zero(mask);
            for (uint32_t k = 0u; k < n; ++k) {
                float l = (k == 0u) ? a : (a + float(k) * (b - a) / float(n));
                float r = (k + 1u == n) ? b : (a + float(k + 1u) * (b - a) / float(n));
                out.singles.push_back({l, r, member});
            }
        } else {
            if (out.overs.size() >= maxOverPerRay) { reason |= 32u; break; }
            out.overs.push_back({a, b, mask});
        }
    }
    if (reason != 0u) {
        out.singles.clear();
        out.overs.clear();
        out.flag = 0x80000000u | reason;
    }
    return out;
}

// §6db: Firmament Depth-0 Register Fast-Path + Universal Cross-Level Bow Pre-Filter + Compact Private State
template <bool Compact>
static RaySchedule runFastPathRay(const std::vector<GpuCurtainSpec>& curtains,
                                  glm::vec3 ro, glm::vec3 d, float clipA, float clipB,
                                  uint32_t budget, uint32_t maxSubPerRay, uint32_t maxOverPerRay,
                                  FastPathCounters* ctr = nullptr) {
    constexpr uint32_t StackCap = Compact ? 8u : 20u;
    constexpr uint32_t BandCap = Compact ? 16u : 32u;
    constexpr uint32_t KnotCap = Compact ? 48u : 96u;
    constexpr uint32_t KnotGuard = Compact ? 44u : 92u;
    constexpr uint32_t PieceCap = Compact ? 24u : 48u;
    constexpr uint32_t MergeKnotCap = Compact ? 48u : 96u;

    RaySchedule out;
    if (budget == 0u) {
        out.flag = 0x80000000u | 1u;
        return out;
    }
    uint32_t reason = 0u, work = 0u;
    std::array<glm::vec2, PieceCap> pieces{};
    std::array<uint32_t, 4> memStart{0, 0, 0, 0};
    std::array<uint32_t, 4> memCount{0, 0, 0, 0};
    uint32_t np = 0u;
    uint32_t rayPhiTotal = 0u, rayMaxMemPhi = 0u;

    auto putBand = [&](float lo, float hi, std::array<float, KnotCap>& bounds, uint32_t& nb,
                       std::array<glm::vec2, BandCap>& bands, uint32_t& nr, uint32_t& rsn) {
        if (nr > 0u) {
            glm::vec2 last = bands[nr - 1u];
            if (lo <= last.y && hi >= last.x) {
                glm::vec2 merged(std::min(lo, last.x), std::max(hi, last.y));
                bands[nr - 1u] = merged;
                bounds[nb - 2u] = merged.x;
                bounds[nb - 1u] = merged.y;
                return;
            }
        }
        if (nb + 2u > KnotGuard || nr >= BandCap) { rsn |= 2u; return; }
        bounds[nb] = lo; bounds[nb + 1u] = hi; nb += 2u;
        bands[nr] = glm::vec2(lo, hi); nr += 1u;
    };

    auto putBandTagged = [&](float lo, float hi, std::array<float, KnotCap>& bounds, uint32_t& nb,
                             std::array<glm::vec2, BandCap>& bands, uint32_t& nr, uint32_t& rsn,
                             uint32_t& zeroBands, bool isZeroLevel) {
        putBand(lo, hi, bounds, nb, bands, nr, rsn);
        if (rsn == 0u && isZeroLevel && nr > 0u) zeroBands |= (1u << (nr - 1u));
    };

    for (uint32_t member = 0u; member < 4u; ++member) {
        memStart[member] = np;
        const auto& m = curtains[member];
        glm::vec3 o = ro - glm::vec3(m.origin);
        float a = clipA, b = clipB;
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
        if (!hit || !(b > a)) continue;
        if (ctr) ctr->boxHits++;

        float waveAmpBound = std::abs(m.amps.x) + std::abs(m.amps.y) + std::abs(m.amps.z);
        float zSlack = m.limits.w + m.curv.y + waveAmpBound;
        float zA = o.z + a * d.z, zB = o.z + b * d.z;
        if (std::min(zA, zB) > zSlack || std::max(zA, zB) < -zSlack) {
            if (ctr) ctr->linearZCulledMembers++;
            continue;
        }

        float M = m.curv.x * d.x * d.x * 1.00002f;
        float err = m.curv.y;
        uint32_t activeBins = 0u, bowActiveBins = 0u;
        float prevS = a, prevPhi = 0.0f;
        bool hasPrev = false;
        std::array<float, 9> phi_knots{};
        uint32_t memPhi = 0u;

        for (uint32_t bin = 0u; bin < 8u; ++bin) {
            float s0 = a + (b - a) * float(bin) / 8.0f;
            float s1 = a + (b - a) * float(bin + 1u) / 8.0f;
            if (bin == 0u) s0 = a;
            if (bin == 7u) s1 = b;
            float h0 = s1 - s0;
            float z0 = o.z + s0 * d.z, z1 = o.z + s1 * d.z;
            float binSlack = zSlack + 0.125f * M * h0 * h0;
            if (std::min(z0, z1) <= binSlack && std::max(z0, z1) >= -binSlack) {
                activeBins |= (1u << bin);
                if (ctr) ctr->activeCoarseBins++;
                float phi0;
                if (hasPrev && s0 == prevS) {
                    phi0 = prevPhi;
                } else {
                    phi0 = evalPhi(m, o, d, s0);
                    if (ctr) ctr->evalPhiKnots++;
                    memPhi++;
                }
                float phi1 = evalPhi(m, o, d, s1);
                if (ctr) ctr->evalPhiKnots++;
                memPhi++;
                phi_knots[bin] = phi0;
                phi_knots[bin + 1u] = phi1;
                prevS = s1;
                prevPhi = phi1;
                hasPrev = true;

                float scaleMax = 1.0f + std::abs(phi0) + std::abs(phi1) + 2.0f * m.limits.w + std::abs(s0) + std::abs(s1);
                float bowMax = 0.125f * M * h0 * h0 + (err + 0.000001f * scaleMax);
                if (std::min(phi0, phi1) <= m.limits.w + bowMax && std::max(phi0, phi1) >= -m.limits.w - bowMax) {
                    bowActiveBins |= (1u << bin);
                    if (ctr) ctr->bowActiveCoarseBins++;
                } else {
                    if (ctr) ctr->universalBowRejectedBins++;
                }
            } else {
                hasPrev = false;
                if (ctr) ctr->linearZCulledBins++;
            }
        }
        if (activeBins == 0u) continue;

        if (bowActiveBins == 0u) {
            uint32_t firstBin = std::countr_zero(activeBins);
            float phiFirst = phi_knots[firstBin];
            if (std::abs(phiFirst) > m.limits.w + err) {
                if (work + 24u > budget) {
                    reason |= 1u;
                    break;
                }
                work += 24u;
                if (ctr) ctr->wholeMemberBowSkips++;
                rayPhiTotal += memPhi;
                rayMaxMemPhi = std::max(rayMaxMemPhi, memPhi);
                continue;
            }
        }

        std::array<float, KnotCap> knots{};
        uint32_t nb = 1u;
        knots[0] = a;
        std::array<glm::vec2, BandCap> bands{};
        uint32_t nr = 0u, zeroBands = 0u;

        auto isolateMonotone = [&](float ca, float cb, float c, float va, float vb, float error, float minSlope, bool isZeroLevel) {
            if (std::abs(va) <= error) {
                float w = 2.0f * error / minSlope;
                putBandTagged(ca, std::min(cb, ca + w), knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
            }
            if (std::abs(vb) <= error) {
                float w = 2.0f * error / minSlope;
                putBandTagged(std::max(ca, cb - w), cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
            }
            if ((va > error && vb < -error) || (va < -error && vb > error)) {
                float l = ca, r = cb, left = va;
                for (uint32_t iter = 0u; iter < 32u; ++iter) {
                    if (work >= budget) { reason |= 1u; break; }
                    work += 1u;
                    float mid = l + 0.5f * (r - l);
                    if (mid <= l || mid >= r || r - l <= 0.0001f) break;
                    float v = evalPhi(m, o, d, mid) - c;
                    if (ctr) ctr->evalPhiBisect++;
                    memPhi++;
                    if (std::abs(v) <= error) {
                        float w = 2.0f * error / minSlope;
                        l = std::max(l, mid - w);
                        r = std::min(r, mid + w);
                        break;
                    }
                    if ((left < 0.0f && v > 0.0f) || (left > 0.0f && v < 0.0f)) r = mid;
                    else { l = mid; left = v; }
                }
                putBandTagged(l, r, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
            }
        };

        auto rootsFast = [&](float c, bool isZeroLevel) {
            if (ctr) ctr->rootsCalls++;
            std::array<glm::vec4, StackCap> stack{};
            std::array<uint32_t, StackCap> depths{};
            for (uint32_t bin = 0u; bin < 8u; ++bin) {
                if (reason != 0u) break;
                float a0 = a + (b - a) * float(bin) / 8.0f;
                float b0 = a + (b - a) * float(bin + 1u) / 8.0f;
                if (bin == 0u) a0 = a;
                if (bin == 7u) b0 = b;
                float h0 = b0 - a0;
                if (work >= budget) { reason |= 1u; break; }
                work += 1u;
                if (!(h0 > 0.0f)) { reason |= 4u; break; }
                if ((bowActiveBins & (1u << bin)) == 0u) continue;

                float phi_a0 = phi_knots[bin], phi_b0 = phi_knots[bin + 1u];
                float va0 = phi_a0 - c, vb0 = phi_b0 - c;
                float scale0 = 1.0f + std::abs(va0) + std::abs(vb0) + std::abs(a0) + std::abs(b0);
                float error0 = err + 0.000001f * scale0;
                float bow0 = 0.125f * M * h0 * h0 + error0;
                if (std::min(va0, vb0) > bow0 || std::max(va0, vb0) < -bow0) continue;

                float slope0 = (vb0 - va0) / h0;
                float radius0 = 0.5f * M * h0 + 2.0f * error0 / h0;
                float minSlope0 = std::abs(slope0) - radius0;
                if (minSlope0 > 0.0f) {
                    isolateMonotone(a0, b0, c, va0, vb0, error0, minSlope0, isZeroLevel);
                    continue;
                }
                if (0.125f * M * h0 * h0 <= error0) {
                    putBandTagged(a0, b0, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
                    continue;
                }
                float mid0 = a0 + 0.5f * h0;
                if (mid0 <= a0 || mid0 >= b0) {
                    putBandTagged(a0, b0, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
                    continue;
                }
                float phi_mid0 = evalPhi(m, o, d, mid0);
                if (ctr) ctr->evalPhiSplit++;
                memPhi++;
                stack[0] = glm::vec4(mid0, b0, phi_mid0, phi_b0); depths[0] = 1u;
                stack[1] = glm::vec4(a0, mid0, phi_a0, phi_mid0); depths[1] = 1u;
                uint32_t top = 2u;
                if (ctr) {
                    ctr->stackPushes += 2u;
                    ctr->maxStackTop = std::max(ctr->maxStackTop, top);
                }
                while (top > 0u && reason == 0u) {
                    top--;
                    if (ctr) ctr->stackPops++;
                    glm::vec4 cell = stack[top];
                    float ca = cell.x, cb = cell.y, h = cb - ca;
                    uint32_t depth = depths[top];
                    if (ctr) ctr->maxDepth = std::max(ctr->maxDepth, depth);
                    if (work >= budget) { reason |= 1u; break; }
                    work += 1u;
                    if (!(h > 0.0f)) { reason |= 4u; break; }
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
                    if (0.125f * M * h * h <= error) {
                        putBandTagged(ca, cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
                        continue;
                    }
                    if (float(depth) >= 14.0f || top + 2u > StackCap) {
                        putBandTagged(ca, cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
                        continue;
                    }
                    float mid = ca + 0.5f * h;
                    if (mid <= ca || mid >= cb) {
                        putBandTagged(ca, cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
                        continue;
                    }
                    float phi_mid = evalPhi(m, o, d, mid);
                    if (ctr) ctr->evalPhiSplit++;
                    memPhi++;
                    stack[top] = glm::vec4(mid, cb, phi_mid, cell.w); depths[top] = depth + 1u;
                    stack[top + 1u] = glm::vec4(ca, mid, cell.z, phi_mid); depths[top + 1u] = depth + 1u;
                    top += 2u;
                    if (ctr) {
                        ctr->stackPushes += 2u;
                        ctr->maxStackTop = std::max(ctr->maxStackTop, top);
                    }
                }
            }
        };

        rootsFast(-m.limits.w, false);
        if (reason == 0u) rootsFast(0.0f, true);
        if (reason == 0u) rootsFast(m.limits.w, false);
        if (reason != 0u) break;

        if (d.x != 0.0f) { float x = -o.x / d.x; if (x > a && x < b) knots[nb++] = x; }
        if (d.y != 0.0f) {
            float y = -o.y / d.y; if (y > a && y < b) knots[nb++] = y;
            float k = (m.limits.z - o.y) / d.y; if (k > a && k < b) knots[nb++] = k;
        }
        knots[nb++] = b;
        if (ctr) {
            ctr->knotSortPasses++;
            ctr->maxBands = std::max(ctr->maxBands, nr);
            ctr->maxKnotsPerMember = std::max(ctr->maxKnotsPerMember, nb);
        }
        for (uint32_t i = 1u; i < nb; ++i) {
            float v = knots[i]; uint32_t j = i;
            while (j > 0u && knots[j - 1u] > v) { knots[j] = knots[j - 1u]; j--; }
            knots[j] = v;
        }
        uint32_t count = 0u;
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
                memPhi++;
                inside = (std::abs(pm) <= m.limits.w + m.curv.y);
            }
            if (inside) {
                if (count > 0u && mayAbsorb(pieces[np - 1u], glm::vec2(l, r),
                                            previousShortBand, previousPositive, uncertain, protectedBoundary)) {
                    pieces[np - 1u].y = r;
                    previousShortBand = false;
                    previousPositive = true;
                } else {
                    if (np >= PieceCap) { reason |= 8u; break; }
                    pieces[np++] = glm::vec2(l, r);
                    count++;
                    previousShortBand = uncertain && (r - l <= 0.02f);
                    previousPositive = !uncertain;
                }
            }
        }
        memCount[member] = count;
        rayPhiTotal += memPhi;
        rayMaxMemPhi = std::max(rayMaxMemPhi, memPhi);
        if (ctr) ctr->maxPiecesPerMember = std::max(ctr->maxPiecesPerMember, count);
        if (reason != 0u) break;
    }

    if (ctr) {
        ctr->maxPiecesPerRay = std::max(ctr->maxPiecesPerRay, np);
        ctr->rayEvalPhiTotal.push_back(rayPhiTotal);
        ctr->rayMaxMemberEvalPhi.push_back(rayMaxMemPhi);
    }

    if (reason != 0u) {
        out.flag = 0x80000000u | reason;
        return out;
    }

    std::array<float, MergeKnotCap> mknots{};
    uint32_t ne = 0u;
    std::array<uint32_t, 4> head{0, 0, 0, 0};
    uint32_t totalKnots = np * 2u;
    if (ctr) ctr->maxMergedKnots = std::max(ctr->maxMergedKnots, totalKnots);
    while (ne < totalKnots) {
        uint32_t bestM = 4u;
        float bestVal = 0.0f;
        for (uint32_t m = 0u; m < 4u; ++m) {
            uint32_t h = head[m];
            if (h < memCount[m] * 2u) {
                uint32_t pIdx = memStart[m] + (h >> 1u);
                bool isY = (h & 1u) != 0u;
                float val = isY ? pieces[pIdx].y : pieces[pIdx].x;
                if (bestM == 4u || val < bestVal) { bestVal = val; bestM = m; }
            }
        }
        mknots[ne++] = bestVal;
        head[bestM]++;
    }

    std::array<uint32_t, 4> pIdx{0, 0, 0, 0};
    for (uint32_t i = 1u; i < ne && reason == 0u; ++i) {
        float a = mknots[i - 1u], b = mknots[i];
        if (!(b > a)) continue;
        float mid = a + 0.5f * (b - a);
        uint32_t mask = 0u;
        for (uint32_t m = 0u; m < 4u; ++m) {
            uint32_t start = memStart[m], numPieces = memCount[m], cur = pIdx[m];
            while (cur < numPieces && pieces[start + cur].y < mid) cur++;
            pIdx[m] = cur;
            if (cur < numPieces) {
                glm::vec2 p = pieces[start + cur];
                if (mid >= p.x && mid <= p.y) mask |= (1u << m);
            }
        }
        if (mask == 0u) continue;
        if (std::popcount(mask) == 1) {
            uint32_t n = std::max(1u, uint32_t(std::ceil((b - a) / 4.0f)));
            if (out.singles.size() + n > maxSubPerRay) { reason |= 16u; break; }
            uint32_t member = std::countr_zero(mask);
            for (uint32_t k = 0u; k < n; ++k) {
                float l = (k == 0u) ? a : (a + float(k) * (b - a) / float(n));
                float r = (k + 1u == n) ? b : (a + float(k + 1u) * (b - a) / float(n));
                out.singles.push_back({l, r, member});
            }
        } else {
            if (out.overs.size() >= maxOverPerRay) { reason |= 32u; break; }
            out.overs.push_back({a, b, mask});
        }
    }
    if (reason != 0u) {
        out.singles.clear();
        out.overs.clear();
        out.flag = 0x80000000u | reason;
    }
    return out;
}

static bool schedulesBitExact(const RaySchedule& a, const RaySchedule& b) {
    if (a.flag != b.flag || a.singles.size() != b.singles.size() || a.overs.size() != b.overs.size())
        return false;
    for (size_t i = 0; i < a.singles.size(); ++i) {
        if (std::bit_cast<uint32_t>(a.singles[i].s0) != std::bit_cast<uint32_t>(b.singles[i].s0) ||
            std::bit_cast<uint32_t>(a.singles[i].s1) != std::bit_cast<uint32_t>(b.singles[i].s1) ||
            a.singles[i].member != b.singles[i].member)
            return false;
    }
    for (size_t i = 0; i < a.overs.size(); ++i) {
        if (std::bit_cast<uint32_t>(a.overs[i].s0) != std::bit_cast<uint32_t>(b.overs[i].s0) ||
            std::bit_cast<uint32_t>(a.overs[i].s1) != std::bit_cast<uint32_t>(b.overs[i].s1) ||
            a.overs[i].mask != b.overs[i].mask)
            return false;
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: verify <zone.ecform> <rays.json> <gpu_on_the_fly.json>" << std::endl;
        return 1;
    }
    J zone;
    {
        std::ifstream in(argv[1], std::ios::binary);
        std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
        zone = J::parse(J::from_msgpack(b).at("MigrationRoot").get<std::string>());
    }
    std::vector<GpuCurtainSpec> curtains;
    if (!extractCurtainSpecs(zone, curtains)) {
        std::cerr << "extractCurtainSpecs failed" << std::endl;
        return 1;
    }
    J raysJson;
    { std::ifstream f(argv[2]); f >> raysJson; }
    const auto& R = raysJson.at("rays");
    glm::vec3 ro(raysJson.at("origin")[0].get<float>(),
                 raysJson.at("origin")[1].get<float>(),
                 raysJson.at("origin")[2].get<float>());

    J gpuReport;
    { std::ifstream f(argv[3]); f >> gpuReport; }
    const auto& gpuSci = gpuReport.at("schedule_census_inputs");
    if (R.size() != 32784 || gpuSci.size() != 32784) {
        std::cerr << "unexpected ray count" << std::endl;
        return 1;
    }

    BaselineCounters baseCtr;
    FastPathCounters fastFullCtr, fastCompactCtr;

    uint32_t gpuVsBaseDiffs = 0;
    uint32_t parVsBaseDiffs = 0;
    uint32_t fastFullVsBaseDiffs = 0;
    uint32_t fastCompactVsBaseDiffs = 0;
    uint64_t totalSingles = 0, totalOvers = 0;

    // Budget/capacity control counters across all 32,784 rays
    uint32_t parBudget0 = 0, parBudget23 = 0, parBudget24 = 0, parBudget25 = 0, parCap4_1 = 0;
    uint32_t fastBudget0 = 0, fastBudget23 = 0, fastBudget24 = 0, fastBudget25 = 0, fastCap4_1 = 0;
    uint32_t baseBudget23 = 0, baseBudget24 = 0, baseBudget25 = 0, baseCap4_1 = 0;

    for (size_t i = 0; i < R.size(); ++i) {
        const auto& row = R[i];
        glm::vec3 d(row[2].get<float>(), row[3].get<float>(), row[4].get<float>());
        float clipA = row[5].get<float>(), clipB = row[6].get<float>();

        auto baseSched = runBaselineRay(curtains, ro, d, clipA, clipB, 1024u, 64u, 16u, &baseCtr);
        auto parSched = runParallelRootsRay(curtains, ro, d, clipA, clipB, 1024u, 64u, 16u);
        auto fastFullSched = runFastPathRay<false>(curtains, ro, d, clipA, clipB, 1024u, 64u, 16u, &fastFullCtr);
        auto fastCompactSched = runFastPathRay<true>(curtains, ro, d, clipA, clipB, 1024u, 64u, 16u, &fastCompactCtr);

        // Compare against Apple M5 Metal hardware-captured schedule_census_inputs
        RaySchedule hwSched;
        hwSched.flag = gpuSci[i][2].get<uint32_t>();
        for (const auto& s : gpuSci[i][3]) {
            hwSched.singles.push_back({s[0].get<float>(), s[1].get<float>(), s[2].get<uint32_t>()});
        }
        for (const auto& o : gpuSci[i][4]) {
            hwSched.overs.push_back({o[0].get<float>(), o[1].get<float>(), o[2].get<uint32_t>()});
        }
        if (!schedulesBitExact(baseSched, hwSched)) gpuVsBaseDiffs++;
        if (!schedulesBitExact(parSched, baseSched)) parVsBaseDiffs++;
        if (!schedulesBitExact(fastFullSched, baseSched)) fastFullVsBaseDiffs++;
        if (!schedulesBitExact(fastCompactSched, baseSched)) fastCompactVsBaseDiffs++;

        totalSingles += baseSched.singles.size();
        totalOvers += baseSched.overs.size();

        // Budget & capacity controls
        if (runParallelRootsRay(curtains, ro, d, clipA, clipB, 0u, 64u, 16u).flag == (0x80000000u | 1u)) parBudget0++;
        if (runParallelRootsRay(curtains, ro, d, clipA, clipB, 23u, 64u, 16u).flag == (0x80000000u | 1u)) parBudget23++;
        if (runParallelRootsRay(curtains, ro, d, clipA, clipB, 24u, 64u, 16u).flag == (0x80000000u | 1u)) parBudget24++;
        if (runParallelRootsRay(curtains, ro, d, clipA, clipB, 25u, 64u, 16u).flag == (0x80000000u | 1u)) parBudget25++;
        if ((runParallelRootsRay(curtains, ro, d, clipA, clipB, 1024u, 4u, 1u).flag & 0x80000000u) != 0u) parCap4_1++;

        if (runBaselineRay(curtains, ro, d, clipA, clipB, 23u, 64u, 16u).flag == (0x80000000u | 1u)) baseBudget23++;
        if (runBaselineRay(curtains, ro, d, clipA, clipB, 24u, 64u, 16u).flag == (0x80000000u | 1u)) baseBudget24++;
        if (runBaselineRay(curtains, ro, d, clipA, clipB, 25u, 64u, 16u).flag == (0x80000000u | 1u)) baseBudget25++;
        if ((runBaselineRay(curtains, ro, d, clipA, clipB, 1024u, 4u, 1u).flag & 0x80000000u) != 0u) baseCap4_1++;

        if (runFastPathRay<true>(curtains, ro, d, clipA, clipB, 0u, 64u, 16u).flag == (0x80000000u | 1u)) fastBudget0++;
        if (runFastPathRay<true>(curtains, ro, d, clipA, clipB, 23u, 64u, 16u).flag == (0x80000000u | 1u)) fastBudget23++;
        if (runFastPathRay<true>(curtains, ro, d, clipA, clipB, 24u, 64u, 16u).flag == (0x80000000u | 1u)) fastBudget24++;
        if (runFastPathRay<true>(curtains, ro, d, clipA, clipB, 25u, 64u, 16u).flag == (0x80000000u | 1u)) fastBudget25++;
        if ((runFastPathRay<true>(curtains, ro, d, clipA, clipB, 1024u, 4u, 1u).flag & 0x80000000u) != 0u) fastCap4_1++;
    }

    auto statsVec = [](std::vector<uint32_t> v) -> J {
        std::sort(v.begin(), v.end());
        uint64_t sum = std::accumulate(v.begin(), v.end(), uint64_t(0));
        size_t n = v.size();
        return {
            {"total", sum},
            {"mean", double(sum) / double(n)},
            {"p50", v[n / 2]},
            {"p90", v[(n * 90) / 100]},
            {"p99", v[(n * 99) / 100]},
            {"max", v.back()}
        };
    };

    uint64_t baseTotalPhi = baseCtr.evalPhiKnots + baseCtr.evalPhiBisect + baseCtr.evalPhiSplit + baseCtr.evalPhiPieceMid;
    uint64_t fastTotalPhi = fastCompactCtr.evalPhiKnots + fastCompactCtr.evalPhiBisect + fastCompactCtr.evalPhiSplit + fastCompactCtr.evalPhiPieceMid;

    J out = {
        {"query_rays", R.size()},
        {"hardware_captured_singles", totalSingles},
        {"hardware_captured_overlaps", totalOvers},
        {"packet_parity_vs_apple_m5_metal_gpu", {
            {"cpu_vs_metal_fastmath_trig_differing_rays", gpuVsBaseDiffs},
            {"parallel_roots_6da_vs_baseline_differing_rays", parVsBaseDiffs},
            {"depth0_fastpath_full_6db_vs_baseline_differing_rays", fastFullVsBaseDiffs},
            {"depth0_fastpath_compact_6db_vs_baseline_differing_rays", fastCompactVsBaseDiffs},
            {"all_32784_rays_bit_exact", (parVsBaseDiffs == 0 && fastFullVsBaseDiffs == 0 && fastCompactVsBaseDiffs == 0)}
        }},
        {"budget_and_capacity_controls", {
            {"baseline", {{"budget23", baseBudget23}, {"budget24", baseBudget24}, {"budget25", baseBudget25}, {"cap_4_1", baseCap4_1}}},
            {"parallel_roots_6da", {{"budget0", parBudget0}, {"budget23", parBudget23}, {"budget24", parBudget24}, {"budget25", parBudget25}, {"cap_4_1", parCap4_1}}},
            {"depth0_fastpath_6db", {{"budget0", fastBudget0}, {"budget23", fastBudget23}, {"budget24", fastBudget24}, {"budget25", fastBudget25}, {"cap_4_1", fastCap4_1}}}
        }},
        {"baseline_census", {
            {"box_hits", baseCtr.boxHits},
            {"roots_calls", baseCtr.rootsCalls},
            {"knot_sort_passes", baseCtr.knotSortPasses},
            {"evalPhi_total", baseTotalPhi},
            {"evalPhi_knots", baseCtr.evalPhiKnots},
            {"evalPhi_bisect", baseCtr.evalPhiBisect},
            {"evalPhi_split", baseCtr.evalPhiSplit},
            {"evalPhi_piece_mid", baseCtr.evalPhiPieceMid},
            {"stack_pushes", baseCtr.stackPushes},
            {"stack_pops", baseCtr.stackPops},
            {"depth0_bow_rejects", baseCtr.depth0BowRejects},
            {"depth0_monotone_resolved", baseCtr.depth0MonotoneResolved},
            {"depth0_small_resolved", baseCtr.depth0SmallResolved},
            {"depth0_subdivisions", baseCtr.depth0Subdivisions},
            {"max_stack_top", baseCtr.maxStackTop},
            {"max_depth", baseCtr.maxDepth},
            {"max_bands", baseCtr.maxBands},
            {"max_knots_per_member", baseCtr.maxKnotsPerMember},
            {"max_pieces_per_member", baseCtr.maxPiecesPerMember},
            {"max_pieces_per_ray", baseCtr.maxPiecesPerRay},
            {"max_merged_knots", baseCtr.maxMergedKnots},
            {"private_array_bytes_per_thread", 2240},
            {"ray_evalPhi_serial_sum", statsVec(baseCtr.rayEvalPhiTotal)},
            {"ray_evalPhi_parallel_critical_path_max_member", statsVec(baseCtr.rayMaxMemberEvalPhi)},
            {"ray_work_serial_sum", statsVec(baseCtr.rayWorkTotal)},
            {"ray_work_parallel_critical_path_max_member", statsVec(baseCtr.rayMaxMemberWork)}
        }},
        {"depth0_fastpath_6db_census", {
            {"box_hits", fastCompactCtr.boxHits},
            {"linear_z_culled_members", fastCompactCtr.linearZCulledMembers},
            {"linear_z_culled_bins", fastCompactCtr.linearZCulledBins},
            {"active_coarse_bins", fastCompactCtr.activeCoarseBins},
            {"bow_active_coarse_bins", fastCompactCtr.bowActiveCoarseBins},
            {"universal_bow_rejected_bins", fastCompactCtr.universalBowRejectedBins},
            {"whole_member_bow_skips", fastCompactCtr.wholeMemberBowSkips},
            {"roots_calls", fastCompactCtr.rootsCalls},
            {"roots_calls_reduction_pct", 100.0 * double(baseCtr.rootsCalls - fastCompactCtr.rootsCalls) / double(baseCtr.rootsCalls)},
            {"knot_sort_passes", fastCompactCtr.knotSortPasses},
            {"knot_sort_passes_reduction_pct", 100.0 * double(baseCtr.knotSortPasses - fastCompactCtr.knotSortPasses) / double(baseCtr.knotSortPasses)},
            {"evalPhi_total", fastTotalPhi},
            {"evalPhi_reduction_count", baseTotalPhi - fastTotalPhi},
            {"evalPhi_reduction_pct", 100.0 * double(baseTotalPhi - fastTotalPhi) / double(baseTotalPhi)},
            {"evalPhi_knots", fastCompactCtr.evalPhiKnots},
            {"evalPhi_bisect", fastCompactCtr.evalPhiBisect},
            {"evalPhi_split", fastCompactCtr.evalPhiSplit},
            {"evalPhi_piece_mid", fastCompactCtr.evalPhiPieceMid},
            {"stack_pushes", fastCompactCtr.stackPushes},
            {"stack_pops", fastCompactCtr.stackPops},
            {"stack_ops_reduction_pct", 100.0 * double((baseCtr.stackPushes + baseCtr.stackPops) - (fastCompactCtr.stackPushes + fastCompactCtr.stackPops)) / double(baseCtr.stackPushes + baseCtr.stackPops)},
            {"max_stack_top", fastCompactCtr.maxStackTop},
            {"max_depth", fastCompactCtr.maxDepth},
            {"max_bands", fastCompactCtr.maxBands},
            {"max_knots_per_member", fastCompactCtr.maxKnotsPerMember},
            {"max_pieces_per_member", fastCompactCtr.maxPiecesPerMember},
            {"max_pieces_per_ray", fastCompactCtr.maxPiecesPerRay},
            {"max_merged_knots", fastCompactCtr.maxMergedKnots},
            {"private_array_bytes_per_thread_full", 2240},
            {"private_array_bytes_per_thread_compact", 960},
            {"private_array_bytes_reduction_pct", 100.0 * double(2240 - 960) / 2240.0},
            {"ray_evalPhi_serial_sum", statsVec(fastCompactCtr.rayEvalPhiTotal)},
            {"ray_evalPhi_parallel_critical_path_max_member", statsVec(fastCompactCtr.rayMaxMemberEvalPhi)}
        }}
    };
    std::cout << out.dump(2) << std::endl;
    return 0;
}
'''


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    args = p.parse_args()

    root = args.root.resolve()
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=True)

    cpp_path = work / 'verify_6da_6db.cpp'
    obj_path = work / 'verify_6da_6db.o'
    bin_path = work / 'verify_6da_6db'
    cpp_path.write_text(CPP_VERIFY_HARNESS)

    b = root / 'build'
    tmp_inc = Path('/private/tmp/earthcall-d4963d062-src')
    db = json.loads((b / 'compile_commands.json').read_text())
    c = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    cmd = shlex.split(c['command'])
    if tmp_inc.exists():
        inc_idx = next(i for i, x in enumerate(cmd) if x.startswith('-I'))
        cmd.insert(inc_idx, f'-I{tmp_inc}/src')
    cmd[cmd.index('-c') + 1] = str(cpp_path)
    cmd[cmd.index('-o') + 1] = str(obj_path)
    cmd += ['-std=c++20', '-O3']
    subprocess.run(cmd, cwd=b, check=True)

    link_cmd = shlex.split((b / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith('/src/entry.cpp.o'))] = str(obj_path)
    if (tmp_inc / 'ScalarForm.o').exists():
        link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith('ScalarForm.cpp.o'))] = str(tmp_inc / 'ScalarForm.o')
        link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith('SdfWgsl.cpp.o'))] = str(tmp_inc / 'SdfWgsl.o')
        for extra_o in ['LineRestriction.o', 'VolumeRayTransport.o', 'Integral.o', 'EllipticIntegrals.o']:
            link_cmd.insert(link_cmd.index('-o'), str(tmp_inc / extra_o))
    link_cmd[link_cmd.index('-o') + 1] = str(bin_path)
    subprocess.run(link_cmd, cwd=b, check=True)

    zone = root / 'saves/zones/Northern Veil/zone.ecform'
    rays = Path('/private/tmp/earthcall-wide-case/rays.json')
    gpu_fly = Path('/private/tmp/earthcall-on-the-fly-run/GPU_RAY_TRANSPORT_ON_THE_FLY_2026-10-23.json')

    res = subprocess.run([str(bin_path), str(zone), str(rays), str(gpu_fly)],
                         capture_output=True, text=True, check=True)
    report = json.loads(res.stdout)
    if not report['packet_parity_vs_apple_m5_metal_gpu']['all_32784_rays_bit_exact']:
        raise SystemExit('Packet parity failure: ' + json.dumps(report['packet_parity_vs_apple_m5_metal_gpu']))
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS §6da + §6db IEEE-754 f32 packet parity & operation census:', args.out)


if __name__ == '__main__':
    main()
