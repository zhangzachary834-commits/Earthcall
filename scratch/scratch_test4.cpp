#include <iostream>
#include <cmath>

struct vec3 { float x, y, z; };
vec3 add(vec3 a, vec3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
vec3 mul(vec3 a, float b) { return {a.x*b, a.y*b, a.z*b}; }
float length(vec3 v) { return std::sqrt(v.x*v.x + v.y*v.y + v.z*v.z); }

float sdfIntersect(vec3 p) {
    // sphere 1: radius 1 at x=0.5
    float d1 = length({p.x - 0.5f, p.y, p.z}) - 1.0f;
    // sphere 2: radius 1 at x=-0.5
    float d2 = length({p.x + 0.5f, p.y, p.z}) - 1.0f;
    return std::max(d1, d2);
}

int main() {
    vec3 ro = {0.0f, 2.0f, 0.0f};
    vec3 rd = {0.0f, -1.0f, 0.0f}; // points directly at intersection seam
    float t = 0.0f;
    
    for(int i=0; i<10; ++i) {
        vec3 p = add(ro, mul(rd, t));
        float raw = sdfIntersect(p);
        
        float ge = 1e-3f;
        float gx = sdfIntersect(add(p, {ge, 0, 0})) - raw;
        float gy = sdfIntersect(add(p, {0, ge, 0})) - raw;
        float gz = sdfIntersect(add(p, {0, 0, ge})) - raw;
        float gl = length({gx/ge, gy/ge, gz/ge});
        
        float d = raw / gl;
        std::cout << "t: " << t << " raw: " << raw << " gl: " << gl << " d: " << d << "\n";
        
        if(std::abs(d) < 1e-4f) break;
        t += d;
    }
}
