#pragma once

// Shared SDF-building helpers: lower a named shape or an existing Object into a
// geom::SdfNode so the blend/boolean/combine tools can compose real shapes.
// Header-only (inline) so both the toolbar panels and the in-scene Combine tool
// use one definition. Object.hpp transitively provides geom:: (Geometry/Sdf.hpp).

#include "Form/Object/Object.hpp"

// Lower a named shape (+params) into an SDF leaf for morph/boolean composition.
inline geom::SdfNode shapeToSdfLeaf(Object::ShapeKind k, const Object::ShapeParams& p) {
    using P = geom::SdfPrim;
    using SN = geom::SdfNode;
    switch (k) {
        case Object::ShapeKind::Sphere:     return SN::leaf(P::Sphere, glm::vec3(p.r));
        case Object::ShapeKind::Ellipsoid:  return SN::leaf(P::Ellipsoid, glm::vec3(p.r, p.ry, p.rz));
        case Object::ShapeKind::Ovoid:      return SN::leaf(P::Ellipsoid, glm::vec3(p.r, p.r * (1.0f - 0.4f * p.ovoidAsym), p.r));
        case Object::ShapeKind::Cylinder:   return SN::leaf(P::Cylinder, glm::vec3(p.r, p.halfH, 0.0f));
        case Object::ShapeKind::Cone:       return SN::leaf(P::Cone, glm::vec3(p.r, p.halfH, 0.0f));
        case Object::ShapeKind::Torus:      return SN::leaf(P::Torus, glm::vec3(p.majorR, p.minorR, 0.0f));
        case Object::ShapeKind::RoundedBox: return SN::leaf(P::RoundBox, glm::vec3(0.5f), p.fillet);
        case Object::ShapeKind::Paraboloid: return SN::leaf(P::Sphere, glm::vec3(p.r)); // no SDF prim yet
        case Object::ShapeKind::Cube:
        default:                            return SN::leaf(P::Box, glm::vec3(0.5f));
    }
}

// Lower a flat-faced polyhedron into a convex-SDF leaf: one outward half-space per
// face, evaluated as max_i(n_i·p - d_i). Exact for convex polyhedra; for a concave
// polyhedron this yields its convex hull (a documented approximation). Planes are
// in the polyhedron's local space (same frame as its vertices).
inline std::vector<glm::vec4> polyhedronToConvexPlanes(const PolyhedronData& pd) {
    std::vector<glm::vec4> planes;
    const int nv = static_cast<int>(pd.vertices.size());
    if (nv < 4 || pd.faces.empty()) return planes;
    glm::vec3 c(0.0f);
    for (const auto& v : pd.vertices) c += v;
    c /= static_cast<float>(nv);
    for (const auto& face : pd.faces) {
        if (face.size() < 3) continue;
        if (face[0] < 0 || face[1] < 0 || face[2] < 0 ||
            face[0] >= nv || face[1] >= nv || face[2] >= nv) continue;
        const glm::vec3& a = pd.vertices[face[0]];
        const glm::vec3& b = pd.vertices[face[1]];
        const glm::vec3& d = pd.vertices[face[2]];
        glm::vec3 nrm = glm::cross(b - a, d - a);
        float len = glm::length(nrm);
        if (len < 1e-8f) continue;          // skip degenerate / collinear faces
        nrm /= len;
        if (glm::dot(nrm, a - c) < 0.0f) nrm = -nrm; // orient outward (away from centroid)
        planes.push_back(glm::vec4(nrm, glm::dot(nrm, a)));
    }
    return planes;
}

// Lower an EXISTING object into an SDF node so blend/boolean/combine can take the
// actual shape the user selected as an operand (not just a dropdown template). A
// field object contributes its expression tree directly; a polyhedron lowers to a
// convex-SDF leaf from its faces; everything else maps through its shape kind/params.
inline geom::SdfNode objectToSdfNode(const Object& o) {
    if (o.hasField()) return o.getFieldData();
    if (o.getGeometryType() == Object::GeometryType::Polyhedron) {
        auto planes = polyhedronToConvexPlanes(o.getPolyhedronData());
        if (!planes.empty()) return geom::SdfNode::convex(std::move(planes));
    }
    return shapeToSdfLeaf(o.getShapeKind(), o.getShapeParams());
}
