// Scratch probe: Deterministic March-Count Histogram across Camera Corpus.
// Measures exact ray-march step distribution, 192-step ceiling hits, and field evaluations
// without GPU timestamp machinery (no Flash phasing risk).

#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <cstdio>
#include <cmath>
#include <vector>
#include <algorithm>
#include <numeric>
#include <string>

namespace {

constexpr uint32_t W = 128, H = 128;

inline glm::vec4 mod289_4(glm::vec4 x) { return x - glm::floor(x * (1.0f / 289.0f)) * 289.0f; }
inline glm::vec3 mod289_3(glm::vec3 x) { return x - glm::floor(x * (1.0f / 289.0f)) * 289.0f; }
inline glm::vec4 permute4(glm::vec4 x) { return mod289_4(((x * 34.0f) + 1.0f) * x); }
inline glm::vec4 taylorInvSqrt(glm::vec4 r) { return 1.79284291400159f - 0.85373472095314f * r; }

struct PerlinJet { float value; glm::vec3 grad; };

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
    float n100_1 = glm::dot(g110, glm::vec3(Pf1.x, Pf1.y, Pf0.z));
    float n001 = glm::dot(g001, glm::vec3(Pf0.x, Pf0.y, Pf1.z));
    float n101 = glm::dot(g101, glm::vec3(Pf1.x, Pf0.y, Pf1.z));
    float n011 = glm::dot(g011, glm::vec3(Pf0.x, Pf1.y, Pf1.z));
    float n111 = glm::dot(g111, Pf1);

    glm::vec3 fade_xyz = Pf0 * Pf0 * Pf0 * (Pf0 * (Pf0 * 6.0f - 15.0f) + 10.0f);
    glm::vec3 d_fade   = Pf0 * Pf0 * (Pf0 * (Pf0 * 30.0f - 60.0f) + 30.0f);

    float u = fade_xyz.x, v = fade_xyz.y, w = fade_xyz.z;
    float du = d_fade.x,  dv = d_fade.y,  dw = d_fade.z;

    float k0 = n000;
    float k1 = n100 - n000;
    float k2 = n010 - n000;
    float k3 = n001 - n000;
    float k4 = n000 - n100 - n010 + n100_1;
    float k5 = n000 - n100 - n001 + n101;
    float k6 = n000 - n010 - n001 + n011;
    float k7 = -n000 + n100 + n010 + n001 - n100_1 - n101 - n011 + n111;

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

// Ray vs AABB intersection
inline glm::vec2 rayAabb(glm::vec3 ro, glm::vec3 rd, glm::vec3 b) {
    glm::vec3 rds = rd;
    if (std::abs(rds.x) < 1e-8f) rds.x = 1e-8f;
    if (std::abs(rds.y) < 1e-8f) rds.y = 1e-8f;
    if (std::abs(rds.z) < 1e-8f) rds.z = 1e-8f;
    glm::vec3 invRd = 1.0f / rds;
    glm::vec3 m = (b - ro) * invRd;
    glm::vec3 n = (-b - ro) * invRd;
    glm::vec3 kmin = glm::min(m, n);
    glm::vec3 kmax = glm::max(m, n);
    float tEnter = std::max(std::max(kmin.x, kmin.y), kmin.z);
    float tExit  = std::min(std::min(kmax.x, kmax.y), kmax.z);
    return glm::vec2(tEnter, tExit);
}

struct CameraCase {
    const char* name;
    glm::vec3 eye;
    glm::vec3 target;
    glm::vec3 up;
    float fovDeg;
};

const CameraCase cameraCorpus[] = {
    {"looking straight down",   {0.0f, 100.0f, 0.0f},    {0.0f, 0.0f, 0.0f},     {0.0f, 0.0f, -1.0f}, 60.0f},
    {"grazing the horizon",     {0.0f, 25.0f, -900.0f},  {0.0f, 20.0f, 900.0f},  {0.0f, 1.0f, 0.0f},  60.0f},
    {"45 degrees down",         {0.0f, 60.0f, -100.0f},  {0.0f, 0.0f, 0.0f},     {0.0f, 1.0f, 0.0f},  60.0f},
    {"close oblique",           {50.0f, 10.0f, 50.0f},   {0.0f, -5.0f, -50.0f},  {0.0f, 1.0f, 0.0f},  70.0f},
    {"near-parallel to ground", {0.0f, 5.0f, -300.0f},   {0.0f, 0.0f, 300.0f},   {0.0f, 1.0f, 0.0f},  50.0f},
    {"camera inside proxy",     {0.0f, 0.0f, 0.0f},      {0.0f, -5.0f, 50.0f},   {0.0f, 1.0f, 0.0f},  60.0f}
};

} // namespace

int main() {
    std::printf("=================================================================\n");
    std::printf("   DETERMINISTIC MARCH-COUNT HISTOGRAM ACROSS CAMERA CORPUS      \n");
    std::printf("   Resolution: %u x %u (%u pixels per frame)                     \n", W, H, W * H);
    std::printf("=================================================================\n\n");

    const glm::vec3 extent(1000.0f, 30.0f, 1000.0f);
    const glm::vec3 proxyExtent = extent * 1.05f;
    const float maxDim = 1000.0f;
    const float farField = 3000.0f;
    const float eps = 1e-4f;

    for (const auto& c : cameraCorpus) {
        const glm::mat4 proj = glm::perspectiveZO(glm::radians(c.fovDeg), 1.0f, 0.1f, 3000.0f);
        const glm::mat4 view = glm::lookAt(c.eye, c.target, c.up);
        const glm::mat4 invViewProj = glm::inverse(proj * view);

        std::vector<int> stepCounts;
        stepCounts.reserve(W * H);

        size_t hitRays = 0;
        size_t missRays = 0;
        size_t maxLimitReached = 0;
        size_t totalEvals = 0;

        for (uint32_t y = 0; y < H; ++y) {
            for (uint32_t x = 0; x < W; ++x) {
                // Screen to NDC [-1, 1]
                float ndcX = (static_cast<float>(x) + 0.5f) / static_cast<float>(W) * 2.0f - 1.0f;
                float ndcY = 1.0f - (static_cast<float>(y) + 0.5f) / static_cast<float>(H) * 2.0f;

                glm::vec4 nearPoint = invViewProj * glm::vec4(ndcX, ndcY, 0.0f, 1.0f);
                glm::vec4 farPoint  = invViewProj * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
                nearPoint /= nearPoint.w;
                farPoint  /= farPoint.w;

                glm::vec3 ro = c.eye;
                glm::vec3 rd = glm::normalize(glm::vec3(farPoint - nearPoint));

                glm::vec2 box = rayAabb(ro, rd, proxyExtent);
                if (box.x > box.y || box.y < 0.0f) {
                    // Misses proxy AABB entirely
                    stepCounts.push_back(0);
                    missRays++;
                    continue;
                }

                float t = std::max(box.x, 0.0f);
                float maxDist = std::min(std::min(box.y, t + maxDim * 8.0f), farField);

                int steps = 0;
                bool hit = false;
                float candidate_step = 0.0f;
                float prev_d = 1e10f;

                for (int i = 0; i < 192; ++i) {
                    if (t > maxDist) break;
                    steps++;
                    totalEvals++; // In Candidate A, fused value/gradient is 1 eval per step

                    glm::vec3 p = ro + rd * t;
                    float current_eps = std::max(eps, t * 0.001f);

                    // Evaluate Perlin terrain field
                    glm::vec3 q = 0.008f * (p + glm::vec3(100.0f, 0.0f, 100.0f));
                    PerlinJet jet = cnoise3_grad(q);
                    float d = p.y - 40.0f * jet.value;

                    // Lipschitz step in Candidate A
                    float gl = glm::length(glm::vec3(0.0f, 1.0f, 0.0f) - 0.32f * jet.grad);
                    if (gl > 1e-6f) d /= gl;

                    if (d <= 0.0f || std::abs(d) < current_eps) {
                        hit = true;
                        break;
                    }

                    candidate_step = std::max(d, current_eps);
                    prev_d = d;
                    t += candidate_step;
                }

                if (steps == 192 && !hit) {
                    maxLimitReached++;
                }

                if (hit) hitRays++;
                else missRays++;

                stepCounts.push_back(steps);
            }
        }

        std::sort(stepCounts.begin(), stepCounts.end());
        double sum = std::accumulate(stepCounts.begin(), stepCounts.end(), 0.0);
        double avg = sum / static_cast<double>(stepCounts.size());
        size_t p95Idx = static_cast<size_t>(0.95 * stepCounts.size());
        int p95 = stepCounts[std::min(p95Idx, stepCounts.size() - 1)];
        int maxSteps = stepCounts.back();
        double pctMaxLimit = 100.0 * static_cast<double>(maxLimitReached) / static_cast<double>(W * H);
        double pctHit = 100.0 * static_cast<double>(hitRays) / static_cast<double>(W * H);

        std::printf("-----------------------------------------------------------------\n");
        std::printf("CAMERA: %s\n", c.name);
        std::printf("  Eye:    (%.1f, %.1f, %.1f)\n", c.eye.x, c.eye.y, c.eye.z);
        std::printf("  Target: (%.1f, %.1f, %.1f)\n", c.target.x, c.target.y, c.target.z);
        std::printf("  Average steps/pixel:     %6.2f\n", avg);
        std::printf("  p95 steps/pixel:         %4d\n", p95);
        std::printf("  Maximum steps/pixel:     %4d\n", maxSteps);
        std::printf("  Reaching 192-step limit: %6.2f%% (%zu pixels)\n", pctMaxLimit, maxLimitReached);
        std::printf("  Hits / Misses:           %zu (%5.1f%%) / %zu (%5.1f%%)\n",
                    hitRays, pctHit, missRays, 100.0 - pctHit);
        std::printf("  Total field evaluations: %zu\n", totalEvals);
    }
    std::printf("=================================================================\n");
    return 0;
}
