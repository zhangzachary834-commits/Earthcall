import sys
import re

file_path = "src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp"
with open(file_path, "r") as f:
    content = f.read()

old_draw = """void WebGpuRenderer::drawParticles(const geom::FieldNode& field, int count) {
    if (!_pass || count <= 0 || !field.vectorField) return;

    const glm::vec3 flow(field.vectorField->baseFlowX,
                         field.vectorField->baseFlowY,
                         field.vectorField->baseFlowZ);
    const float speed = glm::length(flow);
    const glm::vec3 flowDir = speed > 1e-6f ? flow / speed : glm::vec3(0.0f, 1.0f, 0.0f);
    const float travel = field.vectorField->amplitude * glm::length(field.scale);

    // xorshift32, seeded per-particle by index rather than carried across calls:
    // the same (field, count) always produces the same cloud, so this needs no
    // buffer to persist between frames and no dt the caller would have to supply.
    std::vector<glm::vec3> verts;
    verts.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        uint32_t h = static_cast<uint32_t>(i) * 2654435761u + 1u;
        auto rnd = [&h]() {
            h ^= h << 13; h ^= h >> 17; h ^= h << 5;
            return static_cast<float>(h & 0xFFFFFFu) / static_cast<float>(0xFFFFFFu);
        };
        const glm::vec3 local(rnd() * 2.0f - 1.0f, rnd() * 2.0f - 1.0f, rnd() * 2.0f - 1.0f);
        const float phase = rnd();
        verts.push_back(field.origin + local * field.scale + flowDir * (phase * travel));
    }

    drawFlat(flatPipeline(WGPUPrimitiveTopology_PointList, Blend::Additive, DepthMode::TestOnly),
             verts, _viewProj * _model, glm::vec4(0.6f, 0.8f, 1.0f, 0.9f));
}"""

new_draw = """void WebGpuRenderer::drawParticles(const geom::FieldNode& field, int count) {
    if (!_pass || count <= 0 || !field.vectorField) return;

    const glm::vec3 flow(field.vectorField->baseFlowX,
                         field.vectorField->baseFlowY,
                         field.vectorField->baseFlowZ);
    const float speed = glm::length(flow);
    const glm::vec3 flowDir = speed > 1e-6f ? flow / speed : glm::vec3(0.0f, 1.0f, 0.0f);
    const float travel = field.vectorField->amplitude * glm::length(field.scale);

    bindPipeline(_particlePipe);

    ParticleUniforms pu;
    pu.mvp = _viewProj * _model;
    pu.color = glm::vec4(0.6f, 0.8f, 1.0f, 0.9f);
    pu.originAndTravel = glm::vec4(field.origin, travel);
    pu.flowDir = glm::vec4(flowDir, 0.0f);
    pu.scale = glm::vec4(field.scale, 0.0f);

    auto uAlloc = _bufferPool.suballocateUniform(&pu, sizeof(ParticleUniforms));

    WGPUBindGroupEntry bge = {};
    bge.binding = 0;
    bge.buffer = uAlloc.buffer;
    bge.offset = uAlloc.offset;
    bge.size = uAlloc.size;
    WGPUBindGroupDescriptor bgd = {};
    bgd.layout = _particleBgl;
    bgd.entryCount = 1;
    bgd.entries = &bge;
    WGPUBindGroup bg = wgpuDeviceCreateBindGroup(_device, &bgd);
    _frameBindGroups.push_back(bg);

    wgpuRenderPassEncoderSetBindGroup(_pass, 0, bg, 0, nullptr);
    wgpuRenderPassEncoderDraw(_pass, static_cast<uint32_t>(count), 1, 0, 0);
}"""

if old_draw in content:
    content = content.replace(old_draw, new_draw)
    print("drawParticles rewritten")
else:
    print("Could not find drawParticles")

with open(file_path, "w") as f:
    f.write(content)
