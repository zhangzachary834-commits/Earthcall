#include "Singularity/Screen/BrushSystem.hpp"
#include "Singularity/Screen/Renderer.hpp"
#include <cassert>
#include <iostream>
#include <vector>
#include <chrono>

class DummyRenderer : public Renderer {
public:
    Backend backend() const override { return Backend::OpenGL; }
    void beginFrame(uint32_t, uint32_t, const glm::vec4&) override {}
    void endFrame() override {}
    void setCamera(const glm::mat4&, const glm::mat4&, const glm::vec3&) override {}
    void setModel(const glm::mat4&) override {}
    void pushModel(const glm::mat4&) override {}
    void popModel() override {}
    const glm::mat4& model() const override { static glm::mat4 m(1.0f); return m; }
    const glm::mat4& view() const override { static glm::mat4 m(1.0f); return m; }
    const glm::mat4& projection() const override { static glm::mat4 m(1.0f); return m; }
    const glm::vec3& cameraPosition() const override { static glm::vec3 p(0.0f); return p; }
    bool zeroToOneDepth() const override { return false; }
    glm::ivec4 viewport() const override { return glm::ivec4(0, 0, 512, 512); }
    void setWireframe(bool) override {}
    bool wireframe() const override { return false; }
    void setLightingEnabled(bool) override {}
    bool lightingEnabled() const override { return false; }
    void setLight(const glm::vec3&, const glm::vec3&, const glm::vec3&, const glm::vec3&) override {}
    void setHeightGridDdaEnabled(bool) override {}
    bool heightGridDdaEnabled() const override { return false; }
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
    void drawTexQuad2D(TextureHandle, const glm::vec2&, const glm::vec2&, const glm::vec2&, const glm::vec2&, const glm::vec4&) override {}
    void drawParticles(const std::vector<glm::vec3>&, const glm::vec4&, float) override {}
    const FrameStats& frameStats() const override { static FrameStats s{}; return s; }
    void resetFrameStats() override {}
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

    std::cout << "BrushSystem unit test passed!" << std::endl;
    return 0;
}
