#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <map>
#include <glm/glm.hpp>

// Forward declarations
class BrushSystem;
class DesignSystem;

namespace ZonesOfEarth {
    class ZoneManager;
}

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


    // World/Environment Access
    bool createZone(const std::string& name, float x, float y, float width, float height);
    bool addZoneObject(const std::string& zoneName, const std::string& objectType, float x, float y);
    bool setZoneTheme(const std::string& zoneName, const std::string& theme);
    std::vector<std::string> getZones();
    bool createObject(const std::string& type, const glm::vec3& position);
    bool modifyObject(const std::string& id, const glm::vec3& position, const glm::vec3& scale);
    bool deleteObject(const std::string& id);
    glm::vec3 getCameraPosition();
    bool setCameraPosition(const glm::vec3& position);
    
    // Data/Save Access
    bool saveData(const std::string& key, const std::string& value);
    std::string loadData(const std::string& key);
    std::vector<std::string> getDataKeys();

    // Communication
    void registerCallback(const std::string& event_type, std::function<void(const std::string&)> callback);
    void unregisterCallback(const std::string& event_type);
    void sendEvent(const std::string& event_type, const std::string& data);

    // Permissions
    bool requestPermission(const std::string& permission);
    bool hasPermission(const std::string& permission) const;
    std::vector<std::string> getGrantedPermissions() const;

    // System Access Setters
    void setBrushSystem(BrushSystem* system) { _brushSystem = system; }
    void setDesignSystem(DesignSystem* system) { _designSystem = system; }
    void setZoneManager(ZonesOfEarth::ZoneManager* manager) { _zoneManager = manager; }

    // Lifecycle
    void update();
    void shutdown();

private:
    struct DesignElementRecord {
        DesignElement element;
        std::string systemId;
        std::string systemType; // "shape", "text", "effect"
    };

    // System references
    BrushSystem* _brushSystem = nullptr;
    DesignSystem* _designSystem = nullptr;
    ZonesOfEarth::ZoneManager* _zoneManager = nullptr;

    // Design Elements Storage
    std::map<std::string, DesignElementRecord> _designElements;

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