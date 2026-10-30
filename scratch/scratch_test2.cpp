#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

int main() {
    float fov = 45.0f;
    float aspect = 1920.0f / 1080.0f;
    float nearZ = 0.1f, farZ = 100.0f;
    
    float top = tanf(fov * M_PI / 360.0f) * nearZ;
    float right = top * aspect;
    
    glm::mat4 proj = glm::frustumZO(-right, right, -top, top, nearZ, farZ);
    glm::mat4 view = glm::lookAt(glm::vec3(0, 0, 5), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    glm::mat4 viewProj = proj * view;
    glm::mat4 invViewProj = glm::inverse(viewProj);
    
    glm::vec3 roWorld = glm::vec3(0, 0, 5);

    // Let's take a point on the near plane in world space
    // Center is (0,0,4.9), top-right is (0.0414*aspect, 0.0414, 4.9)
    glm::vec3 in_worldPos = glm::vec3(right, top, 5.0f - nearZ);
    
    // Transform to clip space
    glm::vec4 clip = viewProj * glm::vec4(in_worldPos, 1.0f);
    glm::vec3 true_ndc = glm::vec3(clip) / clip.w;
    
    // WebGPU Viewport transform (0 at top, height at bottom)
    float width = 1920.0f, height = 1080.0f;
    float in_clip_x = (true_ndc.x + 1.0f) * 0.5f * width;
    float in_clip_y = (1.0f - true_ndc.y) * 0.5f * height; // +1 maps to 0, -1 maps to height
    
    // Algorithm 1: NDC unprojection
    glm::vec4 ndc((in_clip_x / width) * 2.0f - 1.0f, 1.0f - (in_clip_y / height) * 2.0f, 1.0f, 1.0f);
    glm::vec4 worldPt = invViewProj * ndc;
    glm::vec3 rdWorld1 = glm::normalize(glm::vec3(worldPt) / worldPt.w - roWorld);
    
    // Algorithm 2: in.worldPos - roWorld
    glm::vec3 rdWorld2 = glm::normalize(in_worldPos - roWorld);
    
    std::cout << "ndc: " << ndc.x << ", " << ndc.y << ", " << ndc.z << "\n";
    std::cout << "true_ndc: " << true_ndc.x << ", " << true_ndc.y << ", " << true_ndc.z << "\n";
    std::cout << "rdWorld1: " << rdWorld1.x << ", " << rdWorld1.y << ", " << rdWorld1.z << "\n";
    std::cout << "rdWorld2: " << rdWorld2.x << ", " << rdWorld2.y << ", " << rdWorld2.z << "\n";
    
    float diff = glm::length(rdWorld1 - rdWorld2);
    std::cout << "Difference: " << diff << "\n";
}
