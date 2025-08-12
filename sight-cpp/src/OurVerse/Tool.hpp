#pragma once
#include <string>
#include <vector>
#include <ctime>
#include "Form/Object/Object.hpp"
#include "Form/Object/Formation/Formations.hpp"
#include "Person/Body/BodyPart/BodyPart.hpp"
// Forward declarations to avoid circular dependencies
namespace Core { class Game; }
class ZoneManager;
class Zone;
struct GLFWwindow;
#include <glm/glm.hpp>

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
        Measure
    };

    Tool(Type type) : type(type) {}

    static void use(GLFWwindow* window, ZoneManager& mgr, Zone& zone, Type type, Core::Game& game);

    static void ShapeGenerator3D(GLFWwindow* window, Core::Game* game, ZoneManager& mgr);
    static void Pottery3D(GLFWwindow* window, Core::Game* game, ZoneManager& mgr, float dt);
    static void FacePaint(GLFWwindow* window, Core::Game* game, ZoneManager& mgr, float dt);
    static void FaceBrush(GLFWwindow* window, Core::Game* game, ZoneManager& mgr, float dt);
    static void Selection3D(GLFWwindow* window, Core::Game* game, ZoneManager& mgr, float dt);

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