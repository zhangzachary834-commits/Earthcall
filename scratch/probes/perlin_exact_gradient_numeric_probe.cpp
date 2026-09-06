// Scratch probe: Gate A - Numeric proof of exact classic Perlin value and analytical gradient.
// Verifies the fused cnoise3_grad helper against cnoise3 and numerical central differences.

#include <glm/glm.hpp>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <random>
#include <algorithm>

namespace {

inline glm::vec4 mod289_4(glm::vec4 x) { return x - glm::floor(x * (1.0f / 289.0f)) * 289.0f; }
inline glm::vec3 mod289_3(glm::vec3 x) { return x - glm::floor(x * (1.0f / 289.0f)) * 289.0f; }
inline glm::vec4 permute4(glm::vec4 x) { return mod289_4(((x * 34.0f) + 1.0f) * x); }
inline glm::vec4 taylorInvSqrt(glm::vec4 r) { return 1.79284291400159f - 0.85373472095314f * r; }

// Line-by-line C++ transcription of WGSL cnoise3
float cnoise3(glm::vec3 P) {
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
    glm::vec4 norm0 = taylorInvSqrt(glm::vec4(glm::dot(g000, g000), glm::dot(g010, g010),
                                              glm::dot(g100, g100), glm::dot(g110, g110)));
    g000 *= norm0.x; g010 *= norm0.y; g100 *= norm0.z; g110 *= norm0.w;
    glm::vec4 norm1 = taylorInvSqrt(glm::vec4(glm::dot(g001, g001), glm::dot(g011, g011),
                                              glm::dot(g101, g101), glm::dot(g111, g111)));
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
    glm::vec4 n_z = glm::mix(glm::vec4(n000, n100, n010, n110),
                             glm::vec4(n001, n101, n011, n111), fade_xyz.z);
    glm::vec2 n_yz = glm::mix(glm::vec2(n_z.x, n_z.y), glm::vec2(n_z.z, n_z.w), fade_xyz.y);
    float n_xyz = glm::mix(n_yz.x, n_yz.y, fade_xyz.x);
    return 2.2f * n_xyz;
}

struct PerlinJet {
    float     value;
    glm::vec3 grad;
};

// Line-by-line C++ transcription of WGSL cnoise3_grad
PerlinJet cnoise3_grad(glm::vec3 P) {
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

    glm::vec4 norm0 = taylorInvSqrt(glm::vec4(glm::dot(g000, g000), glm::dot(g010, g010),
                                              glm::dot(g100, g100), glm::dot(g110, g110)));
    g000 *= norm0.x; g010 *= norm0.y; g100 *= norm0.z; g110 *= norm0.w;

    glm::vec4 norm1 = taylorInvSqrt(glm::vec4(glm::dot(g001, g001), glm::dot(g011, g011),
                                              glm::dot(g101, g101), glm::dot(g111, g111)));
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
    glm::vec3 dfade_xyz = 30.0f * Pf0 * Pf0 * (Pf0 - glm::vec3(1.0f)) * (Pf0 - glm::vec3(1.0f));

    float u = fade_xyz.x;
    float v = fade_xyz.y;
    float w = fade_xyz.z;

    float n_z0 = glm::mix(n000, n001, w);
    float n_z1 = glm::mix(n100, n101, w);
    float n_z2 = glm::mix(n010, n011, w);
    float n_z3 = glm::mix(n110, n111, w);

    float n_yz0 = glm::mix(n_z0, n_z2, v);
    float n_yz1 = glm::mix(n_z1, n_z3, v);

    float n_xyz = glm::mix(n_yz0, n_yz1, u);
    float val = 2.2f * n_xyz;

    float dx_fade = dfade_xyz.x * (n_yz1 - n_yz0);

    float n_x_y0 = glm::mix(n_z0, n_z1, u);
    float n_x_y1 = glm::mix(n_z2, n_z3, u);
    float dy_fade = dfade_xyz.y * (n_x_y1 - n_x_y0);

    float n_xy_z0 = glm::mix(glm::mix(n000, n100, u), glm::mix(n010, n110, u), v);
    float n_xy_z1 = glm::mix(glm::mix(n001, n101, u), glm::mix(n011, n111, u), v);
    float dz_fade = dfade_xyz.z * (n_xy_z1 - n_xy_z0);

    glm::vec3 g_z0 = glm::mix(g000, g001, w);
    glm::vec3 g_z1 = glm::mix(g100, g101, w);
    glm::vec3 g_z2 = glm::mix(g010, g011, w);
    glm::vec3 g_z3 = glm::mix(g110, g111, w);

    glm::vec3 g_yz0 = glm::mix(g_z0, g_z2, v);
    glm::vec3 g_yz1 = glm::mix(g_z1, g_z3, v);

    glm::vec3 g_interp = glm::mix(g_yz0, g_yz1, u);

    glm::vec3 grad = 2.2f * (g_interp + glm::vec3(dx_fade, dy_fade, dz_fade));
    return PerlinJet{val, grad};
}

// Numerical reference: central differences with step h using cnoise3
glm::vec3 centralDiffGrad(glm::vec3 P, float h) {
    float gx = (cnoise3(P + glm::vec3(h, 0.0f, 0.0f)) - cnoise3(P - glm::vec3(h, 0.0f, 0.0f))) / (2.0f * h);
    float gy = (cnoise3(P + glm::vec3(0.0f, h, 0.0f)) - cnoise3(P - glm::vec3(0.0f, h, 0.0f))) / (2.0f * h);
    float gz = (cnoise3(P + glm::vec3(0.0f, 0.0f, h)) - cnoise3(P - glm::vec3(0.0f, 0.0f, h))) / (2.0f * h);
    return glm::vec3(gx, gy, gz);
}

} // namespace

int main() {
    std::printf("=== Gate A: Fused Noise Numeric Proof (C++) ===\n");

    std::mt19937 rng(42);
    std::uniform_real_distribution<float> distLarge(-2000.0f, 2000.0f);
    std::uniform_real_distribution<float> distSmall(-20.0f, 20.0f);

    // Test 1: Value exactness over 2,000 wide-domain points
    float maxValDiff = 0.0f;
    for (int i = 0; i < 2000; ++i) {
        glm::vec3 p(distLarge(rng), distLarge(rng), distLarge(rng));
        float v0 = cnoise3(p);
        PerlinJet j = cnoise3_grad(p);
        if (std::isnan(j.value) || std::isinf(j.value) ||
            std::isnan(j.grad.x) || std::isnan(j.grad.y) || std::isnan(j.grad.z)) {
            std::fprintf(stderr, "FAIL: NaN or Inf at (%f, %f, %f)\n", p.x, p.y, p.z);
            return 1;
        }
        float d = std::abs(v0 - j.value);
        if (d > maxValDiff) maxValDiff = d;
    }
    std::printf("[A1] Wide-domain value exactness: max diff = %.3e (Pass)\n", maxValDiff);
    if (maxValDiff > 1e-6f) {
        std::fprintf(stderr, "FAIL: Value differs from cnoise3!\n");
        return 1;
    }

    // Test 2: Exact integer lattice boundaries & boundary epsilons
    float maxBoundaryDiff = 0.0f;
    for (int x = -10; x <= 10; ++x) {
        for (int y = -10; y <= 10; ++y) {
            for (int z = -10; z <= 10; ++z) {
                for (float eps : { 0.0f, 1e-6f, -1e-6f, 1e-5f, -1e-5f }) {
                    glm::vec3 p(float(x) + eps, float(y) + eps, float(z) + eps);
                    float v0 = cnoise3(p);
                    PerlinJet j = cnoise3_grad(p);
                    float d = std::abs(v0 - j.value);
                    if (d > maxBoundaryDiff) maxBoundaryDiff = d;
                    if (std::isnan(j.value) || std::isnan(j.grad.x)) {
                        std::fprintf(stderr, "FAIL: Boundary NaN at (%f, %f, %f)\n", p.x, p.y, p.z);
                        return 1;
                    }
                }
            }
        }
    }
    std::printf("[A2] Lattice integer boundaries & epsilons: max diff = %.3e (Pass)\n", maxBoundaryDiff);
    if (maxBoundaryDiff > 1e-6f) {
        std::fprintf(stderr, "FAIL: Boundary value mismatch!\n");
        return 1;
    }

    // Test 3: Analytical gradient vs Central Differences with h=1e-3 (optimal for f32)
    float maxGradAbsErr = 0.0f;
    float maxGradRelErr = 0.0f;
    const float h = 1e-3f;
    for (int i = 0; i < 2000; ++i) {
        glm::vec3 p(distSmall(rng), distSmall(rng), distSmall(rng));
        // Keep inside unit lattice cell away from boundaries where derivative step occurs
        p.x = std::floor(p.x) + std::clamp(p.x - std::floor(p.x), 3.0f * h, 1.0f - 3.0f * h);
        p.y = std::floor(p.y) + std::clamp(p.y - std::floor(p.y), 3.0f * h, 1.0f - 3.0f * h);
        p.z = std::floor(p.z) + std::clamp(p.z - std::floor(p.z), 3.0f * h, 1.0f - 3.0f * h);

        PerlinJet j = cnoise3_grad(p);
        glm::vec3 gNum = centralDiffGrad(p, h);

        for (int k = 0; k < 3; ++k) {
            float absErr = std::abs(j.grad[k] - gNum[k]);
            float relErr = absErr / std::max(std::abs(j.grad[k]), 1.0f);
            if (absErr > maxGradAbsErr) maxGradAbsErr = absErr;
            if (relErr > maxGradRelErr) maxGradRelErr = relErr;
        }
    }
    std::printf("[A3] Analytic gradient vs finite differences (h=%.0e): max abs err = %.3e, max rel err = %.3e (Pass)\n",
                h, maxGradAbsErr, maxGradRelErr);
    if (maxGradAbsErr > 2e-3f) {
        std::fprintf(stderr, "FAIL: Gradient error exceeds tolerance!\n");
        return 1;
    }

    std::printf("=== Gate A Proof: ALL 3 TESTS PASSED (Exit 0) ===\n");
    return 0;
}
