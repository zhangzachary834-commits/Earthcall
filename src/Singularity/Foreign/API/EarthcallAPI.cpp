#include "Singularity/Foreign/API/EarthcallAPI.hpp"
#include "Singularity/Foreign/API/SecurityManager.hpp"
#include "Singularity/Screen/BrushSystem.hpp"
#include "Singularity/FirstMoverOntology/Legacy/DesignSystem.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include <iostream>
namespace Integration {

EarthcallAPI::EarthcallAPI() {
    // std::cout << "🔧 Earthcall API initialized" << std::endl;
    // Initialize system references (will be connected later)
    _brushSystem = nullptr;
    _designSystem = nullptr;
}

EarthcallAPI::~EarthcallAPI() = default;

bool EarthcallAPI::createBrushStroke(const BrushStroke& stroke) {
    if (!_checkPermission("brush_system")) {
        std::cout << "❌ Permission denied: brush_system" << std::endl;
        return false;
    }
    
    std::cout << "🎨 Creating brush stroke with " << stroke.points.size() << " points" << std::endl;
    if (_brushSystem && !stroke.points.empty()) {
        _brushSystem->setOpacity(stroke.settings.opacity);
        _brushSystem->setRadius(stroke.settings.size);
        _brushSystem->setPressureSimulation(stroke.settings.pressure_sensitive);
        
        if (stroke.points.size() == 1) {
            _brushSystem->paintDab(glm::vec2(stroke.points[0].x, stroke.points[0].y), stroke.settings.color);
        } else {
            for (size_t i = 1; i < stroke.points.size(); ++i) {
                _brushSystem->paintStroke(
                    glm::vec2(stroke.points[i-1].x, stroke.points[i-1].y),
                    glm::vec2(stroke.points[i].x, stroke.points[i].y),
                    stroke.settings.color
                );
            }
        }
        _brushSystem->updateTexture();
        return true;
    }
    return false;
}

bool EarthcallAPI::modifyBrushSettings(const BrushSettings& settings) {
    if (!_checkPermission("brush_system")) {
        std::cout << "❌ Permission denied: brush_system" << std::endl;
        return false;
    }
    
    std::cout << "🎨 Modifying brush settings: size=" << settings.size 
              << ", opacity=" << settings.opacity 
              << ", type=" << settings.brush_type << std::endl;
    if (_brushSystem) {
        _brushSystem->setOpacity(settings.opacity);
        _brushSystem->setRadius(settings.size);
        _brushSystem->setPressureSimulation(settings.pressure_sensitive);
        
        if (settings.brush_type == "normal") _brushSystem->setBrushType(BrushSystem::BrushType::Normal);
        else if (settings.brush_type == "airbrush") _brushSystem->setBrushType(BrushSystem::BrushType::Airbrush);
        else if (settings.brush_type == "chalk") _brushSystem->setBrushType(BrushSystem::BrushType::Chalk);
        else if (settings.brush_type == "spray") _brushSystem->setBrushType(BrushSystem::BrushType::Spray);
        else if (settings.brush_type == "smudge") _brushSystem->setBrushType(BrushSystem::BrushType::Smudge);
        else if (settings.brush_type == "clone") _brushSystem->setBrushType(BrushSystem::BrushType::Clone);
        
        return true;
    }
    return false;
}

std::vector<std::string> EarthcallAPI::getAvailableBrushTypes() {
    return {"default", "soft", "hard", "airbrush", "chalk", "smudge", "clone"};
}

bool EarthcallAPI::clearBrushLayer(const std::string& layer_name) {
    if (!_checkPermission("brush_system")) {
        std::cout << "❌ Permission denied: brush_system" << std::endl;
        return false;
    }
    
    std::cout << "🎨 Clearing brush layer: " << layer_name << std::endl;
    // TODO: Actually clear the layer
    return true;
}

bool EarthcallAPI::exportBrushArtwork(const std::string& filename) {
    if (!_checkPermission("file_system")) {
        std::cout << "❌ Permission denied: file_system" << std::endl;
        return false;
    }
    
    std::cout << "🎨 Exporting brush artwork to: " << filename << std::endl;
    // TODO: Actually export the artwork
    return true;
}

bool EarthcallAPI::createDesignElement(const DesignElement& element) {
    if (!_checkPermission("design_system")) {
        std::cout << "❌ Permission denied: design_system" << std::endl;
        return false;
    }
    
    std::cout << "🎨 Creating design element: " << element.name 
              << " (type: " << element.type << ")" << std::endl;
    _designElements[element.name] = element;

    if (_designSystem) {
        if (element.type == "text" && _designSystem->getTextSystem()) {
            _designSystem->getTextSystem()->addText(element.name, glm::vec2(element.position.x, element.position.y));
        } else if (_designSystem->getShapeSystem()) {
            _designSystem->getShapeSystem()->addShape(
                ShapeSystem::ShapeType::Rectangle,
                glm::vec2(element.position.x, element.position.y),
                glm::vec2(element.scale.x, element.scale.y)
            );
        }
    }
    return true;
}

bool EarthcallAPI::modifyDesignElement(const std::string& name, const DesignElement& element) {
    if (!_checkPermission("design_system")) {
        std::cout << "❌ Permission denied: design_system" << std::endl;
        return false;
    }
    
    auto it = _designElements.find(name);
    if (it == _designElements.end()) {
        std::cout << "❌ Design element not found: " << name << std::endl;
        return false;
    }

    std::cout << "🎨 Modifying design element: " << name << std::endl;
    it->second = element;
    if (element.name != name && !element.name.empty()) {
        _designElements.erase(it);
        _designElements[element.name] = element;
    }

    if (_designSystem) {
        if (element.type == "text" && _designSystem->getTextSystem()) {
            _designSystem->getTextSystem()->addText(element.name, glm::vec2(element.position.x, element.position.y));
        } else if (_designSystem->getShapeSystem()) {
            _designSystem->getShapeSystem()->addShape(
                ShapeSystem::ShapeType::Rectangle,
                glm::vec2(element.position.x, element.position.y),
                glm::vec2(element.scale.x, element.scale.y)
            );
        }
    }
    return true;
}

bool EarthcallAPI::deleteDesignElement(const std::string& name) {
    if (!_checkPermission("design_system")) {
        std::cout << "❌ Permission denied: design_system" << std::endl;
        return false;
    }
    
    auto it = _designElements.find(name);
    if (it == _designElements.end()) {
        return false;
    }

    std::cout << "🎨 Deleting design element: " << name << std::endl;
    _designElements.erase(it);
    return true;
}

std::vector<EarthcallAPI::DesignElement> EarthcallAPI::getDesignElements() {
    std::vector<DesignElement> elements;
    if (!_checkPermission("design_system")) {
        return elements;
    }
    for (const auto& [name, elem] : _designElements) {
        elements.push_back(elem);
    }
    return elements;
}

bool EarthcallAPI::applyDesignTemplate(const std::string& template_name) {
    if (!_checkPermission("design_system")) {
        std::cout << "❌ Permission denied: design_system" << std::endl;
        return false;
    }
    
    std::cout << "🎨 Applying design template: " << template_name << std::endl;
    // TODO: Actually apply the template
    return true;
}

bool EarthcallAPI::createZone(const std::string& name, float x, float y, float width, float height) {
    (void)width; (void)height;
    if (!_checkPermission("world_access")) return false;
    std::cout << "🌍 Creating zone: " << name << " at (" << x << ", " << y << ")" << std::endl;
    if (_zoneManager) {
        // _zoneManager->createZone(...)
        return true;
    }
    return false;
}

bool EarthcallAPI::addZoneObject(const std::string& zoneName, const std::string& objectType, float x, float y) {
    if (!_checkPermission("world_access")) return false;
    std::cout << "🌍 Adding " << objectType << " to zone " << zoneName << " at (" << x << ", " << y << ")" << std::endl;
    return true;
}

bool EarthcallAPI::setZoneTheme(const std::string& zoneName, const std::string& theme) {
    if (!_checkPermission("world_access")) return false;
    std::cout << "🌍 Setting zone " << zoneName << " theme to " << theme << std::endl;
    return true;
}

std::vector<std::string> EarthcallAPI::getZones() {
    std::vector<std::string> zones;
    if (_checkPermission("world_access") && _zoneManager) {
        // ...
    }
    return zones;
}

bool EarthcallAPI::saveData(const std::string& key, const std::string& value) {
    if (!_checkPermission("data_access")) return false;
    std::cout << "💾 Saving data: " << key << " = " << value << std::endl;
    return true;
}

std::string EarthcallAPI::loadData(const std::string& key) {
    if (!_checkPermission("data_access")) return "";
    std::cout << "💾 Loading data: " << key << std::endl;
    return "";
}

std::vector<std::string> EarthcallAPI::getDataKeys() {
    std::vector<std::string> keys;
    if (!_checkPermission("data_access")) return keys;
    std::cout << "💾 Getting data keys" << std::endl;
    return keys;
}

bool EarthcallAPI::createObject(const std::string& type, const glm::vec3& position) {
    if (!_checkPermission("world_access")) {
        std::cout << "❌ Permission denied: world_access" << std::endl;
        return false;
    }
    
    std::cout << "🌍 Creating object: " << type 
              << " at (" << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
    // TODO: Actually create the object
    return true;
}

bool EarthcallAPI::modifyObject(const std::string& id, const glm::vec3& position, const glm::vec3& scale) {
    (void)position; // Suppress unused parameter warning
    (void)scale;    // Suppress unused parameter warning
    if (!_checkPermission("world_access")) {
        std::cout << "❌ Permission denied: world_access" << std::endl;
        return false;
    }
    
    std::cout << "🌍 Modifying object: " << id << std::endl;
    // TODO: Actually modify the object
    return true;
}

bool EarthcallAPI::deleteObject(const std::string& id) {
    if (!_checkPermission("world_access")) {
        std::cout << "❌ Permission denied: world_access" << std::endl;
        return false;
    }
    
    std::cout << "🌍 Deleting object: " << id << std::endl;
    // TODO: Actually delete the object
    return true;
}

glm::vec3 EarthcallAPI::getCameraPosition() {
    // TODO: Get actual camera position from the game
    return glm::vec3(0.0f, 0.0f, 0.0f);
}

bool EarthcallAPI::setCameraPosition(const glm::vec3& position) {
    if (!_checkPermission("world_access")) {
        std::cout << "❌ Permission denied: world_access" << std::endl;
        return false;
    }
    
    std::cout << "🌍 Setting camera position to (" 
              << position.x << ", " << position.y << ", " << position.z << ")" << std::endl;
    // TODO: Actually set camera position
    return true;
}

void EarthcallAPI::registerCallback(const std::string& event_type, std::function<void(const std::string&)> callback) {
    _callbacks[event_type] = callback;
    std::cout << "🔧 Registered callback for event: " << event_type << std::endl;
}

void EarthcallAPI::unregisterCallback(const std::string& event_type) {
    _callbacks.erase(event_type);
    std::cout << "🔧 Unregistered callback for event: " << event_type << std::endl;
}

void EarthcallAPI::sendEvent(const std::string& event_type, const std::string& data) {
    _notifyEvent(event_type, data);
}

bool EarthcallAPI::requestPermission(const std::string& permission) {
    // Map string permission to SecurityManager permission type
    auto& security = SecurityManager::instance();
    
    std::map<std::string, PermissionType> permissionMap = {
        {"brush_system", PermissionType::BRUSH_SYSTEM},
        {"design_system", PermissionType::DESIGN_SYSTEM},
        {"avatar_system", PermissionType::AVATAR_SYSTEM},
        {"world_access", PermissionType::WORLD_ACCESS},
        {"file_system", PermissionType::FILE_SYSTEM},
        {"network_access", PermissionType::NETWORK_ACCESS},
        {"ui_control", PermissionType::UI_CONTROL},
        {"data_access", PermissionType::DATA_ACCESS}
    };
    
    auto it = permissionMap.find(permission);
    if (it != permissionMap.end()) {
        // Use security manager to request permission
        return security.requestPermission(it->second, "earthcall_api");
    }
    
    std::cout << "❌ Unknown permission: " << permission << std::endl;
    return false;
}

bool EarthcallAPI::hasPermission(const std::string& permission) const {
    auto& security = SecurityManager::instance();
    
    std::map<std::string, PermissionType> permissionMap = {
        {"brush_system", PermissionType::BRUSH_SYSTEM},
        {"design_system", PermissionType::DESIGN_SYSTEM},
        {"avatar_system", PermissionType::AVATAR_SYSTEM},
        {"world_access", PermissionType::WORLD_ACCESS},
        {"file_system", PermissionType::FILE_SYSTEM},
        {"network_access", PermissionType::NETWORK_ACCESS},
        {"ui_control", PermissionType::UI_CONTROL},
        {"data_access", PermissionType::DATA_ACCESS}
    };
    
    auto it = permissionMap.find(permission);
    if (it != permissionMap.end()) {
        return security.hasPermission(it->second, "earthcall_api");
    }
    
    return false;
}

std::vector<std::string> EarthcallAPI::getGrantedPermissions() const {
    auto& security = SecurityManager::instance();
    auto permissions = security.getGrantedPermissions("earthcall_api");
    
    std::vector<std::string> result;
    for (const auto& perm : permissions) {
        result.push_back(std::to_string(static_cast<int>(perm)));
    }
    return result;
}

void EarthcallAPI::update() {
    // Handle any pending updates
}

void EarthcallAPI::shutdown() {
    _callbacks.clear();
    _grantedPermissions.clear();
    std::cout << "🔧 Earthcall API shutdown" << std::endl;
}

bool EarthcallAPI::_checkPermission(const std::string& permission) const {
    return hasPermission(permission);
}

void EarthcallAPI::_notifyEvent(const std::string& event_type, const std::string& data) {
    auto it = _callbacks.find(event_type);
    if (it != _callbacks.end()) {
        it->second(data);
    }
}

// Global API instance
EarthcallAPI& getEarthcallAPI() {
    static EarthcallAPI* s_api = nullptr;
    if (!s_api) {
        s_api = new EarthcallAPI();
    }
    return *s_api;
}

} // namespace Integration 