#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
    
    // Pixel (x, y) = (1920, 1080) -> bottom right
    float px = 1920.0f, py = 1080.0f;
    float width = 1920.0f, height = 1080.0f;
    
    glm::vec4 ndc((px / width) * 2.0f - 1.0f, 1.0f - (py / height) * 2.0f, 1.0f, 1.0f);
    glm::vec4 worldPt = invViewProj * ndc;
    glm::vec3 ro = glm::vec3(0, 0, 5);
    glm::vec3 rd = glm::normalize(glm::vec3(worldPt) / worldPt.w - ro);
    
    std::cout << "ndc: " << ndc.x << ", " << ndc.y << ", " << ndc.z << "\n";
    std::cout << "worldPt: " << worldPt.x / worldPt.w << ", " << worldPt.y / worldPt.w << ", " << worldPt.z / worldPt.w << "\n";
    std::cout << "rd: " << rd.x << ", " << rd.y << ", " << rd.z << "\n";
}
