#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <memory>

// Forward declarations
class Object;
struct GLFWwindow;

namespace AdvancedFacePaint {

    // Gradient types for advanced filling
    enum class GradientType {
        Linear = 0,
        Radial,
        Angular,
        Diamond,
        Noise,
        Custom
    };

    // Smudge types for advanced painting
    enum class SmudgeType {
        Normal = 0,
        Directional,
        Radial,
        Spiral,
        Noise,
        Custom
    };

    // Gradient settings structure
    struct GradientSettings {
        GradientType type = GradientType::Linear;
        glm::vec4 startColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
        glm::vec4 endColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
        glm::vec2 startPoint = glm::vec2(0.0f, 0.0f);
        glm::vec2 endPoint = glm::vec2(1.0f, 1.0f);
        float angle = 0.0f;
        float noiseScale = 1.0f;
        int noiseOctaves = 4;
        float noisePersistence = 0.5f;
        float noiseLacunarity = 2.0f;
        bool useAlpha = true;
        float alphaBlend = 1.0f;
    };

    // Smudge settings structure
    struct SmudgeSettings {
        SmudgeType type = SmudgeType::Normal;
        float strength = 0.5f;
        float radius = 0.1f;
        float softness = 0.5f;
        float pressure = 1.0f;
        glm::vec2 direction = glm::vec2(1.0f, 0.0f);
        float speed = 1.0f;
        float turbulence = 0.1f;
        bool usePressure = true;
        bool useDirectional = false;
        float directionalStrength = 0.5f;
        float spiralTurns = 2.0f;
        float noiseIntensity = 0.3f;
        float noiseScale = 1.0f;
    };

    // Advanced paint result structure
    struct PaintResult {
        bool success = false;
        glm::vec4 color = glm::vec4(0.0f);
        float alpha = 0.0f;
        glm::vec2 uv = glm::vec2(0.0f);
        float depth = 0.0f;
        std::string message = "";
    };

    // Main class for advanced face painting
    class AdvancedFacePainter {
    public:
        AdvancedFacePainter();
        ~AdvancedFacePainter();

        // Initialize OpenGL resources
        bool initialize();
        void cleanup();

        // Main painting functions
        PaintResult paintFaceWithGradient(Object* obj, int faceIndex, const GradientSettings& settings);
        PaintResult paintFaceWithSmudge(Object* obj, int faceIndex, const glm::vec2& uv, const SmudgeSettings& settings);
        
        // Utility functions
        glm::vec4 calculateGradientColor(const glm::vec2& uv, const GradientSettings& settings);
        glm::vec4 calculateSmudgeColor(const glm::vec2& uv, const SmudgeSettings& settings, const glm::vec4& baseColor);
        
        // OpenGL rendering functions
        void renderGradientPreview(const GradientSettings& settings);
        void renderSmudgePreview(const SmudgeSettings& settings);
        
        // Settings management
        void setGradientSettings(const GradientSettings& settings) { _gradientSettings = settings; }
        void setSmudgeSettings(const SmudgeSettings& settings) { _smudgeSettings = settings; }
        
        const GradientSettings& getGradientSettings() const { return _gradientSettings; }
        const SmudgeSettings& getSmudgeSettings() const { return _smudgeSettings; }

    private:
        // OpenGL resources
        unsigned int _shaderProgram = 0;
        unsigned int _vertexArrayObject = 0;
        unsigned int _vertexBufferObject = 0;
        unsigned int _textureBuffer = 0;
        
        // Shader locations
        int _uniformProjection = -1;
        int _uniformModelView = -1;
        int _uniformGradientType = -1;
        int _uniformGradientColors = -1;
        int _uniformGradientParams = -1;
        int _uniformSmudgeType = -1;
        int _uniformSmudgeParams = -1;
        
        // Current settings
        GradientSettings _gradientSettings;
        SmudgeSettings _smudgeSettings;
        
        // Internal helper functions
        bool compileShaders();
        bool createBuffers();
        void setupShaders();
        glm::vec4 sampleTexture(Object* obj, int faceIndex, const glm::vec2& uv);
        void updateTexture(Object* obj, int faceIndex, const glm::vec2& uv, const glm::vec4& color);
    };

    // Global instance for easy access
    extern std::unique_ptr<AdvancedFacePainter> g_advancedPainter;

    // Utility functions
    void initializeAdvancedPainter();
    void cleanupAdvancedPainter();
    
    // High-level painting functions
    bool paintFaceAdvanced(Object* obj, int faceIndex, const glm::vec2& uv, 
                          const GradientSettings* gradient = nullptr, 
                          const SmudgeSettings* smudge = nullptr);
    
    // UI helper functions
    void renderGradientUI(GradientSettings& settings);
    void renderSmudgeUI(SmudgeSettings& settings);
    void renderAdvancedPaintUI(Object* selectedObj, int selectedFace);

} // namespace AdvancedFacePaint
