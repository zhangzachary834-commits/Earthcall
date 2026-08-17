// ============================================================================
// ObjectCore.cpp - Core Object implementations
//
// This file contains the core implementation of Object methods including
// basic property accessors, constructor, and polyhedron factory methods.
// ============================================================================

#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Object/Object/ObjectIdentity.hpp"
#include "ConstructedBeing/Object/Object/ObjectComposition.hpp"
#include "ConstructedBeing/Object/Object/ObjectEvents.hpp"
#include "ConstructedBeing/Object/Contour.hpp"
#include "ConstructedBeing/Object/AngleTools.hpp"
#include "ConstructedBeing/Object/Automation/AutomationEvents.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <cstring>
#include <cstdlib> // for rand()
#include <cmath>   // for mathematical functions
#include <limits>  // for numeric_limits
#include <optional>
#include <unordered_set>
#include <unordered_map>
#include "Singularity/Screen/HighlightSystem.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Basic property accessors

int Object::getDimensions() const {
    return static_cast<int>(dimensions);
}

void Object::setDimensions(int d) {
    dimensions = static_cast<float>(d);
}

int Object::getCorners() const {
    if (_shapeKind == ShapeKind::Polyhedron) {
        return polyhedronData.getVertexCount();
    }
    return _composition.corners;
}

void Object::setCorners(int c) {
    _composition.corners = c;
    // For polyhedrons, this could trigger a regeneration of the polyhedron
    // For now, we'll just store the value for compatibility
}

int Object::getFaces() const {
    if (_hasField)   return 1;
    if (_hasComplex) return complexData.patchCount();
    if (_hasSmooth)  return 1;
    if (_hasPatch)   return 1; // a Bezier patch is one surface with one face texture
    if (_shapeKind == ShapeKind::Polyhedron) {
        return polyhedronData.getFaceCount();
    }
    if (_composition.faces > 0) return _composition.faces;   // authored count wins
    // A cube has six faces whether or not anyone said so. Falling through to
    // an unset _composition.faces reported ZERO for every freshly built cube,
    // which is why each paint caller carried a "?: 6" fallback — and why a
    // cube painted by looping to getFaces() was painted not at all.
    return _shapeKind == ShapeKind::Cube ? 6 : 1;
}

void Object::setFaces(int f) {
    _composition.faces = f;
    // For polyhedrons, this could trigger a regeneration of the polyhedron
    // For now, we'll just store the value for compatibility
}

int Object::getMassQuantity() const {
    return _composition.massQuantity;
}

void Object::setMassQuantity(int m) {
    _composition.massQuantity = m;
}

int Object::getElements() const {
    return _composition.elements;
}

void Object::setElements(int e) {
    _composition.elements = e;
}

// Composition: element membership

void Object::addElement(Singular* s) {
    if (!s || s == this) return;   // nothing composes itself
    _composition._elementFormation.addMember(s);
}

bool Object::removeElement(Singular* s) {
    if (!s) return false;
    // releaseMember, not removeMember-if-hasMember: an element that only lives
    // in a subformation was previously reported absent AND left in place, which
    // is exactly the pointer that dangles once the being is destroyed.
    return _composition._elementFormation.releaseMember(s);
}

bool Object::hasElement(const Singular* s) const {
    return s && _composition._elementFormation.hasMember(s);
}

// Identity

std::string Object::getObjectID() const {
    return objectID;
}

void Object::setObjectID(int oi) {
    objectID = std::to_string(oi);
}

std::string Object::getObjectType() const {
    return objectType;
}

void Object::setObjectType(int ot) {
    objectType = std::to_string(ot);
}

// Position

int Object::getX() const {
    return x;
}

void Object::setX(int x) {
    this->x = x;
}

int Object::getY() const {
    return y;
}

void Object::setY(int y) {
    this->y = y;
}

int Object::getZ() const {
    return z;
}

void Object::setZ(int z) {
    this->z = z;
}

std::string Object::screenMode() const {

    if (dimensions == 2.0f) {
        return "2D";
    } else if (dimensions == 3.0f) {
        return "3D";
    } else {
        return "Unknown";
    }

}

// Polyhedron-specific methods

void Object::setPolyhedronData(const PolyhedronData& data) {
    polyhedronData = data;
    _polyhedronDirty = true; // render mesh cache must follow the geometry
    _hasSmooth = false;   // a polyhedron is flat-faced, not a topology surface
    _hasComplex = false;
    if (_shapeKind == ShapeKind::Polyhedron) {
    }
}

void Object::createTetrahedron() {
    _shapeKind = ShapeKind::Polyhedron;
    polyhedronData = PolyhedronData::createRegularPolyhedron(4);
    _polyhedronDirty = true;
}

void Object::createOctahedron() {
    _shapeKind = ShapeKind::Polyhedron;
    polyhedronData = PolyhedronData::createRegularPolyhedron(8);
    _polyhedronDirty = true;
}

void Object::createDodecahedron() {
    _shapeKind = ShapeKind::Polyhedron;
    polyhedronData = PolyhedronData::createRegularPolyhedron(12);
    _polyhedronDirty = true;
}

void Object::createIcosahedron() {
    _shapeKind = ShapeKind::Polyhedron;
    polyhedronData = PolyhedronData::createRegularPolyhedron(20);
    _polyhedronDirty = true;
}

void Object::createCustomPolyhedron(const std::vector<glm::vec3>& vertices, 
                                   const std::vector<std::vector<int>>& faces) {
    _shapeKind = ShapeKind::Polyhedron;
    polyhedronData = PolyhedronData::createCustomPolyhedron(vertices, faces);
    _polyhedronDirty = true;
}

// ID management lives in ObjectIdentity.hpp. The member function that used to
// stand here only forwarded to ObjectIdentity::claimIdentifierAtLeast and had
// no callers — a non-static member shadowing the namespace function it wrapped.

Object::Object(std::string explicitId) {
    if (!explicitId.empty()) {
        objectID = std::move(explicitId);
        ObjectIdentity::claimIdentifierAtLeast(objectID);
    } else {
        objectID = ObjectIdentity::generateObjectId();
        printf("WARNING: Object initialized without a stable string identifier. Assigned volatile ID '%s'. This object should not be reliably targeted by Law text.\n", objectID.c_str());
    }
    syncRotationStateFromTransform(transform);
}

Object::Object() : Object("") {}

void Object::setPosition(const glm::vec3& p) {
    glm::mat4 t = transform;
    t[3] = glm::vec4(p, 1.0f);
    setTransform(t);   // virtual: rotation-state sync + collision-zone update
}

// Property bridge: color

// setFaceColor both paints the object's own material and records the colour in
// the faceColors slot propColor reads back, so "color" round-trips through its
// own surface AND reaches the screen. It used to paint the face textures only,
// leaving the getter reporting the default forever; the two halves were split
// apart when paint moved to Material and are joined again in setFaceColor.
void Object::propSetColor(const glm::vec3& c) {
    const int faces = getFaces() > 0 ? getFaces() : 6;
    for (int f = 0; f < faces; ++f) setFaceColor(f, c.x, c.y, c.z);
}
