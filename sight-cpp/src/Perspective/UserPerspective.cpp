#include "UserPerspective.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

UserPerspective::UserPerspective(const std::string& name, PerspectiveType type)
    : _name(name), _type(type) {
    // Initialize default view state based on type
    switch (_type) {
        case PerspectiveType::FirstPerson:
            _viewState.position = {0.0f, 1.7f, 0.0f}; // Eye level
            _viewState.target = {0.0f, 1.7f, -1.0f};
            break;
        case PerspectiveType::ThirdPerson:
            _viewState.position = {0.0f, 2.0f, 5.0f};
            _viewState.target = {0.0f, 0.0f, 0.0f};
            _viewState.distance = 5.0f;
            break;
        case PerspectiveType::TopDown:
            _viewState.position = {0.0f, 10.0f, 0.0f};
            _viewState.target = {0.0f, 0.0f, 0.0f};
            _viewState.up = {0.0f, 0.0f, -1.0f};
            break;
        case PerspectiveType::Isometric:
            _viewState.position = {5.0f, 5.0f, 5.0f};
            _viewState.target = {0.0f, 0.0f, 0.0f};
            break;
        case PerspectiveType::FreeCamera:
            _viewState.position = {0.0f, 0.0f, 5.0f};
            _viewState.target = {0.0f, 0.0f, -1.0f};
            break;
    }
}

void UserPerspective::update(float deltaTime) {
    (void)deltaTime; // Suppress unused parameter warning
    if (!_isActive || _viewState.isLocked) {
        return;
    }
    
    // Update logic can be overridden by derived classes
    // This base implementation just ensures the view matrix is up to date
}

void UserPerspective::render() {
    if (!_isActive) {
        return;
    }
    
    // Base render implementation - can be overridden
    // This would typically set up the view and projection matrices
}

void UserPerspective::handleInput(GLFWwindow* window) {
    (void)window; // Suppress unused parameter warning
    if (!_isActive || _viewState.isLocked) {
        return;
    }
    
    // Base input handling - can be overridden by derived classes
    // This would handle mouse and keyboard input for camera control
}

void UserPerspective::activate() {
    _isActive = true;
}

void UserPerspective::deactivate() {
    _isActive = false;
}

void UserPerspective::setPosition(const glm::vec3& pos) {
    _viewState.position = pos;
    updateTargetFromPosition();
}

void UserPerspective::setTarget(const glm::vec3& target) {
    _viewState.target = target;
    updatePositionFromTarget();
}

void UserPerspective::setDistance(float distance) {
    _viewState.distance = distance;
    if (_type == PerspectiveType::ThirdPerson) {
        updatePositionFromTarget();
    }
}

void UserPerspective::setSensitivity(float sensitivity) {
    _settings.sensitivity = sensitivity;
}

void UserPerspective::setMoveSpeed(float speed) {
    _settings.moveSpeed = speed;
}

void UserPerspective::rotate(float deltaYaw, float deltaPitch) {
    if (_viewState.isLocked) {
        return;
    }
    
    _viewState.yaw += deltaYaw * _settings.sensitivity;
    _viewState.pitch += deltaPitch * _settings.sensitivity;
    
    // Clamp pitch to prevent gimbal lock
    if (_viewState.pitch > 89.0f) _viewState.pitch = 89.0f;
    if (_viewState.pitch < -89.0f) _viewState.pitch = -89.0f;
    
    updateTargetFromAngles();
}

void UserPerspective::zoom(float delta) {
    if (_viewState.isLocked) {
        return;
    }
    
    glm::vec3 direction;
    
    switch (_type) {
        case PerspectiveType::ThirdPerson:
            _viewState.distance -= delta * _settings.zoomSpeed;
            if (_viewState.distance < 1.0f) _viewState.distance = 1.0f;
            updatePositionFromTarget();
            break;
        case PerspectiveType::FreeCamera:
            // Move forward/backward
            direction = glm::normalize(_viewState.target - _viewState.position);
            _viewState.position += direction * delta * _settings.moveSpeed;
            _viewState.target += direction * delta * _settings.moveSpeed;
            break;
        default:
            // For other perspectives, adjust FOV or move closer/farther
            _settings.fov -= delta * 5.0f;
            if (_settings.fov < 10.0f) _settings.fov = 10.0f;
            if (_settings.fov > 120.0f) _settings.fov = 120.0f;
            break;
    }
}

void UserPerspective::pan(const glm::vec3& delta) {
    if (_viewState.isLocked) {
        return;
    }
    
    _viewState.position += delta * _settings.moveSpeed;
    _viewState.target += delta * _settings.moveSpeed;
}

void UserPerspective::reset() {
    // Reset to default state based on type
    switch (_type) {
        case PerspectiveType::FirstPerson:
            _viewState.position = {0.0f, 1.7f, 0.0f};
            _viewState.target = {0.0f, 1.7f, -1.0f};
            _viewState.yaw = -90.0f;
            _viewState.pitch = 0.0f;
            break;
        case PerspectiveType::ThirdPerson:
            _viewState.position = {0.0f, 2.0f, 5.0f};
            _viewState.target = {0.0f, 0.0f, 0.0f};
            _viewState.distance = 5.0f;
            _viewState.yaw = -90.0f;
            _viewState.pitch = 0.0f;
            break;
        case PerspectiveType::TopDown:
            _viewState.position = {0.0f, 10.0f, 0.0f};
            _viewState.target = {0.0f, 0.0f, 0.0f};
            _viewState.up = {0.0f, 0.0f, -1.0f};
            break;
        case PerspectiveType::Isometric:
            _viewState.position = {5.0f, 5.0f, 5.0f};
            _viewState.target = {0.0f, 0.0f, 0.0f};
            break;
        case PerspectiveType::FreeCamera:
            _viewState.position = {0.0f, 0.0f, 5.0f};
            _viewState.target = {0.0f, 0.0f, -1.0f};
            _viewState.yaw = -90.0f;
            _viewState.pitch = 0.0f;
            break;
    }
}

glm::mat4 UserPerspective::getViewMatrix() const {
    return glm::lookAt(_viewState.position, _viewState.target, _viewState.up);
}

glm::mat4 UserPerspective::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(_settings.fov), aspectRatio, _settings.nearPlane, _settings.farPlane);
}

// Private helper methods
void UserPerspective::updateTargetFromPosition() {
    if (_type == PerspectiveType::ThirdPerson) {
        // Calculate target based on position and distance
        glm::vec3 direction = glm::normalize(_viewState.position - _viewState.target);
        _viewState.target = _viewState.position - direction * _viewState.distance;
    }
}

void UserPerspective::updatePositionFromTarget() {
    if (_type == PerspectiveType::ThirdPerson) {
        // Calculate position based on target and distance
        glm::vec3 direction = glm::normalize(_viewState.position - _viewState.target);
        _viewState.position = _viewState.target + direction * _viewState.distance;
    }
}

void UserPerspective::updateTargetFromAngles() {
    // Calculate target based on yaw and pitch angles
    glm::vec3 direction;
    direction.x = cos(glm::radians(_viewState.yaw)) * cos(glm::radians(_viewState.pitch));
    direction.y = sin(glm::radians(_viewState.pitch));
    direction.z = sin(glm::radians(_viewState.yaw)) * cos(glm::radians(_viewState.pitch));
    
    _viewState.target = _viewState.position + direction;
}
