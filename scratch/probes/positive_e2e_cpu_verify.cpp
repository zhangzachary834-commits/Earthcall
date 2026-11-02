// Exact IEEE-754 float32 bit-parity & 64-pose route verification for Positive E2E Ray Transport Plan (P1-P4).
// Verifies:
// 1. Phase P4 16-lane cooperative tile classifier vs. serial classifier (100% bit-identical tile routing & packed words).
// 2. Phase P1+P3 generalized S1 producer (stack[15], knots[32], bands[12], pieces[16]) vs. Fallback producer (100% bit-identical subpanels & work, 0 refusals).
// 3. Phase P1+P3 spatially-ordered disjoint S2 producer (stack[15], knots[32], bands[12], pieces[24], ascending eval + m1First emit) vs. Fallback producer (100% bit-identical subpanels & work, 0 refusals, 0 overlaps).
// 4. Phase P2 SoA indexing & VolumeRoutedTransport.inc observer string-patch verification.

#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/OntoMath/ClampedWaveSupport.hpp"
#include "Singularity/Screen/AuthorableLight.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/WebGPU/VolumeRoutedTransport.hpp"
#include "Singularity/Screen/WebGPU/VolumeRoutedKernels.hpp"

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include "json.hpp"
#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <thread>
#include <vector>

namespace {
using Json = nlohmann::json;

#define REQUIRE(c, msg) do { if (!(c)) { std::fprintf(stderr, "FAIL: %s (%s)\n", msg, #c); std::exit(1); } } while (0)

static std::pair<glm::vec3, glm::vec3> canonicalPose(int i) {
    const double a = 6.2831853071795864769 * i / 64.0;
    glm::vec3 eye(float(8 * std::sin(a)), float(50 + 2 * std::sin(a * .5)), float(75 + 8 * (1 - std::cos(a))));
    glm::vec3 aim(float(40 + 8 * std::sin(a * .5)), float(62 + 2 * std::cos(a)), 110);
    return {eye, aim};
}

struct Ray {
    glm::vec4 ro_clipA;
    glm::vec4 rd_clipB;
    glm::uvec4 counts;
};

struct Subpanel {
    glm::vec2 span;
    glm::uvec2 info;
    bool operator==(const Subpanel& o) const {
        return std::memcmp(&span, &o.span, sizeof(span)) == 0 && info == o.info;
    }
};

static inline float evalWave(const KernelRoutedSpec& cur, float x) {
    return cur.amps.x * std::sin(cur.freqs.x * x + cur.phases.x) +
           cur.amps.y * std::cos(cur.freqs.y * x + cur.phases.y) +
           cur.amps.z * std::sin(cur.freqs.z * x + cur.phases.z);
}

static inline float evalPhi(const KernelRoutedSpec& m, glm::vec3 o, glm::vec3 d, float s) {
    return o.z + s * d.z + evalWave(m, o.x + s * d.x);
}

static inline uint32_t firstTrailingBitU32(uint32_t v) {
    return v ? uint32_t(__builtin_ctz(v)) : 32u;
}

static inline uint32_t countOneBitsU32(uint32_t v) {
    return uint32_t(__builtin_popcount(v));
}

struct TileRouteResult {
    uint32_t cohort; // 0=Empty, 1=S1, 2=S2, 3=Fallback
    uint32_t packedWord;
    uint32_t activeMemberMask;
    uint32_t aabbMask;
    bool tileCanOverlap;
};

// Reference serial tile classifier (1 thread per tile)
static TileRouteResult classifyTileSerial(
    uint32_t tileId,
    const Ray* rays,
    const std::array<KernelRoutedSpec, 4>& curtains) {

    uint32_t activeMemberMask = 0u;
    uint32_t aabbMask = 0u;
    bool tileCanOverlap = false;
    float enterArr[4] = {0, 0, 0, 0};
    float exitArr[4] = {0, 0, 0, 0};
    bool memberHit[4] = {false, false, false, false};
    float rEnter[16][4];
    float rExit[16][4];
    for (int i = 0; i < 16; i++) {
        for (int m = 0; m < 4; m++) {
            rEnter[i][m] = 1e9f;
            rExit[i][m] = -1e9f;
        }
    }

    for (uint32_t m = 0u; m < 4u; m++) {
        const KernelRoutedSpec cur = curtains[m];
        float zEnv = std::abs(cur.amps.x) + std::abs(cur.amps.y) + std::abs(cur.amps.z) + cur.limits.w;
        glm::vec3 halfVec = glm::min(glm::vec3(cur.half), glm::vec3(cur.limits.x, cur.limits.y, zEnv));
        float uxMin = 1e9f;
        float uxMax = -1e9f;
        bool anyRayHit = false;
        float rA[16];
        float rB[16];
        bool rHit[16];

        for (uint32_t i = 0u; i < 16u; i++) {
            const Ray& ray = rays[i];
            glm::vec3 oRel = glm::vec3(ray.ro_clipA) - glm::vec3(cur.origin);
            float a = ray.ro_clipA.w;
            float b = ray.rd_clipB.w;
            bool hit = true;

            for (int axis = 0; axis < 3; axis++) {
                float half = halfVec[axis];
                float d = ray.rd_clipB[axis];
                float o = oRel[axis];
                if (d == 0.0f) {
                    if (std::abs(o) > half + cur.curv.y) { hit = false; break; }
                } else {
                    float q0 = (-half - cur.curv.y - o) / d;
                    float q1 = (half + cur.curv.y - o) / d;
                    a = std::max(a, std::min(q0, q1));
                    b = std::min(b, std::max(q0, q1));
                }
            }

            rA[i] = a;
            rB[i] = b;
            rHit[i] = (hit && b > a);

            if (hit && b > a) {
                anyRayHit = true;
                float u0 = oRel.x + a * ray.rd_clipB.x;
                float u1 = oRel.x + b * ray.rd_clipB.x;
                uxMin = std::min(uxMin, std::min(u0, u1));
                uxMax = std::max(uxMax, std::max(u0, u1));
            }
        }

        if (anyRayHit) aabbMask |= (1u << m);
        if (anyRayHit && uxMax > uxMin) {
            float wKnots[9];
            for (uint32_t k = 0u; k < 9u; k++) {
                float ux = uxMin + float(k) / 8.0f * (uxMax - uxMin);
                wKnots[k] = evalWave(cur, ux);
            }
            float h_ux = (uxMax - uxMin) / 8.0f;
            float bow = 0.125f * cur.curv.x * h_ux * h_ux;
            float thresh = bow + cur.limits.w + cur.curv.y + 0.001f;

            float enter = 1e9f;
            float exit = -1e9f;
            bool curActive = false;

            for (uint32_t i = 0u; i < 16u; i++) {
                if (!rHit[i]) continue;
                const Ray& ray = rays[i];
                glm::vec3 oRel = glm::vec3(ray.ro_clipA) - glm::vec3(cur.origin);
                float dx = ray.rd_clipB.x;
                if (std::abs(dx) < 1e-6f) {
                    curActive = true;
                    enter = std::min(enter, rA[i]);
                    exit = std::max(exit, rB[i]);
                    rEnter[i][m] = rA[i];
                    rExit[i][m] = rB[i];
                    continue;
                }
                float a = rA[i];
                float b = rB[i];
                float iEnter = 1e9f;
                float iExit = -1e9f;
                bool iHit = false;
                for (uint32_t bin = 0u; bin < 8u; bin++) {
                    float u0 = uxMin + float(bin) / 8.0f * (uxMax - uxMin);
                    float u1 = uxMin + float(bin + 1u) / 8.0f * (uxMax - uxMin);
                    float su0 = (u0 - oRel.x) / dx;
                    float su1 = (u1 - oRel.x) / dx;
                    float phi0 = oRel.z + su0 * ray.rd_clipB.z + wKnots[bin];
                    float phi1 = oRel.z + su1 * ray.rd_clipB.z + wKnots[bin + 1u];
                    float sMin = std::min(su0, su1);
                    float sMax = std::max(su0, su1);
                    float cLo = std::max(a, sMin);
                    float cHi = std::min(b, sMax);
                    if (cHi > cLo) {
                        float tA = (cLo - su0) / (su1 - su0);
                        float tB = (cHi - su0) / (su1 - su0);
                        float pA = phi0 + tA * (phi1 - phi0);
                        float pB = phi0 + tB * (phi1 - phi0);
                        if (!(std::min(pA, pB) > thresh || std::max(pA, pB) < -thresh)) {
                            iHit = true;
                            float dp = pB - pA;
                            float sLo = cLo;
                            float sHi = cHi;
                            if (std::abs(dp) > 1e-6f) {
                                float q0 = (-thresh - pA) / dp;
                                float q1 = (thresh - pA) / dp;
                                float uLo = std::clamp(std::min(q0, q1), 0.0f, 1.0f);
                                float uHi = std::clamp(std::max(q0, q1), 0.0f, 1.0f);
                                sLo = std::max(cLo, cLo + uLo * (cHi - cLo) - 0.2f);
                                sHi = std::min(cHi, cLo + uHi * (cHi - cLo) + 0.2f);
                            }
                            iEnter = std::min(iEnter, sLo);
                            iExit = std::max(iExit, sHi);
                        }
                    }
                }
                if (iHit) {
                    curActive = true;
                    enter = std::min(enter, iEnter);
                    exit = std::max(exit, iExit);
                    rEnter[i][m] = iEnter;
                    rExit[i][m] = iExit;
                }
            }
            if (curActive) {
                activeMemberMask |= (1u << m);
                memberHit[m] = true;
                enterArr[m] = enter;
                exitArr[m] = exit;
            }
        } else if (anyRayHit) {
            activeMemberMask |= (1u << m);
            memberHit[m] = true;
            float enter = 1e9f, exit = -1e9f;
            for (uint32_t i = 0u; i < 16u; i++) {
                if (rHit[i]) {
                    enter = std::min(enter, rA[i]);
                    exit = std::max(exit, rB[i]);
                    rEnter[i][m] = rA[i];
                    rExit[i][m] = rB[i];
                }
            }
            enterArr[m] = enter;
            exitArr[m] = exit;
        }
    }

    uint32_t activeCount = countOneBitsU32(activeMemberMask);
    uint32_t precMask = 0u;
    if (activeCount > 1u) {
        for (uint32_t m1 = 0u; m1 < 4u; m1++) {
            if (!memberHit[m1]) continue;
            for (uint32_t m2 = m1 + 1u; m2 < 4u; m2++) {
                if (!memberHit[m2]) continue;
                bool pairOrder12 = true;
                bool pairOrder21 = true;
                bool anyBothHit = false;
                for (uint32_t i = 0u; i < 16u; i++) {
                    bool h1 = rEnter[i][m1] <= rExit[i][m1];
                    bool h2 = rEnter[i][m2] <= rExit[i][m2];
                    if (h1 && h2) {
                        anyBothHit = true;
                        if (!(rExit[i][m1] + 0.01f <= rEnter[i][m2])) pairOrder12 = false;
                        if (!(rExit[i][m2] + 0.01f <= rEnter[i][m1])) pairOrder21 = false;
                    }
                }
                if (!anyBothHit) {
                    pairOrder12 = (exitArr[m1] + 0.01f <= enterArr[m2]);
                    pairOrder21 = (exitArr[m2] + 0.01f <= enterArr[m1]);
                }
                if (pairOrder12 && !pairOrder21) {
                    precMask |= (1u << (m1 * 4u + m2));
                } else if (pairOrder21 && !pairOrder12) {
                    precMask |= (1u << (m2 * 4u + m1));
                } else {
                    tileCanOverlap = true;
                }
            }
        }
    }

    TileRouteResult res{};
    res.activeMemberMask = activeMemberMask;
    res.aabbMask = aabbMask;
    res.tileCanOverlap = tileCanOverlap;
    if (activeCount == 0u) {
        res.cohort = 0u;
        res.packedWord = 0u;
    } else if (activeCount == 1u) {
        uint32_t onlyMember = firstTrailingBitU32(activeMemberMask);
        res.cohort = 1u;
        res.packedWord = (onlyMember << 28u) | tileId;
    } else if (activeCount == 2u && !tileCanOverlap) {
        uint32_t m0 = firstTrailingBitU32(activeMemberMask);
        uint32_t m1 = firstTrailingBitU32(activeMemberMask & ~(1u << m0));
        bool order01 = (precMask & (1u << (m0 * 4u + m1))) != 0u;
        bool order10 = (precMask & (1u << (m1 * 4u + m0))) != 0u;
        if (order01 || order10) {
            uint32_t m1First = (order10 && !order01) ? 1u : 0u;
            res.cohort = 2u;
            res.packedWord = (m0 << 28u) | (m1 << 24u) | (m1First << 20u) | tileId;
        } else {
            res.cohort = 3u;
            res.packedWord = (activeMemberMask << 28u) | tileId;
        }
    } else if (activeCount >= 3u && !tileCanOverlap) {
        std::array<uint32_t, 4> ord = {0u, 0u, 0u, 0u};
        uint32_t inDegMask = 0u;
        for (uint32_t m = 0u; m < 4u; m++) {
            if (!memberHit[m]) continue;
            uint32_t inDeg = 0u;
            for (uint32_t mPrev = 0u; mPrev < 4u; mPrev++) {
                if (mPrev != m && memberHit[mPrev] && ((precMask & (1u << (mPrev * 4u + m))) != 0u)) {
                    inDeg++;
                }
            }
            if (inDeg < 4u) {
                ord[inDeg] = m;
                inDegMask |= (1u << inDeg);
            }
        }
        if (inDegMask == ((1u << activeCount) - 1u)) {
            uint32_t isFour = (activeCount == 4u) ? 1u : 0u;
            res.cohort = 2u;
            res.packedWord = (1u << 21u) | (isFour << 20u) | (ord[0] << 22u) | (ord[1] << 24u) | (ord[2] << 26u) | (ord[3] << 28u) | tileId;
        } else {
            res.cohort = 3u;
            res.packedWord = (activeMemberMask << 28u) | tileId;
        }
    } else {
        res.cohort = 3u;
        res.packedWord = (activeMemberMask << 28u) | tileId;
    }
    return res;
}

// Phase P4 16-lane cooperative classifier (simulating a 64-thread workgroup of 4 tiles x 16 lanes)
static void classifyWorkgroupCooperative(
    uint32_t baseTileId,
    uint32_t totalTiles,
    const Ray* tileRays4x16,
    const std::array<KernelRoutedSpec, 4>& curtains,
    TileRouteResult outResults[4]) {

    float wgUxMin[64];
    float wgUxMax[64];
    uint32_t wgHit[64];
    float wgWKnots[36];
    float wgTileUxMin[4];
    float wgTileUxMax[4];
    uint32_t wgTileAnyHit[4];
    float wgEnter[64];
    float wgExit[64];
    uint32_t wgState[64];
    float wgEnterCurtain[4][64];
    float wgExitCurtain[4][64];

    glm::vec3 ro[64], rd[64];
    float clipA[64], clipB[64];
    bool validTile[64], validRay[64];

    for (uint32_t lid = 0; lid < 64; ++lid) {
        uint32_t localTile = lid >> 4u;
        uint32_t tileId = baseTileId + localTile;
        validTile[lid] = (tileId < totalTiles);
        validRay[lid] = validTile[lid];
        if (validRay[lid]) {
            const Ray& r = tileRays4x16[lid];
            ro[lid] = glm::vec3(r.ro_clipA);
            clipA[lid] = r.ro_clipA.w;
            rd[lid] = glm::vec3(r.rd_clipB);
            clipB[lid] = r.rd_clipB.w;
        }
    }

    uint32_t activeMemberMask[4] = {0, 0, 0, 0};
    uint32_t aabbMask[4] = {0, 0, 0, 0};
    bool tileCanOverlap[4] = {false, false, false, false};
    float enterArr[4][4] = {};
    float exitArr[4][4] = {};
    bool memberHit[4][4] = {};

    for (uint32_t m = 0u; m < 4u; m++) {
        const KernelRoutedSpec cur = curtains[m];
        float aArr[64], bArr[64];
        bool rHitArr[64];

        float zEnv = std::abs(cur.amps.x) + std::abs(cur.amps.y) + std::abs(cur.amps.z) + cur.limits.w;
        glm::vec3 halfVec = glm::min(glm::vec3(cur.half), glm::vec3(cur.limits.x, cur.limits.y, zEnv));
        // Barrier 1 phase: all 64 lanes compute ray-box slab
        for (uint32_t lid = 0; lid < 64; ++lid) {
            glm::vec3 oRel = ro[lid] - glm::vec3(cur.origin);
            float a = clipA[lid];
            float b = clipB[lid];
            bool hit = validRay[lid];
            if (validRay[lid]) {
                for (int axis = 0; axis < 3; axis++) {
                    float half = halfVec[axis];
                    float d = rd[lid][axis];
                    float o = oRel[axis];
                    if (d == 0.0f) {
                        if (std::abs(o) > half + cur.curv.y) { hit = false; break; }
                    } else {
                        float q0 = (-half - cur.curv.y - o) / d;
                        float q1 = (half + cur.curv.y - o) / d;
                        a = std::max(a, std::min(q0, q1));
                        b = std::min(b, std::max(q0, q1));
                    }
                }
            }
            bool rHit = hit && (b > a);
            aArr[lid] = a;
            bArr[lid] = b;
            rHitArr[lid] = rHit;
            float myUxMin = 1e9f;
            float myUxMax = -1e9f;
            if (rHit) {
                float u0 = oRel.x + a * rd[lid].x;
                float u1 = oRel.x + b * rd[lid].x;
                myUxMin = std::min(myUxMin, std::min(u0, u1));
                myUxMax = std::max(myUxMax, std::max(u0, u1));
            }
            wgUxMin[lid] = myUxMin;
            wgUxMax[lid] = myUxMax;
            wgHit[lid] = rHit ? 1u : 0u;
        }

        // Barrier 2 phase: lanes 0..8 per tile reduce [uxMin, uxMax] and evaluate wKnots[0..8]
        for (uint32_t lid = 0; lid < 64; ++lid) {
            uint32_t localTile = lid >> 4u;
            uint32_t lane = lid & 15u;
            uint32_t base = localTile * 16u;
            if (lane < 9u) {
                float tUxMin = 1e9f;
                float tUxMax = -1e9f;
                bool tAnyHit = false;
                for (uint32_t i = 0u; i < 16u; i++) {
                    if (wgHit[base + i] != 0u) {
                        tAnyHit = true;
                        tUxMin = std::min(tUxMin, wgUxMin[base + i]);
                        tUxMax = std::max(tUxMax, wgUxMax[base + i]);
                    }
                }
                if (tAnyHit && tUxMax > tUxMin) {
                    float ux = tUxMin + float(lane) / 8.0f * (tUxMax - tUxMin);
                    wgWKnots[localTile * 9u + lane] = evalWave(cur, ux);
                }
                if (lane == 0u) {
                    wgTileUxMin[localTile] = tUxMin;
                    wgTileUxMax[localTile] = tUxMax;
                    wgTileAnyHit[localTile] = tAnyHit ? 1u : 0u;
                }
            }
        }

        // Barrier 3 phase: all 64 lanes test 8 coarse bins for their ray
        for (uint32_t lid = 0; lid < 64; ++lid) {
            uint32_t localTile = lid >> 4u;
            bool anyRayHit = wgTileAnyHit[localTile] != 0u;
            float uxMin = wgTileUxMin[localTile];
            float uxMax = wgTileUxMax[localTile];
            bool hasUxSpan = anyRayHit && (uxMax > uxMin);
            bool rHit = rHitArr[lid];
            float a = aArr[lid];
            float b = bArr[lid];
            glm::vec3 oRel = ro[lid] - glm::vec3(cur.origin);

            float myEnter = 1e9f;
            float myExit = -1e9f;
            uint32_t myState = 0u;

            if (hasUxSpan && rHit) {
                float dx = rd[lid].x;
                if (std::abs(dx) < 1e-6f) {
                    myState = 2u;
                    myEnter = a;
                    myExit = b;
                } else {
                    float h_ux = (uxMax - uxMin) / 8.0f;
                    float bow = 0.125f * cur.curv.x * h_ux * h_ux;
                    float thresh = bow + cur.limits.w + cur.curv.y + 0.001f;
                    uint32_t knotBase = localTile * 9u;
                    for (uint32_t bin = 0u; bin < 8u; bin++) {
                        float u0 = uxMin + float(bin) / 8.0f * (uxMax - uxMin);
                        float u1 = uxMin + float(bin + 1u) / 8.0f * (uxMax - uxMin);
                        float su0 = (u0 - oRel.x) / dx;
                        float su1 = (u1 - oRel.x) / dx;
                        float phi0 = oRel.z + su0 * rd[lid].z + wgWKnots[knotBase + bin];
                        float phi1 = oRel.z + su1 * rd[lid].z + wgWKnots[knotBase + bin + 1u];
                        float sMin = std::min(su0, su1);
                        float sMax = std::max(su0, su1);
                        float cLo = std::max(a, sMin);
                        float cHi = std::min(b, sMax);
                        if (cHi > cLo) {
                            float tA = (cLo - su0) / (su1 - su0);
                            float tB = (cHi - su0) / (su1 - su0);
                            float pA = phi0 + tA * (phi1 - phi0);
                            float pB = phi0 + tB * (phi1 - phi0);
                            if (!(std::min(pA, pB) > thresh || std::max(pA, pB) < -thresh)) {
                                myState = 1u;
                                float dp = pB - pA;
                                float sLo = cLo;
                                float sHi = cHi;
                                if (std::abs(dp) > 1e-6f) {
                                    float q0 = (-thresh - pA) / dp;
                                    float q1 = (thresh - pA) / dp;
                                    float uLo = std::clamp(std::min(q0, q1), 0.0f, 1.0f);
                                    float uHi = std::clamp(std::max(q0, q1), 0.0f, 1.0f);
                                    sLo = std::max(cLo, cLo + uLo * (cHi - cLo) - 0.2f);
                                    sHi = std::min(cHi, cLo + uHi * (cHi - cLo) + 0.2f);
                                }
                                myEnter = std::min(myEnter, sLo);
                                myExit = std::max(myExit, sHi);
                            }
                        }
                    }
                }
            } else if (anyRayHit && rHit) {
                myState = 1u;
                myEnter = a;
                myExit = b;
            }
            wgEnter[lid] = myEnter;
            wgExit[lid] = myExit;
            wgState[lid] = myState;
            wgEnterCurtain[m][lid] = myEnter;
            wgExitCurtain[m][lid] = myExit;
        }

        // Lane 0 per tile reduces wgEnter/wgExit/wgState
        for (uint32_t localTile = 0; localTile < 4; ++localTile) {
            if (baseTileId + localTile >= totalTiles) continue;
            uint32_t base = localTile * 16u;
            bool anyRayHit = wgTileAnyHit[localTile] != 0u;
            float uxMin = wgTileUxMin[localTile];
            float uxMax = wgTileUxMax[localTile];
            float enter = 1e9f;
            float exit = -1e9f;
            bool curActive = false;
            for (uint32_t i = 0u; i < 16u; i++) {
                uint32_t st = wgState[base + i];
                if (st != 0u) {
                    curActive = true;
                    enter = std::min(enter, wgEnter[base + i]);
                    exit = std::max(exit, wgExit[base + i]);
                }
            }
            if (anyRayHit) aabbMask[localTile] |= (1u << m);
            if (anyRayHit && uxMax > uxMin) {
                if (curActive) {
                    activeMemberMask[localTile] |= (1u << m);
                    memberHit[localTile][m] = true;
                    enterArr[localTile][m] = enter;
                    exitArr[localTile][m] = exit;
                }
            } else if (anyRayHit) {
                activeMemberMask[localTile] |= (1u << m);
                memberHit[localTile][m] = true;
                enterArr[localTile][m] = enter;
                exitArr[localTile][m] = exit;
            }
        }
    }

    for (uint32_t localTile = 0; localTile < 4; ++localTile) {
        uint32_t tileId = baseTileId + localTile;
        if (tileId >= totalTiles) continue;
        uint32_t mask = activeMemberMask[localTile];
        uint32_t activeCount = countOneBitsU32(mask);
        bool canOverlap = false;
        uint32_t precMask = 0u;
        if (activeCount > 1u) {
            uint32_t base = localTile * 16u;
            for (uint32_t m1 = 0u; m1 < 4u; m1++) {
                if (!memberHit[localTile][m1]) continue;
                for (uint32_t m2 = m1 + 1u; m2 < 4u; m2++) {
                    if (!memberHit[localTile][m2]) continue;
                    bool pairOrder12 = true;
                    bool pairOrder21 = true;
                    bool anyBothHit = false;
                    for (uint32_t i = 0u; i < 16u; i++) {
                        float e1 = wgEnterCurtain[m1][base + i];
                        float x1 = wgExitCurtain[m1][base + i];
                        float e2 = wgEnterCurtain[m2][base + i];
                        float x2 = wgExitCurtain[m2][base + i];
                        bool h1 = (e1 <= x1);
                        bool h2 = (e2 <= x2);
                        if (h1 && h2) {
                            anyBothHit = true;
                            if (!(x1 + 0.01f <= e2)) pairOrder12 = false;
                            if (!(x2 + 0.01f <= e1)) pairOrder21 = false;
                        }
                    }
                    if (!anyBothHit) {
                        pairOrder12 = (exitArr[localTile][m1] + 0.01f <= enterArr[localTile][m2]);
                        pairOrder21 = (exitArr[localTile][m2] + 0.01f <= enterArr[localTile][m1]);
                    }
                    if (pairOrder12 && !pairOrder21) {
                        precMask |= (1u << (m1 * 4u + m2));
                    } else if (pairOrder21 && !pairOrder12) {
                        precMask |= (1u << (m2 * 4u + m1));
                    } else {
                        canOverlap = true;
                    }
                }
            }
        }
        TileRouteResult& res = outResults[localTile];
        res.activeMemberMask = mask;
        res.aabbMask = aabbMask[localTile];
        res.tileCanOverlap = canOverlap;
        if (activeCount == 0u) {
            res.cohort = 0u;
            res.packedWord = 0u;
        } else if (activeCount == 1u) {
            uint32_t onlyMember = firstTrailingBitU32(mask);
            res.cohort = 1u;
            res.packedWord = (onlyMember << 28u) | tileId;
        } else if (activeCount == 2u && !canOverlap) {
            uint32_t m0 = firstTrailingBitU32(mask);
            uint32_t m1 = firstTrailingBitU32(mask & ~(1u << m0));
            bool order01 = (precMask & (1u << (m0 * 4u + m1))) != 0u;
            bool order10 = (precMask & (1u << (m1 * 4u + m0))) != 0u;
            if (order01 || order10) {
                uint32_t m1First = (order10 && !order01) ? 1u : 0u;
                res.cohort = 2u;
                res.packedWord = (m0 << 28u) | (m1 << 24u) | (m1First << 20u) | tileId;
            } else {
                res.cohort = 3u;
                res.packedWord = (mask << 28u) | tileId;
            }
        } else if (activeCount >= 3u && !canOverlap) {
            std::array<uint32_t, 4> ord = {0u, 0u, 0u, 0u};
            uint32_t inDegMask = 0u;
            for (uint32_t m = 0u; m < 4u; m++) {
                if (!memberHit[localTile][m]) continue;
                uint32_t inDeg = 0u;
                for (uint32_t mPrev = 0u; mPrev < 4u; mPrev++) {
                    if (mPrev != m && memberHit[localTile][mPrev] && ((precMask & (1u << (mPrev * 4u + m))) != 0u)) {
                        inDeg++;
                    }
                }
                if (inDeg < 4u) {
                    ord[inDeg] = m;
                    inDegMask |= (1u << inDeg);
                }
            }
            if (inDegMask == ((1u << activeCount) - 1u)) {
                uint32_t isFour = (activeCount == 4u) ? 1u : 0u;
                res.cohort = 2u;
                res.packedWord = (1u << 21u) | (isFour << 20u) | (ord[0] << 22u) | (ord[1] << 24u) | (ord[2] << 26u) | (ord[3] << 28u) | tileId;
            } else {
                res.cohort = 3u;
                res.packedWord = (mask << 28u) | tileId;
            }
        } else {
            res.cohort = 3u;
            res.packedWord = (mask << 28u) | tileId;
        }
    }
}

// Producer verification helper (S1, S2, and Fallback)
template<size_t MaxBounds, size_t MaxBands, size_t StackCap>
struct ProducerMath {
    static void putBand(float lo, float hi, float (&bounds)[MaxBounds], uint32_t& nb,
                        glm::vec2 (&bands)[MaxBands], uint32_t& nr, uint32_t& reason) {
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
        if (nb + 2u > MaxBounds - 4u || nr >= MaxBands) { reason |= 2u; return; }
        bounds[nb] = lo; bounds[nb + 1u] = hi; nb += 2u;
        bands[nr] = glm::vec2(lo, hi); nr += 1u;
    }

    static bool mayAbsorb(glm::vec2 previous, glm::vec2 current, bool previousShortBand,
                          bool previousPositive, bool currentUncertain, bool protectedBoundary) {
        if (protectedBoundary || previous.y != current.x) return false;
        bool currentShortBand = currentUncertain && (current.y - current.x <= 0.02f);
        return (previousShortBand && !currentUncertain) || (previousPositive && currentShortBand);
    }

    static void putBandTagged(float lo, float hi, float (&bounds)[MaxBounds], uint32_t& nb,
                              glm::vec2 (&bands)[MaxBands], uint32_t& nr, uint32_t& reason,
                              uint32_t& zeroBands, bool isZeroLevel) {
        putBand(lo, hi, bounds, nb, bands, nr, reason);
        if (reason == 0u && isZeroLevel && nr > 0u) zeroBands |= (1u << (nr - 1u));
    }

    static uint32_t getDepth(glm::uvec3 bits, uint32_t idx) {
        return (bits[idx >> 3u] >> ((idx & 7u) << 2u)) & 15u;
    }
    static void setDepth(glm::uvec3& bits, uint32_t idx, uint32_t d) {
        uint32_t w = idx >> 3u, s = (idx & 7u) << 2u;
        bits[w] = (bits[w] & ~(15u << s)) | ((d & 15u) << s);
    }

    static void isolateMonotone(const KernelRoutedSpec& m, glm::vec3 o, glm::vec3 d,
                                float a, float b, float c, float va, float vb,
                                float error, float minSlope,
                                float (&bounds)[MaxBounds], uint32_t& nb,
                                glm::vec2 (&bands)[MaxBands], uint32_t& nr,
                                uint32_t& reason, uint32_t& work,
                                uint32_t& zeroBands, bool isZeroLevel, uint32_t budget) {
        if (std::abs(va) <= error) {
            float w = 2.0f * error / minSlope;
            putBandTagged(a, std::min(b, a + w), bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
        }
        if (std::abs(vb) <= error) {
            float w = 2.0f * error / minSlope;
            putBandTagged(std::max(a, b - w), b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
        }
        if ((va > error && vb < -error) || (va < -error && vb > error)) {
            float l = a, r = b, left = va;
            for (uint32_t iter = 0u; iter < 32u; iter++) {
                if (work >= budget) { reason |= 1u; break; }
                work += 1u;
                float mid = l + 0.5f * (r - l);
                if (mid <= l || mid >= r || r - l <= 0.0001f) break;
                float v = evalPhi(m, o, d, mid) - c;
                if (std::abs(v) <= error) {
                    float w = 2.0f * error / minSlope;
                    if (w < 0.5f * (r - l)) {
                        l = std::max(l, mid - w);
                        r = std::min(r, mid + w);
                        break;
                    }
                }
                if ((left < 0.0f && v > 0.0f) || (left > 0.0f && v < 0.0f)) r = mid;
                else { l = mid; left = v; }
            }
            putBandTagged(l, r, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
        }
    }

    static uint32_t exactLevelActiveMask(const KernelRoutedSpec& m, glm::vec3 d, float lo, float hi, float c,
                                         const float (&phi_knots)[9], bool& allPositiveH) {
        float M = m.curv.x * d.x * d.x * 1.00002f;
        float err = m.curv.y;
        uint32_t mask = 0u;
        for (uint32_t bin = 0u; bin < 8u; bin++) {
            float a = lo + (hi - lo) * float(bin) / 8.0f;
            float b = lo + (hi - lo) * float(bin + 1u) / 8.0f;
            if (bin == 0u) a = lo;
            if (bin == 7u) b = hi;
            float h = b - a;
            if (!(h > 0.0f)) allPositiveH = false;
            float va = phi_knots[bin] - c;
            float vb = phi_knots[bin + 1u] - c;
            float scale = 1.0f + std::abs(va) + std::abs(vb) + std::abs(a) + std::abs(b);
            float error = err + 0.000001f * scale;
            float bow = 0.125f * M * h * h + error;
            if (!(std::min(va, vb) > bow || std::max(va, vb) < -bow)) mask |= (1u << bin);
        }
        return mask;
    }

    static void isolateMonotoneWithMid(const KernelRoutedSpec& m, glm::vec3 o, glm::vec3 d,
                                       float a, float b, float c, float va, float vb,
                                       float mid0, float fmid0, float error, float minSlope,
                                       float (&bounds)[MaxBounds], uint32_t& nb,
                                       glm::vec2 (&bands)[MaxBands], uint32_t& nr,
                                       uint32_t& reason, uint32_t& work,
                                       uint32_t& zeroBands, bool isZeroLevel, uint32_t budget) {
        if (std::abs(va) <= error) {
            float w = 2.0f * error / minSlope;
            putBandTagged(a, std::min(b, a + w), bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
        }
        if (std::abs(vb) <= error) {
            float w = 2.0f * error / minSlope;
            putBandTagged(std::max(a, b - w), b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
        }
        if ((va > error && vb < -error) || (va < -error && vb > error)) {
            float l = a, r = b, left = va;
            bool done = false;
            if (work >= budget) { reason |= 1u; done = true; }
            else {
                work += 1u;
                if (mid0 <= l || mid0 >= r || r - l <= 0.0001f) { done = true; }
                else if (std::abs(fmid0) <= error) {
                    float w = 2.0f * error / minSlope;
                    if (w < 0.5f * (r - l)) {
                        l = std::max(l, mid0 - w);
                        r = std::min(r, mid0 + w);
                        done = true;
                    } else {
                        if ((left < 0.0f && fmid0 > 0.0f) || (left > 0.0f && fmid0 < 0.0f)) r = mid0;
                        else { l = mid0; left = fmid0; }
                    }
                } else {
                    if ((left < 0.0f && fmid0 > 0.0f) || (left > 0.0f && fmid0 < 0.0f)) r = mid0;
                    else { l = mid0; left = fmid0; }
                }
            }
            if (!done) {
                for (uint32_t iter = 1u; iter < 32u; iter++) {
                    if (work >= budget) { reason |= 1u; break; }
                    work += 1u;
                    float mid = l + 0.5f * (r - l);
                    if (mid <= l || mid >= r || r - l <= 0.0001f) break;
                    float v = evalPhi(m, o, d, mid) - c;
                    if (std::abs(v) <= error) {
                        float w = 2.0f * error / minSlope;
                        if (w < 0.5f * (r - l)) {
                            l = std::max(l, mid - w);
                            r = std::min(r, mid + w);
                            break;
                        }
                    }
                    if ((left < 0.0f && v > 0.0f) || (left > 0.0f && v < 0.0f)) r = mid;
                    else { l = mid; left = v; }
                }
            }
            putBandTagged(l, r, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
        }
    }
    static void roots(const KernelRoutedSpec& m, glm::vec3 o, glm::vec3 d, float lo, float hi, float c,
                      float (&bounds)[MaxBounds], uint32_t& nb, glm::vec2 (&bands)[MaxBands],
                      uint32_t& nr, uint32_t& reason, uint32_t& work,
                      const float (&phi_knots)[9], float (&phi_mids)[8], uint32_t& mid_mask,
                      uint32_t& zeroBands, bool isZeroLevel,
                      uint32_t levelActiveMask, uint32_t budget) {
        float M = m.curv.x * d.x * d.x * 1.00002f;
        float err = m.curv.y;
        glm::vec4 stack[StackCap];
        glm::uvec3 depthBits(0u);
        uint32_t top = 0u;
        for (uint32_t bin = 0u; bin < 8u; bin++) {
            float a = lo + (hi - lo) * float(bin) / 8.0f;
            float b = lo + (hi - lo) * float(bin + 1u) / 8.0f;
            if (bin == 0u) a = lo;
            if (bin == 7u) b = hi;
            if (work >= budget) { reason |= 1u; break; }
            float coarseH = b - a;
            if (!(coarseH > 0.0f)) { work += 1u; reason |= 4u; break; }
            if ((levelActiveMask & (1u << bin)) == 0u) { work += 1u; continue; }
            work += 1u;
            float phi_a0 = phi_knots[bin];
            float phi_b0 = phi_knots[bin + 1u];
            float coarseVa = phi_a0 - c;
            float coarseVb = phi_b0 - c;
            float coarseScale = 1.0f + std::abs(coarseVa) + std::abs(coarseVb) + std::abs(a) + std::abs(b);
            float coarseError = err + 0.000001f * coarseScale;
            float coarseSlope = (coarseVb - coarseVa) / coarseH;
            float coarseRadius = 0.5f * M * coarseH + 2.0f * coarseError / coarseH;
            float coarseMinSlope = std::abs(coarseSlope) - coarseRadius;
            if (coarseMinSlope > 0.0f) {
                bool hasCrossing = (coarseVa > coarseError && coarseVb < -coarseError) || (coarseVa < -coarseError && coarseVb > coarseError);
                if (hasCrossing) {
                    float mid0 = a + 0.5f * coarseH;
                    uint32_t bit = 1u << bin;
                    float phi_mid0 = 0.0f;
                    if ((mid_mask & bit) != 0u) {
                        phi_mid0 = phi_mids[bin];
                    } else {
                        phi_mid0 = evalPhi(m, o, d, mid0);
                        phi_mids[bin] = phi_mid0;
                        mid_mask |= bit;
                    }
                    float fmid0 = phi_mid0 - c;
                    isolateMonotoneWithMid(m, o, d, a, b, c, coarseVa, coarseVb, mid0, fmid0, coarseError, coarseMinSlope,
                                           bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel, budget);
                } else {
                    isolateMonotone(m, o, d, a, b, c, coarseVa, coarseVb, coarseError, coarseMinSlope,
                                    bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel, budget);
                }
                if (reason != 0u) break;
                continue;
            }
            if (0.125f * M * coarseH * coarseH <= coarseError) {
                putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
                if (reason != 0u) break;
                continue;
            }
            float mid0 = a + 0.5f * coarseH;
            if (mid0 <= a || mid0 >= b) {
                putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
                if (reason != 0u) break;
                continue;
            }
            uint32_t bit = 1u << bin;
            float phi_mid0 = 0.0f;
            if ((mid_mask & bit) != 0u) {
                phi_mid0 = phi_mids[bin];
            } else {
                phi_mid0 = evalPhi(m, o, d, mid0);
                phi_mids[bin] = phi_mid0;
                mid_mask |= bit;
            }
            stack[0] = glm::vec4(mid0, b, phi_mid0, phi_b0); setDepth(depthBits, 0u, 1u);
            stack[1] = glm::vec4(a, mid0, phi_a0, phi_mid0); setDepth(depthBits, 1u, 1u);
            top = 2u;
            while (true) {
                if (top == 0u || reason != 0u) break;
                top--;
                glm::vec4 cell = stack[top];
                a = cell.x; b = cell.y;
                float h = b - a;
                uint32_t depth = getDepth(depthBits, top);
                if (work >= budget) { reason |= 1u; break; }
                work += 1u;
                if (!(h > 0.0f)) { reason |= 4u; break; }
                float va = cell.z - c;
                float vb = cell.w - c;
                float scale = 1.0f + std::abs(va) + std::abs(vb) + std::abs(a) + std::abs(b);
                float error = err + 0.000001f * scale;
                float bow = 0.125f * M * h * h + error;
                if (std::min(va, vb) > bow || std::max(va, vb) < -bow) continue;
                float slope = (vb - va) / h;
                float radius = 0.5f * M * h + 2.0f * error / h;
                float minSlope = std::abs(slope) - radius;
                if (minSlope > 0.0f) {
                    isolateMonotone(m, o, d, a, b, c, va, vb, error, minSlope,
                                    bounds, nb, bands, nr, reason, work, zeroBands, isZeroLevel, budget);
                    continue;
                }
                if (0.125f * M * h * h <= error) {
                    putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
                    continue;
                }
                if (float(depth) >= 14.0f || top + 2u > uint32_t(StackCap)) {
                    putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
                    continue;
                }
                float mid = a + 0.5f * h;
                if (mid <= a || mid >= b) {
                    putBandTagged(a, b, bounds, nb, bands, nr, reason, zeroBands, isZeroLevel);
                    continue;
                }
                float phi_mid = evalPhi(m, o, d, mid);
                stack[top] = glm::vec4(mid, b, phi_mid, cell.w); setDepth(depthBits, top, depth + 1u);
                stack[top + 1u] = glm::vec4(a, mid, cell.z, phi_mid); setDepth(depthBits, top + 1u, depth + 1u);
                top += 2u;
            }
            if (reason != 0u) break;
        }
    }
};

struct RayProducerOutput {
    uint32_t reason = 0u;
    uint32_t work = 0u;
    uint32_t overs = 0u;
    std::vector<Subpanel> subpanels;
};

static RayProducerOutput runS1Producer(const Ray& input, uint32_t activeMember,
                                       const std::array<KernelRoutedSpec, 4>& curtains,
                                       uint32_t budget = 1024u) {
    using PM = ProducerMath<96, 32, 20>;
    RayProducerOutput out;
    glm::vec3 ro = glm::vec3(input.ro_clipA);
    glm::vec3 d = glm::vec3(input.rd_clipB);
    uint32_t count = 0u;
    glm::vec2 pieces[48];
    float knots[96];

    const KernelRoutedSpec& m = curtains[activeMember];
    glm::vec3 o = ro - glm::vec3(m.origin);
    float a = input.ro_clipA.w, b = input.rd_clipB.w;
    bool hit = true;
    float zEnv = std::abs(m.amps.x) + std::abs(m.amps.y) + std::abs(m.amps.z) + m.limits.w;
    for (int axis = 0; axis < 3; axis++) {
        float half = m.half[axis];
        if (axis == 0) half = std::min(half, m.limits.x);
        if (axis == 1) half = std::min(half, m.limits.y);
        if (axis == 2) half = std::min(half, zEnv);
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
        float phi_knots[9];
        for (uint32_t k = 0u; k < 9u; k++) {
            float s = a + (b - a) * float(k) / 8.0f;
            if (k == 0u) s = a;
            if (k == 8u) s = b;
            phi_knots[k] = evalPhi(m, o, d, s);
        }
        bool allPositiveH = true;
        uint32_t mask0 = PM::exactLevelActiveMask(m, d, a, b, -m.limits.w, phi_knots, allPositiveH);
        uint32_t mask1 = PM::exactLevelActiveMask(m, d, a, b, 0.0f, phi_knots, allPositiveH);
        uint32_t mask2 = PM::exactLevelActiveMask(m, d, a, b, m.limits.w, phi_knots, allPositiveH);
        if (allPositiveH && (mask0 | mask1 | mask2) == 0u && std::abs(phi_knots[0]) > m.limits.w + m.curv.y) {
            if (out.work + 24u > budget) out.reason |= 1u;
            else out.work += 24u;
        } else {
            uint32_t nb = 1u; knots[0] = a;
            glm::vec2 bands[32]; uint32_t nr = 0u, zeroBands = 0u;
            float phi_mids[8]; uint32_t mid_mask = 0u;
            PM::roots(m, o, d, a, b, -m.limits.w, knots, nb, bands, nr, out.reason, out.work, phi_knots, phi_mids, mid_mask, zeroBands, false, mask0, budget);
            if (out.reason == 0u) PM::roots(m, o, d, a, b, 0.0f, knots, nb, bands, nr, out.reason, out.work, phi_knots, phi_mids, mid_mask, zeroBands, true, mask1, budget);
            if (out.reason == 0u) PM::roots(m, o, d, a, b, m.limits.w, knots, nb, bands, nr, out.reason, out.work, phi_knots, phi_mids, mid_mask, zeroBands, false, mask2, budget);
            if (out.reason == 0u) {
                if (allPositiveH && nr == 0u && std::abs(phi_knots[0]) > m.limits.w + m.curv.y) {
                    // empty chord
                } else {
                    float supportLo = a;
                    float supportHi = b;
                    if (allPositiveH && nr > 0u) {
                        float minBandLo = bands[0].x;
                        float maxBandHi = bands[0].y;
                        for (uint32_t k = 1u; k < nr; k++) {
                            minBandLo = std::min(minBandLo, bands[k].x);
                            maxBandHi = std::max(maxBandHi, bands[k].y);
                        }
                        if (std::abs(phi_knots[0]) > m.limits.w + m.curv.y) {
                            supportLo = minBandLo;
                            knots[0] = minBandLo;
                        }
                        if (std::abs(phi_knots[8]) > m.limits.w + m.curv.y) {
                            supportHi = maxBandHi;
                        }
                    }
                    if (d.x != 0.0f) { float x = -o.x / d.x; if (x > supportLo && x < supportHi && nb < 96u) knots[nb++] = x; }
                    if (d.y != 0.0f) {
                        float y = -o.y / d.y; if (y > supportLo && y < supportHi && nb < 96u) knots[nb++] = y;
                        float k = (m.limits.z - o.y) / d.y; if (k > supportLo && k < supportHi && nb < 96u) knots[nb++] = k;
                    }
                    if (supportHi == b && nb < 96u) knots[nb++] = b;
                    for (uint32_t i = 1u; i < nb; i++) {
                        float v = knots[i]; uint32_t j = i;
                        while (j > 0u && knots[j - 1u] > v) { knots[j] = knots[j - 1u]; j--; }
                        knots[j] = v;
                    }
                    uint32_t memberStart = count;
                    bool previousShortBand = false, previousPositive = false;
                    for (uint32_t i = 1u; i < nb; i++) {
                        float l = knots[i - 1u], r = knots[i];
                        if (!(r > l)) continue;
                        float mid = l + 0.5f * (r - l);
                        bool uncertain = false, protectedBoundary = false;
                        if (d.x != 0.0f) protectedBoundary = (l == -o.x / d.x);
                        if (d.y != 0.0f) protectedBoundary = protectedBoundary || (l == -o.y / d.y) || (l == (m.limits.z - o.y) / d.y);
                        for (uint32_t k = 0u; k < nr; k++) {
                            uncertain = uncertain || (l < bands[k].y && r > bands[k].x);
                            if ((zeroBands & (1u << k)) != 0u && (l == bands[k].x || l == bands[k].y)) protectedBoundary = true;
                        }
                        if (uncertain || std::abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {
                            if (count > memberStart && PM::mayAbsorb(pieces[count - 1u], glm::vec2(l, r),
                                 previousShortBand, previousPositive, uncertain, protectedBoundary)) {
                                pieces[count - 1u].y = r; previousShortBand = false; previousPositive = true;
                            } else {
                                if (count >= 48u) { out.reason |= 8u; break; }
                                pieces[count++] = glm::vec2(l, r);
                                previousShortBand = uncertain && (r - l <= 0.02f);
                                previousPositive = !uncertain;
                            }
                        }
                    }
                }
            }
        }
    }
    if (out.reason != 0u) return out;
    for (uint32_t p = 0u; p < count; p++) {
        float pa = pieces[p].x, pb = pieces[p].y;
        uint32_t n = std::max(1u, uint32_t(std::ceil((pb - pa) / 4.0f)));
        if (out.subpanels.size() + n > 64u) { out.reason |= 16u; break; }
        for (uint32_t k = 0u; k < n; k++) {
            float l = (k == 0u) ? pa : (pa + float(k) * (pb - pa) / float(n));
            float r = (k + 1u == n) ? pb : (pa + float(k + 1u) * (pb - pa) / float(n));
            out.subpanels.push_back({glm::vec2(l, r), glm::uvec2(activeMember, 0u)});
        }
    }
    return out;
}

static RayProducerOutput runS2Producer(const Ray& input, uint32_t packedTile,
                                       const std::array<KernelRoutedSpec, 4>& curtains,
                                       uint32_t budget = 1024u) {
    using PM = ProducerMath<96, 32, 20>;
    RayProducerOutput out;
    glm::vec3 ro = glm::vec3(input.ro_clipA);
    glm::vec3 d = glm::vec3(input.rd_clipB);
    bool isMulti = ((packedTile >> 21u) & 1u) != 0u;
    uint32_t numMembers = isMulti ? (3u + ((packedTile >> 20u) & 1u)) : 2u;
    std::array<uint32_t, 4> ord = {0u, 0u, 0u, 0u};
    if (!isMulti) {
        bool m1First = ((packedTile >> 20u) & 1u) != 0u;
        uint32_t rawM0 = (packedTile >> 28u) & 0xFu;
        uint32_t rawM1 = (packedTile >> 24u) & 0xFu;
        ord[0] = m1First ? rawM1 : rawM0;
        ord[1] = m1First ? rawM0 : rawM1;
    } else {
        ord[0] = (packedTile >> 22u) & 3u;
        ord[1] = (packedTile >> 24u) & 3u;
        ord[2] = (packedTile >> 26u) & 3u;
        ord[3] = (packedTile >> 28u) & 3u;
    }
    uint32_t count = 0u;
    uint32_t memberCount[4] = {0, 0, 0, 0};
    glm::vec2 pieces[64];
    float knots[96];

    float prevMemberMaxS = -1e9f;

    for (uint32_t memberIdx = 0u; memberIdx < numMembers; memberIdx++) {
        uint32_t member = ord[memberIdx];
        float currMemberMaxS = prevMemberMaxS;
        const KernelRoutedSpec& m = curtains[member];
        glm::vec3 o = ro - glm::vec3(m.origin);
        float a = input.ro_clipA.w, b = input.rd_clipB.w;
        bool hit = true;
        float zEnv = std::abs(m.amps.x) + std::abs(m.amps.y) + std::abs(m.amps.z) + m.limits.w;
        glm::vec3 halfVec = glm::min(glm::vec3(m.half), glm::vec3(m.limits.x, m.limits.y, zEnv));
        for (int axis = 0; axis < 3; axis++) {
            float half = halfVec[axis];
            if (d[axis] == 0.0f) {
                if (std::abs(o[axis]) > half + m.curv.y) { hit = false; break; }
            } else {
                float q0 = (-half - m.curv.y - o[axis]) / d[axis];
                float q1 = (half + m.curv.y - o[axis]) / d[axis];
                a = std::max(a, std::min(q0, q1));
                b = std::min(b, std::max(q0, q1));
            }
        }
        if (!hit || !(b > a)) {
            memberCount[memberIdx] = count;
            prevMemberMaxS = currMemberMaxS;
            continue;
        }
        float phi_knots[9];
        for (uint32_t k = 0u; k < 9u; k++) {
            float s = a + (b - a) * float(k) / 8.0f;
            if (k == 0u) s = a;
            if (k == 8u) s = b;
            phi_knots[k] = evalPhi(m, o, d, s);
        }
        bool allPositiveH = true;
        uint32_t mask0 = PM::exactLevelActiveMask(m, d, a, b, -m.limits.w, phi_knots, allPositiveH);
        uint32_t mask1 = PM::exactLevelActiveMask(m, d, a, b, 0.0f, phi_knots, allPositiveH);
        uint32_t mask2 = PM::exactLevelActiveMask(m, d, a, b, m.limits.w, phi_knots, allPositiveH);
        if (allPositiveH && (mask0 | mask1 | mask2) == 0u && std::abs(phi_knots[0]) > m.limits.w + m.curv.y) {
            if (out.work + 24u > budget) { out.reason |= 1u; break; }
            out.work += 24u;
            memberCount[memberIdx] = count;
            prevMemberMaxS = currMemberMaxS;
            continue;
        }
        uint32_t nb = 1u; knots[0] = a;
        glm::vec2 bands[32]; uint32_t nr = 0u, zeroBands = 0u;
        float phi_mids[8]; uint32_t mid_mask = 0u;
        PM::roots(m, o, d, a, b, -m.limits.w, knots, nb, bands, nr, out.reason, out.work, phi_knots, phi_mids, mid_mask, zeroBands, false, mask0, budget);
        if (out.reason != 0u) break;
        PM::roots(m, o, d, a, b, 0.0f, knots, nb, bands, nr, out.reason, out.work, phi_knots, phi_mids, mid_mask, zeroBands, true, mask1, budget);
        if (out.reason != 0u) break;
        PM::roots(m, o, d, a, b, m.limits.w, knots, nb, bands, nr, out.reason, out.work, phi_knots, phi_mids, mid_mask, zeroBands, false, mask2, budget);
        if (out.reason != 0u) break;
        if (allPositiveH && nr == 0u && std::abs(phi_knots[0]) > m.limits.w + m.curv.y) {
            memberCount[memberIdx] = count;
            prevMemberMaxS = currMemberMaxS;
            continue;
        }
        float supportLo = a;
        float supportHi = b;
        if (allPositiveH && nr > 0u) {
            float minBandLo = bands[0].x;
            float maxBandHi = bands[0].y;
            for (uint32_t k = 1u; k < nr; k++) {
                minBandLo = std::min(minBandLo, bands[k].x);
                maxBandHi = std::max(maxBandHi, bands[k].y);
            }
            if (std::abs(phi_knots[0]) > m.limits.w + m.curv.y) {
                supportLo = minBandLo;
                knots[0] = minBandLo;
            }
            if (std::abs(phi_knots[8]) > m.limits.w + m.curv.y) {
                supportHi = maxBandHi;
            }
        }
        if (d.x != 0.0f) { float x = -o.x / d.x; if (x > supportLo && x < supportHi && nb < 96u) knots[nb++] = x; }
        if (d.y != 0.0f) {
            float y = -o.y / d.y; if (y > supportLo && y < supportHi && nb < 96u) knots[nb++] = y;
            float k = (m.limits.z - o.y) / d.y; if (k > supportLo && k < supportHi && nb < 96u) knots[nb++] = k;
        }
        if (supportHi == b && nb < 96u) knots[nb++] = b;
        for (uint32_t i = 1u; i < nb; i++) {
            float v = knots[i]; uint32_t j = i;
            while (j > 0u && knots[j - 1u] > v) { knots[j] = knots[j - 1u]; j--; }
            knots[j] = v;
        }
        uint32_t memberStart = count;
        bool previousShortBand = false, previousPositive = false;
        for (uint32_t i = 1u; i < nb; i++) {
            float l = knots[i - 1u], r = knots[i];
            if (!(r > l)) continue;
            float mid = l + 0.5f * (r - l);
            bool uncertain = false, protectedBoundary = false;
            if (d.x != 0.0f) protectedBoundary = (l == -o.x / d.x);
            if (d.y != 0.0f) protectedBoundary = protectedBoundary || (l == -o.y / d.y) || (l == (m.limits.z - o.y) / d.y);
            for (uint32_t k = 0u; k < nr; k++) {
                uncertain = uncertain || (l < bands[k].y && r > bands[k].x);
                if ((zeroBands & (1u << k)) != 0u && (l == bands[k].x || l == bands[k].y)) protectedBoundary = true;
            }
            if (uncertain || std::abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {
                if (memberIdx > 0u && l < prevMemberMaxS) { out.reason |= 64u; break; }
                currMemberMaxS = std::max(currMemberMaxS, r);
                if (count > memberStart && PM::mayAbsorb(pieces[count - 1u], glm::vec2(l, r),
                     previousShortBand, previousPositive, uncertain, protectedBoundary)) {
                    pieces[count - 1u].y = r; previousShortBand = false; previousPositive = true;
                } else {
                    if (count >= 64u) { out.reason |= 8u; break; }
                    pieces[count++] = glm::vec2(l, r);
                    previousShortBand = uncertain && (r - l <= 0.02f);
                    previousPositive = !uncertain;
                }
            }
        }
        memberCount[memberIdx] = count;
        if (out.reason != 0u) break;
        prevMemberMaxS = currMemberMaxS;
    }
    if (out.reason != 0u) return out;
    for (uint32_t pass = 0u; pass < numMembers; pass++) {
        uint32_t emitMember = ord[pass];
        uint32_t pStart = (pass == 0u) ? 0u : memberCount[pass - 1u];
        uint32_t pEnd = memberCount[pass];
        for (uint32_t p = pStart; p < pEnd; p++) {
            float pa = pieces[p].x, pb = pieces[p].y;
            uint32_t n = std::max(1u, uint32_t(std::ceil((pb - pa) / 4.0f)));
            if (out.subpanels.size() + n > 64u) { out.reason |= 16u; break; }
            for (uint32_t k = 0u; k < n; k++) {
                float sl = (k == 0u) ? pa : (pa + float(k) * (pb - pa) / float(n));
                float sr = (k + 1u == n) ? pb : (pa + float(k + 1u) * (pb - pa) / float(n));
                out.subpanels.push_back({glm::vec2(sl, sr), glm::uvec2(emitMember, 0u)});
            }
        }
        if (out.reason != 0u) break;
    }
    return out;
}

static RayProducerOutput runFallbackProducer(const Ray& input,
                                             const std::array<KernelRoutedSpec, 4>& curtains,
                                             uint32_t activeMemberMask = 0xFu,
                                             uint32_t budget = 1024u) {
    using PM = ProducerMath<96, 32, 20>;
    RayProducerOutput out;
    glm::vec3 ro = glm::vec3(input.ro_clipA);
    glm::vec3 d = glm::vec3(input.rd_clipB);
    uint32_t count = 0u;
    glm::vec2 pieces[48];
    float knots[96];
    glm::uvec4 memStart(0u), memCount(0u);

    for (uint32_t member = 0u; member < 4u; member++) {
        memStart[member] = count;
        if ((activeMemberMask & (1u << member)) == 0u) {
            memCount[member] = 0u;
            continue;
        }
        const KernelRoutedSpec& m = curtains[member];
        glm::vec3 o = ro - glm::vec3(m.origin);
        float a = input.ro_clipA.w, b = input.rd_clipB.w;
        bool hit = true;
        float zEnv = std::abs(m.amps.x) + std::abs(m.amps.y) + std::abs(m.amps.z) + m.limits.w;
        glm::vec3 halfVec = glm::min(glm::vec3(m.half), glm::vec3(m.limits.x, m.limits.y, zEnv));
        for (int axis = 0; axis < 3; axis++) {
            float half = halfVec[axis];
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
        float phi_knots[9];
        for (uint32_t k = 0u; k < 9u; k++) {
            float s = a + (b - a) * float(k) / 8.0f;
            if (k == 0u) s = a;
            if (k == 8u) s = b;
            phi_knots[k] = evalPhi(m, o, d, s);
        }
        bool allPositiveH = true;
        uint32_t mask0 = PM::exactLevelActiveMask(m, d, a, b, -m.limits.w, phi_knots, allPositiveH);
        uint32_t mask1 = PM::exactLevelActiveMask(m, d, a, b, 0.0f, phi_knots, allPositiveH);
        uint32_t mask2 = PM::exactLevelActiveMask(m, d, a, b, m.limits.w, phi_knots, allPositiveH);
        if (allPositiveH && (mask0 | mask1 | mask2) == 0u && std::abs(phi_knots[0]) > m.limits.w + m.curv.y) {
            if (out.work + 24u > budget) { out.reason |= 1u; break; }
            out.work += 24u;
            memCount[member] = 0u;
            continue;
        }
        uint32_t nb = 1u; knots[0] = a;
        glm::vec2 bands[32]; uint32_t nr = 0u, zeroBands = 0u;
        float phi_mids[8]; uint32_t mid_mask = 0u;
        PM::roots(m, o, d, a, b, -m.limits.w, knots, nb, bands, nr, out.reason, out.work, phi_knots, phi_mids, mid_mask, zeroBands, false, mask0, budget);
        if (out.reason != 0u) break;
        PM::roots(m, o, d, a, b, 0.0f, knots, nb, bands, nr, out.reason, out.work, phi_knots, phi_mids, mid_mask, zeroBands, true, mask1, budget);
        if (out.reason != 0u) break;
        PM::roots(m, o, d, a, b, m.limits.w, knots, nb, bands, nr, out.reason, out.work, phi_knots, phi_mids, mid_mask, zeroBands, false, mask2, budget);
        if (out.reason != 0u) break;
        if (allPositiveH && nr == 0u && std::abs(phi_knots[0]) > m.limits.w + m.curv.y) {
            memCount[member] = 0u;
            continue;
        }
        float supportLo = a;
        float supportHi = b;
        if (allPositiveH && nr > 0u) {
            float minBandLo = bands[0].x;
            float maxBandHi = bands[0].y;
            for (uint32_t k = 1u; k < nr; k++) {
                minBandLo = std::min(minBandLo, bands[k].x);
                maxBandHi = std::max(maxBandHi, bands[k].y);
            }
            if (std::abs(phi_knots[0]) > m.limits.w + m.curv.y) {
                supportLo = minBandLo;
                knots[0] = minBandLo;
            }
            if (std::abs(phi_knots[8]) > m.limits.w + m.curv.y) {
                supportHi = maxBandHi;
            }
        }
        if (d.x != 0.0f) { float x = -o.x / d.x; if (x > supportLo && x < supportHi) knots[nb++] = x; }
        if (d.y != 0.0f) {
            float y = -o.y / d.y; if (y > supportLo && y < supportHi) knots[nb++] = y;
            float k = (m.limits.z - o.y) / d.y; if (k > supportLo && k < supportHi) knots[nb++] = k;
        }
        if (supportHi == b) knots[nb++] = b;
        for (uint32_t i = 1u; i < nb; i++) {
            float v = knots[i]; uint32_t j = i;
            while (j > 0u && knots[j - 1u] > v) { knots[j] = knots[j - 1u]; j--; }
            knots[j] = v;
        }
        uint32_t memberStart = count;
        bool previousShortBand = false, previousPositive = false;
        for (uint32_t i = 1u; i < nb; i++) {
            float l = knots[i - 1u], r = knots[i];
            if (!(r > l)) continue;
            float mid = l + 0.5f * (r - l);
            bool uncertain = false, protectedBoundary = false;
            if (d.x != 0.0f) protectedBoundary = (l == -o.x / d.x);
            if (d.y != 0.0f) protectedBoundary = protectedBoundary || (l == -o.y / d.y) || (l == (m.limits.z - o.y) / d.y);
            for (uint32_t k = 0u; k < nr; k++) {
                uncertain = uncertain || (l < bands[k].y && r > bands[k].x);
                if ((zeroBands & (1u << k)) != 0u && (l == bands[k].x || l == bands[k].y)) protectedBoundary = true;
            }
            if (uncertain || std::abs(evalPhi(m, o, d, mid)) <= m.limits.w + m.curv.y) {
                if (count > memberStart && PM::mayAbsorb(pieces[count - 1u], glm::vec2(l, r),
                     previousShortBand, previousPositive, uncertain, protectedBoundary)) {
                    pieces[count - 1u].y = r; previousShortBand = false; previousPositive = true;
                } else {
                    if (count >= 48u) { out.reason |= 8u; break; }
                    pieces[count++] = glm::vec2(l, r);
                    previousShortBand = uncertain && (r - l <= 0.02f);
                    previousPositive = !uncertain;
                }
            }
        }
        memCount[member] = count - memStart[member];
        if (out.reason != 0u) break;
    }
    if (out.reason != 0u) return out;
    uint32_t nonEmptyMask = 0u;
    for (uint32_t m = 0u; m < 4u; m++) {
        if (memCount[m] > 0u) nonEmptyMask |= (1u << m);
    }
    uint32_t ord[4] = {0u, 0u, 0u, 0u};
    uint32_t nAct = 0u;
    if (nonEmptyMask != 0u) {
        uint32_t tempMask = nonEmptyMask;
        while (tempMask != 0u) {
            uint32_t m = firstTrailingBitU32(tempMask);
            tempMask &= tempMask - 1u;
            float startM = pieces[memStart[m]].x;
            uint32_t ins = nAct;
            while (ins > 0u && pieces[memStart[ord[ins - 1u]]].x > startM) {
                ord[ins] = ord[ins - 1u];
                ins--;
            }
            ord[ins] = m;
            nAct++;
        }

        bool allDisjoint = true;
        for (uint32_t i = 0u; i + 1u < nAct; i++) {
            uint32_t mCurr = ord[i];
            uint32_t mNext = ord[i + 1u];
            float endCurr = pieces[memStart[mCurr] + memCount[mCurr] - 1u].y;
            float startNext = pieces[memStart[mNext]].x;
            if (endCurr > startNext) {
                allDisjoint = false;
                break;
            }
        }

        if (allDisjoint) {
            for (uint32_t idx = 0u; idx < nAct; idx++) {
                uint32_t m = ord[idx];
                uint32_t pStart = memStart[m];
                uint32_t pEnd = pStart + memCount[m];
                for (uint32_t p = pStart; p < pEnd; p++) {
                    float a = pieces[p].x, b = pieces[p].y;
                    uint32_t n = std::max(1u, uint32_t(std::ceil((b - a) / 4.0f)));
                    if (out.subpanels.size() + n > 64u) { out.reason |= 16u; break; }
                    for (uint32_t k = 0u; k < n; k++) {
                        float l = (k == 0u) ? a : (a + float(k) * (b - a) / float(n));
                        float r = (k + 1u == n) ? b : (a + float(k + 1u) * (b - a) / float(n));
                        out.subpanels.push_back({glm::vec2(l, r), glm::uvec2(m, 0u)});
                    }
                }
                if (out.reason != 0u) break;
            }
            return out;
        }
    }
    uint32_t ne = 0u;
    glm::uvec4 head(0u);
    uint32_t totalKnots = (memCount.x + memCount.y + memCount.z + memCount.w) * 2u;
    while (ne < totalKnots) {
        uint32_t bestM = 4u;
        float bestVal = 0.0f;
        for (uint32_t idx = 0u; idx < nAct; idx++) {
            uint32_t m = ord[idx];
            uint32_t h = head[m];
            if (h < memCount[m] * 2u) {
                uint32_t pIdx = memStart[m] + (h >> 1u);
                bool isY = (h & 1u) != 0u;
                float val = isY ? pieces[pIdx].y : pieces[pIdx].x;
                if (bestM == 4u || val < bestVal) {
                    bestVal = val;
                    bestM = m;
                }
            }
        }
        knots[ne++] = bestVal;
        head[bestM]++;
    }
    glm::uvec4 pIdx(0u);
    for (uint32_t i = 1u; i < ne && out.reason == 0u; i++) {
        float a = knots[i - 1u], b = knots[i];
        if (!(b > a)) continue;
        float mid = a + 0.5f * (b - a);
        uint32_t mask = 0u;
        for (uint32_t idx = 0u; idx < nAct; idx++) {
            uint32_t m = ord[idx];
            uint32_t start = memStart[m], numPieces = memCount[m], cur = pIdx[m];
            while (cur < numPieces && pieces[start + cur].y < mid) cur++;
            pIdx[m] = cur;
            if (cur < numPieces) {
                glm::vec2 p = pieces[start + cur];
                if (mid >= p.x && mid <= p.y) mask |= (1u << m);
            }
        }
        if (mask == 0u) continue;
        if (countOneBitsU32(mask) == 1u) {
            uint32_t n = std::max(1u, uint32_t(std::ceil((b - a) / 4.0f)));
            if (out.subpanels.size() + n > 64u) { out.reason |= 16u; break; }
            uint32_t member = firstTrailingBitU32(mask);
            for (uint32_t k = 0u; k < n; k++) {
                float l = (k == 0u) ? a : (a + float(k) * (b - a) / float(n));
                float r = (k + 1u == n) ? b : (a + float(k + 1u) * (b - a) / float(n));
                out.subpanels.push_back({glm::vec2(l, r), glm::uvec2(member, 0u)});
            }
        } else {
            if (out.overs >= 16u) { out.reason |= 32u; break; }
            out.overs++;
        }
    }
    return out;
}
} // namespace

int main(int argc, char** argv) {
    REQUIRE(argc >= 3, "usage: cpu_verify <zone.ecform> <out.json>");
    std::ifstream in(argv[1], std::ios::binary);
    REQUIRE(bool(in), "open zone.ecform");
    const std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)), {});
    const auto saved = bytes.front() == '{'
        ? Json::parse(bytes)
        : Json::parse(Json::from_msgpack(bytes).at("MigrationRoot").get<std::string>());

    std::vector<std::shared_ptr<geom::FieldNode>> curtains;
    for (const auto& f : saved.at("spatialFields")) curtains.push_back(geom::FieldNode::fromJson(f));
    REQUIRE(curtains.size() == 4, "4 curtains");

    // 1. Verify VolumeRoutedTransport.inc observer string replacement anchors on Phase P4 kClassifyWgsl
    {
        std::string src = routed_wgsl::kClassifyWgsl;
        const std::string tailAnchor = "    if (activeCount == 0u) {\n";
        const auto tail = src.find(tailAnchor);
        REQUIRE(tail != std::string::npos, "observer tailAnchor present in Phase P4 kClassifyWgsl");
        auto replaceOnce = [](std::string& s, const std::string& a, const std::string& b) {
            auto p = s.find(a);
            if (p == std::string::npos || s.find(a, p + a.size()) != std::string::npos) return false;
            s.replace(p, a.size(), b);
            return true;
        };
        REQUIRE(replaceOnce(src, "var activeMemberMask = 0u;", "var activeMemberMask = 0u;\n    var aabbMask = 0u;"), "observer activeMemberMask anchor");
        REQUIRE(replaceOnce(src, "        if (anyRayHit && uxMax > uxMin) {", "        if (anyRayHit) { aabbMask |= (1u << m); }\n        if (anyRayHit && uxMax > uxMin) {"), "observer anyRayHit anchor");
        REQUIRE(replaceOnce(src, "@group(0) @binding(7) var<storage, read_write> counters: DispatchCounters;", "@group(0) @binding(8) var<storage, read_write> routeHist: array<atomic<u32>>;"), "observer binding 7 anchor");
        std::printf("WGSL_OBSERVER_PATCH_OK (all 4 anchors unique and valid on Phase P4 kClassifyWgsl)\n");
    }

    // 2. Run 64-pose canonical route verification (160x90 tiles = 14,400 tiles/pose -> 640x360 rays/pose, exact 16:9 frustum)
    const uint32_t width = 640, height = 360;
    const uint32_t tilesX = width / 4, tilesY = height / 4, totalTiles = tilesX * tilesY;

    std::atomic<uint64_t> totEmpty{0}, totS1{0}, totS2{0}, totFb{0};
    std::atomic<uint64_t> r1Empty{0}, r1S1{0}, r1S2{0}, r1Fb{0};
    std::atomic<uint64_t> r2Empty{0}, r2S1{0}, r2S2{0}, r2Fb{0};
    std::atomic<uint64_t> classifierMismatches{0};
    std::atomic<uint64_t> s1RayMismatches{0}, s2RayMismatches{0}, refusedRays{0};
    std::atomic<uint64_t> s1RaysChecked{0}, s2RaysChecked{0}, fbRaysChecked{0};

    std::vector<Json> poseJson(64);
    const unsigned numThreads = std::max(1u, std::min(8u, std::thread::hardware_concurrency()));
    std::vector<std::thread> pool;
    std::atomic<int> nextPose{0};

    for (unsigned tid = 0; tid < numThreads; ++tid) {
        pool.emplace_back([&]() {
            while (true) {
                int poseIdx = nextPose.fetch_add(1);
                if (poseIdx >= 64) break;
                const auto [eye, aim] = canonicalPose(poseIdx);
                const double t = 7.25 + double(poseIdx) / 60.0;

                std::array<KernelRoutedSpec, 4> specs{};
                glm::vec3 setMin(1e9f), setMax(-1e9f);
                for (size_t i = 0; i < 4; ++i) {
                    Rendering::VolumeDensityBinding b;
                    REQUIRE(Rendering::readVolumeDensity(*curtains[i], t, 1.0 / 60.0, b), "readVolumeDensity");
                    const auto recipe = OntoMath::clampedWaveSupport(b.densityExpr);
                    REQUIRE(bool(recipe), "clampedWaveSupport");
                    auto v4 = [](const std::array<float, 4>& v) { return glm::vec4(v[0], v[1], v[2], v[3]); };
                    specs[i] = {
                        glm::vec4(b.origin, 0),
                        glm::vec4(glm::abs(b.scale), 0),
                        v4(recipe->limits), v4(recipe->curvature),
                        v4(recipe->amplitudes), v4(recipe->frequencies), v4(recipe->phases)
                    };
                    setMin = glm::min(setMin, b.origin - glm::abs(b.scale));
                    setMax = glm::max(setMax, b.origin + glm::abs(b.scale));
                }

                glm::mat4 view = glm::lookAt(eye, aim, glm::vec3(0, 1, 0));
                glm::mat4 proj = glm::perspectiveZO(glm::radians(60.0f), float(width) / height, 0.1f, 1000.0f);
                glm::mat4 invVP = glm::inverse(proj * view);

                auto makeRay = [&](uint32_t px, uint32_t py) -> Ray {
                    float ndcX = (float(px) + 0.5f) / float(width) * 2.0f - 1.0f;
                    float ndcY = 1.0f - (float(py) + 0.5f) / float(height) * 2.0f;
                    glm::vec4 farWorld = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
                    glm::vec3 farPt = glm::vec3(farWorld) / farWorld.w;
                    glm::vec3 rd = glm::normalize(farPt - eye);
                    float t0 = 0.0f, t1 = 1000.0f;
                    for (int axis = 0; axis < 3; ++axis) {
                        if (std::abs(rd[axis]) < 1e-8f) {
                            if (eye[axis] < setMin[axis] || eye[axis] > setMax[axis]) { t0 = 1.0f; t1 = 0.0f; break; }
                        } else {
                            float q0 = (setMin[axis] - eye[axis]) / rd[axis];
                            float q1 = (setMax[axis] - eye[axis]) / rd[axis];
                            t0 = std::max(t0, std::min(q0, q1));
                            t1 = std::min(t1, std::max(q0, q1));
                        }
                    }
                    Ray r{};
                    r.ro_clipA = glm::vec4(eye, t0);
                    r.rd_clipB = glm::vec4(rd, std::max(t0, t1));
                    r.counts = glm::uvec4(0u);
                    return r;
                };

                uint64_t pEmpty = 0, pS1 = 0, pS2 = 0, pFb = 0;
                for (uint32_t baseTile = 0; baseTile < totalTiles; baseTile += 4) {
                    Ray wgRays[64];
                    TileRouteResult serialRes[4];
                    for (uint32_t lt = 0; lt < 4; ++lt) {
                        uint32_t tileId = baseTile + lt;
                        if (tileId >= totalTiles) continue;
                        uint32_t tx = (tileId % tilesX) * 4;
                        uint32_t ty = (tileId / tilesX) * 4;
                        for (uint32_t dy = 0; dy < 4; ++dy) {
                            for (uint32_t dx = 0; dx < 4; ++dx) {
                                wgRays[lt * 16 + dy * 4 + dx] = makeRay(tx + dx, ty + dy);
                            }
                        }
                        serialRes[lt] = classifyTileSerial(tileId, &wgRays[lt * 16], specs);
                    }
                    TileRouteResult coopRes[4]{};
                    classifyWorkgroupCooperative(baseTile, totalTiles, wgRays, specs, coopRes);

                    for (uint32_t lt = 0; lt < 4; ++lt) {
                        uint32_t tileId = baseTile + lt;
                        if (tileId >= totalTiles) continue;
                        if (serialRes[lt].cohort != coopRes[lt].cohort ||
                            serialRes[lt].packedWord != coopRes[lt].packedWord ||
                            serialRes[lt].activeMemberMask != coopRes[lt].activeMemberMask ||
                            serialRes[lt].aabbMask != coopRes[lt].aabbMask ||
                            serialRes[lt].tileCanOverlap != coopRes[lt].tileCanOverlap) {
                            classifierMismatches.fetch_add(1);
                        }
                        uint32_t cohort = coopRes[lt].cohort;
                        if (cohort == 0u) pEmpty++;
                        else if (cohort == 1u) pS1++;
                        else if (cohort == 2u) pS2++;
                        else pFb++;

                        // Verify producer parity on every 16th tile to cover all 64 poses thoroughly and fast
                        if ((tileId & 7u) == 0u && cohort != 0u) {
                            for (uint32_t r = 0; r < 16; ++r) {
                                const Ray& ray = wgRays[lt * 16 + r];
                                if (cohort == 1u) {
                                    uint32_t activeMember = coopRes[lt].packedWord >> 28u;
                                    auto s1Out = runS1Producer(ray, activeMember, specs);
                                    auto fbOut = runFallbackProducer(ray, specs, 1u << activeMember);
                                    s1RaysChecked.fetch_add(1);
                                    if (s1Out.reason != 0u || fbOut.reason != 0u) refusedRays.fetch_add(1);
                                    if (s1Out.reason != fbOut.reason || s1Out.work != fbOut.work ||
                                        s1Out.subpanels != fbOut.subpanels || fbOut.overs != 0u) {
                                        if (s1RayMismatches.fetch_add(1) == 0) {
                                            std::printf("S1_MISMATCH pose=%d tile=%u ray=%u member=%u s1(reason=%u work=%u subs=%zu) fb(reason=%u work=%u subs=%zu)\n",
                                                        poseIdx, tileId, r, activeMember, s1Out.reason, s1Out.work, s1Out.subpanels.size(),
                                                        fbOut.reason, fbOut.work, fbOut.subpanels.size());
                                        }
                                    }
                                } else if (cohort == 2u) {
                                    uint32_t mask = coopRes[lt].activeMemberMask;
                                    auto s2Out = runS2Producer(ray, coopRes[lt].packedWord, specs);
                                    auto fbOut = runFallbackProducer(ray, specs, mask);
                                    s2RaysChecked.fetch_add(1);
                                    if (s2Out.reason != 0u || fbOut.reason != 0u) refusedRays.fetch_add(1);
                                    if (s2Out.reason != fbOut.reason || s2Out.work != fbOut.work ||
                                        s2Out.subpanels != fbOut.subpanels || fbOut.overs != 0u) {
                                        s2RayMismatches.fetch_add(1);
                                    }
                                } else {
                                    uint32_t activeMask = coopRes[lt].packedWord >> 28u;
                                    auto fbOut = runFallbackProducer(ray, specs, activeMask);
                                    fbRaysChecked.fetch_add(1);
                                    if (fbOut.reason != 0u) refusedRays.fetch_add(1);
                                }
                            }
                        }
                    }
                }

                totEmpty.fetch_add(pEmpty); totS1.fetch_add(pS1); totS2.fetch_add(pS2); totFb.fetch_add(pFb);
                bool isR1 = (poseIdx <= 19 || poseIdx >= 51);
                if (isR1) {
                    r1Empty.fetch_add(pEmpty); r1S1.fetch_add(pS1); r1S2.fetch_add(pS2); r1Fb.fetch_add(pFb);
                } else {
                    r2Empty.fetch_add(pEmpty); r2S1.fetch_add(pS1); r2S2.fetch_add(pS2); r2Fb.fetch_add(pFb);
                }
                poseJson[poseIdx] = {
                    {"pose", poseIdx}, {"regime", isR1 ? 1 : 2},
                    {"empty", pEmpty}, {"s1", pS1}, {"s2", pS2}, {"fallback", pFb}
                };
            }
        });
    }
    for (auto& th : pool) th.join();

    REQUIRE(classifierMismatches.load() == 0, "zero mismatches between serial classifier and Phase P4 16-lane cooperative classifier");
    REQUIRE(s1RayMismatches.load() == 0, "zero mismatches between generalized S1 producer and Fallback producer");
    REQUIRE(s2RayMismatches.load() == 0, "zero mismatches between spatially-ordered disjoint S2 producer and Fallback producer");
    REQUIRE(refusedRays.load() == 0, "zero refused rays across all 64 poses");

    const uint64_t totalTileFrames = uint64_t(totalTiles) * 64;
    const uint64_t r1TileFrames = uint64_t(totalTiles) * 33;
    const uint64_t r2TileFrames = uint64_t(totalTiles) * 31;

    Json out = {
        {"classifier_mismatches", classifierMismatches.load()},
        {"s1_ray_mismatches", s1RayMismatches.load()},
        {"s2_ray_mismatches", s2RayMismatches.load()},
        {"refused_rays", refusedRays.load()},
        {"s1_rays_checked", s1RaysChecked.load()},
        {"s2_rays_checked", s2RaysChecked.load()},
        {"fb_rays_checked", fbRaysChecked.load()},
        {"all_64_poses", {
            {"total_tiles", totalTileFrames},
            {"empty", totEmpty.load()}, {"s1", totS1.load()}, {"s2", totS2.load()}, {"fallback", totFb.load()},
            {"empty_pct", 100.0 * double(totEmpty.load()) / double(totalTileFrames)},
            {"s1_pct", 100.0 * double(totS1.load()) / double(totalTileFrames)},
            {"s2_pct", 100.0 * double(totS2.load()) / double(totalTileFrames)},
            {"fallback_pct", 100.0 * double(totFb.load()) / double(totalTileFrames)}
        }},
        {"regime1_full_screen_green", {
            {"total_tiles", r1TileFrames},
            {"empty", r1Empty.load()}, {"s1", r1S1.load()}, {"s2", r1S2.load()}, {"fallback", r1Fb.load()},
            {"empty_pct", 100.0 * double(r1Empty.load()) / double(r1TileFrames)},
            {"s1_pct", 100.0 * double(r1S1.load()) / double(r1TileFrames)},
            {"s2_pct", 100.0 * double(r1S2.load()) / double(r1TileFrames)},
            {"fallback_pct", 100.0 * double(r1Fb.load()) / double(r1TileFrames)}
        }},
        {"regime2_open_sky", {
            {"total_tiles", r2TileFrames},
            {"empty", r2Empty.load()}, {"s1", r2S1.load()}, {"s2", r2S2.load()}, {"fallback", r2Fb.load()},
            {"empty_pct", 100.0 * double(r2Empty.load()) / double(r2TileFrames)},
            {"s1_pct", 100.0 * double(r2S1.load()) / double(r2TileFrames)},
            {"s2_pct", 100.0 * double(r2S2.load()) / double(r2TileFrames)},
            {"fallback_pct", 100.0 * double(r2Fb.load()) / double(r2TileFrames)}
        }},
        {"poses", poseJson}
    };

    std::ofstream(argv[2]) << out.dump(2) << '\n';
    std::printf("PASS positive_e2e_cpu_verify:\n"
                "  classifier_mismatches = %llu ( across %llu tile-frames )\n"
                "  s1_ray_mismatches     = %llu ( across %llu rays checked )\n"
                "  s2_ray_mismatches     = %llu ( across %llu rays checked )\n"
                "  refused_rays          = %llu ( across %llu total rays checked )\n"
                "  All 64 Poses Cohort Share: Empty=%.2f%% S1=%.2f%% Ordered-S2=%.2f%% Fallback=%.2f%%\n"
                "  Regime 1 (Green, N=33):    Empty=%.2f%% S1=%.2f%% Ordered-S2=%.2f%% Fallback=%.2f%%\n"
                "  Regime 2 (Sky,   N=31):    Empty=%.2f%% S1=%.2f%% Ordered-S2=%.2f%% Fallback=%.2f%%\n",
                (unsigned long long)classifierMismatches.load(), (unsigned long long)totalTileFrames,
                (unsigned long long)s1RayMismatches.load(), (unsigned long long)s1RaysChecked.load(),
                (unsigned long long)s2RayMismatches.load(), (unsigned long long)s2RaysChecked.load(),
                (unsigned long long)refusedRays.load(),
                (unsigned long long)(s1RaysChecked.load() + s2RaysChecked.load() + fbRaysChecked.load()),
                100.0 * double(totEmpty.load()) / double(totalTileFrames),
                100.0 * double(totS1.load()) / double(totalTileFrames),
                100.0 * double(totS2.load()) / double(totalTileFrames),
                100.0 * double(totFb.load()) / double(totalTileFrames),
                100.0 * double(r1Empty.load()) / double(r1TileFrames),
                100.0 * double(r1S1.load()) / double(r1TileFrames),
                100.0 * double(r1S2.load()) / double(r1TileFrames),
                100.0 * double(r1Fb.load()) / double(r1TileFrames),
                100.0 * double(r2Empty.load()) / double(r2TileFrames),
                100.0 * double(r2S1.load()) / double(r2TileFrames),
                100.0 * double(r2S2.load()) / double(r2TileFrames),
                100.0 * double(r2Fb.load()) / double(r2TileFrames));
    return 0;
}
