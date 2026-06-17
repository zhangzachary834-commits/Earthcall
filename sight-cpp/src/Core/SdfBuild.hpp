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

// Lower an EXISTING object into an SDF node so blend/boolean/combine can take the
// actual shape the user selected as an operand (not just a dropdown template). A
// field object contributes its expression tree directly; everything else maps
// through its shape kind + params.
inline geom::SdfNode objectToSdfNode(const Object& o) {
    if (o.hasField()) return o.getFieldData();
    return shapeToSdfLeaf(o.getShapeKind(), o.getShapeParams());
}
