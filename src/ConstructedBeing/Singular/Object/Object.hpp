#pragma once

// ============================================================================
// Object — the core spatial entity. This is one class, but its implementation
// is split across several translation units by responsibility (the declaration
// lives here; definitions are grouped where noted):
//
//   Object.cpp            constructor, simple property accessors, screenMode,
//                         polyhedron factories, hover state, attributes & tags
//   ObjectPaint.cpp       per-face texture init + paint/stroke/smudge/clone/
//                         airbrush + texture layers & stroke history
//   ObjectRender.cpp      drawCube / smooth / complex / field / drawObject /
//                         highlight outline / drawPolyhedron (+ GL helpers)
//   ObjectRaycast.cpp     raycastFace per-geometry picking (+ tri-soup ray test)
//   ObjectCollision.cpp   collision zone/AABB, support cloud & support points,
//                         point penetration, isPointInside / isTouching
//   ObjectMotion.cpp      setTransform, rotation state/advance, automation clips
//
// (PolyhedronData and FaceTexture have their own files under Object/.)
// ============================================================================

#include <vector>
#include <atomic>
#include <cstdint>
#include "Relation/Formation/Formation.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "ConstructedBeing/Singular/Singular.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Contour.hpp"
#include "AngleTools.hpp"
#include "Object/ObjectIdentity.hpp"
#include "Object/ObjectTypes.hpp"
#include "Object/ObjectEvents.hpp"
#include "Object/ObjectComposition.hpp"
#include "Object/PolyhedronData.hpp"
#include "Object/FaceTexture.hpp"
#include "Object/CollisionZone.hpp"
#include "Geometry/SmoothSurface.hpp"
#include "Geometry/ComplexShape.hpp"
#include "Geometry/Sdf.hpp"
#include "Geometry/Patch.hpp"
#include "Automation/Automation.hpp"
#include <unordered_map>
#include <memory>
#include <string>

// Forward declaration to break circular dependency
class BodyPart;
// Material includes FaceTexture, which includes Renderer.hpp; an Object only
// ever holds its material by identifier, so the declaration is enough here.
class Material;

class Object : public Singular {

public:
    // Geometry types are defined in ObjectTypes.hpp
    using ShapeKind = ObjectTypes::ShapeKind;
    using ShapeParams = ObjectTypes::ShapeParams;
    using SpatialKind = ObjectTypes::SpatialKind;
    using StateSnapshot = ObjectTypes::StateSnapshot;
    using RenderMode = ObjectTypes::RenderMode;

    std::string screenMode() const;

    /*There was previously a comment block here about 'recursive Object set to set creation.'
     *That's now retired, since it's simply a case witin Singular set-to-set. */

    // Get the dimensions of the object.
    int getDimensions() const;
    void setDimensions(int d);

    // ------------------------------------------------------------------
    // Elements — the beings this object is composed of. "Recursive Object
    // Creation" above, made real: any Singular may be an element of an
    // Object, so a law can build a composite out of beings it created
    // (ActionNode::Create + ActionNode::AddElement) without the engine
    // knowing what it is building. Membership is a Formation, not a
    // vector<Object*>: elements are beings held in relation, so the
    // structure among them (Formation::relations) rides along, and
    // Formation's own identity makes the composition addressable.
    //
    // Ownership: the Formation holds NON-OWNING pointers, exactly like every
    // other Formation. A newborn created by a law is owned by the Zone; its
    // element membership is a second, relational fact about it.
    // ------------------------------------------------------------------
    Formation& elementFormation() { return _composition._elementFormation; }
    const Formation& elementFormation() const { return _composition._elementFormation; }
    void addElement(Singular* s);
    bool removeElement(Singular* s);
    bool hasElement(const Singular* s) const;
    int elementCount() const {
        return static_cast<int>(_composition._elementFormation.getMembers().size());
    }

    // Transient load state: element identifiers read from a save, waiting for
    // the rest of the world to exist before they can be re-linked into the
    // element Formation (World's from_json does the pass, then clears this).
    std::vector<std::string>& getPendingElementIds() { return _composition.pendingElementIds; }
    const std::vector<std::string>& getPendingElementIds() const { return _composition.pendingElementIds; }

    // Unique identifier for the object.
    std::string getObjectID() const;
    void setObjectID(int oi);
    void setObjectID(const std::string& oi) {
        objectID = oi;
        ObjectIdentity::claimIdentifierAtLeast(oi);   // a restored id advances the counter
    }

    std::string getObjectType() const;
    void setObjectType(int ot);
    // A law naming what it made needs a WORD, not a number. Physics law
    // targeting already filters on this string (LawTarget::limitByObjectType),
    // so an authored kind is selectable by the engine's own laws.
    void setObjectType(const std::string& ot) { objectType = ot; }

    // Position in 2D/3D space. The anchor point. Replace with glm::vec3 if using GLM for better math operations.
    int getX() const; // x coordinate
    void setX(int x);

    int getY() const; // y coordinate
    void setY(int y);

    int getZ() const; // z coordinate
    void setZ(int z);

private:
    // Private members can be added here if needed, such as properties for the object
    // e.g., position, rotation, scale, texture, etc.

    // Legacy position properties (kept for compatibility)
    float x = 0.0f, y = 0.0f, z = 0.0f;
    bool isElement = true;
    float dimensions = 3.0f;

    std::string _name;
    std::string _textString;
    std::string _entityName;
    std::string objectType;
    std::string objectID;

    // BodyPart* part = nullptr; // MOVED to elementFormation or removed

    // The primitive shape this Object represents. Default is Cube for compatibility and easy testing.
    ShapeKind _shapeKind = ShapeKind::Cube;

    // How an analytic shape (quadric, SDF) reaches the screen — Auto lets the
    // backend's own capability decide (today's behavior), Mesh forces the
    // tessellated fallback even on a backend that could raymarch it exactly.
    // See ObjectTypes::RenderMode for why this is Sense-Act substrate, not a
    // domain kind.
    RenderMode _renderMode = RenderMode::Auto;

    // Screen-space position for Shape2D / Text2D. In pixels, top-left origin.
    // Not part of the 3D transform: a 2D object has no world position.
    float _x2D = 100.0f;
    float _y2D = 100.0f;
    int   _zOrder2D = 0;  // draw order; higher is in front

    // Polyhedron data for arbitrary polyhedrons
    PolyhedronData polyhedronData;

    // Topology-based geometry model. When _hasComplex / _hasSmooth is set these
    // supersede _shapeKind for rendering, raycast and collision; _shapeKind
    // is retained for the polyhedron path and legacy save migration.
    geom::SmoothSurfaceData smoothData;
    geom::ComplexShapeData  complexData;
    geom::SdfNode           fieldData;     // SDF expression when _hasField
    glm::vec3               _fieldExtent{1.0f, 1.0f, 1.0f};
    mutable uint64_t        _memoIdBase = 0;
    mutable uint32_t        _fieldRevision = 0;
    // Cached render tessellations. Tessellating is O(slices*stacks) and allocates;
    // doing it in the draw path rebuilt every surface in the world every frame for
    // geometry that changes only when a Person edits it. Built once per change by
    // rebuildGeometryCaches(), which every mutation point already calls.
    mutable geom::TessMesh  _fieldMesh;    // cached field tessellation (built on demand)
    mutable bool            _fieldMeshDirty = true; // lazy evaluation flag
    std::shared_ptr<geom::TessMesh> _smoothMesh;   // cached smooth-surface tessellation
    // One mesh per patch, not one merged mesh: each patch is a real face and binds
    // its own face texture, so the render path must keep them separable.
    std::vector<geom::TessMesh> _complexMeshes;
    geom::BezierPatch       patchData;     // control-net surface when _hasPatch
    geom::TessMesh          _patchMesh;    // cached patch tessellation
    // Polyhedron face meshes, one per face so each binds its own face texture.
    // drawPolyhedron is const, so these are mutable and lazily rebuilt from
    // polyhedronData when _polyhedronDirty is set (at every mutation point).
    mutable std::vector<geom::TessMesh> _polyhedronFaceMeshes;
    mutable bool _polyhedronDirty = true;
    bool _hasSmooth  = false;
    bool _hasComplex = false;
    bool _hasField   = false;
    bool _hasPatch   = false;
    ShapeParams _shapeParams;

    // Which Material being paints this object, referenced BY IDENTIFIER (the same
    // by-name model Relation uses for its endpoints) — never an owning pointer, so
    // materials stay shared and the reference survives save/load. Resolved against
    // the global MaterialManager at draw time. Defaults to the always-present
    // material.default, so an object with no material assigned still renders.
    std::string _materialId = "material.default";

    // Cached local-space surface vertices for GJK support queries on the new
    // topology shapes (argmax dot(v,dir)). Rebuilt when the shape changes.
    mutable std::vector<glm::vec3> _supportCloud;   // mutable: rebuildFieldMesh() is const
    // Cached local-space AABB of the topology mesh, so updateCollisionZone only
    // transforms 8 corners instead of the whole (huge) support cloud per call.
    glm::vec3 _localMin{-0.5f};
    glm::vec3 _localMax{ 0.5f};
    mutable glm::mat4 _lastCollisionTransform = glm::mat4(0.0f);
    mutable uint32_t _lastCollisionFieldRevision = 0xffffffff;

    void drawSmoothModel() const;
    void drawComplexModel() const;
    void drawFieldModel() const;
    void drawPatchModel() const;
    FaceAlbedo faceAlbedo(size_t face) const; // per-face albedo (handle + CPU pixels)
    void rebuildPolyhedronMeshes() const;          // rebuild _polyhedronFaceMeshes
    void rebuildFieldMesh() const;                 // lazily rebuild _fieldMesh
    void rebuildGeometryCaches();

public:
    // LEGACY flat colours kept for save/load compatibility (first 6 faces)
    float faceColors[6][3] = {
        {1.f, 0.f, 0.f}, {1.f, 0.f, 0.f},
        {0.f, 1.f, 0.f}, {0.f, 1.f, 0.f},
        {0.f, 0.f, 1.f}, {0.f, 0.f, 1.f}
    };

    // Animation System code

    /** 
     * Dynamic Interaction-State memory. Keeps a record of interactions, actions, and states, 
     * This enables us to to recusrively build them like a fractal. 
     * Perhaps a fibbonacci, in which we would have a Formation or Object that resembles the Golden Ratio.
     * It should be so that we can dynamically use processes to create new processes, so it can propogate forward with new meaning.
     * We should be able to reconstruct every state of the object, from start to finish. 
     * Use simple data structures like vectors or lists to store these states. Loading, simulating, and creating new states should feel like plugging simple numbers into an equation to produce soemthing beautiful.
     * 
     * However in real world implementatino, we have to ensure that this powerful ability works at the agency and respects the dignity of our people and communities. 
     * In other words, privacy, property, autonomy, relational obligations, rights, etc..
     * Always records, but must record in a form undiscoverable to anyone except the owner(s) of the object, or those who have been granted permission to reload and propogate it. Zach himself must not have access to other's recording without permission.
     * If its not owned by anyone, either it is claimed through a process, or if it is unclaimable it can be partially loaded.
     * 
     * **/

protected:
    glm::mat4 transform = glm::mat4(1.0f);

// elsehwere faces must be separated into round faces and flat faces. 

public:


    mutable CollisionZone collisionZone;

    // -----------------------------------------------------------------
    // Per-face texture painting support
    // -----------------------------------------------------------------

    // One texture per logical face (vector size determined by geometry type)

    // Initialise or reinitialise textures after geometry type set/changed

    // Convenience: fill entire face with a colour (compatibility with old fill tool)

    // Paint a circular dab onto a face at UV (0-1) with given radius (0-1)

    // Advanced brush painting with pressure and dynamics

    // Paint stroke between two points with interpolation

    // Smudge tool - blend existing colors

    // Clone tool - copy from source to destination

    // Airbrush effect

    // Layer management

    // Undo/Redo

    // Older API remains but now delegates to fillFaceColor for backward compatibility

    void updateCollisionZone(const glm::mat4& transform) const;
    bool isPointInside(const glm::vec3& point) const;
    bool computePointPenetration(const glm::vec3& point, glm::vec3& outCorrection) const;

    // Exact convex-polyhedron overlap test via the Separating Axis Theorem.
    // Operates directly on the world-space polyhedron geometry (1:1 with the
    // rendered shape) rather than an AABB approximation. Polyhedra only for now;
    // returns false if either object lacks valid polyhedron data.
    // TODO: concave shapes need convex decomposition; round shapes handled later.
    bool isTouching(const Object& other) const;
    glm::vec3 getSupportPointWorld(const glm::vec3& worldDirection) const;
    bool isCollisionShapeConvex() const;

    void drawCube() const;
    void drawPolyhedron() const;

    void drawObject() const;
    void drawHighlightOutline() const;

    // Screen-space 2D rendering (Shape2D / Text2D). Called after the 3D pass
    // inside a begin2D / end2D bracket. draw2DObject() uses the same faceColors[0]
    // convention as 3D so a single "color" law paints both.
    bool  is2D() const { return _shapeKind == ShapeKind::Shape2D || _shapeKind == ShapeKind::Text2D; }
    float getX2D() const { return _x2D; }
    float getY2D() const { return _y2D; }
    int   getZOrder2D() const { return _zOrder2D; }
    void  setX2D(const float& v) { _x2D = v; }
    void  setY2D(const float& v) { _y2D = v; }
    void  setZOrder2D(const int& v) { _zOrder2D = v; }
    // The screen-space AABB for picking. Returns {x0, y0, x1, y1}.
    glm::vec4 getRect2D() const {
        return glm::vec4(_x2D, _y2D,
                         _x2D + _shapeParams.width2D,
                         _y2D + _shapeParams.height2D);
    }
    void draw2DObject(uint32_t screenW, uint32_t screenH) const;

    // renderMode is Law-writable as an int (append-only, like ShapeKind — see
    // ObjectTypes::RenderMode). An out-of-range write clamps to Auto rather
    // than storing garbage a switch elsewhere would silently fall through on.
    RenderMode renderMode() const { return _renderMode; }
    void setRenderMode(RenderMode m) { _renderMode = m; }
    int  getRenderModeProp() const { return static_cast<int>(_renderMode); }
    void setRenderModeProp(const int& v) {
        _renderMode = (v >= static_cast<int>(RenderMode::Auto) &&
                       v <= static_cast<int>(RenderMode::Mesh))
                    ? static_cast<RenderMode>(v) : RenderMode::Auto;
    }

    // not implemented yet
    void interactWith(Formation&);
    void onInteraction(Formation&);

    void drawSymbolicForm(); // or drawAsGeometry, drawPhysicalShell

    Object();
    explicit Object(std::string explicitId);
    Object(Object&&) = default;
    Object& operator=(Object&&) = default;
    Object(const Object&) = delete;
    Object& operator=(const Object&) = delete;

    virtual void setTransform(const glm::mat4& t);
    virtual glm::mat4 getTransform() const { return transform; }
    virtual glm::mat4 getRaycastTransform() const { return transform; }

    // Property bridge: the anchor's world translation (column 3 of transform).
    // setPosition routes through setTransform so rotation-state sync and the
    // collision-zone update run — a law moving an object really moves it.
    glm::vec3 getPosition() const { return glm::vec3(transform[3]); }
    void setPosition(const glm::vec3& p);

    const glm::vec3& getCenter() const { return center; }
    void setCenter(const glm::vec3& c) { center = c; }
    glm::vec3 getWorldCenter() const { return glm::vec3(getTransform() * glm::vec4(center, 1.0f)); }

    const glm::vec3& getAuthoritativeAxis() const { return authoritativeAxis; }
    void setAuthoritativeAxis(const glm::vec3& axis);

    glm::vec3 getRotationEulerDegrees() const { return rotationEulerDegrees; }
    glm::vec3 getTargetRotationEulerDegrees() const { return targetRotationEulerDegrees; }
    void setRotationEulerDegrees(const glm::vec3& degrees);
    void setTargetRotationEulerDegrees(const glm::vec3& degrees);
    void addTargetRotationDegrees(const glm::vec3& deltaDegrees);

    float getRotationResponsiveness() const { return rotationResponsiveness; }
    void setRotationResponsiveness(float responsiveness);

    bool hasPendingRotation() const;
    bool updateRotation(float dt);
    bool advanceRotation(const glm::mat4& sourceTransform, float dt, glm::mat4& outTransform);
    void syncRotationStateFromTransform(const glm::mat4& sourceTransform, bool syncTarget = true);

    // -----------------------------------------------------------------
    // Automation (time-driven motion). See Automation/Automation.hpp.
    // -----------------------------------------------------------------
    // Add a clip; the current transform is captured as the rest pose the first
    // time a clip is added (so offsets layer on top of where the object sits).
    void addAutomation(const Automation::Clip& clip);
    void clearAutomations();
    bool hasAutomations() const { return !_automation.clips.empty(); }
    Automation::State& automationState() { return _automation; }
    // Explicitly set the rest pose that animated channels build on.
    void setAutomationRest(const glm::mat4& rest);

    // World-object path: advance clips by dt and apply to this object's
    // transform in one call (use once per frame). Returns true if applied.
    bool updateAutomations(float dt);

    // Decoupled path for hierarchical targets (e.g. form parts): advance the
    // clock once per frame, then sample as many times as needed against a base.
    void advanceAutomations(float dt);
    glm::mat4 sampleAutomations(const glm::mat4& base) const;

    // Generalized ray-face intersection for painting across all geometry types.
    // Returns true if hit, along with distance t in world units, the face index, and UV in [0,1].
    bool raycastFace(const glm::vec3& rayOriginWorld, const glm::vec3& rayDirWorld,
                     float& outT, int& outFaceIndex, glm::vec2& outUV) const;

    // Hover detection methods
    bool isMouseHovering(const glm::vec2& mousePos, const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix, int windowWidth, int windowHeight) const;
    bool isMouseHovering(const glm::vec3& worldMousePos) const;
    void updateHoverState(bool isHovering);
    // The form the pointer channel calls: the hit point and screen position
    // the hover events advertise were hard-coded to (0,0,0) and (0,0) because
    // no caller had one to give. InteractionChannel does.
    void updateHoverState(bool isHovering, const glm::vec3& hoverPoint,
                          const glm::vec2& screenPosition);
    bool getIsHovered() const { return _isHovered; }
    glm::vec3 getHoverPoint() const { return _hoverPoint; }

    // Setter / getter so tools can pick the shape.
    // Reclassifies legacy primitives into the topology-based model:
    //   Sphere   → SmoothSurface (quadric)
    //   Cylinder → ComplexShape (round side + 2 flat caps, Hard rims)
    //   Cone     → ComplexShape (round side + flat base, Hard rim)
    //   Cube/Polyhedron keep the flat-faced path. This is also the legacy-save
    //   migration point (from_json calls setShapeKind).
    // Named kinds rebuild topology. setShapeKind used to switch only Cube /
    // Polyhedron / Sphere / Cylinder / Cone — Ellipsoid and the rest cleared
    // _hasSmooth and then drew the 16-slice glu tessellation fallback. One
    // door: setShape. Cube/Polyhedron stay flat-faced; they do not recurse.
    void setShapeKind(ShapeKind t) { setShape(t, ShapeParams{}); }

    // Build any named shape in the framework (superset of setShapeKind).
    void setShape(ShapeKind k) { setShape(k, ShapeParams{}); }
    void setShape(ShapeKind k, const ShapeParams& p) {
        _hasSmooth = false;
        _hasComplex = false;
        _hasField = false;
        _hasPatch = false;
        switch (k) {
            case ShapeKind::Cube:
            case ShapeKind::Polyhedron:
                break;
            case ShapeKind::Sphere:     setSmoothSurface(geom::makeSphere(p.r));   break;
            case ShapeKind::Cylinder:   setComplexShape(geom::cappedCylinder(p.r, p.halfH)); break;
            case ShapeKind::Cone:       setComplexShape(geom::cappedCone(p.r, p.halfH));      break;
            case ShapeKind::Ellipsoid:  setSmoothSurface(geom::makeEllipsoid(p.r, p.ry, p.rz)); break;
            case ShapeKind::Ovoid:      setSmoothSurface(geom::makeOvoid(p.r, p.ovoidAsym));    break;
            case ShapeKind::Paraboloid: setSmoothSurface(geom::makeParaboloid(p.paraboloidA));  break;
            case ShapeKind::Torus:      setSmoothSurface(geom::makeTorus(p.majorR, p.minorR));  break;
            case ShapeKind::RoundedBox: setComplexShape(geom::roundedBox(0.5f, p.fillet));      break;
            case ShapeKind::Shape2D:
            case ShapeKind::Text2D:
                break;
            case ShapeKind::Field:
            case ShapeKind::Patch:
                break;
        }
        _shapeKind = k;
        _shapeParams = p;
    }
    ShapeKind getShapeKind() const { return _shapeKind; }
    const ShapeParams& getShapeParams() const { return _shapeParams; }
    // Raw assignment WITHOUT geometry regeneration — for kinds whose visible
    // form does not come from the params (cube/polyhedron/field/patch).
    void assignShapeParams(const ShapeParams& p) { _shapeParams = p; }

    // --- Topology-based geometry model (smooth surfaces / complex shapes) ---
    // The fundamental category of the object. Named primitives are merely
    // parameterizations inside a category, never the identity itself.
    SpatialKind getSpatialKind() const {
        if (_hasPatch)   return SpatialKind::Patch;
        if (_hasField)   return SpatialKind::Field;
        if (_hasComplex) return SpatialKind::ComplexShape;
        if (_hasSmooth)  return SpatialKind::SmoothSurface;
        return SpatialKind::Polyhedron; // legacy cube/poly map here for now
    }

    // --- Control-net (Bezier patch) editing -------------------------------
    bool hasPatch() const { return _hasPatch; }
    bool isPatch() const { return hasPatch(); }
    const geom::BezierPatch& getPatchData() const { return patchData; }
    void setBezierPatch(const geom::BezierPatch& p) {
        patchData = p; _hasPatch = true;
        _hasField = _hasSmooth = _hasComplex = false;
        _shapeKind = ShapeKind::Patch;
        rebuildGeometryCaches();
    }
    int getPatchControlCount() const { return _hasPatch ? static_cast<int>(patchData.ctrl.size()) : 0; }
    glm::vec3 getPatchControlLocal(int i) const {
        return (_hasPatch && i >= 0 && i < static_cast<int>(patchData.ctrl.size()))
                   ? patchData.ctrl[i] : glm::vec3(0.0f);
    }
    void setPatchControlLocal(int i, const glm::vec3& v) {
        if (_hasPatch && i >= 0 && i < static_cast<int>(patchData.ctrl.size())) {
            patchData.ctrl[i] = v; rebuildGeometryCaches();
        }
    }
    int getPatchDegreeU() const { return _hasPatch ? patchData.du : 0; }
    int getPatchDegreeV() const { return _hasPatch ? patchData.dv : 0; }
    void elevatePatchU() { if (_hasPatch) { geom::elevateU(patchData); rebuildGeometryCaches(); } }
    void elevatePatchV() { if (_hasPatch) { geom::elevateV(patchData); rebuildGeometryCaches(); } }
    bool hasSmoothSurface() const { return _hasSmooth; }
    bool hasComplexShape()  const { return _hasComplex; }
    bool hasField()         const { return _hasField; }

    // Material reference (by identifier — resolved against the global
    // MaterialManager at draw time). Also registered as the Law-addressable
    // property "material", so a Law can reassign an object's material by name.
    const std::string& materialId() const { return _materialId; }
    void setMaterialId(std::string id) { _materialId = std::move(id); }

    // -----------------------------------------------------------------
    // Paint. The per-face textures live on the Material being (they moved
    // there with the rest of the paint system), and a Material is SHARED by
    // identifier — every object naming "material.default" names the same
    // being. So painting is copy-on-write: the first stroke gives this object
    // its own Material, named after it, carrying over the appearance of the
    // one it was sharing. Objects that are never painted keep sharing, which
    // is the point of materials being beings in the first place.
    //
    // The own material's name is "material.<this object's identifier>", so an
    // object with a volatile id gets a material with a volatile name — the
    // same caveat the object already carries and already warns about.
    // -----------------------------------------------------------------
    std::shared_ptr<Material> ownMaterial();
    // Resize this object's own material's face textures to the face count its
    // current geometry actually has. Idempotent.
    void initFaceTextures();
    // Fill one face with a colour, through the object's own material, and
    // record it in the object's own faceColors slot so the "color" property
    // reads back what was painted.
    void setFaceColor(int faceIndex, float r, float g, float b);
    const geom::SmoothSurfaceData& getSmoothData()  const { return smoothData; }
    const geom::ComplexShapeData&  getComplexData() const { return complexData; }
    const geom::SdfNode&           getFieldData()   const { return fieldData; }
    // Forces the lazy field tessellation. Asking for the support cloud IS the
    // demand that builds it -- handing back an empty vector instead would be the
    // silent-wrong-answer this cache exists to prevent (an empty cloud makes an
    // object unpickable and non-collidable, which is the exact bug the Bezier
    // assertion in geometry_cache_test locks down).
    const std::vector<glm::vec3>& getSupportCloud() const {
        if (_hasField) rebuildFieldMesh();
        return _supportCloud;
    }

    // Smooth-surface tessellation cache garbage collection (evicts entries with use_count() <= 1)
    static size_t gcSmoothTessellationCache();
    static size_t smoothTessellationCacheSize();
    static void clearSmoothTessellationCache();
    void setSmoothSurface(const geom::SmoothSurfaceData& s) {
        smoothData = s; _hasSmooth = true; _hasComplex = false; _hasField = false; _hasPatch = false; rebuildGeometryCaches();
    }
    void setComplexShape(const geom::ComplexShapeData& c) {
        complexData = c; _hasComplex = true; _hasSmooth = false; _hasField = false; _hasPatch = false; rebuildGeometryCaches();
    }
    // An SDF-defined shape (morph / boolean / implicit). `extent` is the half-size
    // of the region the field is meshed/marched over.
    void setFieldShape(const geom::SdfNode& f, const glm::vec3& extent = glm::vec3(1.0f)) {
        fieldData = f; _fieldExtent = extent;
        _hasField = true; _hasSmooth = false; _hasComplex = false; _hasPatch = false;
        _shapeKind = ShapeKind::Field;
        rebuildGeometryCaches();
    }
    const glm::vec3& getFieldExtent() const { return _fieldExtent; }
    // Live-edit the blend of a Morph/SmoothUnion field (re-tessellates).
    bool isMorphField() const {
        return _hasField && (fieldData.op == geom::SdfOp::Morph || fieldData.op == geom::SdfOp::SmoothUnion);
    }
    float getMorphParam() const { return fieldData.t; }
    void setMorphParam(float t) {
        if (!isMorphField()) return;
        fieldData.t = glm::clamp(t, 0.0f, 1.0f);
        rebuildGeometryCaches(); // re-tessellates the field and rebuilds the support cloud
    }

    // A binary field (blend/boolean) — operand B can be moved in the scene.
    bool isBinaryField() const { return _hasField && fieldData.children.size() == 2 && fieldData.children[1]; }
    glm::vec3 getFieldOperandBOffset() const {
        return isBinaryField() ? fieldData.children[1]->offset : glm::vec3(0.0f);
    }
    void setFieldOperandBOffset(const glm::vec3& off) {
        if (!isBinaryField()) return;
        fieldData.children[1]->offset = off;
        rebuildGeometryCaches();
    }
    void clearTopologyModel() { _hasSmooth = false; _hasComplex = false; _hasField = false; _hasPatch = false; _supportCloud.clear(); }

    // Polyhedron-specific methods
    void setPolyhedronData(const PolyhedronData& data);
    const PolyhedronData& getPolyhedronData() const { return polyhedronData; }

    // --- Direct topology editing (the Morph tool) -------------------------
    int getPolyhedronVertexCount() const { return static_cast<int>(polyhedronData.vertices.size()); }
    glm::vec3 getPolyhedronVertexLocal(int i) const {
        return (i >= 0 && i < static_cast<int>(polyhedronData.vertices.size()))
                   ? polyhedronData.vertices[i] : glm::vec3(0.0f);
    }
    // Move one vertex in local space and refresh derived data (normals, bounds).
    void setPolyhedronVertexLocal(int i, const glm::vec3& v) {
        if (i < 0 || i >= static_cast<int>(polyhedronData.vertices.size())) return;
        polyhedronData.vertices[i] = v;
        polyhedronData.computeNormals();
        polyhedronData.computeFaceAreas();
        updateCollisionZone(getTransform());
    }
    
    // Create common polyhedrons
    void createTetrahedron();
    void createOctahedron();
    void createDodecahedron();
    void createIcosahedron();
    void createCustomPolyhedron(const std::vector<glm::vec3>& vertices, 
                               const std::vector<std::vector<int>>& faces);

    virtual ~Object() = default;

    // Owning form part (non-null when this Object is a sub-object of a BodyPart)
    // void setOwnerBodyPart(BodyPart* owner) { part = owner; }
    // BodyPart* getOwnerBodyPart() const { return part; }
    // Singular interface implementation
    std::string getIdentifier() const override { return objectID; }

    // --------------------------------------------------------------
    // Attributes and Tags for selection/filtering (for physics laws, etc.)
    // --------------------------------------------------------------
    void setAttribute(const std::string& key, const std::string& value);
    bool hasAttribute(const std::string& key) const;
    const std::string& getAttribute(const std::string& key) const; // empty string if missing
    const std::unordered_map<std::string, std::string>& getAttributes() const { return _composition.attributes; }
    void addTag(const std::string& tag);
    void removeTag(const std::string& tag);
    bool hasTag(const std::string& tag) const;
    const std::vector<std::string>& getTags() const { return _composition.tags; }
    
    // Legacy property accessors (composition-related)
    // Methods with out-of-line implementations in Object.cpp
    int getCorners() const;
    void setCorners(int c);
    int getFaces() const;
    void setFaces(int f);
    int getMassQuantity() const;
    void setMassQuantity(int m);
    int getElements() const;
    void setElements(int e);
    
    // Methods with inline implementations (no cpp definitions)

    uint64_t getMemoId(int suffix = 0) const {
        if (_memoIdBase == 0) {
            static std::atomic<uint64_t> counter{1000};
            _memoIdBase = counter.fetch_add(100);
        }
        return _memoIdBase + suffix;
    }
    uint32_t getFieldRevision() const { return _fieldRevision; }
    int getRelationships() const { return _composition.relationships; }
    void setRelationships(int r) { _composition.relationships = r; }
    int getComplexityLevel() const { return _composition.complexityLevel; }
    void setComplexityLevel(int cl) { _composition.complexityLevel = cl; }
    int getPhysicalObject() const { return _composition.physicalObject ? 1 : 0; }
    void setPhysicalObject(int po) { _composition.physicalObject = (po != 0); }
    int getSymbolicObject() const { return _composition.physicalObject ? 0 : 1; }
    void setSymbolicObject(int so) { _composition.physicalObject = (so == 0); }
    
    // ID management: ObjectIdentity::claimIdentifierAtLeast is the one entry
    // point (setObjectID already calls it). The member function that used to be
    // declared here forwarded to it, had no callers, and shadowed the namespace
    // name at every call site that said `claimIdentifierAtLeast` unqualified.
    void setName(const std::string& name) { _name = name; }
    
    std::string getTextString() const { return _textString; }
    void setTextString(const std::string& text) { _textString = text; }

    const std::string& getEntityName() const { return _entityName; }

private:
    // Registers the first-mover properties (position/rotation/center/shape.*)
    // that make this Object legible to PropertyPath and the Law system.
    // Defined in Object.cpp.
    void buildProperties() override;
    // Property bridges: more of the object's being made legible, so laws
    // can govern it and set-to-set creation can carry it.
    bool propPhysical() const { return _composition.physicalObject; }
    void propSetPhysical(const bool& v) { _composition.physicalObject = v; }
    glm::vec3 propColor() const {
        return glm::vec3(faceColors[0][0], faceColors[0][1], faceColors[0][2]);
    }
    void propSetColor(const glm::vec3& c);

    glm::vec3 getLocalSupportPoint(const glm::vec3& localDirection) const;
    bool computeLocalPointPenetration(const glm::vec3& localPoint,
                                      glm::vec3& outSurfacePoint,
                                      glm::vec3& outLocalNormal) const;
    glm::mat4 composeTransformWithRotation(const glm::mat4& sourceTransform,
                                           const glm::vec3& rotationDegrees) const;

    // Composition state (element membership, attributes, tags, legacy properties)
    ObjectCompositionState _composition;

    // Hover state tracking
    mutable bool _isHovered = false;
    mutable glm::vec3 _hoverPoint{0.0f, 0.0f, 0.0f};
    mutable bool _wasHoveredLastFrame = false;

    glm::vec3 center{0.0f, 0.0f, 0.0f};
    glm::vec3 authoritativeAxis{0.0f, 1.0f, 0.0f};
    glm::vec3 rotationEulerDegrees{0.0f, 0.0f, 0.0f};
    glm::vec3 targetRotationEulerDegrees{0.0f, 0.0f, 0.0f};
    float rotationResponsiveness = 10.0f;
    bool preserveRotationTargetOnTransformSet = false;

    Automation::State _automation;
    
    // Delta save tracking
    bool _isDirty = true;
public:
    bool getIsDirty() const { return _isDirty; }
    void clearDirty() { _isDirty = false; }
    void markDirty() { _isDirty = true; }
};

// Out-of-line definition: by this point Object is a complete type, so
// `object.collisionZone` is a legal member access.
inline bool CollisionZone::isTouching(const Object& object) const {
    return isTouching(object.collisionZone);
}
