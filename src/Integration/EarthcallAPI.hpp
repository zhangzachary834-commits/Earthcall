#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <glm/glm.hpp>
#include "../Person/AvatarManager.hpp"

// Forward declarations
namespace Rendering {
    class BrushSystem;
    class DesignSystem;
}

class AvatarManager;

namespace Integration {

// API for external applications to access Earthcall's creative features
class EarthcallAPI {
public:
    // Brush System API
    struct BrushSettings {
        float size = 1.0f;
        float opacity = 1.0f;
        glm::vec3 color = glm::vec3(1.0f);
        std::string brush_type = "default";
        bool pressure_sensitive = true;
    };

    struct BrushStroke {
        std::vector<glm::vec3> points;
        BrushSettings settings;
        std::string layer_name;
    };

    // Design System API
    struct DesignElement {
        std::string name;
        std::string type; // "shape", "texture", "pattern"
        glm::vec3 position;
        glm::vec3 scale;
        glm::vec3 rotation;
        std::map<std::string, std::string> properties;
    };

    // Avatar System API
    struct AvatarModification {
        std::string part_name;
        std::string modification_type; // "color", "shape", "texture"
        std::map<std::string, std::string> parameters;
    };

    // Constructor
    EarthcallAPI();
    ~EarthcallAPI();

    // Brush System Access
    bool createBrushStroke(const BrushStroke& stroke);
    bool modifyBrushSettings(const BrushSettings& settings);
    std::vector<std::string> getAvailableBrushTypes();
    bool clearBrushLayer(const std::string& layer_name);
    bool exportBrushArtwork(const std::string& filename);

    // Design System Access
    bool createDesignElement(const DesignElement& element);
    bool modifyDesignElement(const std::string& name, const DesignElement& element);
    bool deleteDesignElement(const std::string& name);
    std::vector<DesignElement> getDesignElements();
    bool applyDesignTemplate(const std::string& template_name);

    // Avatar System Access
    bool modifyAvatar(const AvatarModification& modification);
    bool resetAvatarPart(const std::string& part_name);
    bool exportAvatar(const std::string& filename);
    bool importAvatar(const std::string& filename);
    std::vector<std::string> getAvailableAvatarParts();

    // World/Environment Access
    bool createObject(const std::string& type, const glm::vec3& position);
    bool modifyObject(const std::string& id, const glm::vec3& position, const glm::vec3& scale);
    bool deleteObject(const std::string& id);
    glm::vec3 getCameraPosition();
    bool setCameraPosition(const glm::vec3& position);

    // Communication
    void registerCallback(const std::string& event_type, std::function<void(const std::string&)> callback);
    void unregisterCallback(const std::string& event_type);
    void sendEvent(const std::string& event_type, const std::string& data);

    // Permissions
    bool requestPermission(const std::string& permission);
    bool hasPermission(const std::string& permission) const;
    std::vector<std::string> getGrantedPermissions() const;

    // Lifecycle
    void update();
    void shutdown();

private:
    // System references
    Rendering::BrushSystem* _brushSystem = nullptr;
    Rendering::DesignSystem* _designSystem = nullptr;
    AvatarManager* _avatarManager = nullptr;

    // Permissions
    std::vector<std::string> _grantedPermissions;
    std::map<std::string, std::function<void(const std::string&)>> _callbacks;

    // Internal methods
    bool _checkPermission(const std::string& permission) const;
    void _notifyEvent(const std::string& event_type, const std::string& data);
};

// Global API instance
EarthcallAPI& getEarthcallAPI();

} // namespace Integration 