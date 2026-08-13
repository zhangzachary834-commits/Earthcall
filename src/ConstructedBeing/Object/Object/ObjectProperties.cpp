// ============================================================================
// ObjectProperties.cpp - Property system implementations for Object
//
// This file contains the property registry setup and property bridge classes
// that make Object properties addressable through the PropertyPath system.
// ============================================================================

#include "ConstructedBeing/Object/Object.hpp"
#include "ConstructedBeing/Object/Object/ObjectComposition.hpp"
#include "ConstructedBeing/Material/MaterialManager.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"

#include <algorithm>

// The global Material beings (globals.cpp). An object names its material by
// identifier; the face bridges below resolve it here at access time.
extern MaterialManager materials;

namespace {

// Shape parameters that REGENERATE geometry when written: a law that sets
// shape.r on a sphere reshapes the sphere, not just a number. Kinds whose
// visible form does not come from the params (cube/polyhedron: vertex data;
// field/patch: sculpted payloads) take the raw write — regenerating would
// wipe paint or sculpt for nothing.
bool paramsShapeGeometry(const Object& o) {
    switch (o.getShapeKind()) {
        case Object::ShapeKind::Sphere:
        case Object::ShapeKind::Cylinder:
        case Object::ShapeKind::Cone:
        case Object::ShapeKind::Ellipsoid:
        case Object::ShapeKind::Ovoid:
        case Object::ShapeKind::Paraboloid:
        case Object::ShapeKind::Torus:
        case Object::ShapeKind::RoundedBox:
            break;
        default:
            return false;
    }
    const auto spatial = o.getSpatialKind();
    return spatial != Object::SpatialKind::Field &&
           spatial != Object::SpatialKind::Patch;
}

// Shape parameter bridge: connects shape parameters to the property system
class ShapeParamBridge : public Property {
public:
    ShapeParamBridge(std::string name, Object* owner,
                     float Object::ShapeParams::*member)
        : _name(std::move(name)), _owner(owner), _member(member) {}

    std::string name() const override { return _name; }
    std::string typeName() const override { return "float"; }

    PropertyValue value() const override {
        return PropertyValue(_owner->getShapeParams().*_member);
    }
    bool setValue(const PropertyValue& v) override {
        double n = 0.0;
        if (!propertyValueToNumber(v, n)) return false;
        Object::ShapeParams params = _owner->getShapeParams();
        params.*_member = static_cast<float>(n);
        if (paramsShapeGeometry(*_owner)) {
            _owner->setShape(_owner->getShapeKind(), params);   // regenerate
        } else {
            _owner->assignShapeParams(params);
        }
        return true;
    }

private:
    std::string _name;
    Object* _owner;
    float Object::ShapeParams::*_member;
};

// Shape kind bridge: allows changing the object's shape type
class ShapeKindBridge : public Property {
public:
    explicit ShapeKindBridge(Object* owner) : _owner(owner) {}
    std::string name() const override { return "shape.kind"; }
    std::string typeName() const override { return "int"; }
    PropertyValue value() const override {
        return PropertyValue(static_cast<int>(_owner->getShapeKind()));
    }
    bool setValue(const PropertyValue& v) override {
        double n = 0.0;
        if (!propertyValueToNumber(v, n)) return false;
        const int kind = static_cast<int>(n);
        if (kind < 0 || kind > static_cast<int>(Object::ShapeKind::RoundedBox)) {
            return false;
        }
        _owner->setShape(static_cast<Object::ShapeKind>(kind),
                         _owner->getShapeParams());
        return true;
    }

private:
    Object* _owner;
};

// Motion state bridge: connects physics rigid form to property system
class RigidFormBridge : public Property {
public:
    enum class Field { Velocity, Mass };
    RigidFormBridge(std::string name, Object* owner, Field field)
        : _name(std::move(name)), _owner(owner), _field(field) {}

    std::string name() const override { return _name; }
    std::string typeName() const override {
        return _field == Field::Velocity ? "vec3" : "float";
    }
    PropertyValue value() const override {
        Physics::RigidForm& form = Physics::getFormFor(_owner);
        return _field == Field::Velocity ? PropertyValue(form.velocity)
                                         : PropertyValue(form.mass);
    }
    bool setValue(const PropertyValue& v) override {
        Physics::RigidForm& form = Physics::getFormFor(_owner);
        if (_field == Field::Velocity) {
            const auto* vec = std::get_if<glm::vec3>(&v);
            if (!vec) return false;
            form.velocity = *vec;
            return true;
        }
        double n = 0.0;
        if (!propertyValueToNumber(v, n) || n <= 0.0) return false;   // massless
        form.mass = static_cast<float>(n);                            // is a lie
        return true;
    }

private:
    std::string _name;
    Object* _owner;
    Field _field;
};

// Face property bridge: makes an object's painted surface legible, face by
// face (colour, layer structure, opacity, blend mode, texture size), so a law
// can fade one face's layer or recolour one side. Pixels and stroke history
// stay source-code-only: set-to-set carries surface STRUCTURE, not a bitmap.
//
// The paint itself lives on the Material being an object references by name
// (Material::faceTextures), not on the Object, so every field but Color is
// resolved through the MaterialManager at read/write time rather than held.
// Color stays on the Object's own faceColors slots: it is this object's
// chosen colour, and a Material is shared by identifier — writing it through
// the material would repaint every other object naming the same one.
class FacePropertyBridge : public Property {
public:
    enum class Field { Color, LayerCount, ActiveLayer, UseLayers, LayerOpacity, BlendMode, TextureSize };

    FacePropertyBridge(std::string name, Object* owner, int face, Field field)
        : _name(std::move(name)), _owner(owner), _face(face), _field(field) {}

    std::string name() const override { return _name; }
    std::string typeName() const override {
        switch (_field) {
            case Field::Color: return "vec3";
            case Field::UseLayers: return "bool";
            case Field::LayerOpacity: return "float";
            default: return "int";
        }
    }

    PropertyValue value() const override {
        const FaceTexture* tex = texture();
        switch (_field) {
            case Field::Color:
                if (_face < 6) {
                    return PropertyValue(glm::vec3(_owner->faceColors[_face][0],
                                                   _owner->faceColors[_face][1],
                                                   _owner->faceColors[_face][2]));
                }
                return PropertyValue(glm::vec3(1.0f));
            case Field::LayerCount:
                return PropertyValue(tex ? static_cast<int>(tex->layers.size()) : 0);
            case Field::ActiveLayer:
                return PropertyValue(tex ? tex->activeLayer : 0);
            case Field::UseLayers:
                return PropertyValue(tex ? tex->useLayers : false);
            case Field::LayerOpacity: {
                if (!tex || tex->layers.empty()) return PropertyValue(1.0f);
                const int layer = clampLayer(*tex);
                return PropertyValue(tex->layerOpacities[layer]);
            }
            case Field::BlendMode: {
                if (!tex || tex->layers.empty()) return PropertyValue(0);
                const int layer = clampLayer(*tex);
                return PropertyValue(tex->blendModes[layer]);
            }
            case Field::TextureSize:
                return PropertyValue(tex ? tex->size : 0);
        }
        return PropertyValue{};
    }

    bool setValue(const PropertyValue& v) override {
        FaceTexture* tex = texture();
        switch (_field) {
            case Field::Color: {
                const auto* c = std::get_if<glm::vec3>(&v);
                if (!c || _face >= 6) return false;
                // Paints as well as records: a law that recolours one side
                // must actually recolour that side.
                _owner->setFaceColor(_face, c->x, c->y, c->z);
                return true;
            }
            case Field::ActiveLayer: {
                double n = 0.0;
                if (!tex || tex->layers.empty() || !propertyValueToNumber(v, n)) return false;
                tex->activeLayer = std::max(
                    0, std::min(static_cast<int>(tex->layers.size()) - 1,
                                static_cast<int>(n)));
                return true;
            }
            case Field::UseLayers: {
                if (!tex) return false;
                if (const auto* b = std::get_if<bool>(&v)) {
                    tex->useLayers = *b;
                } else {
                    double n = 0.0;
                    if (!propertyValueToNumber(v, n)) return false;
                    tex->useLayers = n != 0.0;
                }
                recomposite(*tex);
                return true;
            }
            case Field::LayerOpacity: {
                double n = 0.0;
                if (!tex || tex->layers.empty() || !propertyValueToNumber(v, n)) return false;
                tex->setLayerOpacity(clampLayer(*tex), static_cast<float>(n));
                recomposite(*tex);
                return true;
            }
            case Field::BlendMode: {
                double n = 0.0;
                if (!tex || tex->layers.empty() || !propertyValueToNumber(v, n)) return false;
                tex->setBlendMode(clampLayer(*tex), static_cast<int>(n));
                recomposite(*tex);
                return true;
            }
            case Field::LayerCount:
            case Field::TextureSize:
                return false;   // structure is made with tools, not assigned
        }
        return false;
    }

private:
    // The paint this face wears, reached through the Material the object
    // names. Absent material, or a material with fewer faces than this bridge
    // addresses, reads as "no paint" rather than inventing any.
    FaceTexture* texture() const {
        auto mat = materials.resolveOrDefault(_owner->materialId());
        if (!mat || _face < 0 || _face >= static_cast<int>(mat->faceTextures.size())) {
            return nullptr;
        }
        return &mat->faceTextures[static_cast<std::size_t>(_face)];
    }
    static int clampLayer(const FaceTexture& tex) {
        return std::max(0, std::min(static_cast<int>(tex.layers.size()) - 1,
                                    tex.activeLayer));
    }
    static void recomposite(const FaceTexture& tex) {
        if (tex.useLayers) tex.compositeLayers();
        tex.updateWholeGPU();
    }

    std::string _name;
    Object* _owner;
    int _face;
    Field _field;
};

} // namespace

// Build the property registry for this Object
void Object::buildProperties() {
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Object, glm::vec3>>(
        "position", this, &Object::getPosition, &Object::setPosition));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Object, glm::vec3>>(
        "rotation", this, &Object::getRotationEulerDegrees, &Object::setRotationEulerDegrees));
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Object, glm::mat4>>(
        "transform", this, &Object::getTransform, &Object::setTransform));
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Object, glm::vec3>>(
        "center", this, &Object::center));
    // A Law can reassign which Material being paints this object, by identifier.
    _propertyRegistry.push_back(std::make_unique<PropertyRef<Object, std::string>>(
        "material", this, &Object::_materialId));

    // Shape parameters, flat under dotted names. v1 is raw read/write;
    // geometry regeneration on change follows through the setShape path
    // (LAW_AND_CREATION_SYSTEM.md, Stage 2 follow-up).
    auto addShapeParam = [this](const char* name, float ShapeParams::*member) {
        _propertyRegistry.push_back(
            std::make_unique<ShapeParamBridge>(name, this, member));
    };
    _propertyRegistry.push_back(std::make_unique<ShapeKindBridge>(this));
    addShapeParam("shape.r", &ShapeParams::r);
    addShapeParam("shape.ry", &ShapeParams::ry);
    addShapeParam("shape.rz", &ShapeParams::rz);
    addShapeParam("shape.halfH", &ShapeParams::halfH);
    addShapeParam("shape.majorR", &ShapeParams::majorR);
    addShapeParam("shape.minorR", &ShapeParams::minorR);
    addShapeParam("shape.paraboloidA", &ShapeParams::paraboloidA);
    addShapeParam("shape.ovoidAsym", &ShapeParams::ovoidAsym);
    addShapeParam("shape.fillet", &ShapeParams::fillet);
    addShapeParam("shape.width2D", &ShapeParams::width2D);
    addShapeParam("shape.height2D", &ShapeParams::height2D);

    _propertyRegistry.push_back(std::make_unique<PropertyRef<Object, std::string>>(
        "textString", this, &Object::_textString));

    // Whether the world's physics touches this being — governable state
    // ("make this object immaterial while the ritual runs").
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Object, bool>>(
        "physical", this, &Object::propPhysical, &Object::propSetPhysical));
    // Motion state: the rigid form's truth, addressable — collision RESPONSE
    // becomes authorable law-text.
    _propertyRegistry.push_back(std::make_unique<RigidFormBridge>(
        "velocity", this, RigidFormBridge::Field::Velocity));
    _propertyRegistry.push_back(std::make_unique<RigidFormBridge>(
        "mass", this, RigidFormBridge::Field::Mass));
    // The object's tint (uniform across faces when written; face 0 when read).
    _propertyRegistry.push_back(std::make_unique<ComputedProperty<Object, glm::vec3>>(
        "color", this, &Object::propColor, &Object::propSetColor));

    // The whole face-texture surface, face by face (color, layers, opacity,
    // blend) — laws can fade a face's layer or recolor one side; set-to-set
    // carries surface structure. Pixels/strokes stay source-code-only.
    //
    // The count is the object's OWN six colour slots, not the material's
    // texture count. buildProperties runs while the object is being
    // constructed, before it has a shape or a material assigned, so counting
    // paint here would register nothing and leave every face path unresolvable
    // for the object's whole life. Six is what faceColors holds; the bridge
    // reads through to the material at access time and reports "no paint" for
    // a face the material does not carry.
    for (int f = 0; f < 6; ++f) {
        const std::string base = "face." + std::to_string(f) + ".";
        auto addFace = [&](const char* leaf, FacePropertyBridge::Field field) {
            _propertyRegistry.push_back(std::make_unique<FacePropertyBridge>(
                base + leaf, this, f, field));
        };
        addFace("color", FacePropertyBridge::Field::Color);
        addFace("layerCount", FacePropertyBridge::Field::LayerCount);
        addFace("activeLayer", FacePropertyBridge::Field::ActiveLayer);
        addFace("useLayers", FacePropertyBridge::Field::UseLayers);
        addFace("layerOpacity", FacePropertyBridge::Field::LayerOpacity);
        addFace("blendMode", FacePropertyBridge::Field::BlendMode);
        addFace("textureSize", FacePropertyBridge::Field::TextureSize);
    }
}
