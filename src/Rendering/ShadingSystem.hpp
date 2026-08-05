#ifndef SHADING_SYSTEM_HPP
#define SHADING_SYSTEM_HPP

#include <glm/glm.hpp>

class ShadingSystem {
public:
    // Initialize lighting and shading state. Call once after GL context creation.
    static void init();

    // Update dynamic parts of the shading system each frame (e.g., light position)
    static void update(const glm::vec3& cameraPos);

    // Toggle shading on/off
    static void setEnabled(bool enabled);
    static bool isEnabled();

private:
    static bool s_enabled;
};

#endif // SHADING_SYSTEM_HPP 