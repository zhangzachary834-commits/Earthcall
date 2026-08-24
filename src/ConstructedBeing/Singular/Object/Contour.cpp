#include "Contour.hpp"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ===========================================================================
//  FlatContour
// ===========================================================================

FlatContour::FlatContour(const std::vector<int>& verts,
                         const std::vector<glm::vec3>& vertices)
    : _boundaryVertices(verts), _vertexBuffer(&vertices)
{
    recomputeNormal(vertices);
    recomputeArea(vertices);
}

void FlatContour::recomputeNormal(const std::vector<glm::vec3>& vertices) {
    // Newell's method -- stable for polygons with more than 3 vertices
    glm::vec3 n(0.0f);
    size_t m = _boundaryVertices.size();
    if (m < 3) { _normal = glm::vec3(0, 1, 0); return; }
    for (size_t i = 0; i < m; ++i) {
        const glm::vec3& cur  = vertices[_boundaryVertices[i]];
        const glm::vec3& next = vertices[_boundaryVertices[(i + 1) % m]];
        n.x += (cur.y - next.y) * (cur.z + next.z);
        n.y += (cur.z - next.z) * (cur.x + next.x);
        n.z += (cur.x - next.x) * (cur.y + next.y);
    }
    float len = glm::length(n);
    _normal = (len > 1e-8f) ? (n / len) : glm::vec3(0, 1, 0);
}

void FlatContour::recomputeArea(const std::vector<glm::vec3>& vertices) {
    _area = 0.0f;
    if (_boundaryVertices.size() < 3) return;
    const glm::vec3& v0 = vertices[_boundaryVertices[0]];
    for (size_t i = 1; i + 1 < _boundaryVertices.size(); ++i) {
        const glm::vec3& v1 = vertices[_boundaryVertices[i]];
        const glm::vec3& v2 = vertices[_boundaryVertices[i + 1]];
        _area += glm::length(glm::cross(v1 - v0, v2 - v0)) * 0.5f;
    }
}

glm::vec3 FlatContour::pointAt(const glm::vec2& uv) const {
    if (!_vertexBuffer || _boundaryVertices.size() < 3) return glm::vec3(0);
    // Weighted centroid interpolation using (u,v) as barycentric-like coords
    glm::vec3 centroid(0.0f);
    for (int idx : _boundaryVertices)
        centroid += (*_vertexBuffer)[idx];
    centroid /= static_cast<float>(_boundaryVertices.size());

    // Map u to an edge index, v to center-to-edge blend
    size_t n = _boundaryVertices.size();
    float fi = uv.x * static_cast<float>(n);
    size_t i0 = static_cast<size_t>(fi) % n;
    size_t i1 = (i0 + 1) % n;
    float frac = fi - std::floor(fi);
    glm::vec3 edgePt = glm::mix((*_vertexBuffer)[_boundaryVertices[i0]],
                                 (*_vertexBuffer)[_boundaryVertices[i1]], frac);
    return glm::mix(centroid, edgePt, uv.y);
}

Contour::CurvatureInfo FlatContour::curvatureAt(const glm::vec2& /*uv*/) const {
    // A flat surface has zero curvature everywhere
    CurvatureInfo info;
    info.principalDir1 = glm::vec3(1, 0, 0);
    info.principalDir2 = glm::vec3(0, 1, 0);
    if (_boundaryVertices.size() >= 2 && _vertexBuffer) {
        glm::vec3 edge = (*_vertexBuffer)[_boundaryVertices[1]]
                       - (*_vertexBuffer)[_boundaryVertices[0]];
        float len = glm::length(edge);
        if (len > 1e-8f) {
            info.principalDir1 = edge / len;
            info.principalDir2 = glm::normalize(glm::cross(_normal, info.principalDir1));
        }
    }
    return info;
}

float FlatContour::planarityError(const std::vector<glm::vec3>& vertices) const {
    if (_boundaryVertices.size() < 4) return 0.0f;
    // Measure max distance of any vertex from the plane defined by normal + first vertex
    const glm::vec3& p0 = vertices[_boundaryVertices[0]];
    float maxDist = 0.0f;
    for (size_t i = 1; i < _boundaryVertices.size(); ++i) {
        float d = std::fabs(glm::dot(vertices[_boundaryVertices[i]] - p0, _normal));
        maxDist = std::max(maxDist, d);
    }
    return maxDist;
}


// ===========================================================================
//  RoundContour
// ===========================================================================

float RoundContour::area() const {
    switch (_surfaceKind) {
        case SurfaceKind::Spherical: {
            // Area of a spherical zone: A = R^2 * |deltaPhi| * |cos(theta1) - cos(theta2)|
            float dPhi = _endPhi - _startPhi;
            float dCosTheta = std::fabs(std::cos(_startTheta) - std::cos(_endTheta));
            return _radius1 * _radius1 * std::fabs(dPhi) * dCosTheta;
        }
        case SurfaceKind::Cylindrical:
            return _radius1 * _arcAngle * _height;
        case SurfaceKind::Conical: {
            float slantHeight = _height / std::cos(_halfAngle);
            float r = _height * std::tan(_halfAngle);
            return 0.5f * r * slantHeight * (_arcAngle / static_cast<float>(M_PI));
        }
        case SurfaceKind::Toroidal: {
            // Full torus: A = 4 * pi^2 * R * r
            // Partial: scale by arc fraction
            float fraction = _arcAngle / (2.0f * static_cast<float>(M_PI));
            return 4.0f * static_cast<float>(M_PI * M_PI) * _radius1 * _radius2 * fraction;
        }
        case SurfaceKind::Freeform:
            // Numerical approximation would go here in the future
            return 0.0f;
    }
    return 0.0f;
}

glm::vec3 RoundContour::normalAt(const glm::vec2& uv) const {
    if (normalFunction) return normalFunction(uv.x, uv.y);

    switch (_surfaceKind) {
        case SurfaceKind::Spherical: {
            float theta = _startTheta + uv.y * (_endTheta - _startTheta);
            float phi   = _startPhi   + uv.x * (_endPhi   - _startPhi);
            return glm::vec3(std::sin(theta) * std::cos(phi),
                             std::cos(theta),
                             std::sin(theta) * std::sin(phi));
        }
        case SurfaceKind::Cylindrical: {
            float angle = _startAngle + uv.x * _arcAngle;
            // Build a local frame from the axis
            glm::vec3 up = glm::normalize(_axis);
            glm::vec3 ref = (std::fabs(up.y) < 0.9f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
            glm::vec3 t1 = glm::normalize(glm::cross(ref, up));
            glm::vec3 t2 = glm::normalize(glm::cross(up, t1));
            return glm::normalize(t1 * std::cos(angle) + t2 * std::sin(angle));
        }
        case SurfaceKind::Conical: {
            float angle = _startAngle + uv.x * _arcAngle;
            glm::vec3 up = glm::normalize(_axis);
            glm::vec3 ref = (std::fabs(up.y) < 0.9f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
            glm::vec3 t1 = glm::normalize(glm::cross(ref, up));
            glm::vec3 t2 = glm::normalize(glm::cross(up, t1));
            glm::vec3 radial = t1 * std::cos(angle) + t2 * std::sin(angle);
            glm::vec3 n = glm::normalize(radial * std::cos(_halfAngle) + up * std::sin(_halfAngle));
            return n;
        }
        case SurfaceKind::Toroidal: {
            float majorAngle = _startAngle + uv.x * _arcAngle;
            float minorAngle = uv.y * 2.0f * static_cast<float>(M_PI);
            glm::vec3 up = glm::normalize(_axis);
            glm::vec3 ref = (std::fabs(up.y) < 0.9f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
            glm::vec3 t1 = glm::normalize(glm::cross(ref, up));
            glm::vec3 t2 = glm::normalize(glm::cross(up, t1));
            glm::vec3 radialDir = t1 * std::cos(majorAngle) + t2 * std::sin(majorAngle);
            return glm::normalize(radialDir * std::cos(minorAngle) + up * std::sin(minorAngle));
        }
        case SurfaceKind::Freeform:
            return glm::vec3(0, 1, 0);
    }
    return glm::vec3(0, 1, 0);
}

glm::vec3 RoundContour::pointAt(const glm::vec2& uv) const {
    if (surfaceFunction) return surfaceFunction(uv.x, uv.y);

    switch (_surfaceKind) {
        case SurfaceKind::Spherical: {
            float theta = _startTheta + uv.y * (_endTheta - _startTheta);
            float phi   = _startPhi   + uv.x * (_endPhi   - _startPhi);
            return _center + _radius1 * glm::vec3(
                std::sin(theta) * std::cos(phi),
                std::cos(theta),
                std::sin(theta) * std::sin(phi));
        }
        case SurfaceKind::Cylindrical: {
            float angle = _startAngle + uv.x * _arcAngle;
            float h     = uv.y * _height;
            glm::vec3 up = glm::normalize(_axis);
            glm::vec3 ref = (std::fabs(up.y) < 0.9f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
            glm::vec3 t1 = glm::normalize(glm::cross(ref, up));
            glm::vec3 t2 = glm::normalize(glm::cross(up, t1));
            return _center + (t1 * std::cos(angle) + t2 * std::sin(angle)) * _radius1 + up * h;
        }
        case SurfaceKind::Conical: {
            float angle = _startAngle + uv.x * _arcAngle;
            float t = uv.y; // 0 = base, 1 = apex
            float r = _height * std::tan(_halfAngle) * (1.0f - t);
            float h = t * _height;
            glm::vec3 up = glm::normalize(_axis);
            glm::vec3 ref = (std::fabs(up.y) < 0.9f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
            glm::vec3 t1 = glm::normalize(glm::cross(ref, up));
            glm::vec3 t2 = glm::normalize(glm::cross(up, t1));
            return _center + (t1 * std::cos(angle) + t2 * std::sin(angle)) * r + up * h;
        }
        case SurfaceKind::Toroidal: {
            float majorAngle = _startAngle + uv.x * _arcAngle;
            float minorAngle = uv.y * 2.0f * static_cast<float>(M_PI);
            glm::vec3 up = glm::normalize(_axis);
            glm::vec3 ref = (std::fabs(up.y) < 0.9f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
            glm::vec3 t1 = glm::normalize(glm::cross(ref, up));
            glm::vec3 t2 = glm::normalize(glm::cross(up, t1));
            glm::vec3 ringCenter = _center + (t1 * std::cos(majorAngle) + t2 * std::sin(majorAngle)) * _radius1;
            glm::vec3 radialDir = t1 * std::cos(majorAngle) + t2 * std::sin(majorAngle);
            return ringCenter + (radialDir * std::cos(minorAngle) + up * std::sin(minorAngle)) * _radius2;
        }
        case SurfaceKind::Freeform:
            return _center;
    }
    return _center;
}

Contour::CurvatureInfo RoundContour::curvatureAt(const glm::vec2& uv) const {
    CurvatureInfo info;

    switch (_surfaceKind) {
        case SurfaceKind::Spherical: {
            // Sphere: both principal curvatures equal 1/R
            float k = 1.0f / std::max(1e-8f, _radius1);
            info.principalK1 = k;
            info.principalK2 = k;
            info.gaussian = k * k;
            info.mean = k;
            glm::vec3 n = normalAt(uv);
            glm::vec3 ref = (std::fabs(n.y) < 0.9f) ? glm::vec3(0,1,0) : glm::vec3(1,0,0);
            info.principalDir1 = glm::normalize(glm::cross(ref, n));
            info.principalDir2 = glm::normalize(glm::cross(n, info.principalDir1));
            break;
        }
        case SurfaceKind::Cylindrical: {
            // Cylinder: k1 = 1/R along circumferential direction, k2 = 0 along axis
            float k = 1.0f / std::max(1e-8f, _radius1);
            info.principalK1 = k;
            info.principalK2 = 0.0f;
            info.gaussian = 0.0f;
            info.mean = k / 2.0f;
            info.principalDir2 = glm::normalize(_axis);
            glm::vec3 n = normalAt(uv);
            info.principalDir1 = glm::normalize(glm::cross(info.principalDir2, n));
            break;
        }
        case SurfaceKind::Conical: {
            // Cone curvature varies with distance from apex
            float t = uv.y;
            float r = _height * std::tan(_halfAngle) * (1.0f - t);
            float k1 = (r > 1e-8f) ? (1.0f / r) : 0.0f;
            info.principalK1 = k1;
            info.principalK2 = 0.0f;
            info.gaussian = 0.0f;
            info.mean = k1 / 2.0f;
            break;
        }
        case SurfaceKind::Toroidal: {
            // Torus: k1 = 1/r (minor), k2 depends on position
            float minorAngle = uv.y * 2.0f * static_cast<float>(M_PI);
            float k1 = 1.0f / std::max(1e-8f, _radius2);
            float k2 = std::cos(minorAngle) / std::max(1e-8f, _radius1 + _radius2 * std::cos(minorAngle));
            info.principalK1 = k1;
            info.principalK2 = k2;
            info.gaussian = k1 * k2;
            info.mean = (k1 + k2) / 2.0f;
            break;
        }
        case SurfaceKind::Freeform:
            break;
    }

    return info;
}


// ---------------------------------------------------------------------------
// Factory methods
// ---------------------------------------------------------------------------

RoundContour RoundContour::createSpherical(
    const glm::vec3& center, float radius,
    float startTheta, float endTheta,
    float startPhi, float endPhi,
    const std::vector<int>& boundaryVerts)
{
    RoundContour c;
    c._surfaceKind = SurfaceKind::Spherical;
    c._center = center;
    c._radius1 = radius;
    c._axis = glm::vec3(0, 1, 0);
    c._startTheta = startTheta;
    c._endTheta = endTheta;
    c._startPhi = startPhi;
    c._endPhi = endPhi;
    c._boundaryVertices = boundaryVerts;
    return c;
}

RoundContour RoundContour::createCylindrical(
    const glm::vec3& center, const glm::vec3& axis, float radius,
    float height, float arcAngle,
    const std::vector<int>& boundaryVerts)
{
    RoundContour c;
    c._surfaceKind = SurfaceKind::Cylindrical;
    c._center = center;
    c._axis = axis;
    c._radius1 = radius;
    c._height = height;
    c._arcAngle = arcAngle;
    c._boundaryVertices = boundaryVerts;
    return c;
}

RoundContour RoundContour::createConical(
    const glm::vec3& apex, const glm::vec3& axis, float halfAngle,
    float height, float arcAngle,
    const std::vector<int>& boundaryVerts)
{
    RoundContour c;
    c._surfaceKind = SurfaceKind::Conical;
    c._center = apex;
    c._axis = axis;
    c._halfAngle = halfAngle;
    c._height = height;
    c._arcAngle = arcAngle;
    c._boundaryVertices = boundaryVerts;
    return c;
}

RoundContour RoundContour::createToroidal(
    const glm::vec3& center, const glm::vec3& axis,
    float majorRadius, float minorRadius,
    float arcAngle,
    const std::vector<int>& boundaryVerts)
{
    RoundContour c;
    c._surfaceKind = SurfaceKind::Toroidal;
    c._center = center;
    c._axis = axis;
    c._radius1 = majorRadius;
    c._radius2 = minorRadius;
    c._arcAngle = arcAngle;
    c._boundaryVertices = boundaryVerts;
    return c;
}
