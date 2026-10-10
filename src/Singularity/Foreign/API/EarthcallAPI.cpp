#include "Singularity/Foreign/API/EarthcallAPI.hpp"
#include "Singularity/Foreign/API/SecurityManager.hpp"
#include "Singularity/Screen/BrushSystem.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include <iostream>
#include <ctime>
namespace Integration {

EarthcallAPI::EarthcallAPI() {
    // std::cout << "🔧 Earthcall API initialized" << std::endl;
    // Initialize system references (will be connected later)
    _brushSystem = nullptr;
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
    if (!_brushSystem) {
        return false;
    }

    if (layer_name == "all") {
        _brushSystem->clearAllLayers();
    } else if (layer_name == "active" || layer_name == "current" || layer_name == "default" || layer_name.empty()) {
        _brushSystem->clearLayer(_brushSystem->getActiveLayer());
    } else {
        try {
            int idx = std::stoi(layer_name);
            if (idx >= 0 && idx < _brushSystem->getLayerCount()) {
                _brushSystem->clearLayer(idx);
            } else {
                _brushSystem->clearLayer(_brushSystem->getActiveLayer());
            }
        } catch (...) {
            _brushSystem->clearLayer(_brushSystem->getActiveLayer());
        }
    }
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

    std::string elementName = element.name;
    if (elementName.empty()) {
        static int autoId = 1;
        elementName = "element_" + std::to_string(autoId++);
    }

    std::cout << "🎨 Creating design element: " << elementName
              << " (type: " << element.type << ")" << std::endl;

    DesignElement recordElement = element;
    recordElement.name = elementName;

    DesignElementRecord record;
    record.element = recordElement;
    record.systemId = elementName;
    record.systemType = element.type;

    _designElements[elementName] = record;
    return true;
}

bool EarthcallAPI::modifyDesignElement(const std::string& name, const DesignElement& element) {
    if (!_checkPermission("design_system")) {
        std::cout << "❌ Permission denied: design_system" << std::endl;
        return false;
    }

    std::cout << "🎨 Modifying design element: " << name << std::endl;
    auto it = _designElements.find(name);
    if (it == _designElements.end()) {
        std::cout << "❌ Design element not found: " << name << std::endl;
        return false;
    }

    it->second.element = element;
    it->second.element.name = name;

    return true;
}

bool EarthcallAPI::deleteDesignElement(const std::string& name) {
    if (!_checkPermission("design_system")) {
        std::cout << "❌ Permission denied: design_system" << std::endl;
        return false;
    }

    std::cout << "🎨 Deleting design element: " << name << std::endl;
    auto it = _designElements.find(name);
    if (it == _designElements.end()) {
        std::cout << "❌ Design element not found for deletion: " << name << std::endl;
        return false;
    }

    _designElements.erase(it);
    return true;
}

std::vector<EarthcallAPI::DesignElement> EarthcallAPI::getDesignElements() {
    std::vector<DesignElement> elements;
    for (const auto& [name, record] : _designElements) {
        elements.push_back(record.element);
    }
    return elements;
}

bool EarthcallAPI::applyDesignTemplate(const std::string& template_name) {
    if (!_checkPermission("design_system")) {
        std::cout << "❌ Permission denied: design_system" << std::endl;
        return false;
    }

    std::cout << "🎨 Applying design template: " << template_name << std::endl;

    if (template_name == "banner" || template_name == "card" || template_name == "default" || template_name == "basic") {
        DesignElement bg;
        bg.name = template_name + "_background";
        bg.type = "shape";
        bg.position = glm::vec3(0.0f, 0.0f, 0.0f);
        bg.scale = glm::vec3(400.0f, 200.0f, 1.0f);
        bg.properties["shape_type"] = "rectangle";
        bg.properties["color"] = "#2d3748";
        bg.properties["strokeColor"] = "#4a5568";
        bg.properties["strokeWidth"] = "2";
        createDesignElement(bg);

        DesignElement title;
        title.name = template_name + "_title";
        title.type = "text";
        title.position = glm::vec3(0.0f, 50.0f, 0.0f);
        title.properties["text"] = "Template: " + template_name;
        title.properties["color"] = "#ffffff";
        title.properties["fontSize"] = "28";
        createDesignElement(title);

        DesignElement subtitle;
        subtitle.name = template_name + "_subtitle";
        subtitle.type = "text";
        subtitle.position = glm::vec3(0.0f, 10.0f, 0.0f);
        subtitle.properties["text"] = "Created via EarthcallAPI";
        subtitle.properties["color"] = "#a0aec0";
        subtitle.properties["fontSize"] = "18";
        createDesignElement(subtitle);

        return true;
    }

    // Generic fall-back element for custom template names
    DesignElement el;
    el.name = template_name + "_element";
    el.type = "shape";
    el.position = glm::vec3(0.0f, 0.0f, 0.0f);
    el.scale = glm::vec3(200.0f, 200.0f, 1.0f);
    el.properties["shape_type"] = "rectangle";
    el.properties["color"] = "#3182ce";
    createDesignElement(el);

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

    if (_zoneManager && !_zoneManager->zones().empty() &&
        _zoneManager->currentIndex() < _zoneManager->zones().size() &&
        _zoneManager->zones()[_zoneManager->currentIndex()]) {
        auto obj = std::make_shared<Object>();
        obj->setPosition(position);
        obj->setObjectType(type);
        obj->setName(type);
        _zoneManager->active().addObject(obj);
        return true;
    }

    return false;
}

bool EarthcallAPI::modifyObject(const std::string& id, const glm::vec3& position, const glm::vec3& scale) {
    if (!_checkPermission("world_access")) {
        std::cout << "❌ Permission denied: world_access" << std::endl;
        return false;
    }
    
    std::cout << "🌍 Modifying object: " << id << std::endl;

    if (_zoneManager) {
        for (auto& zone : _zoneManager->zones()) {
            for (auto& obj : zone->getOwnedObjectsMutable()) {
                if (obj->getIdentifier() == id) {
                    glm::mat4 t = obj->getTransform();
                    glm::vec3 oldScale(glm::length(glm::vec3(t[0])), glm::length(glm::vec3(t[1])), glm::length(glm::vec3(t[2])));

                    if (oldScale.x > 1e-6f) t[0] = (t[0] / oldScale.x) * scale.x;
                    if (oldScale.y > 1e-6f) t[1] = (t[1] / oldScale.y) * scale.y;
                    if (oldScale.z > 1e-6f) t[2] = (t[2] / oldScale.z) * scale.z;

                    t[3] = glm::vec4(position, 1.0f);
                    obj->setTransform(t);
                    return true;
                }
            }
        }
    }

    std::cout << "❌ Object not found: " << id << std::endl;
    return false;
}

bool EarthcallAPI::deleteObject(const std::string& id) {
    if (!_checkPermission("world_access")) {
        std::cout << "❌ Permission denied: world_access" << std::endl;
        return false;
    }
    
    std::cout << "🌍 Deleting object: " << id << std::endl;
    if (_zoneManager && !_zoneManager->zones().empty() &&
        _zoneManager->currentIndex() < _zoneManager->zones().size() &&
        _zoneManager->zones()[_zoneManager->currentIndex()]) {
        return _zoneManager->active().removeObjectById(id);
    }
    return false;
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

bool EarthcallAPI::setCursorType(const std::string& cursorType) {
    if (!_checkPermission("ui_control")) {
        std::cout << "❌ Permission denied: ui_control" << std::endl;
        return false;
    }
    _currentCursorType = cursorType;
    std::cout << "🖱️ EarthcallAPI set cursor type: " << cursorType << std::endl;
    _notifyEvent("cursor_changed", cursorType);
    return true;
}

std::string EarthcallAPI::getCursorType() const {
    return _currentCursorType;
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
    // A foreign caller's named event, admitted to Law under its own name -
    // same bridging role as Person::raiseEvent, but for this Foreign channel.
    Core::EventBus::instance().publish(ECA::Event{event_type, nullptr, nullptr, std::time(nullptr)});
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