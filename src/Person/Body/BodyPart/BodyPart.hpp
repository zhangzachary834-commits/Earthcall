#pragma once
#include <string>
#include <vector>
#include <memory>
#include <ctime>
#include "Form/Object/Object.hpp"
#include "Form/Object/Formation/Formation.hpp"
#include "Form/Form.hpp"
#include <glm/glm.hpp>

class BodyPart : public Object, public Formation {

    public:

    enum class Type {
        Undefined,
        Head,
        Torso,
        Arm,
        Leg,
        Hand,
        Foot
    };



    BodyPart(const std::string& name = "", Type type = Type::Undefined, const Form& form = Form());
    BodyPart(const std::string& name, Type type, const Form& form, const glm::mat4& initialTransform);

    void draw() const;
    void update(float deltaTime);

    // Accessors
    const std::string& getName() const { return partName; }
    Type getType() const { return partType; }
    std::string getIdentifier() const override { return partName; }
    glm::mat4 getRaycastTransform() const override;

    // A literal body part is an actual limb or organ. Have a constant updater that always sets these variables to the opposite of each other.
    bool isLiteral, isSymbolic = true;

    // Override to ensure collision box follows visual scale
    void setTransform(const glm::mat4& t) override;
    void setLocalTransform(const glm::mat4& t) { _localTransform = t; setTransform(t); }

    // Color accessor
    void setColor(float r,float g,float b){color[0]=r;color[1]=g;color[2]=b;}
    const float* getColor() const {return color;}

    // Geometry accessor for editor
    Form&       getGeometry()       { return geometry; }
    const Form& getGeometry() const { return geometry; }

    const glm::mat4& localTransform() const {return _localTransform;}

    // -----------------------------------------------------------------
    // Shape management — change the primary shape of this body part
    // -----------------------------------------------------------------
    void setPrimaryShape(Object::GeometryType gt);
    Object::GeometryType getPrimaryShape() const { return getGeometryType(); }

    // True if any face texture has been painted (i.e. not a flat fill).
    bool hasCustomTextures() const;

    // -----------------------------------------------------------------
    // Composite sub-objects — a body part is a Formation of 1+ Objects
    // Sub-objects store world transforms on the Object for raycasting/
    // collision; the *local* offsets (relative to this body part) are
    // kept in a parallel vector so they survive transform propagation.
    // -----------------------------------------------------------------
    Object* addSubObject(Object::GeometryType gt = Object::GeometryType::Cube,
                         const glm::mat4& localOffset = glm::mat4(1.0f));
    Object* addSubObject(Object::ShapeKind kind,
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



    private:

    std::string partName;
    Type        partType {Type::Undefined};
    Form        geometry;
    float color[3] = {1.0f,1.0f,1.0f};
    glm::mat4 _localTransform = glm::mat4(1.0f);

    // Owned sub-objects forming the composite shape of this body part
    std::vector<std::unique_ptr<Object>> _subObjects;
    // Parallel vector: each sub-object's transform relative to this body part
    std::vector<glm::mat4> _subObjectLocalOffsets;
    
    void updateColor();
};