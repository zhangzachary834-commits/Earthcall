#pragma once

// Native numerical witness through complete production-emitted modules, using
// their real parameter buffers and evaluator entrypoints. No source extraction,
// alternate arithmetic mode, save mutation, or ordinary-frame instrumentation.
// Codex / GPT-6.1 Sol / session 01a122d7 / 2026-10-15.
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include <array>
#include <cstring>
#include <functional>

namespace PowerWitness {
using Input = std::array<float, 4>;

inline std::vector<Input> evaluate(wgpu::Device& gpu, const sdfwgsl::Program& program,
                                  const std::string& expression, const std::vector<Input>& inputs) {
    if (!program.ok) return {};
    const std::string code = program.wgsl + R"WGSL(
@group(2) @binding(0) var<storage, read> powerInputs: array<vec4<f32>>;
@group(2) @binding(1) var<storage, read_write> powerOutputs: array<vec4<f32>>;
@compute @workgroup_size(1)
fn powerWitness(@builtin(global_invocation_id) id: vec3<u32>) {
    g_instIdx = 0u;
    let p = powerInputs[id.x].xyz;
    powerOutputs[id.x] = )WGSL" + expression + ";\n}\n";
    WGPUShaderSourceWGSL wgsl = {};
    wgsl.chain.sType = WGPUSType_ShaderSourceWGSL;
    wgsl.code = wgpu::Device::str(code.c_str());
    WGPUShaderModuleDescriptor smd = {}; smd.nextInChain = &wgsl.chain;
    auto module = wgpuDeviceCreateShaderModule(gpu.device, &smd);
    std::array<WGPUBindGroupLayout, 3> layouts{};
    for (unsigned group = 0; group < 3; ++group) {
        std::array<WGPUBindGroupLayoutEntry, 2> entries{};
        const unsigned count = group == 2 ? 2 : 1;
        for (unsigned i = 0; i < count; ++i) {
            entries[i].binding = group == 0 ? 1 : i;
            entries[i].visibility = WGPUShaderStage_Compute;
            entries[i].buffer.type = group == 2 && i == 1
                ? WGPUBufferBindingType_Storage : WGPUBufferBindingType_ReadOnlyStorage;
        }
        WGPUBindGroupLayoutDescriptor ld = {}; ld.entryCount = count; ld.entries = entries.data();
        layouts[group] = wgpuDeviceCreateBindGroupLayout(gpu.device, &ld);
    }
    WGPUPipelineLayoutDescriptor pld = {}; pld.bindGroupLayoutCount = 3; pld.bindGroupLayouts = layouts.data();
    auto layout = wgpuDeviceCreatePipelineLayout(gpu.device, &pld);
    WGPUComputePipelineDescriptor pd = {}; pd.layout = layout;
    pd.compute.module = module; pd.compute.entryPoint = wgpu::Device::str("powerWitness");
    auto pipeline = wgpuDeviceCreateComputePipeline(gpu.device, &pd);
    auto buffer = [&](size_t size, WGPUBufferUsage usage, const void* data = nullptr) {
        WGPUBufferDescriptor d = {}; d.size = size; d.usage = usage;
        auto result = wgpuDeviceCreateBuffer(gpu.device, &d);
        if (data) wgpuQueueWriteBuffer(gpu.queue, result, 0, data, size);
        return result;
    };
    auto params = program.params;
    if (params.empty()) params.push_back(0);
    std::array<uint8_t, 1024> instance{}; // all offsets/index zero, both existing layouts fit
    auto pb = buffer(params.size() * 4, WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, params.data());
    auto ib = buffer(instance.size(), WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, instance.data());
    auto input = buffer(inputs.size() * sizeof(Input), WGPUBufferUsage_Storage | WGPUBufferUsage_CopyDst, inputs.data());
    auto output = buffer(inputs.size() * sizeof(Input), WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc);
    auto readback = buffer(inputs.size() * sizeof(Input), WGPUBufferUsage_MapRead | WGPUBufferUsage_CopyDst);
    std::array<WGPUBindGroup, 3> groups{};
    for (unsigned group = 0; group < 3; ++group) {
        std::array<WGPUBindGroupEntry, 2> entries{};
        const unsigned count = group == 2 ? 2 : 1;
        entries[0].binding = group == 0 ? 1 : 0;
        entries[0].buffer = group == 0 ? pb : group == 1 ? ib : input;
        entries[0].size = group == 0 ? params.size() * 4 : group == 1 ? instance.size() : inputs.size() * sizeof(Input);
        entries[1].binding = 1; entries[1].buffer = output; entries[1].size = inputs.size() * sizeof(Input);
        WGPUBindGroupDescriptor gd = {}; gd.layout = layouts[group]; gd.entryCount = count; gd.entries = entries.data();
        groups[group] = wgpuDeviceCreateBindGroup(gpu.device, &gd);
    }
    auto encoder = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
    auto pass = wgpuCommandEncoderBeginComputePass(encoder, nullptr);
    wgpuComputePassEncoderSetPipeline(pass, pipeline);
    for (unsigned i = 0; i < 3; ++i) wgpuComputePassEncoderSetBindGroup(pass, i, groups[i], 0, nullptr);
    wgpuComputePassEncoderDispatchWorkgroups(pass, inputs.size(), 1, 1);
    wgpuComputePassEncoderEnd(pass); wgpuComputePassEncoderRelease(pass);
    wgpuCommandEncoderCopyBufferToBuffer(encoder, output, 0, readback, 0, inputs.size() * sizeof(Input));
    auto command = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpuQueueSubmit(gpu.queue, 1, &command);
    struct Mapped { bool done = false, ok = false; } mapped;
    WGPUBufferMapCallbackInfo cb = {}; cb.mode = WGPUCallbackMode_AllowProcessEvents; cb.userdata1 = &mapped;
    cb.callback = [](WGPUMapAsyncStatus s, WGPUStringView, void* p, void*) {
        auto* r = static_cast<Mapped*>(p); r->ok = s == WGPUMapAsyncStatus_Success; r->done = true;
    };
    wgpuBufferMapAsync(readback, WGPUMapMode_Read, 0, inputs.size() * sizeof(Input), cb);
    while (!mapped.done) wgpuInstanceProcessEvents(gpu.instance);
    std::vector<Input> result;
    if (mapped.ok) {
        result.resize(inputs.size());
        std::memcpy(result.data(), wgpuBufferGetConstMappedRange(readback, 0, result.size() * sizeof(Input)), result.size() * sizeof(Input));
        wgpuBufferUnmap(readback);
    }
    wgpuCommandBufferRelease(command); wgpuCommandEncoderRelease(encoder);
    for (auto g : groups) wgpuBindGroupRelease(g);
    for (auto l : layouts) wgpuBindGroupLayoutRelease(l);
    for (auto b : {pb, ib, input, output, readback}) wgpuBufferRelease(b);
    wgpuComputePipelineRelease(pipeline); wgpuPipelineLayoutRelease(layout); wgpuShaderModuleRelease(module);
    return result;
}

inline void run(wgpu::Device& gpu, const std::function<void(bool, const std::string&)>& check) {
    const float inf = std::numeric_limits<float>::infinity(), nan = std::numeric_limits<float>::quiet_NaN();
    std::vector<Input> cases = {{1e-13f,-1,0,0},{1e-13f,.25f,0,0},{-2,2.00005f,0,0},
        {-2,2,0,0},{-2,3,0,0},{-2,-3,0,0},{0,0,0,0},{0,-1,0,0},{-0.0f,-3,0,0},
        {-0.0f,3,0,0},{0,1e-13f,0,0},{-1,8388607,0,0},{-1,8388608,0,0},
        {-1,8388609,0,0},{-1,16777215,0,0},{-1,16777216,0,0},{-1,16777218,0,0},
        {inf,2,0,0},{-inf,3,0,0},{-inf,.5f,0,0},{inf,-1,0,0},{2,inf,0,0},
        {.5f,inf,0,0},{-1,inf,0,0},{nan,0,0,0},{1,nan,0,0},{-2,nan,0,0},{nan,2,0,0},
        {1e-30f,2,0,0},{std::numeric_limits<float>::max(),2,0,0}};
    auto variable = [](const char* name) { auto n = std::make_unique<OntoMath::MathNode>();
        n->op = OntoMath::MathNode::Op::ValueLeaf; n->variableName = name; return n; };
    auto root = std::make_shared<OntoMath::MathNode>(); root->op = OntoMath::MathNode::Op::Pow;
    root->children.push_back(variable("x")); root->children.push_back(variable("y"));
    OntoMath::Piecewise pw; pw.inputVariable = "x";
    OntoMath::Piecewise::Piece piece; piece.mathNode = root; pw.pieces.push_back(std::move(piece));
    auto equal = [](float got, float expected) {
        if (std::isnan(expected)) return std::isnan(got);
        if (std::isinf(expected)) return got == expected;
        if (expected == 0) return got == 0 && std::signbit(got) == std::signbit(expected);
        return std::isfinite(got) && std::abs(got - expected) <= std::abs(expected) * 3e-5f;
    };
    auto verify = [&](const std::string& name, const sdfwgsl::Program& prog, const std::string& expr,
                      const std::function<float(const Input&)>& oracle, const std::vector<Input>& input, int component = 0) {
        auto result = evaluate(gpu, prog, expr, input);
        bool valid = result.size() == input.size();
        std::vector<size_t> subnormal;
        for (size_t i = 0; i < result.size(); ++i) {
            float expected = oracle(input[i]);
            if (std::fpclassify(expected) == FP_SUBNORMAL) {
                // Existing native Fast arithmetic may flush denormals. Do not
                // mislabel this profile boundary as CPU numerical parity or
                // introduce a new finite clamp to hide it.
                subnormal.push_back(i);
                std::printf("  POWER profile boundary %s case=%zu native=%g CPU-subnormal=%g\n",name.c_str(),i,result[i][component],expected);
                continue;
            }
            if (!equal(result[i][component], expected)) {
                std::printf("  POWER mismatch %s case=%zu base=%g exp=%g got=%g expected=%g\n",name.c_str(),i,input[i][0],input[i][1],result[i][component],expected);
                valid = false;
            }
        }
        check(valid, name + ": native normal/domain cases match the uploaded CPU route");
        if (!subnormal.empty()) {
            auto intrinsic = evaluate(gpu,prog,"vec4<f32>(pow(abs(p.x),p.y),0.0,0.0,0.0)",input);
            bool same = intrinsic.size() == result.size();
            for (size_t i : subnormal) {
                if (!same) break;
                same &= std::abs(result[i][component]) == intrinsic[i][0] &&
                        std::signbit(result[i][component]) == std::signbit(oracle(input[i]));
            }
            check(same, name + ": subnormal boundary matches the native intrinsic (not CPU parity)");
        }
    };
    verify("MathNode scalar Pow", sdfwgsl::compileVolume(&pw), "vec4<f32>(volumeDensityEval(p),0.0,0.0,0.0)",
        [&](const Input& v) { auto value = root->evaluate({{"x",double(v[0])},{"y",double(v[1])}});
            return value ? float(std::get<double>(*value)) : nan; }, cases);
    auto rpn = geom::makeImplicit("x^y");
    // The contemporary string authoring route prefers MathNode. Explicitly
    // construct the retained legacy representation to witness its own CPU and
    // WGSL consumers rather than testing MathNode a second time under its name.
    rpn.mathNode.reset(); rpn.piecewise.reset(); rpn.rpn = geom::compileExpr("x^y");
    check(!rpn.rpn.empty(), "legacy power witness actually supplies RPN tokens");
    verify("legacy RPN Pow", sdfwgsl::compile(rpn), "vec4<f32>(sdfEval(p),0.0,0.0,0.0)",
        [&](const Input& v) { return geom::evalSdf(rpn, glm::vec3(v[0],v[1],0)); }, cases);
    for (double exp : {-1., .25, 2., 2.00005}) {
        auto leaf = std::make_shared<OntoMath::MathNode>(); leaf->op = OntoMath::MathNode::Op::ScalarLeaf;
        OntoMath::Term term(1); term.factors["x"] = exp; leaf->scalarForm.terms.push_back(term);
        auto form = geom::makeImplicit(leaf);
        // Explicitly compare the uploaded exponent; CPU double semantics on the
        // unconverted authored exponent are a distinct precision question.
        auto input = cases;
        for (auto& row : input) row[1] = float(exp);
        verify("ScalarForm Pow exponent=" + std::to_string(exp), sdfwgsl::compile(form),
            "vec4<f32>(sdfEval(p),0.0,0.0,0.0)",
            [&](const Input& v) { return float(std::pow(double(v[0]),double(float(exp)))); },input);
    }
    root->children[0] = variable("p");
    for (int c = 0; c < 3; ++c)
        verify("MathNode vector Pow component " + std::to_string(c), sdfwgsl::compileVolume(nullptr,nullptr,nullptr,&pw),
            "vec4<f32>(volumeChromaEval(p),0.0)",
            [&](const Input& v) { auto value = root->evaluate({{"p",glm::vec3(v[0],v[1],0)},{"y",double(v[1])}});
                return value ? std::get<glm::vec3>(*value)[c] : nan; },cases,c);
}
} // namespace PowerWitness
