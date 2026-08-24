#pragma once

#include <glm/glm.hpp>
#include <vector>
#include <functional>
#include <cmath>
#include <memory>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// Contour: a generalized "face" or "surface" of a 3D shape.
//
// A *flat* contour is a traditional polygonal face -- all vertices on one plane,
// the normal is the same everywhere, and curvature is zero.
//
// A *round* contour has curvature -- normals vary across the surface, and
// its derivatives (rates of change) may not be uniform even if the surface
// looks smooth.  Think of the side of a cylinder or a patch of a sphere.
//
// "Contour" is intentionally more general than "face" or "side" because it
// covers both flat polygons and curved patches under a single concept.
// ---------------------------------------------------------------------------

class Contour {
public:
    enum class Type {
        Flat,
        Round
    };

    // Curvature descriptor (set of numbers describing how much a surface bends)
    struct CurvatureInfo {
        float gaussian    = 0.0f;  // Gaussian curvature  K = k1 * k2
        float mean        = 0.0f;  // Mean curvature      H = (k1 + k2) / 2
        float principalK1 = 0.0f;  // Maximum bending (largest principal curvature)
        float principalK2 = 0.0f;  // Minimum bending (smallest principal curvature)
        glm::vec3 principalDir1{1, 0, 0}; // Direction of max bending
        glm::vec3 principalDir2{0, 1, 0}; // Direction of min bending
    };

    virtual ~Contour() = default;

    virtual Type getType() const = 0;

    // Surface area of this contour
    virtual float area() const = 0;

    // Normal vector at a parametric (u,v) point on the contour (u,v in [0,1])
    virtual glm::vec3 normalAt(const glm::vec2& uv) const = 0;

    // 3D point on the surface at a parametric (u,v) coordinate
    virtual glm::vec3 pointAt(const glm::vec2& uv) const = 0;

    // Curvature descriptor at a parametric (u,v) point
    virtual CurvatureInfo curvatureAt(const glm::vec2& uv) const = 0;

    // Indices of boundary vertices (referencing the parent vertex buffer)
    virtual const std::vector<int>& getBoundaryVertices() const = 0;

    // Number of boundary edges
    virtual int edgeCount() const = 0;

    int id = -1;
};


// ---------------------------------------------------------------------------
// FlatContour -- a planar polygon face
// ---------------------------------------------------------------------------

class FlatContour : public Contour {
public:
    FlatContour() = default;

    // Construct from a list of vertex indices and the shared vertex buffer
    FlatContour(const std::vector<int>& verts, const std::vector<glm::vec3>& vertices);

    Type getType() const override { return Type::Flat; }
    float area() const override { return _area; }

    // For a flat contour the normal is constant everywhere
    glm::vec3 normalAt(const glm::vec2& /*uv*/) const override { return _normal; }

    // Bilinear-ish interpolation across the polygon
    glm::vec3 pointAt(const glm::vec2& uv) const override;

    // Curvature is zero everywhere on a flat surface
    CurvatureInfo curvatureAt(const glm::vec2& /*uv*/) const override;

    const std::vector<int>& getBoundaryVertices() const override { return _boundaryVertices; }
    int edgeCount() const override { return static_cast<int>(_boundaryVertices.size()); }

    // Recompute derived data after vertices move
    void recomputeNormal(const std::vector<glm::vec3>& vertices);
    void recomputeArea(const std::vector<glm::vec3>& vertices);

    // How far off from truly planar the vertices are (0 = perfectly flat)
    float planarityError(const std::vector<glm::vec3>& vertices) const;

    // Direct access
    const glm::vec3& getNormal() const { return _normal; }

private:
    std::vector<int> _boundaryVertices;
    glm::vec3 _normal{0, 1, 0};
    float _area = 0.0f;
    const std::vector<glm::vec3>* _vertexBuffer = nullptr;
};


// ---------------------------------------------------------------------------
// RoundContour -- a curved surface patch
// ---------------------------------------------------------------------------

class RoundContour : public Contour {
public:
    enum class SurfaceKind {
        Spherical,      // Patch of a sphere
        Cylindrical,    // Patch of a cylinder
        Conical,        // Patch of a cone
        Toroidal,       // Patch of a torus (donut shape)
        Freeform        // General curved surface (custom parametric function)
    };

    RoundContour() = default;

    Type getType() const override { return Type::Round; }
    float area() const override;
    glm::vec3 normalAt(const glm::vec2& uv) const override;
    glm::vec3 pointAt(const glm::vec2& uv) const override;
    CurvatureInfo curvatureAt(const glm::vec2& uv) const override;
    const std::vector<int>& getBoundaryVertices() const override { return _boundaryVertices; }
    int edgeCount() const override { return static_cast<int>(_boundaryVertices.size()); }

    // ----- Factory methods -----

    static RoundContour createSpherical(
        const glm::vec3& center, float radius,
        float startTheta, float endTheta,   // polar angle range
        float startPhi, float endPhi,       // azimuthal angle range
        const std::vector<int>& boundaryVerts);

    static RoundContour createCylindrical(
        const glm::vec3& center, const glm::vec3& axis, float radius,
        float height, float arcAngle,
        const std::vector<int>& boundaryVerts);

    static RoundContour createConical(
        const glm::vec3& apex, const glm::vec3& axis, float halfAngle,
        float height, float arcAngle,
        const std::vector<int>& boundaryVerts);

    static RoundContour createToroidal(
        const glm::vec3& center, const glm::vec3& axis,
        float majorRadius, float minorRadius,
        float arcAngle,
        const std::vector<int>& boundaryVerts);

    // Access
    SurfaceKind getSurfaceKind() const { return _surfaceKind; }
    const glm::vec3& getCenter() const { return _center; }
    float getRadius1() const { return _radius1; }
    float getRadius2() const { return _radius2; }
    const glm::vec3& getAxis() const { return _axis; }

    // For freeform: plug in your own parametric surface and normal functions
    // These are the hooks for future calculus / vector-calculus extensions
    std::function<glm::vec3(float u, float v)> surfaceFunction;
    std::function<glm::vec3(float u, float v)> normalFunction;

private:
    SurfaceKind _surfaceKind = SurfaceKind::Freeform;
    std::vector<int> _boundaryVertices;

    glm::vec3 _center{0, 0, 0};
    float _radius1 = 1.0f;        // Primary radius
    float _radius2 = 0.0f;        // Secondary radius (torus minor, ellipsoid, etc.)
    glm::vec3 _axis{0, 1, 0};     // Axis of symmetry
    float _arcAngle = 2.0f * static_cast<float>(M_PI);
    float _startAngle = 0.0f;
    float _height = 1.0f;
    float _halfAngle = 0.5f;      // For cones

    // Polar angle range (for spherical patches)
    float _startTheta = 0.0f;
    float _endTheta = static_cast<float>(M_PI);
    float _startPhi = 0.0f;
    float _endPhi = 2.0f * static_cast<float>(M_PI);
};
