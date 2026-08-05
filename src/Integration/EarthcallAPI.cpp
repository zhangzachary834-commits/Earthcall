#include "Integration/EarthcallAPI.hpp"
#include "Integration/SecurityManager.hpp"
#include <iostream>

namespace Integration {

EarthcallAPI::EarthcallAPI() {
    // std::cout << "🔧 Earthcall API initialized" << std::endl;
    // Initialize system references (will be connected later)
    _brushSystem = nullptr;
    _designSystem = nullptr;
    _avatarManager = nullptr;
}

EarthcallAPI::~EarthcallAPI() = default;

bool EarthcallAPI::createBrushStroke(const BrushStroke& stroke) {
    if (!_checkPermission("brush_system")) {
        std::cout << "❌ Permission denied: brush_system" << std::endl;
        return false;
    }
    
    std::cout << "🎨 Creating brush stroke with " << stroke.points.size() << " points" << std::endl;
    // TODO: Actually create the brush stroke using the brush system
    return true;
}

bool EarthcallAPI::modifyBrushSettings(const BrushSettings& settings) {
    if (!_checkPermission("brush_system")) {
        std::cout << "❌ Permission denied: brush_system" << std::endl;
        return false;
    }
    
    std::cout << "🎨 Modifying brush settings: size=" << settings.size 
              << ", opacity=" << settings.opacity 
              << ", type=" << settings.brush_type << std::endl;
    // TODO: Actually modify brush settings
    return true;
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
    // TODO: Actually create the design element
    return true;
}

bool EarthcallAPI::modifyDesignElement(const std::string& name, const DesignElement& element) {
    (void)element; // Suppress unused parameter warning
    if (!_checkPermission("design_system")) {
        std::cout << "❌ Permission denied: design_system" << std::endl;
        return false;
    }
    
    std::cout << "🎨 Modifying design element: " << name << std::endl;
    // TODO: Actually modify the design element
    return true;
}

bool EarthcallAPI::deleteDesignElement(const std::string& name) {
    if (!_checkPermission("design_system")) {
        std::cout << "❌ Permission denied: design_system" << std::endl;
        return false;
    }
    
    std::cout << "🎨 Deleting design element: " << name << std::endl;
    // TODO: Actually delete the design element
    return true;
}

std::vector<EarthcallAPI::DesignElement> EarthcallAPI::getDesignElements() {
    std::vector<DesignElement> elements;
    // TODO: Get actual design elements
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

bool EarthcallAPI::modifyAvatar(const AvatarModification& modification) {
    if (!_checkPermission("avatar_system")) {
        std::cout << "❌ Permission denied: avatar_system" << std::endl;
        return false;
    }
    
    std::cout << "👤 Modifying avatar part: " << modification.part_name 
              << " (type: " << modification.modification_type << ")" << std::endl;
    // TODO: Actually modify the avatar
    return true;
}

bool EarthcallAPI::resetAvatarPart(const std::string& part_name) {
    if (!_checkPermission("avatar_system")) {
        std::cout << "❌ Permission denied: avatar_system" << std::endl;
        return false;
    }
    
    std::cout << "👤 Resetting avatar part: " << part_name << std::endl;
    // TODO: Actually reset the avatar part
    return true;
}

bool EarthcallAPI::exportAvatar(const std::string& filename) {
    if (!_checkPermission("file_system")) {
        std::cout << "❌ Permission denied: file_system" << std::endl;
        return false;
    }
    
    std::cout << "👤 Exporting avatar to: " << filename << std::endl;
    // TODO: Actually export the avatar
    return true;
}

bool EarthcallAPI::importAvatar(const std::string& filename) {
    if (!_checkPermission("file_system")) {
        std::cout << "❌ Permission denied: file_system" << std::endl;
        return false;
    }
    
    std::cout << "👤 Importing avatar from: " << filename << std::endl;
    // TODO: Actually import the avatar
    return true;
}

std::vector<std::string> EarthcallAPI::getAvailableAvatarParts() {
    return {"head", "body", "arms", "legs", "hands", "feet", "eyes", "hair"};
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