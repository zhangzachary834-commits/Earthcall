// Topology adapters for the renderer boundary.
//
// Split out of Renderer.cpp deliberately: these are pure geometry with no backend
// dependency, whereas Renderer.cpp owns the active-backend singleton and therefore
// pulls in OpenGL. Keeping them separate lets a WebGPU-only target (the webgpu-*
// verification builds) use them without linking a GL backend it will never call.

#include "Rendering/Renderer.hpp"

#include <algorithm>
#include <cstring>

namespace draw {
namespace {
// Shared body for both quadsToTris overloads: quad (a,b,c,d) -> (a,b,c) + (a,c,d),
// the same winding GL_QUADS decomposed to internally.
template <typename V>
std::vector<V> quadsToTrisImpl(const std::vector<V>& quads) {
    std::vector<V> out;
    out.reserve(quads.size() / 4 * 6);
    for (size_t i = 0; i + 3 < quads.size(); i += 4) {
        out.push_back(quads[i]);     out.push_back(quads[i + 1]); out.push_back(quads[i + 2]);
        out.push_back(quads[i]);     out.push_back(quads[i + 2]); out.push_back(quads[i + 3]);
    }
    return out;
}
} // namespace

std::vector<glm::vec2> quadsToTris(const std::vector<glm::vec2>& q) { return quadsToTrisImpl(q); }
std::vector<glm::vec3> quadsToTris(const std::vector<glm::vec3>& q) { return quadsToTrisImpl(q); }

std::vector<glm::vec2> fanToTris(const std::vector<glm::vec2>& fan) {
    std::vector<glm::vec2> out;
    if (fan.size() < 3) return out;
    out.reserve((fan.size() - 2) * 3);
    for (size_t i = 1; i + 1 < fan.size(); ++i) {
        out.push_back(fan[0]); out.push_back(fan[i]); out.push_back(fan[i + 1]);
    }
    return out;
}

std::vector<glm::vec2> stripToSegments(const std::vector<glm::vec2>& pts, bool closed) {
    std::vector<glm::vec2> out;
    if (pts.size() < 2) return out;
    out.reserve((pts.size() - (closed ? 0 : 1)) * 2);
    for (size_t i = 0; i + 1 < pts.size(); ++i) {
        out.push_back(pts[i]); out.push_back(pts[i + 1]);
    }
    if (closed) { out.push_back(pts.back()); out.push_back(pts.front()); }
    return out;
}

std::vector<glm::vec2> dashSegments(const std::vector<glm::vec2>& segments,
                                    float dash, float gap) {
    std::vector<glm::vec2> out;
    const float period = dash + gap;
    if (period <= 0.0f) return segments;
    for (size_t i = 0; i + 1 < segments.size(); i += 2) {
        const glm::vec2 a = segments[i], b = segments[i + 1];
        const float len = glm::length(b - a);
        if (len <= 0.0f) continue;
        const glm::vec2 dir = (b - a) / len;
        for (float t = 0.0f; t < len; t += period) {
            out.push_back(a + dir * t);
            out.push_back(a + dir * std::min(t + dash, len));
        }
    }
    return out;
}

std::vector<glm::vec2> pointsToTris(const std::vector<glm::vec2>& points, float size) {
    const float h = size * 0.5f;
    std::vector<glm::vec2> quads;
    quads.reserve(points.size() * 4);
    for (const glm::vec2& p : points) {
        quads.push_back({p.x - h, p.y - h});
        quads.push_back({p.x + h, p.y - h});
        quads.push_back({p.x + h, p.y + h});
        quads.push_back({p.x - h, p.y + h});
    }
    return quadsToTris(quads);
}

std::vector<glm::vec2> rectTris(const glm::vec4& r) {
    return quadsToTris(std::vector<glm::vec2>{
        {r.x, r.y}, {r.z, r.y}, {r.z, r.w}, {r.x, r.w}});
}

std::vector<glm::vec2> rectOutline(const glm::vec4& r) {
    return stripToSegments({{r.x, r.y}, {r.z, r.y}, {r.z, r.w}, {r.x, r.w}}, /*closed=*/true);
}

std::vector<glm::vec2> easyFontToTris(const void* buffer, int quadCount) {
    // 16-byte stride: 3 floats of position then 4 bytes of colour. Only x and y
    // are used — stb_easy_font's z is always 0 and the colour is set by the caller.
    constexpr size_t kStride = 16;
    const auto* bytes = static_cast<const unsigned char*>(buffer);
    std::vector<glm::vec2> quads;
    quads.reserve(static_cast<size_t>(quadCount) * 4);
    for (int i = 0; i < quadCount * 4; ++i) {
        float xy[2];
        std::memcpy(xy, bytes + static_cast<size_t>(i) * kStride, sizeof(xy));
        quads.push_back({xy[0], xy[1]});
    }
    return quadsToTris(quads);
}

} // namespace draw
