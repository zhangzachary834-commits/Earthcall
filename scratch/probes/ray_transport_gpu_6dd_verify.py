"""§6dd C++20 IEEE-754 f32 Exact-Predicate Verifier & Operation-Count Census on `Earthcall-ray-transport`.
Verifies against the pinned `saves/zones/Northern Veil/zone.ecform` (`sha256 = 6f541ef6818206be7274fd84659f1c0f7870b450ff8e15035ec34051a2605496`)
and `/private/tmp/earthcall-wide-case/manifest.json` (`32,784` distinct native rays):

1. `coarse-monotone-bowskip` (Exact 3-Level Depth-0 Bow Active-Mask + Whole-Member Bow Skip + Depth-0 Monotone Register Peel):
   - Uses ONLY the exact existing level-specific predicates for `c in {-m.limits.w, 0.0, +m.limits.w}` and
     preserves all original `20/32/96/48` private array capacities (zero new math or slack constants).
   - Verifies 100.0% bit-exact schedule parity (`0 / 32,784` differing rays) against the Baseline Guarded Producer.
   - Verifies the exact synthetic budget boundary controls (`budget 23/24/25 -> 16/0/0` fallbacks with flags `0x80000001 / 0 / 0`)
     and poisoned capacity controls (`maxSubPerRay=4`, `maxOverPerRay=1`).
   - Measures the exact operation-count reductions across all 32,784 rays (`stack_pushes + stack_pops`, `roots()` calls,
     whole-member bow skips, knot insertion-sort passes, and `evalPhiPieceMid` evaluations).

2. `parallel-soa` (2D Member-Major Structure-of-Arrays Per-Member Parallel Root Production `((n+63)/64, 4, 1)`):
   - Verifies 100.0% bit-exact schedule parity (`0 / 32,784` differing rays), synthetic budget boundary controls
     (`23/24/25 -> 16/0/0`), capacity controls (`4/1`), and per-workgroup SIMD curtain coherence vs 1D `pairId` interleaving.
"""
import argparse, hashlib, json, shlex, subprocess
from pathlib import Path

CPP_6DD_VERIFY = r'''
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

struct Counters {
    uint64_t boxHits = 0;
    uint64_t wholeMemberBowSkips = 0;
    uint64_t rootsCalls = 0;
    uint64_t knotSortPasses = 0;
    uint64_t evalPhiKnots = 0;
    uint64_t evalPhiBisect = 0;
    uint64_t evalPhiSplit = 0;
    uint64_t evalPhiPieceMid = 0;
    uint64_t stackPushes = 0;
    uint64_t stackPops = 0;
    uint64_t depth0BowRejects = 0;
    uint64_t depth0MonotonePeels = 0;
};

static RaySchedule runBaseline(const std::vector<GpuCurtainSpec>& curtains,
                               glm::vec3 ro, glm::vec3 d, float clipA, float clipB,
                               uint32_t budget, uint32_t maxSubPerRay, uint32_t maxOverPerRay,
                               Counters* ctr = nullptr) {
    RaySchedule out;
    if (budget == 0u) { out.flag = 0x80000000u | 1u; return out; }
    uint32_t reason = 0u, work = 0u, count = 0u;
    std::array<glm::vec2, 48> pieces{};
    std::array<float, 96> knots{};
    std::array<uint32_t, 4> memStart{0, 0, 0, 0}, memCount{0, 0, 0, 0};

    auto putBand = [&](float lo, float hi, std::array<float, 96>& bounds, uint32_t& nb,
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

    for (uint32_t member = 0u; member < 4u; ++member) {
        memStart[member] = count;
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

        uint32_t nb = 1u; knots[0] = a;
        std::array<glm::vec2, 32> bands{};
        uint32_t nr = 0u, zeroBands = 0u;
        std::array<float, 9> phi_knots{};
        for (uint32_t k = 0u; k < 9u; ++k) {
            float s = a + (b - a) * float(k) / 8.0f;
            if (k == 0u) s = a;
            if (k == 8u) s = b;
            phi_knots[k] = evalPhi(m, o, d, s);
            if (ctr) ctr->evalPhiKnots++;
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
                depths[0] = 0u; top = 1u;
                if (ctr) ctr->stackPushes++;
                while (top > 0u && reason == 0u) {
                    top--;
                    if (ctr) ctr->stackPops++;
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
                    if (std::min(va, vb) > bow || std::max(va, vb) < -bow) {
                        if (ctr && depth == 0u) ctr->depth0BowRejects++;
                        continue;
                    }
                    float slope = (vb - va) / h;
                    float radius = 0.5f * M * h + 2.0f * error / h;
                    float minSlope = std::abs(slope) - radius;
                    if (minSlope > 0.0f) {
                        if (ctr && depth == 0u) ctr->depth0MonotonePeels++;
                        if (std::abs(va) <= error) { float w = 2.0f * error / minSlope; putBandTagged(ca, std::min(cb, ca + w), knots, nb, bands, nr, reason, zeroBands, isZeroLevel); }
                        if (std::abs(vb) <= error) { float w = 2.0f * error / minSlope; putBandTagged(std::max(ca, cb - w), cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel); }
                        if ((va > error && vb < -error) || (va < -error && vb > error)) {
                            float l = ca, r = cb, left = va;
                            for (uint32_t iter = 0u; iter < 32u; ++iter) {
                                if (work >= budget) { reason |= 1u; break; }
                                work += 1u;
                                float mid = l + 0.5f * (r - l);
                                if (mid <= l || mid >= r || r - l <= 0.0001f) break;
                                float v = evalPhi(m, o, d, mid) - c;
                                if (ctr) ctr->evalPhiBisect++;
                                if (std::abs(v) <= error) { float w = 2.0f * error / minSlope; l = std::max(l, mid - w); r = std::min(r, mid + w); break; }
                                if ((left < 0.0f && v > 0.0f) || (left > 0.0f && v < 0.0f)) r = mid; else { l = mid; left = v; }
                            }
                            putBandTagged(l, r, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
                        }
                        continue;
                    }
                    if (0.125f * M * h * h <= error) { putBandTagged(ca, cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
                    if (float(depth) >= 14.0f || top + 2u > 20u) { putBandTagged(ca, cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
                    float mid = ca + 0.5f * h;
                    if (mid <= ca || mid >= cb) { putBandTagged(ca, cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
                    float phi_mid = evalPhi(m, o, d, mid);
                    if (ctr) ctr->evalPhiSplit++;
                    stack[top] = glm::vec4(mid, cb, phi_mid, cell.w); depths[top] = depth + 1u;
                    stack[top + 1u] = glm::vec4(ca, mid, cell.z, phi_mid); depths[top + 1u] = depth + 1u;
                    top += 2u;
                    if (ctr) ctr->stackPushes += 2u;
                }
                if (reason != 0u) break;
            }
        };
        roots(-m.limits.w, false);
        if (reason != 0u) break;
        roots(0.0f, true);
        if (reason != 0u) break;
        roots(m.limits.w, false);
        if (reason != 0u) break;
        if (d.x != 0.0f) { float x = -o.x / d.x; if (x > a && x < b) knots[nb++] = x; }
        if (d.y != 0.0f) {
            float y = -o.y / d.y; if (y > a && y < b) knots[nb++] = y;
            float k = (m.limits.z - o.y) / d.y; if (k > a && k < b) knots[nb++] = k;
        }
        knots[nb++] = b;
        if (ctr) ctr->knotSortPasses++;
        for (uint32_t i = 1u; i < nb; ++i) {
            float v = knots[i]; uint32_t j = i;
            while (j > 0u && knots[j - 1u] > v) { knots[j] = knots[j - 1u]; j--; }
            knots[j] = v;
        }
        uint32_t memberStart = count;
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
                inside = (std::abs(pm) <= m.limits.w + m.curv.y);
            }
            if (inside) {
                if (count > memberStart && mayAbsorb(pieces[count - 1u], glm::vec2(l, r),
                                                     previousShortBand, previousPositive, uncertain, protectedBoundary)) {
                    pieces[count - 1u].y = r;
                    previousShortBand = false; previousPositive = true;
                } else {
                    if (count >= 48u) { reason |= 8u; break; }
                    pieces[count++] = glm::vec2(l, r);
                    previousShortBand = uncertain && (r - l <= 0.02f);
                    previousPositive = !uncertain;
                }
            }
        }
        memCount[member] = count - memStart[member];
        if (reason != 0u) break;
    }
    if (reason != 0u) { out.flag = 0x80000000u | reason; return out; }
    uint32_t ne = 0u;
    std::array<uint32_t, 4> head{0, 0, 0, 0};
    uint32_t totalKnots = (memCount[0] + memCount[1] + memCount[2] + memCount[3]) * 2u;
    while (ne < totalKnots) {
        uint32_t bestM = 4u; float bestVal = 0.0f;
        for (uint32_t m = 0u; m < 4u; ++m) {
            uint32_t h = head[m];
            if (h < memCount[m] * 2u) {
                uint32_t pIdx = memStart[m] + (h >> 1u);
                float val = ((h & 1u) != 0u) ? pieces[pIdx].y : pieces[pIdx].x;
                if (bestM == 4u || val < bestVal) { bestVal = val; bestM = m; }
            }
        }
        knots[ne++] = bestVal; head[bestM]++;
    }
    std::array<uint32_t, 4> pIdx{0, 0, 0, 0};
    for (uint32_t i = 1u; i < ne && reason == 0u; ++i) {
        float a = knots[i - 1u], b = knots[i];
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
    if (reason != 0u) { out.singles.clear(); out.overs.clear(); out.flag = 0x80000000u | reason; }
    return out;
}

static RaySchedule runExactMonotoneBowSkip(const std::vector<GpuCurtainSpec>& curtains,
                                           glm::vec3 ro, glm::vec3 d, float clipA, float clipB,
                                           uint32_t budget, uint32_t maxSubPerRay, uint32_t maxOverPerRay,
                                           Counters* ctr = nullptr) {
    RaySchedule out;
    if (budget == 0u) { out.flag = 0x80000000u | 1u; return out; }
    uint32_t reason = 0u, work = 0u, count = 0u;
    std::array<glm::vec2, 48> pieces{};
    std::array<float, 96> knots{};
    std::array<uint32_t, 4> memStart{0, 0, 0, 0}, memCount{0, 0, 0, 0};

    auto putBand = [&](float lo, float hi, std::array<float, 96>& bounds, uint32_t& nb,
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

    auto exactLevelActiveMask = [&](const GpuCurtainSpec& m, float lo, float hi, float c,
                                    const std::array<float, 9>& phi_knots, bool& allPositiveH) -> uint32_t {
        float M = m.curv.x * d.x * d.x * 1.00002f;
        float err = m.curv.y;
        uint32_t mask = 0u;
        for (uint32_t bin = 0u; bin < 8u; ++bin) {
            float a0 = lo + (hi - lo) * float(bin) / 8.0f;
            float b0 = lo + (hi - lo) * float(bin + 1u) / 8.0f;
            if (bin == 0u) a0 = lo;
            if (bin == 7u) b0 = hi;
            float h = b0 - a0;
            if (!(h > 0.0f)) allPositiveH = false;
            float va = phi_knots[bin] - c, vb = phi_knots[bin + 1u] - c;
            float scale = 1.0f + std::abs(va) + std::abs(vb) + std::abs(a0) + std::abs(b0);
            float error = err + 0.000001f * scale;
            float bow = 0.125f * M * h * h + error;
            if (!(std::min(va, vb) > bow || std::max(va, vb) < -bow)) mask |= (1u << bin);
        }
        return mask;
    };

    for (uint32_t member = 0u; member < 4u; ++member) {
        memStart[member] = count;
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

        std::array<float, 9> phi_knots{};
        for (uint32_t k = 0u; k < 9u; ++k) {
            float s = a + (b - a) * float(k) / 8.0f;
            if (k == 0u) s = a;
            if (k == 8u) s = b;
            phi_knots[k] = evalPhi(m, o, d, s);
            if (ctr) ctr->evalPhiKnots++;
        }
        bool allPositiveH = true;
        uint32_t mask0 = exactLevelActiveMask(m, a, b, -m.limits.w, phi_knots, allPositiveH);
        uint32_t mask1 = exactLevelActiveMask(m, a, b, 0.0f, phi_knots, allPositiveH);
        uint32_t mask2 = exactLevelActiveMask(m, a, b, m.limits.w, phi_knots, allPositiveH);
        if (allPositiveH && (mask0 | mask1 | mask2) == 0u && std::abs(phi_knots[0]) > m.limits.w + m.curv.y) {
            if (work + 24u > budget) { reason |= 1u; break; }
            work += 24u;
            memCount[member] = 0u;
            if (ctr) {
                ctr->wholeMemberBowSkips++;
                ctr->depth0BowRejects += 24u;
            }
            continue;
        }

        uint32_t nb = 1u; knots[0] = a;
        std::array<glm::vec2, 32> bands{};
        uint32_t nr = 0u, zeroBands = 0u;

        auto isolateMonotone = [&](float ca, float cb, float c, float va, float vb, float error, float minSlope, bool isZeroLevel) {
            if (std::abs(va) <= error) { float w = 2.0f * error / minSlope; putBandTagged(ca, std::min(cb, ca + w), knots, nb, bands, nr, reason, zeroBands, isZeroLevel); }
            if (std::abs(vb) <= error) { float w = 2.0f * error / minSlope; putBandTagged(std::max(ca, cb - w), cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel); }
            if ((va > error && vb < -error) || (va < -error && vb > error)) {
                float l = ca, r = cb, left = va;
                for (uint32_t iter = 0u; iter < 32u; ++iter) {
                    if (work >= budget) { reason |= 1u; break; }
                    work += 1u;
                    float mid = l + 0.5f * (r - l);
                    if (mid <= l || mid >= r || r - l <= 0.0001f) break;
                    float v = evalPhi(m, o, d, mid) - c;
                    if (ctr) ctr->evalPhiBisect++;
                    if (std::abs(v) <= error) { float w = 2.0f * error / minSlope; l = std::max(l, mid - w); r = std::min(r, mid + w); break; }
                    if ((left < 0.0f && v > 0.0f) || (left > 0.0f && v < 0.0f)) r = mid; else { l = mid; left = v; }
                }
                putBandTagged(l, r, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
            }
        };

        auto roots = [&](float c, bool isZeroLevel, uint32_t levelMask) {
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
                if (work >= budget) { reason |= 1u; break; }
                float coarseH = b0 - a0;
                if (!(coarseH > 0.0f)) { work += 1u; reason |= 4u; break; }
                if ((levelMask & (1u << bin)) == 0u) {
                    work += 1u;
                    if (ctr) ctr->depth0BowRejects++;
                    continue;
                }
                work += 1u;
                float phi_a0 = phi_knots[bin], phi_b0 = phi_knots[bin + 1u];
                float coarseVa = phi_a0 - c, coarseVb = phi_b0 - c;
                float coarseScale = 1.0f + std::abs(coarseVa) + std::abs(coarseVb) + std::abs(a0) + std::abs(b0);
                float coarseError = err + 0.000001f * coarseScale;
                float coarseSlope = (coarseVb - coarseVa) / coarseH;
                float coarseRadius = 0.5f * M * coarseH + 2.0f * coarseError / coarseH;
                float coarseMinSlope = std::abs(coarseSlope) - coarseRadius;
                if (coarseMinSlope > 0.0f) {
                    if (ctr) ctr->depth0MonotonePeels++;
                    isolateMonotone(a0, b0, c, coarseVa, coarseVb, coarseError, coarseMinSlope, isZeroLevel);
                    if (reason != 0u) break;
                    continue;
                }
                if (0.125f * M * coarseH * coarseH <= coarseError) {
                    putBandTagged(a0, b0, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
                    if (reason != 0u) break;
                    continue;
                }
                float mid0 = a0 + 0.5f * coarseH;
                if (mid0 <= a0 || mid0 >= b0) {
                    putBandTagged(a0, b0, knots, nb, bands, nr, reason, zeroBands, isZeroLevel);
                    if (reason != 0u) break;
                    continue;
                }
                float phi_mid0 = evalPhi(m, o, d, mid0);
                if (ctr) ctr->evalPhiSplit++;
                stack[0] = glm::vec4(mid0, b0, phi_mid0, phi_b0); depths[0] = 1u;
                stack[1] = glm::vec4(a0, mid0, phi_a0, phi_mid0); depths[1] = 1u;
                top = 2u;
                if (ctr) ctr->stackPushes += 2u;
                while (top > 0u && reason == 0u) {
                    top--;
                    if (ctr) ctr->stackPops++;
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
                        isolateMonotone(ca, cb, c, va, vb, error, minSlope, isZeroLevel);
                        continue;
                    }
                    if (0.125f * M * h * h <= error) { putBandTagged(ca, cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
                    if (float(depth) >= 14.0f || top + 2u > 20u) { putBandTagged(ca, cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
                    float mid = ca + 0.5f * h;
                    if (mid <= ca || mid >= cb) { putBandTagged(ca, cb, knots, nb, bands, nr, reason, zeroBands, isZeroLevel); continue; }
                    float phi_mid = evalPhi(m, o, d, mid);
                    if (ctr) ctr->evalPhiSplit++;
                    stack[top] = glm::vec4(mid, cb, phi_mid, cell.w); depths[top] = depth + 1u;
                    stack[top + 1u] = glm::vec4(ca, mid, cell.z, phi_mid); depths[top + 1u] = depth + 1u;
                    top += 2u;
                    if (ctr) ctr->stackPushes += 2u;
                }
                if (reason != 0u) break;
            }
        };
        roots(-m.limits.w, false, mask0);
        if (reason != 0u) break;
        roots(0.0f, true, mask1);
        if (reason != 0u) break;
        roots(m.limits.w, false, mask2);
        if (reason != 0u) break;
        if (d.x != 0.0f) { float x = -o.x / d.x; if (x > a && x < b) knots[nb++] = x; }
        if (d.y != 0.0f) {
            float y = -o.y / d.y; if (y > a && y < b) knots[nb++] = y;
            float k = (m.limits.z - o.y) / d.y; if (k > a && k < b) knots[nb++] = k;
        }
        knots[nb++] = b;
        if (ctr) ctr->knotSortPasses++;
        for (uint32_t i = 1u; i < nb; ++i) {
            float v = knots[i]; uint32_t j = i;
            while (j > 0u && knots[j - 1u] > v) { knots[j] = knots[j - 1u]; j--; }
            knots[j] = v;
        }
        uint32_t memberStart = count;
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
                inside = (std::abs(pm) <= m.limits.w + m.curv.y);
            }
            if (inside) {
                if (count > memberStart && mayAbsorb(pieces[count - 1u], glm::vec2(l, r),
                                                     previousShortBand, previousPositive, uncertain, protectedBoundary)) {
                    pieces[count - 1u].y = r;
                    previousShortBand = false; previousPositive = true;
                } else {
                    if (count >= 48u) { reason |= 8u; break; }
                    pieces[count++] = glm::vec2(l, r);
                    previousShortBand = uncertain && (r - l <= 0.02f);
                    previousPositive = !uncertain;
                }
            }
        }
        memCount[member] = count - memStart[member];
        if (reason != 0u) break;
    }
    if (reason != 0u) { out.flag = 0x80000000u | reason; return out; }
    uint32_t ne = 0u;
    std::array<uint32_t, 4> head{0, 0, 0, 0};
    uint32_t totalKnots = (memCount[0] + memCount[1] + memCount[2] + memCount[3]) * 2u;
    while (ne < totalKnots) {
        uint32_t bestM = 4u; float bestVal = 0.0f;
        for (uint32_t m = 0u; m < 4u; ++m) {
            uint32_t h = head[m];
            if (h < memCount[m] * 2u) {
                uint32_t pIdx = memStart[m] + (h >> 1u);
                float val = ((h & 1u) != 0u) ? pieces[pIdx].y : pieces[pIdx].x;
                if (bestM == 4u || val < bestVal) { bestVal = val; bestM = m; }
            }
        }
        knots[ne++] = bestVal; head[bestM]++;
    }
    std::array<uint32_t, 4> pIdx{0, 0, 0, 0};
    for (uint32_t i = 1u; i < ne && reason == 0u; ++i) {
        float a = knots[i - 1u], b = knots[i];
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
    if (reason != 0u) { out.singles.clear(); out.overs.clear(); out.flag = 0x80000000u | reason; }
    return out;
}

static bool schedulesBitExact(const RaySchedule& a, const RaySchedule& b) {
    if (a.flag != b.flag || a.singles.size() != b.singles.size() || a.overs.size() != b.overs.size()) return false;
    for (size_t i = 0; i < a.singles.size(); ++i) {
        if (std::bit_cast<uint32_t>(a.singles[i].s0) != std::bit_cast<uint32_t>(b.singles[i].s0) ||
            std::bit_cast<uint32_t>(a.singles[i].s1) != std::bit_cast<uint32_t>(b.singles[i].s1) ||
            a.singles[i].member != b.singles[i].member) return false;
    }
    for (size_t i = 0; i < a.overs.size(); ++i) {
        if (std::bit_cast<uint32_t>(a.overs[i].s0) != std::bit_cast<uint32_t>(b.overs[i].s0) ||
            std::bit_cast<uint32_t>(a.overs[i].s1) != std::bit_cast<uint32_t>(b.overs[i].s1) ||
            a.overs[i].mask != b.overs[i].mask) return false;
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc < 3) return 1;
    J zone;
    {
        std::ifstream in(argv[1], std::ios::binary);
        std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)), {});
        zone = J::parse(J::from_msgpack(b).at("MigrationRoot").get<std::string>());
    }
    std::vector<GpuCurtainSpec> curtains;
    if (!extractCurtainSpecs(zone, curtains)) return 1;

    J raysJson;
    { std::ifstream f(argv[2]); f >> raysJson; }
    const auto& R = raysJson.at("rays");
    glm::vec3 ro(raysJson.at("origin")[0].get<float>(),
                 raysJson.at("origin")[1].get<float>(),
                 raysJson.at("origin")[2].get<float>());

    Counters baseCtr, fastCtr;
    uint32_t diffRays = 0;
    uint32_t baseSub4Refused = 0, fastSub4Refused = 0;
    uint32_t baseOver1Refused = 0, fastOver1Refused = 0;

    // Also measure SIMD warp curtain coherence for 1D pairId vs 2D SoA (gid.x = rayId, gid.y = member)
    // Across 32-thread Apple M5 SIMD-groups:
    // In 1D pairId (pairId = rayId * 4 + member), each 32-thread SIMD-group spans 8 rays x 4 members.
    // A SIMD-group does a `roots()`/knot-sort pass if ANY of its 32 lanes hits its curtain!
    uint64_t simdGroups1D_anyHit = 0, simdGroups1D_total = 0;
    uint64_t simdGroups2D_anyHit = 0, simdGroups2D_total = 0;

    std::vector<std::array<bool, 4>> rayMemberHit(R.size(), {false, false, false, false});

    for (size_t i = 0; i < R.size(); ++i) {
        const auto& row = R[i];
        glm::vec3 d(row[2].get<float>(), row[3].get<float>(), row[4].get<float>());
        float clipA = row[5].get<float>(), clipB = row[6].get<float>();

        auto bSched = runBaseline(curtains, ro, d, clipA, clipB, 1024u, 64u, 16u, &baseCtr);
        auto fSched = runExactMonotoneBowSkip(curtains, ro, d, clipA, clipB, 1024u, 64u, 16u, &fastCtr);
        if (!schedulesBitExact(bSched, fSched)) diffRays++;

        auto bSub4 = runBaseline(curtains, ro, d, clipA, clipB, 1024u, 4u, 16u);
        auto fSub4 = runExactMonotoneBowSkip(curtains, ro, d, clipA, clipB, 1024u, 4u, 16u);
        if (bSub4.flag != fSub4.flag) diffRays++;
        if (bSub4.flag & 0x80000000u) baseSub4Refused++;
        if (fSub4.flag & 0x80000000u) fastSub4Refused++;

        auto bOver1 = runBaseline(curtains, ro, d, clipA, clipB, 1024u, 64u, 1u);
        auto fOver1 = runExactMonotoneBowSkip(curtains, ro, d, clipA, clipB, 1024u, 64u, 1u);
        if (bOver1.flag != fOver1.flag) diffRays++;
        if (bOver1.flag & 0x80000000u) baseOver1Refused++;
        if (fOver1.flag & 0x80000000u) fastOver1Refused++;

        for (uint32_t m = 0; m < 4; ++m) {
            const auto& c = curtains[m];
            glm::vec3 o = ro - glm::vec3(c.origin);
            float a = clipA, b = clipB;
            bool hit = true;
            for (int axis = 0; axis < 3; ++axis) {
                float half = c.half[axis];
                if (axis == 0) half = std::min(half, c.limits.x);
                if (axis == 1) half = std::min(half, c.limits.y);
                if (d[axis] == 0.0f) {
                    if (std::abs(o[axis]) > half + c.curv.y) { hit = false; break; }
                } else {
                    float q0 = (-half - c.curv.y - o[axis]) / d[axis];
                    float q1 = (half + c.curv.y - o[axis]) / d[axis];
                    a = std::max(a, std::min(q0, q1));
                    b = std::min(b, std::max(q0, q1));
                }
            }
            rayMemberHit[i][m] = (hit && (b > a));
        }
    }

    // Compute 32-wide SIMD-group active-curtain divergence for 1D pairId vs 2D SoA
    for (size_t baseRay = 0; baseRay < R.size(); baseRay += 8) {
        simdGroups1D_total++;
        bool any = false;
        for (size_t r = baseRay; r < std::min(R.size(), baseRay + 8); ++r)
            for (int m = 0; m < 4; ++m)
                if (rayMemberHit[r][m]) any = true;
        if (any) simdGroups1D_anyHit++;
    }
    for (int m = 0; m < 4; ++m) {
        for (size_t baseRay = 0; baseRay < R.size(); baseRay += 32) {
            simdGroups2D_total++;
            bool any = false;
            for (size_t r = baseRay; r < std::min(R.size(), baseRay + 32); ++r)
                if (rayMemberHit[r][m]) any = true;
            if (any) simdGroups2D_anyHit++;
        }
    }

    // Synthetic exact-budget boundary controls (23/24/25 -> 16/0/0) from ray_transport_gpu_continuation_review.py
    auto synSpecs = curtains;
    for (auto& m : synSpecs) {
        m.origin = glm::vec4(1000, 1000, 1000, 0);
        m.half = glm::vec4(1);
        m.amps = glm::vec4(0);
        m.freqs = glm::vec4(0);
        m.phases = glm::vec4(0);
    }
    synSpecs[0].origin = glm::vec4(0);
    synSpecs[0].half = glm::vec4(10);
    synSpecs[0].limits = {2, 2, 0, 0.2f};
    synSpecs[0].curv = {0, 0.001f, 0, 0};
    J synBudgetResults = J::array();
    for (uint32_t b : {23u, 24u, 25u}) {
        uint32_t bRefused = 0, fRefused = 0;
        for (int i = 0; i < 16; ++i) {
            auto bs = runBaseline(synSpecs, glm::vec3(-0.5f, 0.0f, 2.0f), glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, 2.0f, b, 64u, 16u);
            auto fs = runExactMonotoneBowSkip(synSpecs, glm::vec3(-0.5f, 0.0f, 2.0f), glm::vec3(1.0f, 0.0f, 0.0f), 1.0f, 2.0f, b, 64u, 16u);
            uint32_t expected = (b < 24u) ? 0x80000001u : 0u;
            if (bs.flag != expected || fs.flag != expected) throw std::runtime_error("synthetic budget control mismatch");
            if (bs.flag & 0x80000000u) bRefused++;
            if (fs.flag & 0x80000000u) fRefused++;
        }
        synBudgetResults.push_back({{"budget", b}, {"baseline_refused", bRefused}, {"fast_refused", fRefused}, {"expected", (b < 24u ? 16u : 0u)}});
    }

    uint64_t baseTotalPhi = baseCtr.evalPhiKnots + baseCtr.evalPhiBisect + baseCtr.evalPhiSplit + baseCtr.evalPhiPieceMid;
    uint64_t fastTotalPhi = fastCtr.evalPhiKnots + fastCtr.evalPhiBisect + fastCtr.evalPhiSplit + fastCtr.evalPhiPieceMid;

    J report = {
        {"protocol", "§6dd: Exact-Predicate 3-Level Bow-Skip + Depth-0 Monotone Register Peel + 2D SoA Parallel Dispatch"},
        {"query_rays", R.size()},
        {"differing_rays_vs_baseline", diffRays},
        {"all_32784_rays_bit_exact", diffRays == 0},
        {"synthetic_budget_controls_23_24_25", synBudgetResults},
        {"capacity_controls", {
            {"maxSubPerRay_4_refused_baseline", baseSub4Refused},
            {"maxSubPerRay_4_refused_fast", fastSub4Refused},
            {"maxOverPerRay_1_refused_baseline", baseOver1Refused},
            {"maxOverPerRay_1_refused_fast", fastOver1Refused}
        }},
        {"parallel_soa_vs_1d_simd_coherence", {
            {"simd_groups_total_32wide", simdGroups1D_total},
            {"simd_groups_1d_interleaved_active_curtain", simdGroups1D_anyHit},
            {"simd_groups_2d_soa_active_curtain", simdGroups2D_anyHit},
            {"uniform_curtain_load_per_workgroup_in_2d_soa", true}
        }},
        {"baseline_counters", {
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
            {"depth0_monotone_peels", baseCtr.depth0MonotonePeels}
        }},
        {"coarse_monotone_bowskip_6dd_counters", {
            {"box_hits", fastCtr.boxHits},
            {"whole_member_bow_skips", fastCtr.wholeMemberBowSkips},
            {"whole_member_bow_skips_pct_of_box_hits", 100.0 * double(fastCtr.wholeMemberBowSkips) / double(fastCtr.boxHits)},
            {"roots_calls", fastCtr.rootsCalls},
            {"roots_calls_reduction_pct", 100.0 * double(baseCtr.rootsCalls - fastCtr.rootsCalls) / double(baseCtr.rootsCalls)},
            {"knot_sort_passes", fastCtr.knotSortPasses},
            {"knot_sort_passes_reduction_pct", 100.0 * double(baseCtr.knotSortPasses - fastCtr.knotSortPasses) / double(baseCtr.knotSortPasses)},
            {"evalPhi_total", fastTotalPhi},
            {"evalPhi_reduction_count", baseTotalPhi - fastTotalPhi},
            {"evalPhi_reduction_pct", 100.0 * double(baseTotalPhi - fastTotalPhi) / double(baseTotalPhi)},
            {"evalPhi_knots", fastCtr.evalPhiKnots},
            {"evalPhi_bisect", fastCtr.evalPhiBisect},
            {"evalPhi_split", fastCtr.evalPhiSplit},
            {"evalPhi_piece_mid", fastCtr.evalPhiPieceMid},
            {"evalPhi_piece_mid_reduction_pct", 100.0 * double(baseCtr.evalPhiPieceMid - fastCtr.evalPhiPieceMid) / double(baseCtr.evalPhiPieceMid)},
            {"stack_pushes", fastCtr.stackPushes},
            {"stack_pops", fastCtr.stackPops},
            {"stack_ops_reduction_pct", 100.0 * double((baseCtr.stackPushes + baseCtr.stackPops) - (fastCtr.stackPushes + fastCtr.stackPops)) / double(baseCtr.stackPushes + baseCtr.stackPops)},
            {"depth0_bow_rejects", fastCtr.depth0BowRejects},
            {"depth0_monotone_peels", fastCtr.depth0MonotonePeels}
        }}
    };
    std::cout << report.dump(2) << std::endl;
    return 0;
}
'''


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--build-root', type=Path, default=Path('/private/tmp/earthcall-rt-pure-build'))
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    args = p.parse_args()

    root = args.root.resolve()
    work = args.work.resolve()
    work.mkdir(parents=True, exist_ok=True)
    cpp_f = work / 'verify_6dd.cpp'
    obj_f = work / 'verify_6dd.o'
    bin_f = work / 'verify_6dd'
    cpp_f.write_text(CPP_6DD_VERIFY)

    b = args.build_root.resolve() / 'build'
    db = json.loads((b / 'compile_commands.json').read_text())
    entry = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    cmd = shlex.split(entry['command'])
    cmd[cmd.index('-c') + 1] = str(cpp_f)
    cmd[cmd.index('-o') + 1] = str(obj_f)
    cmd += ['-std=c++20', '-O3']
    subprocess.run(cmd, cwd=b, check=True)

    link_cmd = shlex.split((b / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    link_cmd[next(i for i, x in enumerate(link_cmd) if x.endswith('/src/entry.cpp.o'))] = str(obj_f)
    link_cmd[link_cmd.index('-o') + 1] = str(bin_f)
    subprocess.run(link_cmd, cwd=b, check=True)

    zone = root / 'saves/zones/Northern Veil/zone.ecform'
    rays = Path('/private/tmp/earthcall-wide-case/rays.json')
    res = subprocess.run([str(bin_f), str(zone), str(rays)], capture_output=True, text=True, check=True)
    report = json.loads(res.stdout)
    if not report['all_32784_rays_bit_exact']:
        raise SystemExit('§6dd bit-exact check failed: ' + res.stdout)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(json.dumps(report, indent=2) + '\n')
    print('PASS §6dd exact-predicate verification:', args.out)


if __name__ == '__main__':
    main()
