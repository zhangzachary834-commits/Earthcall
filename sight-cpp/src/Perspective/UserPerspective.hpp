#pragma once
#define GLM_ENABLE_EXPERIMENTAL
#include <string>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

class UserPerspective {
public:
    enum class PerspectiveType {
        FirstPerson,
        ThirdPerson,
        TopDown,
        Isometric,
        FreeCamera
    };

    struct CameraSettings {
        float fov = 45.0f;
        float nearPlane = 0.1f;
        float farPlane = 1000.0f;
        float sensitivity = 0.1f;
        float moveSpeed = 5.0f;
        float zoomSpeed = 2.0f;
    };

    struct ViewState {
        glm::vec3 position{0.0f, 0.0f, 0.0f};
        glm::vec3 target{0.0f, 0.0f, -1.0f};
        glm::vec3 up{0.0f, 1.0f, 0.0f};
        float yaw = -90.0f;
        float pitch = 0.0f;
        float distance = 5.0f;  // For third person
        bool isLocked = false;
    };

protected:
    std::string _name;
    PerspectiveType _type;
    CameraSettings _settings;
    ViewState _viewState;
    bool _isActive = false;

public:
    UserPerspective(const std::string& name, PerspectiveType type = PerspectiveType::ThirdPerson);
    virtual ~UserPerspective() = default;

    // Core functionality
    virtual void update(float deltaTime);
    virtual void render();
    virtual void handleInput(GLFWwindow* window);
    
    // State management
    void activate();
    void deactivate();
    bool isActive() const { return _isActive; }
    
    // Camera control
    void setPosition(const glm::vec3& pos);
    void setTarget(const glm::vec3& target);
    void setDistance(float distance);
    void setSensitivity(float sensitivity);
    void setMoveSpeed(float speed);
    
    // View manipulation
    void rotate(float deltaYaw, float deltaPitch);
    void zoom(float delta);
    void pan(const glm::vec3& delta);
    void reset();
    
    // Getters
    const std::string& getName() const { return _name; }
    PerspectiveType getType() const { return _type; }
    const ViewState& getViewState() const { return _viewState; }
    const CameraSettings& getSettings() const { return _settings; }
    
    // Matrix calculations
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
    
    // Utility
    void lockView(bool locked) { _viewState.isLocked = locked; }
    bool isViewLocked() const { return _viewState.isLocked; }

private:
    // Private helper methods
    void updateTargetFromPosition();
    void updatePositionFromTarget();
    void updateTargetFromAngles();
};
