#include <iostream>
#include <cmath>

struct vec3 { float x, y, z; };
vec3 add(vec3 a, vec3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
vec3 sub(vec3 a, vec3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
vec3 mul(vec3 a, float b) { return {a.x*b, a.y*b, a.z*b}; }
float length(vec3 v) { return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }

float sdfCube(vec3 p, vec3 b) {
    vec3 d = {std::abs(p.x) - b.x, std::abs(p.y) - b.y, std::abs(p.z) - b.z};
    float max_d = std::max(d.x, std::max(d.y, d.z));
    vec3 max_d_vec = {std::max(d.x, 0.0f), std::max(d.y, 0.0f), std::max(d.z, 0.0f)};
    return std::min(max_d, 0.0f) + length(max_d_vec);
}

int main() {
    vec3 ro = {2.0f, 2.0f, 2.0f};
    vec3 rd = {-0.577f, -0.577f, -0.577f}; // points directly at corner
    float t = 0.0f;
    
    for(int i=0; i<10; ++i) {
        vec3 p = add(ro, mul(rd, t));
        float raw = sdfCube(p, {1.0f, 1.0f, 1.0f});
        
        float ge = 1e-3f;
        float gx = sdfCube(add(p, {ge, 0, 0}), {1.0f, 1.0f, 1.0f}) - raw;
        float gy = sdfCube(add(p, {0, ge, 0}), {1.0f, 1.0f, 1.0f}) - raw;
        float gz = sdfCube(add(p, {0, 0, ge}), {1.0f, 1.0f, 1.0f}) - raw;
        float gl = length({gx/ge, gy/ge, gz/ge});
        
        float d = raw / gl;
        std::cout << "t: " << t << " raw: " << raw << " gl: " << gl << " d: " << d << "\n";
        
        if(d < 1e-4f) break;
        t += d;
    }
}
