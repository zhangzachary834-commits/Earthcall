#include <iostream>
#include <cmath>
#include <algorithm>

struct vec3 { float x, y, z; };
vec3 add(vec3 a, vec3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
vec3 mul(vec3 a, float b) { return {a.x*b, a.y*b, a.z*b}; }
float length(vec3 v) { return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }
float mix(float a, float b, float t) { return a + t * (b - a); }
float clamp(float x, float a, float b) { return std::max(a, std::min(x, b)); }

float opSmoothDifference(float d1, float d2, float k) {
    float h = clamp(0.5f - 0.5f * (d2 + d1) / k, 0.0f, 1.0f);
    return mix(d2, -d1, h) + k * h * (1.0f - h);
}

float sdfSmoothDiff(vec3 p) {
    float d1 = length({p.x, p.y, p.z}) - 1.0f; // subtract this
    float d2 = length({p.x - 1.0f, p.y, p.z}) - 1.0f; // from this
    return opSmoothDifference(d1, d2, 0.5f);
}

int main() {
    vec3 ro = {0.5f, 2.0f, 0.0f};
    vec3 rd = {0.0f, -1.0f, 0.0f}; 
    
    for(float t = 1.0f; t <= 1.4f; t += 0.05f) {
        vec3 p = add(ro, mul(rd, t));
        float raw = sdfSmoothDiff(p);
        std::cout << "t: " << t << " raw: " << raw << "\n";
    }
}
