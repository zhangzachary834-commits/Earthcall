import re
import os

HPP_PATH = "src/Person/Body/BodyPart/BodyPart.hpp"
CPP_PATH = "src/Person/Body/BodyPart/BodyPart.cpp"

with open(HPP_PATH, 'r') as f:
    hpp = f.read()

hpp = hpp.replace("glm::mat4 getRaycastTransform() const override;", "glm::mat4 getRaycastTransform() const;\n    glm::mat4 getTransform() const { return _transform; }\n    void setFaceColor(int faceIndex, float r, float g, float b);")
hpp = hpp.replace("void setTransform(const glm::mat4& t) override;", "void setTransform(const glm::mat4& t);")
hpp = hpp.replace("void setPrimaryShape(Object::ShapeKind gt);", "void setPrimaryShape(ObjectTypes::ShapeKind gt);")
hpp = hpp.replace("Object::ShapeKind getPrimaryShape() const { return getShapeKind(); }", "ObjectTypes::ShapeKind getPrimaryShape() const;")
hpp = hpp.replace("Object* addSubObject(Object::ShapeKind kind,", "Object* addSubObject(ObjectTypes::ShapeKind kind,")
hpp = hpp.replace("private:", "private:\n    std::unique_ptr<Object> _primaryObject;\n    glm::mat4 _transform = glm::mat4(1.0f);")

with open(HPP_PATH, 'w') as f:
    f.write(hpp)

with open(CPP_PATH, 'r') as f:
    cpp = f.read()

cpp = cpp.replace(" : Object(name.empty() ? \"\" : \"bodypart.\" + name), Formation(), partName(name), partType(type), _dimensions(dimensions)", 
                  " : Formation(), partName(name), partType(type), _dimensions(dimensions)")

cpp = cpp.replace("setShapeKind(geometryType);", 
                  "_primaryObject = std::make_unique<Object>(name.empty() ? \"\" : \"bodypart.\" + name);\n    _primaryObject->setShape(geometryType);\n    addMember(_primaryObject.get());")

cpp = cpp.replace("r.pushModel(transform);", "r.pushModel(_transform);")
cpp = cpp.replace("drawObject();", "_primaryObject->drawObject();")
cpp = cpp.replace("drawHighlightOutline();", "_primaryObject->drawHighlightOutline();")
cpp = cpp.replace("Object::setTransform(t);", "_primaryObject->setTransform(t);\n    _transform = t;")
cpp = cpp.replace("updateCollisionZone(scaled);", "_primaryObject->updateCollisionZone(scaled);")
cpp = cpp.replace("return getTransform() * glm::scale(glm::mat4(1.0f), _dimensions);", "return _transform * glm::scale(glm::mat4(1.0f), _dimensions);")
cpp = cpp.replace("void BodyPart::setPrimaryShape(Object::ShapeKind gt) {", "void BodyPart::setPrimaryShape(ObjectTypes::ShapeKind gt) {\n    _primaryObject->setShape(gt);")
cpp = cpp.replace("setShapeKind(gt);", "")
cpp = cpp.replace("ObjectTypes::ShapeKind BodyPart::getPrimaryShape() const {\n    return _primaryObject ? _primaryObject->getShapeKind() : ObjectTypes::ShapeKind::Cube;\n}", "")
cpp = cpp.replace("Object* BodyPart::addSubObject(Object::ShapeKind kind, const glm::mat4& localOffset) {", "Object* BodyPart::addSubObject(ObjectTypes::ShapeKind kind, const glm::mat4& localOffset) {")
cpp = cpp.replace("glm::mat4 worldT = getTransform() * localOffset;", "glm::mat4 worldT = _transform * localOffset;")
cpp = cpp.replace("result.push_back(this);", "if (_primaryObject) result.push_back(_primaryObject.get());")
cpp = cpp.replace("for (int f = 0; f < 6; ++f) setFaceColor(f, color[0], color[1], color[2]);", "for (int f = 0; f < 6; ++f) setFaceColor(f, color[0], color[1], color[2]);") # This stays the same because I'll add setFaceColor to BodyPart.

# Also need to add setFaceColor implementation
face_color_impl = """
void BodyPart::setFaceColor(int faceIndex, float r, float g, float b) {
    if (_primaryObject) _primaryObject->setFaceColor(faceIndex, r, g, b);
}

ObjectTypes::ShapeKind BodyPart::getPrimaryShape() const {
    return _primaryObject ? _primaryObject->getShapeKind() : ObjectTypes::ShapeKind::Cube;
}
"""
cpp += face_color_impl

with open(CPP_PATH, 'w') as f:
    f.write(cpp)

