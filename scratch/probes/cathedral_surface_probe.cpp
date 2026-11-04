// Codex / GPT-6 / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-11-03.
// Read-only census of the saved Cathedral's actual surface compiler inputs.
// Default execution is CPU-only. --render explicitly opens a WebGPU device.
#include "ConstructedBeing/Singular/Object/Geometry/SdfJson.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "Singularity/Screen/AuthorableLight.hpp"
#include "Singularity/Screen/RenderMaterial.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <iostream>
#include <map>
#include <algorithm>
#include <numeric>
#include <stdexcept>

using Json = nlohmann::json;
using Clock = std::chrono::steady_clock;
extern MaterialManager materials;

static Json readZone(const char* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open input Zone");
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
    if (bytes.empty()) throw std::runtime_error("empty input Zone");
    if (bytes.front() == '{') return Json::parse(bytes);
    return Json::parse(Json::from_msgpack(bytes).at("MigrationRoot").get<std::string>());
}

static std::shared_ptr<OntoMath::Piecewise> expression(const Json& material, const char* key) {
    if (!material.contains(key)) return {};
    return std::make_shared<OntoMath::Piecewise>(OntoMath::Piecewise::fromJson(material.at(key)));
}

static RenderMaterial materialFor(const Json& object, const Json& materials) {
    // Match identifier resolution, including explicit IDs, without loading paint
    // textures or registering/modifying Material beings in the live world.
    const auto id = object.value("materialId", "material.default");
    for (const auto& material : materials) {
        if (material.value("id", "material." + material.value("name", "")) != id) continue;
        RenderMaterial result;
        const auto color = material.value("baseColor", std::vector<float>{1, 1, 1});
        if (color.size() >= 3) result.baseColor = glm::vec3(color[0], color[1], color[2]);
        result.opacity = material.value("opacity", 1.0f);
        result.shininess = material.value("shininess", 32.0f);
        result.specular = material.value("specular", 1.0f);
        result.ambient = material.value("ambient", .2f);
        result.diffuse = material.value("diffuse", .8f);
        result.colorExpr = expression(material, "colorExpr");
        result.responseExpr = expression(material, "responseExpr");
        return result;
    }
    throw std::runtime_error("unresolved material " + id);
}

static glm::vec3 vec3(const Json& array) {
    return glm::vec3(array.at(0).get<float>(), array.at(1).get<float>(), array.at(2).get<float>());
}

static bool fence(wgpu::Device& gpu) {
    struct Result { bool done = false, ok = false; } result;
    WGPUQueueWorkDoneCallbackInfo callback{};
    callback.mode = WGPUCallbackMode_AllowProcessEvents;
    callback.userdata1 = &result;
    callback.callback = [](WGPUQueueWorkDoneStatus status, WGPUStringView, void* data, void*) {
        auto& result = *static_cast<Result*>(data);
        result.ok = status == WGPUQueueWorkDoneStatus_Success;
        result.done = true;
    };
    wgpuQueueOnSubmittedWorkDone(gpu.queue, callback);
    while (!result.done) wgpuInstanceProcessEvents(gpu.instance);
    return result.ok;
}

int main(int argc, char** argv) {
    try {
        if (argc < 3) throw std::runtime_error("usage: probe zone.ecform report.json [--render image.rgba]");
        const std::string mode = argc == 5 ? argv[3] : "cpu";
        const bool profile = mode == "--profile-world";
        const bool world = mode == "--render-world" || mode == "--render-world-unit-light" || profile;
        const bool unitLight = mode == "--render-unit-light" || mode == "--render-world-unit-light";
        const bool render = argc == 5 && (mode == "--render" || world || unitLight);
        if (argc != 3 && !render) throw std::runtime_error("unrecognized arguments");
        const auto zone = readZone(argv[1]);
        auto source = geom::FieldNode::fromJson(zone.at("spatialRoot"));
        Rendering::AuthorableLightState light;
        if (!source || !Rendering::readAuthorableLight(*source, light))
            throw std::runtime_error("expected authored spatial-root light source");
        const auto* radiance = &source->field->astDefinition;
        Json report{{"scope", "saved surface subset and production compiler, fixed clock, no Law/Engine execution"},
                    {"zone", zone.at("name")}, {"gpu_executed", false}, {"fields", Json::array()},
                    {"material_type_diagnostics", Json::array()}};
        std::map<std::string, size_t> structures;
        size_t gradientFields = 0, analyticFields = 0, parameterMismatches = 0;
        const auto started = Clock::now();
        for (const auto& object : zone.at("world").at("objects")) {
            if (object.value("shapeKind", -1) != 10) continue;
            auto field = geom::sdfFromJson(object.at("field"));
            auto material = materialFor(object, zone.at("materials"));
            if (material.colorExpr) {
                for (const auto& piece : material.colorExpr->pieces) {
                    std::string error;
                    if (piece.mathNode && !piece.mathNode->checkTypes({}, error, nullptr, true))
                        report["material_type_diagnostics"].push_back({{"id", object.at("objectID")},
                            {"material", object.at("materialId")}, {"error", error}});
                }
            }
            auto program = sdfwgsl::compile(field, nullptr, material.colorExpr.get(), radiance,
                nullptr, nullptr, nullptr, nullptr, sdfwgsl::DensityInputKind::LegacyField,
                nullptr, nullptr, nullptr, nullptr, nullptr, material.responseExpr.get());
            if (!program.ok) throw std::runtime_error(object.at("objectID").get<std::string>() + ": " + program.error);
            const auto collected = sdfwgsl::collectParams(field, nullptr, material.colorExpr.get(), radiance,
                nullptr, nullptr, nullptr, nullptr, sdfwgsl::DensityInputKind::LegacyField,
                nullptr, nullptr, nullptr, nullptr, nullptr, material.responseExpr.get());
            const bool paramsMatch = collected.ok && collected.values == program.params;
            parameterMismatches += !paramsMatch;
            const bool analytic = program.wgsl.find("fn sdfEvalGrad(") != std::string::npos;
            gradientFields += program.needsGradientStep;
            analyticFields += analytic;
            ++structures[program.wgsl];
            report["fields"].push_back({{"id", object.at("objectID")},
                {"gradient_step", program.needsGradientStep}, {"analytic_gradient", analytic},
                {"shader_bytes", program.wgsl.size()}, {"parameter_count", program.params.size()},
                {"collected_parameters_match", paramsMatch}});
        }
        report["cpu_compile_collect_ms"] = std::chrono::duration<double, std::milli>(Clock::now() - started).count();
        report["field_count"] = report["fields"].size();
        report["unique_shader_structures"] = structures.size();
        report["gradient_step_fields"] = gradientFields;
        report["analytic_gradient_fields"] = analyticFields;
        report["parameter_mismatches"] = parameterMismatches;
        report["radiance_is_spatial_root"] = true;

        if (render) {
            // Future isolated surface screen. This omits mesh Objects, paint,
            // Person, tools and Laws. Its timing cannot establish live Zone FPS.
            constexpr uint32_t width = 2560, height = 1440, stride = width * 4;
            wgpu::Device gpu;
            if (!gpu.init()) throw std::runtime_error("WebGPU device refused");
            WebGpuRenderer renderer;
            if (!renderer.init(gpu)) throw std::runtime_error("renderer initialization refused");
            renderer.setLight(light.position, Rendering::lightAmbientRadiance(light),
                Rendering::lightDiffuseRadiance(light), Rendering::lightSpecularRadiance(light));
            renderer.setLightingEnabled(light.enabled);
            renderer.setRadianceField(unitLight ? nullptr : radiance, 1);
            renderer.setRadianceTemporalCoordinate(7.25, 1.0 / 60.0);
            renderer.setRadianceSourceCoefficients(light.intensity, light.ambient, light.diffuse, light.specular);
            const glm::vec3 eye(0, 1.6f, 28), aim(0, 5, -20);
            renderer.setCamera(glm::lookAt(eye, aim, glm::vec3(0, 1, 0)),
                glm::perspectiveZO(glm::radians(45.0f), float(width) / height, .1f, 100.0f), eye);
            WGPUTextureDescriptor textureDescriptor{};
            textureDescriptor.size = {width, height, 1};
            textureDescriptor.dimension = WGPUTextureDimension_2D;
            textureDescriptor.format = WGPUTextureFormat_RGBA8Unorm;
            textureDescriptor.usage = WGPUTextureUsage_RenderAttachment | WGPUTextureUsage_CopySrc;
            textureDescriptor.mipLevelCount = textureDescriptor.sampleCount = 1;
            auto texture = wgpuDeviceCreateTexture(gpu.device, &textureDescriptor);
            auto view = wgpuTextureCreateView(texture, nullptr);
            std::vector<std::unique_ptr<Object>> objects;
            if (world) {
                // Decode only the supplied snapshot. Any incidental storage is
                // confined to the report directory, outside the live data home.
                const auto sandbox = std::filesystem::absolute(argv[2]).parent_path() / "save-root";
                std::filesystem::create_directories(sandbox);
                SaveSystem::setSaveRoot(sandbox.string());
                ::materials.mergeFromJson(zone.at("materials"));
                setCurrentRenderer(&renderer);
                for (const auto& saved : zone.at("world").at("objects")) {
                    auto object = std::make_unique<Object>(saved.at("objectID").get<std::string>());
                    from_json(saved, *object);
                    objects.push_back(std::move(object));
                }
            }
            std::vector<geom::SdfNode> fields;
            std::vector<RenderMaterial> materials;
            std::vector<glm::mat4> models;
            std::vector<glm::vec3> extents;
            for (const auto& object : zone.at("world").at("objects")) {
                if (object.value("shapeKind", -1) != 10) continue;
                fields.push_back(geom::sdfFromJson(object.at("field")));
                materials.push_back(materialFor(object, zone.at("materials")));
                extents.push_back(vec3(object.at("fieldExtent")));
                const auto model = object.at("transform").get<std::vector<float>>();
                if (model.size() != 16) throw std::runtime_error("unexpected transform layout");
                glm::mat4 matrix(1);
                for (int i = 0; i < 16; ++i) matrix[i / 4][i % 4] = model[i];
                models.push_back(matrix);
            }
            Json times = Json::array(), encodeTimes = Json::array(), waitTimes = Json::array(), flushTimes = Json::array();
            std::vector<double> objectCosts(objects.size(), 0.0);
            for (int frame = 0; frame < (profile ? 35 : 7); ++frame) {
                if (profile && frame == 3)
                    std::ofstream(std::string(argv[2]) + ".ready") << "warmed renderer\n";
                const auto start = Clock::now();
                renderer.beginFrameOffscreen(view, width, height, glm::vec4(.1f, .1f, .15f, 1));
                if (world) {
                    for (size_t index = 0; index < objects.size(); ++index) {
                        const auto& object = objects[index];
                        const auto objectStart = profile ? Clock::now() : Clock::time_point{};
                        renderer.setModel(object->getTransform());
                        object->drawObject();
                        if (profile && frame >= 3)
                            objectCosts[index] += std::chrono::duration<double, std::milli>(Clock::now() - objectStart).count();
                    }
                } else {
                    for (size_t index = 0; index < fields.size(); ++index) {
                        renderer.setModel(models[index]);
                        renderer.drawImplicit(fields[index], extents[index], materials[index],
                            nullptr, index + 1, 1, nullptr, 1);
                    }
                }
                const auto beforeFlush = Clock::now();
                renderer.endFrame();
                const auto submitted = Clock::now();
                if (!fence(gpu)) throw std::runtime_error("queue completion refused");
                if (renderer.frameStats().sdfProgramRefusals) throw std::runtime_error("surface compiler refused");
                if (frame >= 3) {
                    if (renderer.frameStats().sdfProgramCompiles) throw std::runtime_error("cold compile in timed frame");
                    times.push_back(std::chrono::duration<double, std::milli>(Clock::now() - start).count());
                    encodeTimes.push_back(std::chrono::duration<double, std::milli>(submitted - start).count());
                    waitTimes.push_back(std::chrono::duration<double, std::milli>(Clock::now() - submitted).count());
                    flushTimes.push_back(std::chrono::duration<double, std::milli>(submitted - beforeFlush).count());
                }
            }
            WGPUBufferDescriptor bufferDescriptor{};
            bufferDescriptor.size = uint64_t(stride) * height;
            bufferDescriptor.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
            auto buffer = wgpuDeviceCreateBuffer(gpu.device, &bufferDescriptor);
            auto encoder = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);
            WGPUTexelCopyTextureInfo from{}; from.texture = texture; from.aspect = WGPUTextureAspect_All;
            WGPUTexelCopyBufferInfo to{}; to.buffer = buffer; to.layout.bytesPerRow = stride; to.layout.rowsPerImage = height;
            WGPUExtent3D extent{width, height, 1};
            wgpuCommandEncoderCopyTextureToBuffer(encoder, &from, &to, &extent);
            auto command = wgpuCommandEncoderFinish(encoder, nullptr);
            wgpuQueueSubmit(gpu.queue, 1, &command);
            bool mapped = false, mapOk = false;
            std::pair<bool*, bool*> state{&mapped, &mapOk};
            WGPUBufferMapCallbackInfo callback{};
            callback.mode = WGPUCallbackMode_AllowProcessEvents; callback.userdata1 = &state;
            callback.callback = [](WGPUMapAsyncStatus status, WGPUStringView, void* data, void*) {
                auto& state = *static_cast<std::pair<bool*, bool*>*>(data);
                *state.second = status == WGPUMapAsyncStatus_Success; *state.first = true;
            };
            wgpuBufferMapAsync(buffer, WGPUMapMode_Read, 0, bufferDescriptor.size, callback);
            while (!mapped) wgpuInstanceProcessEvents(gpu.instance);
            if (!mapOk) throw std::runtime_error("image readback refused");
            std::ofstream image(argv[4], std::ios::binary);
            image.write(static_cast<const char*>(wgpuBufferGetConstMappedRange(buffer, 0, bufferDescriptor.size)), bufferDescriptor.size);
            if (!image) throw std::runtime_error("image output failed");
            wgpuBufferUnmap(buffer);
            wgpuCommandBufferRelease(command); wgpuCommandEncoderRelease(encoder); wgpuBufferRelease(buffer);
            wgpuTextureViewRelease(view); wgpuTextureRelease(texture);
            report["gpu_executed"] = true;
            report["surface_subset_complete_ms"] = times;
            report["encode_ms"] = encodeTimes;
            report["completion_wait_ms"] = waitTimes;
            report["flush_ms"] = flushTimes;
            report["render_scope"] = world ? "all saved Objects through drawObject, without Laws/Person/tools" : "Field subset";
            report["unit_radiance_ablation"] = unitLight;
            report["objects_loaded"] = objects.size();
            report["camera"] = {{"eye", {0, 1.6, 28}}, {"aim", {0, 5, -20}}, {"fov", 45}, {"far", 100}};
            report["resolution"] = {width, height};
            if (profile) {
                std::vector<size_t> indices(objects.size());
                std::iota(indices.begin(), indices.end(), 0);
                std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) { return objectCosts[a] > objectCosts[b]; });
                Json kinds = Json::object(), top = Json::array();
                const auto& saved = zone.at("world").at("objects");
                for (size_t i = 0; i < objects.size(); ++i) {
                    auto& kind = kinds[std::to_string(saved[i].at("shapeKind").get<int>())];
                    if (kind.is_null()) kind = 0.0;
                    kind = kind.get<double>() + objectCosts[i] / times.size();
                }
                for (size_t i = 0; i < std::min<size_t>(20, indices.size()); ++i) {
                    const auto index = indices[i];
                    top.push_back({{"id", saved[index].at("objectID")},
                        {"kind", saved[index].at("shapeKind")}, {"draw_cpu_ms", objectCosts[index] / times.size()}});
                }
                report["profile_cpu_ms_by_kind"] = kinds;
                report["profile_top_objects"] = top;
            }
            if (world) setCurrentRenderer(nullptr);
        }
        std::ofstream output(argv[2]);
        output << report.dump(2) << '\n';
        if (!output) throw std::runtime_error("report output failed");
        std::cout << "fields=" << report["field_count"] << " gradient_step=" << gradientFields
                  << " analytic_gradient=" << analyticFields << " unique_shaders=" << structures.size()
                  << " parameter_mismatches=" << parameterMismatches << " GPU=" << render << '\n';
        return parameterMismatches ? 1 : 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
