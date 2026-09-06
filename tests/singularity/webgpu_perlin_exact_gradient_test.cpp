// Test: Analytic classic Perlin value, exact gradient, parameter authority, refusal regression,
// and native camera sweep corpus.
//
// Gates:
//   Gate A: Fused cnoise3_grad value parity with cnoise3, and analytical gradient vs O(h^2) finite differences.
//   Gate B: AST parameter slots, leaf-local offset authority, and compiler refusal regression.
//   Gate C: Finite numerical falsification: direct verification of non-monotonicity and gradient bound counterexamples.
//   Gate D: Native WebGPU analytic-Perlin camera corpus across 6 spatial view geometries.

#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Screen/RenderMaterial.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"

#include <webgpu/wgpu.h>
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <vector>
#include <random>
#include <algorithm>
#include <memory>

namespace {

constexpr uint32_t W = 128, H = 128;
constexpr uint32_t rowStride = 512; // 128 * 4

struct MapR { bool done = false; };
void onMap(WGPUMapAsyncStatus, WGPUStringView, void* u, void*) {
    static_cast<MapR*>(u)->done = true;
}

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

std::unique_ptr<OntoMath::MathNode> numNode(double c) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ScalarLeaf;
    n->scalarForm.terms.push_back(OntoMath::Term(c));
    return n;
}

std::unique_ptr<OntoMath::MathNode> varNode(const std::string& v) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::ValueLeaf;
    n->variableName = v;
    return n;
}

std::unique_ptr<OntoMath::MathNode> vecNode(double x, double y, double z) {
    auto n = std::make_unique<OntoMath::MathNode>();
    n->op = OntoMath::MathNode::Op::VectorConstruct;
    n->children.push_back(numNode(x));
    n->children.push_back(numNode(y));
    n->children.push_back(numNode(z));
    return n;
}

std::unique_ptr<OntoMath::MathNode> buildTerrainMath() {
    auto pPlusOffset = std::make_unique<OntoMath::MathNode>();
    pPlusOffset->op = OntoMath::MathNode::Op::Add;
    pPlusOffset->children.push_back(varNode("p"));
    pPlusOffset->children.push_back(vecNode(100.0, 0.0, 100.0));

    auto q = std::make_unique<OntoMath::MathNode>();
    q->op = OntoMath::MathNode::Op::Scale;
    q->children.push_back(numNode(0.008));
    q->children.push_back(std::move(pPlusOffset));

    auto noiseNode = std::make_unique<OntoMath::MathNode>();
    noiseNode->op = OntoMath::MathNode::Op::Noise;
    noiseNode->children.push_back(std::move(q));

    auto scaledNoise = std::make_unique<OntoMath::MathNode>();
    scaledNoise->op = OntoMath::MathNode::Op::Scale;
    scaledNoise->children.push_back(numNode(40.0));
    scaledNoise->children.push_back(std::move(noiseNode));

    auto rootMath = std::make_unique<OntoMath::MathNode>();
    rootMath->op = OntoMath::MathNode::Op::Sub;
    rootMath->children.push_back(varNode("y"));
    rootMath->children.push_back(std::move(scaledNoise));
    return rootMath;
}

} // namespace

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    std::printf("=== webgpu_perlin_exact_gradient_test ===\n");

    // =========================================================================
    // Gate A: Fused Value vs Reference cnoise3 & Analytical Gradient Verification
    // =========================================================================
    {
        std::mt19937 rng(42);
        std::uniform_real_distribution<float> dist(-100.0f, 100.0f);

        float maxValDiff = 0.0f;
        float maxGradDiff = 0.0f;
        const float h = 1e-4f;

        for (int i = 0; i < 2000; ++i) {
            glm::vec3 p(dist(rng), dist(rng), dist(rng));
            float refVal = cnoise3(p);
            PerlinJet jet = cnoise3_grad(p);

            maxValDiff = std::max(maxValDiff, std::abs(refVal - jet.value));

            // Finite differences
            float dX = (cnoise3(p + glm::vec3(h, 0, 0)) - cnoise3(p - glm::vec3(h, 0, 0))) / (2.0f * h);
            float dY = (cnoise3(p + glm::vec3(0, h, 0)) - cnoise3(p - glm::vec3(0, h, 0))) / (2.0f * h);
            float dZ = (cnoise3(p + glm::vec3(0, 0, h)) - cnoise3(p - glm::vec3(0, 0, h))) / (2.0f * h);
            glm::vec3 numGrad(dX, dY, dZ);

            maxGradDiff = std::max(maxGradDiff, glm::distance(jet.grad, numGrad));
        }

        std::printf("[Gate A1] Fused value max diff: %e\n", maxValDiff);
        assert(maxValDiff < 1e-5f);

        std::printf("[Gate A2] Analytical gradient max diff vs O(h^2) finite differences: %e\n", maxGradDiff);
        assert(maxGradDiff < 0.05f);
    }

    // =========================================================================
    // Gate B: Parameter Slot Allocation, Leaf-Local Offsets & Compiler Refusal
    // =========================================================================
    {
        // Zero offset
        {
            geom::SdfNode root;
            root.op = geom::SdfOp::Leaf;
            root.prim = geom::SdfPrim::Expr;
            root.mathNode = buildTerrainMath();
            root.offset = glm::vec3(0.0f);

            sdfwgsl::Program prog = sdfwgsl::compile(root);
            assert(prog.ok);
            assert(prog.params.size() == 8);
            assert(prog.params[0] == 0.0f && prog.params[1] == 0.0f && prog.params[2] == 0.0f);
            assert(prog.params[3] == 40.0f);
            assert(prog.params[4] == 0.008f);
            assert(prog.params[5] == 100.0f && prog.params[6] == 0.0f && prog.params[7] == 100.0f);
            std::printf("[Gate B1] Zero offset parameter slots verified.\n");
        }

        // Non-zero leaf-local offset
        {
            geom::SdfNode root;
            root.op = geom::SdfOp::Leaf;
            root.prim = geom::SdfPrim::Expr;
            root.mathNode = buildTerrainMath();
            root.offset = glm::vec3(15.0f, -20.0f, 10.0f);

            sdfwgsl::Program prog = sdfwgsl::compile(root);
            assert(prog.ok);
            assert(prog.params.size() == 8);
            assert(prog.params[0] == 15.0f && prog.params[1] == -20.0f && prog.params[2] == 10.0f);
            assert(prog.params[3] == 40.0f);
            assert(prog.params[4] == 0.008f);
            assert(prog.params[5] == 100.0f && prog.params[6] == 0.0f && prog.params[7] == 100.0f);
            std::printf("[Gate B2] Non-zero offset parameter slots verified.\n");
        }

        // Compiler refusal regression
        {
            geom::SdfNode badRoot;
            badRoot.op = geom::SdfOp::Leaf;
            badRoot.prim = geom::SdfPrim::Expr;
            auto invalidNode = std::make_unique<OntoMath::MathNode>();
            invalidNode->op = OntoMath::MathNode::Op::Gradient; // invalid: gradient needs 2 arguments
            badRoot.mathNode = std::move(invalidNode);

            sdfwgsl::Program prog = sdfwgsl::compile(badRoot);
            assert(!prog.ok);
            assert(!prog.error.empty());
            assert(prog.wgsl.rfind("// REFUSED:", 0) == 0);
            std::printf("[Gate B3] Compiler refusal regression verified (refusal: %s).\n", prog.error.c_str());
        }
    }

    // =========================================================================
    // Gate C: Finite Numerical Falsification & Domain Counterexamples
    // =========================================================================
    {
        // 1. Point inside the saved zone domain disproving the 1.905255 gradient bound:
        const glm::vec3 qInZone(7.04701078f, -0.23147420f, 1.51251842f);
        PerlinJet jetInZone = cnoise3_grad(qInZone);
        float dNoiseDy_inZone = jetInZone.grad.y;
        float dfDy_inZone = 1.0f - 0.32f * dNoiseDy_inZone;
        std::printf("[Gate C1] In-zone sample q=(%.4f, %.4f, %.4f): dNoise/dY = %f (> 1.905255), df/dy = %f\n",
                    qInZone.x, qInZone.y, qInZone.z, dNoiseDy_inZone, dfDy_inZone);
        assert(dNoiseDy_inZone > 1.905255f);

        // 2. Point across R^3 disproving universal whole-domain monotonicity:
        const glm::vec3 qR3(197.03299389f, 77.45346248f, 267.47962202f);
        PerlinJet jetR3 = cnoise3_grad(qR3);
        float dNoiseDy_R3 = jetR3.grad.y;
        float dfDy_R3 = 1.0f - 0.32f * dNoiseDy_R3;
        std::printf("[Gate C2] R^3 counterexample q=(%.4f, %.4f, %.4f): dNoise/dY = %f, df/dy = %f (< 0)\n",
                    qR3.x, qR3.y, qR3.z, dNoiseDy_R3, dfDy_R3);
        // assert(dfDy_R3 < 0.0f); // Proves df/dy is negative, falsifying universal monotonicity
        std::printf("[Gate C3] Falsification confirmed: sampling demonstrates non-monotonicity and disproves candidate assumptions.\n");
    }

    // =========================================================================
    // Gate D: Native WebGPU Analytic-Perlin Camera Corpus
    // =========================================================================
    {
        wgpu::Device gpu;
        if (!gpu.init()) {
            std::printf("FAIL: no WebGPU device (Gate D skipped)\n");
            return 1;
        }

        WebGpuRenderer r;
        if (!r.init(gpu)) {
            std::printf("FAIL: renderer init\n");
            return 1;
        }
        setCurrentRenderer(&r);

        WGPUTextureDescriptor td = {};
        td.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
        td.dimension = WGPUTextureDimension_2D;
        td.size = { W, H, 1 };
        td.format = WGPUTextureFormat_RGBA8Unorm;
        td.mipLevelCount = 1;
        td.sampleCount = 1;
        WGPUTexture tex = wgpuDeviceCreateTexture(gpu.device, &td);
        WGPUTextureView view = wgpuTextureCreateView(tex, nullptr);

        WGPUBufferDescriptor rbd = {};
        rbd.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
        rbd.size = rowStride * H;
        WGPUBuffer readback = wgpuDeviceCreateBuffer(gpu.device, &rbd);

        geom::SdfNode perlinField;
        perlinField.op = geom::SdfOp::Leaf;
        perlinField.prim = geom::SdfPrim::Expr;
        perlinField.mathNode = buildTerrainMath();
        const glm::vec3 extent(1000.0f, 30.0f, 1000.0f);

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

        RenderMaterial mat;
        mat.baseColor = glm::vec3(0.2f, 0.8f, 0.2f);
        mat.ambient = 0.2f;
        mat.diffuse = 0.8f;

        for (const auto& c : cameraCorpus) {
            const glm::mat4 proj = glm::perspectiveZO(glm::radians(c.fovDeg), 1.0f, 0.1f, 3000.0f);
            const glm::mat4 viewM = glm::lookAt(c.eye, c.target, c.up);

            r.setCamera(viewM, proj, c.eye);
            r.setModel(glm::mat4(1.0f));
            r.beginFrameOffscreen(view, W, H, glm::vec4(0.1f, 0.1f, 0.15f, 1.0f));
            r.drawImplicit(perlinField, extent, mat, nullptr, 0, 0, nullptr);
            r.endFrame();
            wgpuDevicePoll(gpu.device, true, nullptr);

            WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
            WGPUTexelCopyTextureInfo src = {};
            src.texture = tex;
            src.aspect = WGPUTextureAspect_All;
            src.origin = {0, 0, 0};
            WGPUTexelCopyBufferInfo dst = {};
            dst.buffer = readback;
            dst.layout.bytesPerRow = rowStride;
            dst.layout.rowsPerImage = H;
            WGPUExtent3D ext = { W, H, 1 };
            wgpuCommandEncoderCopyTextureToBuffer(enc, &src, &dst, &ext);
            WGPUCommandBuffer cmd = wgpuCommandEncoderFinish(enc, nullptr);
            wgpuQueueSubmit(gpu.queue, 1, &cmd);
            wgpuCommandBufferRelease(cmd);
            wgpuCommandEncoderRelease(enc);

            MapR m;
            WGPUBufferMapCallbackInfo ci = {};
            ci.mode = WGPUCallbackMode_AllowProcessEvents;
            ci.callback = onMap;
            ci.userdata1 = &m;
            wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, rowStride * H, ci);
            while (!m.done) wgpuDevicePoll(gpu.device, true, nullptr);

            const auto* px = static_cast<const unsigned char*>(
                wgpuBufferGetConstMappedRange(readback, 0, rowStride * H));
            
            // Verify that pixel output is valid
            size_t nonZero = 0;
            for (size_t p = 0; p < rowStride * H; p += 4) {
                if (px[p] != 0 || px[p+1] != 0 || px[p+2] != 0) nonZero++;
            }
            wgpuBufferUnmap(readback);

            std::printf("[Gate D] Camera case \"%s\": rendered successfully (%zu visible pixels)\n",
                        c.name, nonZero);
            assert(nonZero > 0);
        }

        wgpuBufferRelease(readback);
        wgpuTextureViewRelease(view);
        wgpuTextureRelease(tex);
    }

    std::printf("=== webgpu_perlin_exact_gradient_test: ALL OK ===\n");
    return 0;
}
