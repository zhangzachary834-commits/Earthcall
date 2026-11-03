#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <numeric>
#include <set>
#include <sstream>
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

struct DetailedRayInfo {
    uint32_t rayIndex;
    int pixelX, pixelY;
    int gridX, gridY;
    glm::vec3 dir;
    glm::vec2 clip;
    uint32_t boxHitMask = 0;
    uint32_t activeMemberMask = 0;
    std::array<uint32_t, 4> subpanelsPerMember{0, 0, 0, 0};
    std::array<uint8_t, 4> activeBinsPerMember{0, 0, 0, 0};
    uint32_t totalSubpanels = 0;
    uint32_t totalOverlaps = 0;
    bool hasOverlaps = false;
};

static std::vector<RaySchedule> runInstrumentedProducer(
    const std::vector<GpuCurtainSpec>& curtains,
    glm::vec3 ro,
    const std::vector<glm::vec3>& dirs,
    const std::vector<glm::vec2>& clips,
    std::vector<DetailedRayInfo>& rayInfos)
{
    size_t count = dirs.size();
    std::vector<RaySchedule> schedules(count);

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
                    rayInfos[id].boxHitMask |= (1u << member);
                }
            }

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
                uint32_t combinedMask = mask0 | mask1 | mask2;
                rayInfos[id].activeBinsPerMember[member] = uint8_t(combinedMask & 0xffu);

                if (allPositiveH && combinedMask == 0u && std::abs(phi_knots[0]) > m.limits.w + err) {
                    st.work += 24u;
                    st.memCount[member] = 0u;
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
                            st.work += 1u;
                            float mid = l + 0.5f * (r - l);
                            if (mid <= l || mid >= r || r - l <= 0.0001f) break;
                            float v = evalPhi(m, o, d, mid) - c;
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
                        stack[0] = glm::vec4(mid0, b0, phi_mid0, phi_b0); depths[0] = 1u;
                        stack[1] = glm::vec4(a0, mid0, phi_a0, phi_mid0); depths[1] = 1u;
                        top = 2u;
                        while (top > 0u && st.reason == 0u) {
                            top--;
                            glm::vec4 cell = stack[top];
                            float ca = cell.x, cb = cell.y, h = cb - ca;
                            uint32_t depth = depths[top];
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
                rayInfos[id].subpanelsPerMember[member] = st.memCount[member];
                if (st.memCount[member] > 0) {
                    rayInfos[id].activeMemberMask |= (1u << member);
                }
            }
        }

        // Interval sorting, single/overlap separation
        for (size_t lid = 0; lid < (wgEnd - wgBase); ++lid) {
            size_t id = wgBase + lid;
            auto& st = lanes[lid];
            schedules[id].flag = st.reason;
            if (st.reason != 0u) continue;

            struct Evt { float t; int type; uint32_t mem; };
            std::vector<Evt> evts;
            for (uint32_t m = 0; m < 4; ++m) {
                for (uint32_t k = 0; k < st.memCount[m]; ++k) {
                    auto piece = st.pieces[st.memStart[m] + k];
                    evts.push_back({piece.x, 1, m});
                    evts.push_back({piece.y, -1, m});
                }
            }
            std::sort(evts.begin(), evts.end(), [](const Evt& a, const Evt& b) {
                if (a.t != b.t) return a.t < b.t;
                return a.type < b.type;
            });

            uint32_t mask = 0u;
            float prevT = 0.0f;
            for (const auto& ev : evts) {
                if (mask != 0u && ev.t > prevT) {
                    int c = std::popcount(mask);
                    if (c == 1) {
                        uint32_t m = std::countr_zero(mask);
                        schedules[id].singles.push_back({prevT, ev.t, m});
                    } else if (c > 1) {
                        schedules[id].overs.push_back({prevT, ev.t, mask});
                    }
                }
                if (ev.type == 1) mask |= (1u << ev.mem);
                else mask &= ~(1u << ev.mem);
                prevT = ev.t;
            }

            rayInfos[id].totalSubpanels = uint32_t(schedules[id].singles.size());
            rayInfos[id].totalOverlaps = uint32_t(schedules[id].overs.size());
            rayInfos[id].hasOverlaps = (rayInfos[id].totalOverlaps > 0);
        }
    }

    return schedules;
}

int main(int argc, char** argv) {
    if (argc < 4) {
        std::cerr << "Usage: structural_census <zone.ecform> <rays.json> <out.json>\n";
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
    std::vector<DetailedRayInfo> rayInfos(count);

    for (size_t i = 0; i < count; ++i) {
        rayInfos[i].rayIndex = uint32_t(i);
        rayInfos[i].pixelX = R[i][0].get<int>();
        rayInfos[i].pixelY = R[i][1].get<int>();
        rayInfos[i].gridX = rayInfos[i].pixelX / 10;
        rayInfos[i].gridY = rayInfos[i].pixelY / 10;
        dirs[i] = glm::vec3(R[i][2].get<float>(), R[i][3].get<float>(), R[i][4].get<float>());
        clips[i] = glm::vec2(R[i][5].get<float>(), R[i][6].get<float>());
        rayInfos[i].dir = dirs[i];
        rayInfos[i].clip = clips[i];
    }

    std::cerr << "Running instrumented producer on " << count << " rays...\n";
    auto scheds = runInstrumentedProducer(specs, ro, dirs, clips, rayInfos);

    // Global work census totals
    uint64_t totalSingles = 0, totalOvers = 0;
    std::array<uint64_t, 4> memberSubpanelTotals{0, 0, 0, 0};
    uint64_t raysWithOverlaps = 0;
    uint64_t raysZeroSubpanels = 0;

    for (const auto& r : rayInfos) {
        totalSingles += r.totalSubpanels;
        totalOvers += r.totalOverlaps;
        if (r.hasOverlaps) raysWithOverlaps++;
        if (r.totalSubpanels == 0 && r.totalOverlaps == 0) raysZeroSubpanels++;
        for (int m = 0; m < 4; ++m) {
            memberSubpanelTotals[m] += r.subpanelsPerMember[m];
        }
    }

    std::cerr << "Total singles: " << totalSingles << ", total overs: " << totalOvers << "\n";
    std::cerr << "Rays with overlaps: " << raysWithOverlaps << " (" << (100.0 * raysWithOverlaps / count) << "%)\n";

    // Analyze Query Families across different group topologies:
    // 1. 1D 16-ray sub-beams (as in 6de)
    // 2. 2D 4x4 screen tiles
    // 3. 2D 8x8 screen tiles
    // 4. 2D 16x16 screen tiles
    auto analyzeGrouping = [&](const std::string& name, const std::map<uint32_t, std::vector<uint32_t>>& groups) -> J {
        J gj;
        gj["grouping_name"] = name;
        gj["total_groups"] = groups.size();

        uint32_t certifiedZeroGroups = 0;
        uint32_t zeroOverlapGroups = 0;

        // Shape definitions:
        // Coarse Shape: (active_member_mask, has_overlaps)
        std::map<std::pair<uint32_t, bool>, uint32_t> coarseShapeCounts;
        std::map<std::pair<uint32_t, bool>, uint32_t> coarseShapeRayCounts;

        // Medium Shape: (member_subpanel_signature, has_overlaps)
        std::map<std::string, uint32_t> mediumShapeCounts;
        std::map<std::string, uint32_t> mediumShapeRayCounts;

        // Fine Shape: (active_member_mask, bin_mask_string, has_overlaps)
        std::map<std::string, uint32_t> fineShapeCounts;
        std::map<std::string, uint32_t> fineShapeRayCounts;

        for (const auto& [gid, rIndices] : groups) {
            uint32_t grpBoxHits = 0;
            uint32_t grpActiveMembers = 0;
            bool grpHasOverlaps = false;
            std::array<uint32_t, 4> grpMemberSubpanelMax{0, 0, 0, 0};
            std::array<uint8_t, 4> grpActiveBins{0, 0, 0, 0};

            for (uint32_t idx : rIndices) {
                const auto& r = rayInfos[idx];
                grpBoxHits |= r.boxHitMask;
                grpActiveMembers |= r.activeMemberMask;
                if (r.hasOverlaps) grpHasOverlaps = true;
                for (int m = 0; m < 4; ++m) {
                    grpMemberSubpanelMax[m] = std::max(grpMemberSubpanelMax[m], r.subpanelsPerMember[m]);
                    grpActiveBins[m] |= r.activeBinsPerMember[m];
                }
            }

            if (grpActiveMembers == 0) certifiedZeroGroups++;
            if (!grpHasOverlaps) zeroOverlapGroups++;

            uint32_t nRays = uint32_t(rIndices.size());

            // Coarse
            auto coarseKey = std::make_pair(grpActiveMembers, grpHasOverlaps);
            coarseShapeCounts[coarseKey]++;
            coarseShapeRayCounts[coarseKey] += nRays;

            // Medium
            std::ostringstream medOss;
            medOss << "M[" << grpMemberSubpanelMax[0] << "," << grpMemberSubpanelMax[1] << ","
                   << grpMemberSubpanelMax[2] << "," << grpMemberSubpanelMax[3] << "]"
                   << (grpHasOverlaps ? "_OVR" : "_NOVR");
            std::string medKey = medOss.str();
            mediumShapeCounts[medKey]++;
            mediumShapeRayCounts[medKey] += nRays;

            // Fine
            std::ostringstream fineOss;
            fineOss << "ACT_" << grpActiveMembers << "_BINS[";
            for (int m = 0; m < 4; ++m) {
                fineOss << (m > 0 ? "," : "") << std::hex << int(grpActiveBins[m]) << std::dec;
            }
            fineOss << "]" << (grpHasOverlaps ? "_OVR" : "_NOVR");
            std::string fineKey = fineOss.str();
            fineShapeCounts[fineKey]++;
            fineShapeRayCounts[fineKey] += nRays;
        }

        gj["certified_zero_groups"] = certifiedZeroGroups;
        gj["certified_zero_group_pct"] = (100.0 * certifiedZeroGroups / groups.size());
        gj["zero_overlap_groups"] = zeroOverlapGroups;
        gj["zero_overlap_group_pct"] = (100.0 * zeroOverlapGroups / groups.size());

        // Sort coarse shapes by ray count descending
        std::vector<std::pair<std::pair<uint32_t, bool>, uint32_t>> sortedCoarse(coarseShapeRayCounts.begin(), coarseShapeRayCounts.end());
        std::sort(sortedCoarse.begin(), sortedCoarse.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

        J coarseJ = J::array();
        double cumCoarsePct = 0.0;
        for (const auto& [k, rCount] : sortedCoarse) {
            double pct = 100.0 * rCount / count;
            cumCoarsePct += pct;
            coarseJ.push_back({
                {"active_member_mask", k.first},
                {"has_overlaps", k.second},
                {"groups", coarseShapeCounts[k]},
                {"rays", rCount},
                {"ray_pct", pct},
                {"cumulative_ray_pct", cumCoarsePct}
            });
        }
        gj["unique_coarse_shapes"] = coarseShapeCounts.size();
        gj["coarse_shapes"] = coarseJ;

        // Sort medium shapes by ray count descending
        std::vector<std::pair<std::string, uint32_t>> sortedMed(mediumShapeRayCounts.begin(), mediumShapeRayCounts.end());
        std::sort(sortedMed.begin(), sortedMed.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

        J medJ = J::array();
        double cumMedPct = 0.0;
        for (const auto& [k, rCount] : sortedMed) {
            double pct = 100.0 * rCount / count;
            cumMedPct += pct;
            medJ.push_back({
                {"shape_signature", k},
                {"groups", mediumShapeCounts[k]},
                {"rays", rCount},
                {"ray_pct", pct},
                {"cumulative_ray_pct", cumMedPct}
            });
        }
        gj["unique_medium_shapes"] = mediumShapeCounts.size();
        gj["medium_shapes_top10"] = J::array();
        for (size_t i = 0; i < std::min<size_t>(10, medJ.size()); ++i) gj["medium_shapes_top10"].push_back(medJ[i]);

        // Sort fine shapes by ray count descending
        std::vector<std::pair<std::string, uint32_t>> sortedFine(fineShapeRayCounts.begin(), fineShapeRayCounts.end());
        std::sort(sortedFine.begin(), sortedFine.end(), [](const auto& a, const auto& b) { return a.second > b.second; });

        gj["unique_fine_shapes"] = fineShapeCounts.size();
        gj["fine_shapes_top10"] = J::array();
        double cumFinePct = 0.0;
        for (size_t i = 0; i < std::min<size_t>(10, sortedFine.size()); ++i) {
            double pct = 100.0 * sortedFine[i].second / count;
            cumFinePct += pct;
            gj["fine_shapes_top10"].push_back({
                {"shape_signature", sortedFine[i].first},
                {"groups", fineShapeCounts[sortedFine[i].first]},
                {"rays", sortedFine[i].second},
                {"ray_pct", pct},
                {"cumulative_ray_pct", cumFinePct}
            });
        }

        // Coverage metrics: rays served by top 1, 3, 5, 10 shapes
        auto computeCoverage = [&](const auto& sortedList) -> J {
            J cov;
            uint32_t r1 = sortedList.size() > 0 ? sortedList[0].second : 0;
            uint32_t r3 = 0, r5 = 0, r10 = 0;
            for (size_t i = 0; i < sortedList.size(); ++i) {
                if (i < 3) r3 += sortedList[i].second;
                if (i < 5) r5 += sortedList[i].second;
                if (i < 10) r10 += sortedList[i].second;
            }
            cov["top1_pct"] = 100.0 * r1 / count;
            cov["top3_pct"] = 100.0 * r3 / count;
            cov["top5_pct"] = 100.0 * r5 / count;
            cov["top10_pct"] = 100.0 * r10 / count;
            return cov;
        };

        gj["coarse_coverage"] = computeCoverage(sortedCoarse);
        gj["medium_coverage"] = computeCoverage(sortedMed);
        gj["fine_coverage"] = computeCoverage(sortedFine);

        return gj;
    };

    // Build groupings:
    // 1. 1D 16-ray beam
    std::map<uint32_t, std::vector<uint32_t>> groups1D;
    for (size_t i = 0; i < count; ++i) {
        groups1D[uint32_t(i / 16)].push_back(uint32_t(i));
    }

    // 2. 2D 4x4 screen tiles
    std::map<uint32_t, std::vector<uint32_t>> groups4x4;
    for (size_t i = 0; i < count; ++i) {
        uint32_t tx = rayInfos[i].gridX / 4;
        uint32_t ty = rayInfos[i].gridY / 4;
        uint32_t key = (ty << 16) | tx;
        groups4x4[key].push_back(uint32_t(i));
    }

    // 3. 2D 8x8 screen tiles
    std::map<uint32_t, std::vector<uint32_t>> groups8x8;
    for (size_t i = 0; i < count; ++i) {
        uint32_t tx = rayInfos[i].gridX / 8;
        uint32_t ty = rayInfos[i].gridY / 8;
        uint32_t key = (ty << 16) | tx;
        groups8x8[key].push_back(uint32_t(i));
    }

    // 4. 2D 16x16 screen tiles
    std::map<uint32_t, std::vector<uint32_t>> groups16x16;
    for (size_t i = 0; i < count; ++i) {
        uint32_t tx = rayInfos[i].gridX / 16;
        uint32_t ty = rayInfos[i].gridY / 16;
        uint32_t key = (ty << 16) | tx;
        groups16x16[key].push_back(uint32_t(i));
    }

    J out;
    out["protocol"] = "6df: Structural Census and Query-Family Residual Program Reuse";
    out["total_rays"] = count;
    out["total_single_subpanels"] = totalSingles;
    out["total_overlap_intervals"] = totalOvers;
    out["rays_with_overlaps"] = raysWithOverlaps;
    out["rays_with_overlaps_pct"] = (100.0 * raysWithOverlaps / count);
    out["rays_zero_subpanels"] = raysZeroSubpanels;
    out["member_subpanel_totals"] = {
        {"member_0", memberSubpanelTotals[0]},
        {"member_1", memberSubpanelTotals[1]},
        {"member_2", memberSubpanelTotals[2]},
        {"member_3", memberSubpanelTotals[3]}
    };

    out["grouping_1d_16ray"] = analyzeGrouping("1D_16ray_subbeam", groups1D);
    out["grouping_2d_4x4"] = analyzeGrouping("2D_4x4_screentile", groups4x4);
    out["grouping_2d_8x8"] = analyzeGrouping("2D_8x8_screentile", groups8x8);
    out["grouping_2d_16x16"] = analyzeGrouping("2D_16x16_screentile", groups16x16);

    std::ofstream o(argv[3]);
    o << out.dump(2) << std::endl;
    std::cerr << "Wrote structural census to " << argv[3] << "\n";
    return 0;
}
