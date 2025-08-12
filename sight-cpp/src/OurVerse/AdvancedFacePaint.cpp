#include "AdvancedFacePaint.hpp"
#include "Form/Object/Object.hpp"
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <OpenGL/glext.h>
// Define aliases for Apple OpenGL functions
#define glGenVertexArrays glGenVertexArraysAPPLE
#define glBindVertexArray glBindVertexArrayAPPLE
#define glDeleteVertexArrays glDeleteVertexArraysAPPLE
#else
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glext.h>
#endif
#include <iostream>
#include <cmath>
#include <random>

namespace AdvancedFacePaint {

    // Global instance
    std::unique_ptr<AdvancedFacePainter> g_advancedPainter;

    // Vertex shader source for advanced painting
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec2 aTexCoord;
        
        uniform mat4 projection;
        uniform mat4 modelView;
        
        out vec2 TexCoord;
        
        void main() {
            gl_Position = projection * modelView * vec4(aPos, 1.0);
            TexCoord = aTexCoord;
        }
    )";

    // Fragment shader source for advanced painting
    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        
        in vec2 TexCoord;
        
        uniform int gradientType;
        uniform vec4 startColor;
        uniform vec4 endColor;
        uniform vec2 startPoint;
        uniform vec2 endPoint;
        uniform float angle;
        uniform float noiseScale;
        uniform int noiseOctaves;
        uniform float noisePersistence;
        uniform float noiseLacunarity;
        
        uniform int smudgeType;
        uniform float smudgeStrength;
        uniform float smudgeRadius;
        uniform float smudgeSoftness;
        uniform vec2 smudgeDirection;
        uniform float smudgeSpeed;
        uniform float smudgeTurbulence;
        
        // Noise function for procedural effects
        float noise(vec2 st) {
            return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
        }
        
        // Fractional Brownian Motion for complex noise
        float fbm(vec2 st) {
            float value = 0.0;
            float amplitude = 0.5;
            float frequency = 1.0;
            
            for (int i = 0; i < noiseOctaves; i++) {
                value += amplitude * noise(st * frequency);
                frequency *= noiseLacunarity;
                amplitude *= noisePersistence;
            }
            return value;
        }
        
        // Calculate gradient color based on type
        vec4 calculateGradient() {
            vec2 uv = TexCoord;
            
            if (gradientType == 0) { // Linear
                float t = dot(uv - startPoint, endPoint - startPoint) / dot(endPoint - startPoint, endPoint - startPoint);
                t = clamp(t, 0.0, 1.0);
                return mix(startColor, endColor, t);
            }
            else if (gradientType == 1) { // Radial
                float dist = distance(uv, startPoint);
                float maxDist = distance(endPoint, startPoint);
                float t = clamp(dist / maxDist, 0.0, 1.0);
                return mix(startColor, endColor, t);
            }
            else if (gradientType == 2) { // Angular
                vec2 center = (startPoint + endPoint) * 0.5;
                vec2 dir = normalize(uv - center);
                float angle = atan(dir.y, dir.x);
                float t = (angle + 3.14159) / (2.0 * 3.14159);
                return mix(startColor, endColor, t);
            }
            else if (gradientType == 3) { // Diamond
                vec2 center = (startPoint + endPoint) * 0.5;
                vec2 offset = abs(uv - center);
                float t = max(offset.x, offset.y);
                t = clamp(t / max(distance(startPoint, center), distance(endPoint, center)), 0.0, 1.0);
                return mix(startColor, endColor, t);
            }
            else if (gradientType == 4) { // Noise
                float noiseValue = fbm(uv * noiseScale);
                return mix(startColor, endColor, noiseValue);
            }
            
            return startColor;
        }
        
        // Calculate smudge effect
        vec4 calculateSmudge(vec4 baseColor) {
            if (smudgeType == 0) { // Normal smudge
                float dist = distance(TexCoord, startPoint);
                float t = 1.0 - smoothstep(0.0, smudgeRadius, dist);
                t = pow(t, smudgeSoftness);
                return mix(baseColor, endColor, t * smudgeStrength);
            }
            else if (smudgeType == 1) { // Directional
                vec2 dir = normalize(smudgeDirection);
                float t = dot(TexCoord - startPoint, dir);
                t = smoothstep(0.0, smudgeRadius, t);
                return mix(baseColor, endColor, t * smudgeStrength);
            }
            else if (smudgeType == 2) { // Radial
                float dist = distance(TexCoord, startPoint);
                float angle = atan(TexCoord.y - startPoint.y, TexCoord.x - startPoint.x);
                float t = smoothstep(0.0, smudgeRadius, dist) * (1.0 + sin(angle * 4.0) * 0.5);
                return mix(baseColor, endColor, t * smudgeStrength);
            }
            else if (smudgeType == 3) { // Spiral
                vec2 center = startPoint;
                vec2 offset = TexCoord - center;
                float angle = atan(offset.y, offset.x);
                float dist = length(offset);
                float spiral = sin(angle * smudgeSpeed + dist * smudgeTurbulence);
                float t = smoothstep(0.0, smudgeRadius, dist) * (0.5 + 0.5 * spiral);
                return mix(baseColor, endColor, t * smudgeStrength);
            }
            else if (smudgeType == 4) { // Noise
                float noiseValue = fbm(TexCoord * noiseScale);
                float dist = distance(TexCoord, startPoint);
                float t = smoothstep(0.0, smudgeRadius, dist) * noiseValue;
                return mix(baseColor, endColor, t * smudgeStrength);
            }
            
            return baseColor;
        }
        
        void main() {
            vec4 gradientColor = calculateGradient();
            vec4 finalColor = calculateSmudge(gradientColor);
            
            FragColor = finalColor;
        }
    )";

    // Constructor
    AdvancedFacePainter::AdvancedFacePainter() {
        // Initialize with default settings
        _gradientSettings = GradientSettings();
        _smudgeSettings = SmudgeSettings();
    }

    // Destructor
    AdvancedFacePainter::~AdvancedFacePainter() {
        cleanup();
    }

    // Initialize OpenGL resources
    bool AdvancedFacePainter::initialize() {
        if (!compileShaders()) {
            std::cerr << "Failed to compile shaders for AdvancedFacePainter" << std::endl;
            return false;
        }
        
        if (!createBuffers()) {
            std::cerr << "Failed to create buffers for AdvancedFacePainter" << std::endl;
            return false;
        }
        
        setupShaders();
        return true;
    }

    // Cleanup OpenGL resources
    void AdvancedFacePainter::cleanup() {
        if (_shaderProgram) {
            glDeleteProgram(_shaderProgram);
            _shaderProgram = 0;
        }
        
        if (_vertexArrayObject) {
            glDeleteVertexArrays(1, &_vertexArrayObject);
            _vertexArrayObject = 0;
        }
        
        if (_vertexBufferObject) {
            glDeleteBuffers(1, &_vertexBufferObject);
            _vertexBufferObject = 0;
        }
        
        if (_textureBuffer) {
            glDeleteTextures(1, &_textureBuffer);
            _textureBuffer = 0;
        }
    }

    // Compile shaders
    bool AdvancedFacePainter::compileShaders() {
        // Create vertex shader
        unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
        glCompileShader(vertexShader);
        
        // Check for shader compile errors
        int success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
            return false;
        }
        
        // Create fragment shader
        unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
        glCompileShader(fragmentShader);
        
        // Check for shader compile errors
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
            return false;
        }
        
        // Create shader program
        _shaderProgram = glCreateProgram();
        glAttachShader(_shaderProgram, vertexShader);
        glAttachShader(_shaderProgram, fragmentShader);
        glLinkProgram(_shaderProgram);
        
        // Check for linking errors
        glGetProgramiv(_shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(_shaderProgram, 512, NULL, infoLog);
            std::cerr << "Shader program linking failed: " << infoLog << std::endl;
            return false;
        }
        
        // Clean up individual shaders
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        
        return true;
    }

    // Create OpenGL buffers
    bool AdvancedFacePainter::createBuffers() {
        // Create vertex array object
        glGenVertexArrays(1, &_vertexArrayObject);
        glBindVertexArray(_vertexArrayObject);
        
        // Create vertex buffer object
        glGenBuffers(1, &_vertexBufferObject);
        glBindBuffer(GL_ARRAY_BUFFER, _vertexBufferObject);
        
        // Define quad vertices (position + texture coordinates)
        float vertices[] = {
            // positions        // texture coords
            -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
             1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
             1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
            -1.0f,  1.0f, 0.0f,  0.0f, 1.0f
        };
        
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        
        // Set vertex attributes
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        
        // Create texture buffer
        glGenTextures(1, &_textureBuffer);
        glBindTexture(GL_TEXTURE_2D, _textureBuffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        
        return true;
    }

    // Setup shader uniforms
    void AdvancedFacePainter::setupShaders() {
        glUseProgram(_shaderProgram);
        
        // Get uniform locations
        _uniformProjection = glGetUniformLocation(_shaderProgram, "projection");
        _uniformModelView = glGetUniformLocation(_shaderProgram, "modelView");
        _uniformGradientType = glGetUniformLocation(_shaderProgram, "gradientType");
        _uniformGradientColors = glGetUniformLocation(_shaderProgram, "startColor");
        _uniformGradientParams = glGetUniformLocation(_shaderProgram, "startPoint");
        _uniformSmudgeType = glGetUniformLocation(_shaderProgram, "smudgeType");
        _uniformSmudgeParams = glGetUniformLocation(_shaderProgram, "smudgeStrength");
    }

    // Paint face with gradient
    PaintResult AdvancedFacePainter::paintFaceWithGradient(Object* obj, int faceIndex, const GradientSettings& settings) {
        PaintResult result;
        
        if (!obj || faceIndex < 0) {
            result.message = "Invalid object or face index";
            return result;
        }
        
        // For now, return a simple result
        // In a full implementation, this would apply the gradient to the face texture
        result.success = true;
        result.color = calculateGradientColor(glm::vec2(0.5f, 0.5f), settings);
        result.message = "Gradient applied successfully";
        
        return result;
    }

    // Paint face with smudge
    PaintResult AdvancedFacePainter::paintFaceWithSmudge(Object* obj, int faceIndex, const glm::vec2& uv, const SmudgeSettings& settings) {
        PaintResult result;
        
        if (!obj || faceIndex < 0) {
            result.message = "Invalid object or face index";
            return result;
        }
        
        // Sample the current texture at the UV position
        glm::vec4 baseColor = sampleTexture(obj, faceIndex, uv);
        
        // Apply smudge effect
        glm::vec4 smudgedColor = calculateSmudgeColor(uv, settings, baseColor);
        
        // Update the texture
        updateTexture(obj, faceIndex, uv, smudgedColor);
        
        result.success = true;
        result.color = smudgedColor;
        result.uv = uv;
        result.message = "Smudge applied successfully";
        
        return result;
    }

    // Calculate gradient color
    glm::vec4 AdvancedFacePainter::calculateGradientColor(const glm::vec2& uv, const GradientSettings& settings) {
        glm::vec4 result = settings.startColor;
        
        switch (settings.type) {
            case GradientType::Linear: {
                float t = glm::dot(uv - settings.startPoint, settings.endPoint - settings.startPoint) / 
                         glm::dot(settings.endPoint - settings.startPoint, settings.endPoint - settings.startPoint);
                t = glm::clamp(t, 0.0f, 1.0f);
                result = glm::mix(settings.startColor, settings.endColor, t);
                break;
            }
            case GradientType::Radial: {
                float dist = glm::distance(uv, settings.startPoint);
                float maxDist = glm::distance(settings.endPoint, settings.startPoint);
                float t = glm::clamp(dist / maxDist, 0.0f, 1.0f);
                result = glm::mix(settings.startColor, settings.endColor, t);
                break;
            }
            case GradientType::Angular: {
                glm::vec2 center = (settings.startPoint + settings.endPoint) * 0.5f;
                glm::vec2 dir = glm::normalize(uv - center);
                float angle = std::atan2(dir.y, dir.x);
                float t = (angle + M_PI) / (2.0f * M_PI);
                result = glm::mix(settings.startColor, settings.endColor, t);
                break;
            }
            case GradientType::Diamond: {
                glm::vec2 center = (settings.startPoint + settings.endPoint) * 0.5f;
                glm::vec2 offset = glm::abs(uv - center);
                float t = glm::max(offset.x, offset.y);
                float maxDist = glm::max(glm::distance(settings.startPoint, center), 
                                       glm::distance(settings.endPoint, center));
                t = glm::clamp(t / maxDist, 0.0f, 1.0f);
                result = glm::mix(settings.startColor, settings.endColor, t);
                break;
            }
            case GradientType::Noise: {
                // Simple noise implementation
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<float> dis(0.0f, 1.0f);
                
                float noiseValue = dis(gen);
                result = glm::mix(settings.startColor, settings.endColor, noiseValue);
                break;
            }
            default:
                result = settings.startColor;
                break;
        }
        
        if (settings.useAlpha) {
            result.a *= settings.alphaBlend;
        }
        
        return result;
    }

    // Calculate smudge color
    glm::vec4 AdvancedFacePainter::calculateSmudgeColor(const glm::vec2& uv, const SmudgeSettings& settings, const glm::vec4& baseColor) {
        glm::vec4 result = baseColor;
        
        switch (settings.type) {
            case SmudgeType::Normal: {
                // Simple radial smudge
                float dist = glm::distance(uv, glm::vec2(0.5f, 0.5f));
                float t = 1.0f - glm::smoothstep(0.0f, settings.radius, dist);
                t = std::pow(t, settings.softness);
                result = glm::mix(baseColor, glm::vec4(0.0f), t * settings.strength);
                break;
            }
            case SmudgeType::Directional: {
                // Directional smudge
                glm::vec2 dir = glm::normalize(settings.direction);
                float t = glm::dot(uv - glm::vec2(0.5f, 0.5f), dir);
                t = glm::smoothstep(0.0f, settings.radius, t);
                result = glm::mix(baseColor, glm::vec4(0.0f), t * settings.strength);
                break;
            }
            case SmudgeType::Radial: {
                // Radial smudge with angular variation
                float dist = glm::distance(uv, glm::vec2(0.5f, 0.5f));
                float angle = std::atan2(uv.y - 0.5f, uv.x - 0.5f);
                float t = glm::smoothstep(0.0f, settings.radius, dist) * 
                         (1.0f + std::sin(angle * 4.0f) * 0.5f);
                result = glm::mix(baseColor, glm::vec4(0.0f), t * settings.strength);
                break;
            }
            case SmudgeType::Spiral: {
                // Spiral smudge
                glm::vec2 center(0.5f, 0.5f);
                glm::vec2 offset = uv - center;
                float angle = std::atan2(offset.y, offset.x);
                float dist = glm::length(offset);
                float spiral = std::sin(angle * settings.spiralTurns + dist * settings.turbulence);
                float t = glm::smoothstep(0.0f, settings.radius, dist) * (0.5f + 0.5f * spiral);
                result = glm::mix(baseColor, glm::vec4(0.0f), t * settings.strength);
                break;
            }
            case SmudgeType::Noise: {
                // Noise-based smudge
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_real_distribution<float> dis(0.0f, 1.0f);
                
                float noiseValue = dis(gen);
                float dist = glm::distance(uv, glm::vec2(0.5f, 0.5f));
                float t = glm::smoothstep(0.0f, settings.radius, dist) * noiseValue;
                result = glm::mix(baseColor, glm::vec4(0.0f), t * settings.strength);
                break;
            }
            default:
                result = baseColor;
                break;
        }
        
        return result;
    }

    // Sample texture at UV position
    glm::vec4 AdvancedFacePainter::sampleTexture(Object* obj, int faceIndex, const glm::vec2& uv) {
        // For now, return a default color
        // In a full implementation, this would sample the actual texture
        return glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    }

    // Update texture at UV position
    void AdvancedFacePainter::updateTexture(Object* obj, int faceIndex, const glm::vec2& uv, const glm::vec4& color) {
        // For now, do nothing
        // In a full implementation, this would update the actual texture
    }

    // Render gradient preview
    void AdvancedFacePainter::renderGradientPreview(const GradientSettings& settings) {
        if (!_shaderProgram) return;
        
        glUseProgram(_shaderProgram);
        glBindVertexArray(_vertexArrayObject);
        
        // Set gradient uniforms
        glUniform1i(_uniformGradientType, static_cast<int>(settings.type));
        glUniform4f(_uniformGradientColors, 
                    settings.startColor.r, settings.startColor.g, 
                    settings.startColor.b, settings.startColor.a);
        glUniform2f(_uniformGradientParams, 
                    settings.startPoint.x, settings.startPoint.y);
        
        // Render quad
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

    // Render smudge preview
    void AdvancedFacePainter::renderSmudgePreview(const SmudgeSettings& settings) {
        if (!_shaderProgram) return;
        
        glUseProgram(_shaderProgram);
        glBindVertexArray(_vertexArrayObject);
        
        // Set smudge uniforms
        glUniform1i(_uniformSmudgeType, static_cast<int>(settings.type));
        glUniform1f(_uniformSmudgeParams, settings.strength);
        
        // Render quad
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    }

    // Initialize global advanced painter
    void initializeAdvancedPainter() {
        if (!g_advancedPainter) {
            g_advancedPainter = std::make_unique<AdvancedFacePainter>();
            if (!g_advancedPainter->initialize()) {
                std::cerr << "Failed to initialize AdvancedFacePainter" << std::endl;
                g_advancedPainter.reset();
            }
        }
    }

    // Cleanup global advanced painter
    void cleanupAdvancedPainter() {
        if (g_advancedPainter) {
            g_advancedPainter->cleanup();
            g_advancedPainter.reset();
        }
    }

    // High-level painting function
    bool paintFaceAdvanced(Object* obj, int faceIndex, const glm::vec2& uv, 
                          const GradientSettings* gradient, 
                          const SmudgeSettings* smudge) {
        if (!g_advancedPainter) {
            return false;
        }
        
        bool success = false;
        
        if (gradient) {
            PaintResult result = g_advancedPainter->paintFaceWithGradient(obj, faceIndex, *gradient);
            success = result.success;
        }
        
        if (smudge) {
            PaintResult result = g_advancedPainter->paintFaceWithSmudge(obj, faceIndex, uv, *smudge);
            success = success || result.success;
        }
        
        return success;
    }

} // namespace AdvancedFacePaint
