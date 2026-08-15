#pragma once
#include <string>
#include <vector>
#include <ctime>
#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Object/Formation/Formation.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
#include <glm/glm.hpp>

// Forward declarations to avoid circular dependencies
namespace Core { class Engine; }
namespace Singularity { namespace Core { class CreationChannel; } }
class ZoneManager;
class Zone;
struct GLFWwindow;

// Rich surface pick used by tools that need the hit point / normal.
struct SurfaceHit {
    Object* obj = nullptr;
    float   t = 0.0f;
    int     face = -1;
    glm::vec3 point{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    bool    isCube = false;
    int     axis = -1; // cube only: 0=X,1=Y,2=Z
    int     sign = 1;  // cube only: +1/-1 (outward face direction)
};

bool buildMouseRay(GLFWwindow* window, Core::Engine* engine, glm::vec3& rayOrigin, glm::vec3& rayDir);
bool pickSurface(const std::vector<Object*>& targets, const glm::vec3& rayOrigin, const glm::vec3& rayDir, SurfaceHit& out);

class Tool {
public:
    enum class Type {
        // Drawing Tools
        Brush,
        Pencil,
        Pen,
        Marker,
        Airbrush,
        Chalk,
        Spray,
        Smudge,
        Clone,
        
        // Erasing Tools
        Eraser,
        Delete,
        MagicEraser,
        
        // Selection Tools
        Selection,
        Lasso,
        MagicWand,
        Marquee,
        
        // Shape Tools
        Rectangle,
        Ellipse,
        Polygon,
        Line,
        Arrow,
        Star,
        Heart,
        CustomShape,
        
        // Text Tools
        Text,
        TextVertical,
        TextPath,
        
        // Transform Tools
        Move,
        Scale,
        Rotate,
        Skew,
        Distort,
        Perspective,
        
        // Effects Tools
        Blur,
        Sharpen,
        Noise,
        Emboss,
        Glow,
        Shadow,
        Gradient,
        Pattern,
        
        // Utility Tools
        ColorPicker,
        Eyedropper,
        Hand,
        Zoom,
        Crop,
        Slice,
        
        // Layer Tools
        Layer,
        LayerMask,
        LayerStyle,
        
        // 3D Tools (for compatibility)
        FaceBrush,
        FacePaint,
        
        // Special Tools
        Symmetry,
        Mirror,
        Grid,
        Ruler,
        Measure,
        Identity
    };

    Tool(Type type) : type(type) {}

    static void use(GLFWwindow* window, ZoneManager& mgr, Zone& zone, Type type, Core::Engine& engine);

    // Developer tool, restored from the raw pre-law implementation (deleted in
    // 0da7237, recovered in AGENTS.md's 2026-08-13 restore). Direct-spawns the
    // shape/placement/colour CreationChannel currently holds, bypassing
    // Law::applyTo entirely -- this is the debug bypass, not the Person-facing
    // creation path. That path is the "Tool: Shape Generator 3D" law
    // (saves/fixtures/shape_generator_3d_law.json), which reads the SAME
    // CreationChannel fields but fires off onMouseClicked only when
    // active3DMode == "Create", armed by a distinct input
    // (see DeveloperToolsWindow's law-mode key toggle) so the two paths never
    // both fire off one click.
    static void ShapeGenerator3D(GLFWwindow* window, Core::Engine* engine, ZoneManager& mgr,
                                 Singularity::Core::CreationChannel& channel,
                                 BodyPart* targetPart = nullptr);

    static void Pottery3D(GLFWwindow* window, Core::Engine* engine, ZoneManager& mgr, float dt,
                          const std::vector<Object*>& targets, const glm::mat4* avatarRoot);
    static void Rotate3D(GLFWwindow* window, Core::Engine* engine, ZoneManager& mgr, float dt,
                         const std::vector<Object*>& targets, const glm::mat4* avatarRoot);
    static void FacePaint(GLFWwindow* window, Core::Engine* engine, ZoneManager& mgr, float dt,
                          const std::vector<Object*>& targets);
    static void FaceBrush(GLFWwindow* window, Core::Engine* engine, ZoneManager& mgr, float dt,
                          const std::vector<Object*>& targets);
    static void Selection3D(GLFWwindow* window, Core::Engine* engine,
                            const std::vector<Object*>& targets);
    // Pick the object under the cursor/crosshair and RETURN it (no side effects).
    // Shares the same ray/pick path as Selection3D; used by tools that need to
    // pick an operand without changing the current selection.
    static Object* PickObject3D(GLFWwindow* window, Core::Engine* engine,
                                const std::vector<Object*>& targets);

    Type getType() const;
    std::string getTypeName() const;
    
    std::string getIcon() const;
    
    // Tool categories for organization
    enum class Category {
        Drawing,
        Erasing,
        Selection,
        Shape,
        Text,
        Transform,
        Effects,
        Utility,
        Layer,
        Special
    };
    
    Category getCategory() const;

private:
    Type type;
};
