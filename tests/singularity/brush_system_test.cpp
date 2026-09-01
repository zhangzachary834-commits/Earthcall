#include "Singularity/Screen/BrushSystem.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include "Singularity/Foreign/API/EarthcallAPI.hpp"
#include "Singularity/Foreign/API/SecurityManager.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <chrono>

class DummyRenderer : public Renderer {
public:
    void drawMesh(const geom::TessMesh&, const RenderMaterial&) override {}
    void drawImplicit(const geom::SdfNode&, const glm::vec3&, const RenderMaterial&, const geom::FieldNode*, uint64_t, uint32_t, const geom::HeightGrid*) override {}
    void drawLines(const std::vector<std::pair<glm::vec3, glm::vec3>>&, const glm::vec4&, float, Blend) override {}
    void drawOverlay(const geom::TessMesh&, const glm::vec4&, float, bool) override {}
    void drawSolid(const std::vector<glm::vec3>&, const glm::vec4&, Blend, bool) override {}
    void drawImage2D(const uint8_t*, uint32_t, uint32_t, const glm::vec4&, const glm::vec4&) override {}
    TextureHandle uploadTexture(TextureHandle, const uint8_t*, uint32_t, uint32_t) override { return 1; }
    void releaseTexture(TextureHandle) override {}
    void begin2D(uint32_t, uint32_t) override {}
    void end2D() override {}
    void drawLines2D(const std::vector<glm::vec2>&, const glm::vec4&, float) override {}
    void drawTris2D(const std::vector<glm::vec2>&, const glm::vec4&) override {}
};

int main() {
    std::cout << "Running BrushSystem unit test..." << std::endl;

    DummyRenderer dummy;
    setCurrentRenderer(&dummy);

    constexpr int textureSize = 512;
    BrushSystem brushSystem(textureSize);

    // Prepare sample layers
    std::vector<BrushSystem::Layer> testLayers;
    for (int i = 0; i < 4; ++i) {
        BrushSystem::Layer layer;
        layer.pixels.resize(textureSize * textureSize * 4, static_cast<uint8_t>(i * 50));
        layer.opacity = 0.8f;
        layer.blendMode = BrushSystem::BlendMode::Normal;
        layer.visible = true;
        testLayers.push_back(layer);
    }

    // 1. Test replaceLayers with const lvalue ref
    brushSystem.replaceLayers(testLayers, 2, true);
    assert(brushSystem.getLayerCount() == 4);
    assert(brushSystem.getActiveLayer() == 2);
    assert(brushSystem.getUseLayers() == true);
    assert(brushSystem.getLayers()[1].opacity == 0.8f);

    // 2. Test replaceLayers with rvalue move ref
    std::vector<BrushSystem::Layer> moveLayers = testLayers;
    brushSystem.replaceLayers(std::move(moveLayers), 1, false);
    assert(brushSystem.getLayerCount() == 4);
    assert(brushSystem.getActiveLayer() == 1);
    assert(brushSystem.getUseLayers() == false);
    assert(moveLayers.empty() || moveLayers[0].pixels.empty()); // moved out

    // 3. Test edge case: empty layers vector
    std::vector<BrushSystem::Layer> emptyLayers;
    brushSystem.replaceLayers(emptyLayers, 0, true);
    assert(brushSystem.getLayerCount() == 1);
    assert(brushSystem.getActiveLayer() == 0);

    // Benchmark comparison (lvalue vs rvalue move)
    constexpr int numBenchLayers = 16;
    constexpr int benchIterations = 100;
    std::vector<BrushSystem::Layer> benchLayers(numBenchLayers);
    for (auto& l : benchLayers) {
        l.pixels.resize(textureSize * textureSize * 4, 128);
    }

    auto t0 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < benchIterations; ++i) {
        brushSystem.replaceLayers(benchLayers, 0, true);
    }
    auto t1 = std::chrono::high_resolution_clock::now();
    double ms_lvalue = std::chrono::duration<double, std::milli>(t1 - t0).count();

    double ms_rvalue = 0;
    for (int i = 0; i < benchIterations; ++i) {
        std::vector<BrushSystem::Layer> temp = benchLayers;
        auto start = std::chrono::high_resolution_clock::now();
        brushSystem.replaceLayers(std::move(temp), 0, true);
        auto end = std::chrono::high_resolution_clock::now();
        ms_rvalue += std::chrono::duration<double, std::milli>(end - start).count();
    }

    std::cout << "lvalue replaceLayers total (" << benchIterations << " iterations, 16 layers of 512x512 RGBA): "
              << ms_lvalue << " ms (" << (ms_lvalue / benchIterations) << " ms/op)" << std::endl;
    std::cout << "rvalue replaceLayers move execution total: "
              << ms_rvalue << " ms (" << (ms_rvalue / benchIterations) << " ms/op)" << std::endl;

    // 4. Test clearLayer and clearAllLayers
    brushSystem.paintDab(glm::vec2(0.5f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), 1.0f);
    int centerIdx = (256 * 512 + 256) * 4 + 3;
    assert(brushSystem.getLayers()[brushSystem.getActiveLayer()].pixels[centerIdx] > 0);
    brushSystem.clearLayer(brushSystem.getActiveLayer());
    assert(brushSystem.getLayers()[brushSystem.getActiveLayer()].pixels[centerIdx] == 0);

    // 5. Test EarthcallAPI::clearBrushLayer
    Integration::SecurityManager::instance().grantPermission(Integration::PermissionType::BRUSH_SYSTEM, "earthcall_api");
    Integration::EarthcallAPI api;
    api.setBrushSystem(&brushSystem);
    brushSystem.paintDab(glm::vec2(0.5f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), 1.0f);
    assert(brushSystem.getLayers()[brushSystem.getActiveLayer()].pixels[centerIdx] > 0);
    bool cleared = api.clearBrushLayer("active");
    assert(cleared);
    assert(brushSystem.getLayers()[brushSystem.getActiveLayer()].pixels[centerIdx] == 0);

    std::cout << "BrushSystem unit test passed!" << std::endl;
    return 0;
}
