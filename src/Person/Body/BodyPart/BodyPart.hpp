#pragma once
#include <string>
#include <vector>
#include <memory>
#include <ctime>
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Relation/Formation/Formation.hpp"
#include "Relation/Relation.hpp"

#include "ConstructedBeing/Singular/Object/Object/ObjectTypes.hpp"
#include <glm/glm.hpp>

class BodyPart : public Formation {

    public:

    enum class Type {
        Undefined,
        Head,
        Neck,
        Torso,
        Shoulder,
        Arm,
        ForeArm,
        Hand,
        Finger,
        Leg,
        ForeLeg,
        Foot
    };



    BodyPart(const std::string& name = "", Type type = Type::Undefined, 
             ObjectTypes::ShapeKind geometryType = ObjectTypes::ShapeKind::Cube,
             const glm::vec3& dimensions = glm::vec3(1.0f, 1.0f, 1.0f));
    BodyPart(const std::string& name, Type type, 
             ObjectTypes::ShapeKind geometryType, const glm::vec3& dimensions,
             const glm::mat4& initialTransform);

    void draw() const;

    // Accessors
    const std::string& getName() const { return partName; }
    Type getType() const { return partType; }
    std::string getIdentifier() const override { return partName; }
    glm::mat4 getTransform() const { return _transform; }
    // transform ∘ scale(dimensions): the geometry the primary manifestation
    // is DRAWN and COLLIDED with. Before 2026-10-13 this was named
    // getRaycastTransform — a leftover override from when BodyPart inherited
    // Object, which has overridden nothing since. Object face-raycasting still
    // uses the Object's own unscaled transform: an open picking seam, see
    // BodyPart_Constitution_As_Explicit_Relation.md.
    glm::mat4 scaledTransform() const;

    // The primary manifestation. Machine custody (lifetime) stays in the
    // unique_ptr below; the ONTOLOGY of why this Object belongs to this part
    // is the `constituted-by` Relation in relations(). Geometry, paint,
    // collision and Automation clips are the Object's own affordances — call
    // them on the Object. BodyPart does not re-sell them (o3 seam audit,
    // 2026-10-13: the forwarding "gift shop" was removed).
    Object* getPrimaryObject() { return _primaryObject.get(); }
    const Object* getPrimaryObject() const { return _primaryObject.get(); }

    // Zach's literal/symbolic distinction (2025-08-11): a literal body part is
    // an actual limb or organ. Registered as `isLiteral` / `isSymbolic`.
    bool isLiteral, isSymbolic = true;

    // Override to ensure collision box follows visual scale
    void setTransform(const glm::mat4& t);
    void setLocalTransform(const glm::mat4& t) { _localTransform = t; setTransform(t); }

    // The part's tint. Paint lives on the Material: setColor paints the
    // primary Object through Object::setFaceColor (diverging it onto its own
    // material); `color` is also the seed paint for newly added sub-Objects.
    void setColor(float r, float g, float b);
    const float* getColor() const {return color;}

    // Dimensions accessor
    const glm::vec3& getDimensions() const { return _dimensions; }
    void setDimensions(const glm::vec3& dims);

    const glm::mat4& localTransform() const {return _localTransform;}

    // Changes the primary Object's shape AND repaints it with this part's
    // color — a shape change re-sizes the face set, so it is not a forwarder.
    void setPrimaryShape(ObjectTypes::ShapeKind gt);

    // -----------------------------------------------------------------
    // Composite sub-objects — a body part is a Formation of 1+ Objects
    // Sub-objects store world transforms on the Object for raycasting/
    // collision; the *local* offsets (relative to this body part) are
    // kept in a parallel vector so they survive transform propagation.
    // Each addition forms a `constituted-by` Relation (and publishes
    // relation-formed, so Laws hear it); removal dissolves it.
    // -----------------------------------------------------------------
    Object* addSubObject(ObjectTypes::ShapeKind kind,
                         const glm::mat4& localOffset = glm::mat4(1.0f));
    Object* addSubObject(std::unique_ptr<Object> obj,
                         const glm::mat4& localOffset = glm::mat4(1.0f));
    void removeSubObject(size_t index);
    size_t getSubObjectCount() const { return _subObjects.size(); }
    Object* getSubObject(size_t index);
    const Object* getSubObject(size_t index) const;
    const std::vector<std::unique_ptr<Object>>& getSubObjects() const { return _subObjects; }
    std::vector<Object*> getAllObjects();

    // Read / write the local offset for a sub-object (body-part-relative)
    const glm::mat4& getSubObjectLocalOffset(size_t index) const;
    void setSubObjectLocalOffset(size_t index, const glm::mat4& localOffset);

    // ---- Constitution (BodyPart_Constitution_As_Explicit_Relation) -----
    // Zach's decision, 2026-10-13: BodyPart <-constituted-by-> Object(s).
    // Constitution does not imply identity, and shared manifestation does
    // not imply shared being. Directed edges, a = this part, b = the Object.
    static constexpr const char* kConstitutedByType = "constituted-by";
    // The constitution edge naming this Object, or null if it does not
    // constitute this part.
    Relation* constitutionOf(const Object* obj) const;

    void buildProperties() override;

    private:
    std::unique_ptr<Object> _primaryObject;
    glm::mat4 _transform = glm::mat4(1.0f);

    std::string partName;
    Type        partType {Type::Undefined};
    glm::vec3   _dimensions = glm::vec3(1.0f, 1.0f, 1.0f);
    float color[3] = {1.0f,1.0f,1.0f};
    glm::mat4 _localTransform = glm::mat4(1.0f);

    // Owned sub-objects forming the composite shape of this body part
    std::vector<std::unique_ptr<Object>> _subObjects;
    // Parallel vector: each sub-object's transform relative to this body part
    std::vector<glm::mat4> _subObjectLocalOffsets;

    void constitute(Object* obj, bool newInteraction);
    void dissolveConstitution(Object* obj);

    // Property bridge (see buildProperties).
    std::string propPartName() const { return partName; }
    int propPartType() const { return static_cast<int>(partType); }
    glm::vec3 propDimensions() const { return _dimensions; }
    glm::vec3 propColor() const { return glm::vec3(color[0], color[1], color[2]); }
    void propSetColor(const glm::vec3& c) { setColor(c.x, c.y, c.z); }
    glm::mat4 propLocalTransform() const { return _localTransform; }
    std::string propPrimaryObject() const;
    std::shared_ptr<PropertyList> propConstitutedBy() const;
    std::shared_ptr<PropertyList> propSubObjectLocalOffsets() const;
};