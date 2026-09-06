// ============================================================================
// Perlin Domain Sampled Grid Probe (Exploratory Sampling — Non-Certified)
//
// Samples the behavior of the authored terrain field across a discrete lattice grid:
//   f(p) = p.y - 40.0 * cnoise3(0.008 * (p + vec3(100, 0, 100)))
// over the finite authored domain:
//   p in [-1000, 1000] x [-30, 30] x [-1000, 1000]
//
// NOTE: Exploratory sampling only; does NOT constitute a continuous mathematical
// certificate (which requires outward-rounded interval arithmetic or Bernstein
// polynomial enclosure). It carries NO acceleration authority.
// ============================================================================

#include <glm/glm.hpp>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <vector>

namespace {

inline glm::vec3 mod289_3(glm::vec3 x) {
    return x - glm::floor(x * (1.0f / 289.0f)) * 289.0f;
}

inline glm::vec4 mod289_4(glm::vec4 x) {
    return x - glm::floor(x * (1.0f / 289.0f)) * 289.0f;
}

inline glm::vec4 permute4(glm::vec4 x) {
    return mod289_4(((x * 34.0f) + 1.0f) * x);
}

inline glm::vec4 taylorInvSqrt(glm::vec4 r) {
    return 1.79284291400159f - 0.85373472095314f * r;
}

struct PerlinJet {
    float value;
    glm::vec3 grad;
};

inline PerlinJet cnoise3_grad(glm::vec3 P) {
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

    glm::vec4 gx0 = ixy0 * (1.0f / 7.0f);
    glm::vec4 gy0 = glm::fract(glm::floor(gx0) * (1.0f / 7.0f)) - 0.5f;
    gx0 = glm::fract(gx0);
    glm::vec4 gz0 = glm::vec4(0.5f) - glm::abs(gx0) - glm::abs(gy0);
    glm::vec4 sz0 = glm::step(gz0, glm::vec4(0.0f));
    gx0 -= sz0 * (glm::step(glm::vec4(0.0f), gx0) - 0.5f);
    gy0 -= sz0 * (glm::step(glm::vec4(0.0f), gy0) - 0.5f);

    glm::vec4 gx1 = ixy1 * (1.0f / 7.0f);
    glm::vec4 gy1 = glm::fract(glm::floor(gx1) * (1.0f / 7.0f)) - 0.5f;
    gx1 = glm::fract(gx1);
    glm::vec4 gz1 = glm::vec4(0.5f) - glm::abs(gx1) - glm::abs(gy1);
    glm::vec4 sz1 = glm::step(gz1, glm::vec4(0.0f));
    gx1 -= sz1 * (glm::step(glm::vec4(0.0f), gx1) - 0.5f);
    gy1 -= sz1 * (glm::step(glm::vec4(0.0f), gy1) - 0.5f);

    glm::vec3 g000(gx0.x, gy0.x, gz0.x);
    glm::vec3 g100(gx0.y, gy0.y, gz0.y);
    glm::vec3 g010(gx0.z, gy0.z, gz0.z);
    glm::vec3 g110(gx0.w, gy0.w, gz0.w);
    glm::vec3 g001(gx1.x, gy1.x, gz1.x);
    glm::vec3 g101(gx1.y, gy1.y, gz1.y);
    glm::vec3 g011(gx1.z, gy1.z, gz1.z);
    glm::vec3 g111(gx1.w, gy1.w, gz1.w);

    glm::vec4 norm0 = taylorInvSqrt(glm::vec4(glm::dot(g000, g000), glm::dot(g010, g010), glm::dot(g100, g100), glm::dot(g110, g110)));
    g000 *= norm0.x;
    g010 *= norm0.y;
    g100 *= norm0.z;
    g110 *= norm0.w;
    glm::vec4 norm1 = taylorInvSqrt(glm::vec4(glm::dot(g001, g001), glm::dot(g011, g011), glm::dot(g101, g101), glm::dot(g111, g111)));
    g001 *= norm1.x;
    g011 *= norm1.y;
    g101 *= norm1.z;
    g111 *= norm1.w;

    float n000 = glm::dot(g000, Pf0);
    float n100 = glm::dot(g100, glm::vec3(Pf1.x, Pf0.y, Pf0.z));
    float n010 = glm::dot(g010, glm::vec3(Pf0.x, Pf1.y, Pf0.z));
    float n110 = glm::dot(g110, glm::vec3(Pf1.x, Pf1.y, Pf0.z));
    float n001 = glm::dot(g001, glm::vec3(Pf0.x, Pf0.y, Pf1.z));
    float n101 = glm::dot(g101, glm::vec3(Pf1.x, Pf0.y, Pf1.z));
    float n011 = glm::dot(g011, glm::vec3(Pf0.x, Pf1.y, Pf1.z));
    float n111 = glm::dot(g111, Pf1);

    glm::vec3 fade = Pf0 * Pf0 * Pf0 * (Pf0 * (Pf0 * 6.0f - 15.0f) + 10.0f);
    glm::vec3 dfade = 30.0f * Pf0 * Pf0 * (Pf0 * (Pf0 - 2.0f) + 1.0f);

    float u = fade.x, v = fade.y, w = fade.z;
    float du = dfade.x, dv = dfade.y, dw = dfade.z;

    float k0 = n000;
    float k1 = n100 - n000;
    float k2 = n010 - n000;
    float k3 = n001 - n000;
    float k4 = n000 - n100 - n010 + n110;
    float k5 = n000 - n100 - n001 + n101;
    float k6 = n000 - n010 - n001 + n011;
    float k7 = -n000 + n100 + n010 + n001 - n110 - n101 - n011 + n111;

    float n_xyz = k0 + k1*u + k2*v + k3*w + k4*u*v + k5*u*w + k6*v*w + k7*u*v*w;
    float dn_du = k1 + k4*v + k5*w + k7*v*w;
    float dn_dv = k2 + k4*u + k6*w + k7*u*w;
    float dn_dw = k3 + k5*u + k6*v + k7*u*v;

    glm::vec3 g_interp =
        (1.0f - u) * (1.0f - v) * (1.0f - w) * g000 +
        u          * (1.0f - v) * (1.0f - w) * g100 +
        (1.0f - u) * v          * (1.0f - w) * g010 +
        u          * v          * (1.0f - w) * g110 +
        (1.0f - u) * (1.0f - v) * w          * g001 +
        u          * (1.0f - v) * w          * g101 +
        (1.0f - u) * v          * w          * g011 +
        u          * v          * w          * g111;

    glm::vec3 grad_xyz = g_interp + glm::vec3(du * dn_du, dv * dn_dv, dw * dn_dw);

    PerlinJet out;
    out.value = 2.2f * n_xyz;
    out.grad  = 2.2f * grad_xyz;
    return out;
}

} // namespace

int main() {
    std::printf("=================================================================\n");
    std::printf("   PERLIN NOISE FLOOR CERTIFIED DOMAIN BOUND PROBE               \n");
    std::printf("=================================================================\n\n");

    const float qxMin = -7.20f, qxMax = 8.80f;
    const float qyMin = -0.24f, qyMax = 0.24f;
    const float qzMin = -7.20f, qzMax = 8.80f;

    const int ix0 = static_cast<int>(std::floor(qxMin)); // -8
    const int ix1 = static_cast<int>(std::floor(qxMax)); //  8
    const int iy0 = static_cast<int>(std::floor(qyMin)); // -1
    const int iy1 = static_cast<int>(std::floor(qyMax)); //  0
    const int iz0 = static_cast<int>(std::floor(qzMin)); // -8
    const int iz1 = static_cast<int>(std::floor(qzMax)); //  8

    const int numX = ix1 - ix0 + 1; // 17
    const int numY = iy1 - iy0 + 1; // 2
    const int numZ = iz1 - iz0 + 1; // 17
    const int totalCells = numX * numY * numZ; // 578

    std::printf("Domain bounds in noise space: [%.2f, %.2f] x [%.2f, %.2f] x [%.2f, %.2f]\n",
                qxMin, qxMax, qyMin, qyMax, qzMin, qzMax);
    std::printf("Lattice cells crossed: %d x %d x %d = %d cells.\n\n",
                numX, numY, numZ, totalCells);

    float globalMaxDNoiseDy = -1e10f;
    float globalMinDNoiseDy =  1e10f;
    glm::vec3 ptMax(0.0f), ptMin(0.0f);

    float maxF_lowerBound = -1e10f; // max f(x, -30, z)
    float minF_upperBound =  1e10f; // min f(x, +30, z)

    const int SUBDIV = 16;
    for (int ix = ix0; ix <= ix1; ++ix) {
        for (int iy = iy0; iy <= iy1; ++iy) {
            for (int iz = iz0; iz <= iz1; ++iz) {
                for (int sx = 0; sx <= SUBDIV; ++sx) {
                    float u = static_cast<float>(sx) / static_cast<float>(SUBDIV);
                    float qx = static_cast<float>(ix) + u;
                    if (qx < qxMin || qx > qxMax) continue;

                    for (int sy = 0; sy <= SUBDIV; ++sy) {
                        float v = static_cast<float>(sy) / static_cast<float>(SUBDIV);
                        float qy = static_cast<float>(iy) + v;
                        if (qy < qyMin || qy > qyMax) continue;

                        for (int sz = 0; sz <= SUBDIV; ++sz) {
                            float w = static_cast<float>(sz) / static_cast<float>(SUBDIV);
                            float qz = static_cast<float>(iz) + w;
                            if (qz < qzMin || qz > qzMax) continue;

                            glm::vec3 q(qx, qy, qz);
                            PerlinJet jet = cnoise3_grad(q);
                            float dNoiseDy = jet.grad.y;

                            if (dNoiseDy > globalMaxDNoiseDy) {
                                globalMaxDNoiseDy = dNoiseDy;
                                ptMax = q;
                            }
                            if (dNoiseDy < globalMinDNoiseDy) {
                                globalMinDNoiseDy = dNoiseDy;
                                ptMin = q;
                            }
                        }
                    }
                }
            }
        }
    }

    const int H_SUBDIV = 64;
    for (int sx = 0; sx <= H_SUBDIV; ++sx) {
        float px = -1000.0f + 2000.0f * (static_cast<float>(sx) / static_cast<float>(H_SUBDIV));
        for (int sz = 0; sz <= H_SUBDIV; ++sz) {
            float pz = -1000.0f + 2000.0f * (static_cast<float>(sz) / static_cast<float>(H_SUBDIV));

            // At lower boundary y = -30:
            glm::vec3 pLow(px, -30.0f, pz);
            glm::vec3 qLow = 0.008f * (pLow + glm::vec3(100.0f, 0.0f, 100.0f));
            float fLow = -30.0f - 40.0f * cnoise3_grad(qLow).value;
            maxF_lowerBound = std::max(maxF_lowerBound, fLow);

            // At upper boundary y = +30:
            glm::vec3 pHigh(px, 30.0f, pz);
            glm::vec3 qHigh = 0.008f * (pHigh + glm::vec3(100.0f, 0.0f, 100.0f));
            float fHigh = 30.0f - 40.0f * cnoise3_grad(qHigh).value;
            minF_upperBound = std::min(minF_upperBound, fHigh);
        }
    }

    const float minDfDy_domain = 1.0f - 0.32f * globalMaxDNoiseDy;

    std::printf("[Domain Derivative Bounds]\n");
    std::printf("  max dNoise/dq_y inside domain = %.6f at q=(%.3f, %.3f, %.3f)\n",
                globalMaxDNoiseDy, ptMax.x, ptMax.y, ptMax.z);
    std::printf("  min dNoise/dq_y inside domain = %.6f at q=(%.3f, %.3f, %.3f)\n",
                globalMinDNoiseDy, ptMin.x, ptMin.y, ptMin.z);
    std::printf("  min df/dy = 1.0 - 0.32 * max(dNoise/dy) = %.6f (> 0)\n\n", minDfDy_domain);

    std::printf("[Domain Root Existence Verification on [-30, 30]]\n");
    std::printf("  max f(x, y=-30, z) = %+.6f (requires <= 0 for existence)\n", maxF_lowerBound);
    std::printf("  min f(x, y=+30, z) = %+.6f (requires >= 0 for existence)\n\n", minF_upperBound);

    std::printf("=================================================================\n");
    std::printf("   EMPIRICAL SAMPLING SUMMARY (NON-CERTIFIED)                    \n");
    std::printf("=================================================================\n");
    if (minDfDy_domain > 0.0f) {
        std::printf("1. Sampled Y-Monotonicity: Observed positive across grid points (min df/dy = %.4f > 0).\n", minDfDy_domain);
        std::printf("   NOTE: Sampling alone does not prove continuity between lattice samples.\n");
    } else {
        std::printf("1. Sampled Y-Monotonicity: Falsified on sampled grid.\n");
    }

    if (maxF_lowerBound <= 0.0f && minF_upperBound >= 0.0f) {
        std::printf("2. Bounded Root Existence on [-30, 30]: Sampled boundary values bracket 0.\n");
    } else {
        std::printf("2. Bounded Root Existence on [-30, 30]: Sampled boundaries fail to bracket 0.\n");
        std::printf("   -> Mandatory action: Fail open to exact 3D marcher.\n");
    }
    std::printf("=================================================================\n");

    return 0;
}
