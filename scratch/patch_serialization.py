import re

CPP_PATH = "src/Singularity/Storage/Serialization.cpp"
with open(CPP_PATH, 'r') as f:
    cpp = f.read()

# Replace getCenter, etc.
cpp = cpp.replace("part.getCenter()", "part.getPrimaryObject()->getCenter()")
cpp = cpp.replace("part.getAuthoritativeAxis()", "part.getPrimaryObject()->getAuthoritativeAxis()")
cpp = cpp.replace("part.getTargetRotationEulerDegrees()", "part.getPrimaryObject()->getTargetRotationEulerDegrees()")
cpp = cpp.replace("part.getRotationResponsiveness()", "part.getPrimaryObject()->getRotationResponsiveness()")

# Replace setCenter, etc.
cpp = cpp.replace("part.setCenter(", "part.getPrimaryObject()->setCenter(")
cpp = cpp.replace("part.setAuthoritativeAxis(", "part.getPrimaryObject()->setAuthoritativeAxis(")
cpp = cpp.replace("part.setRotationResponsiveness(", "part.getPrimaryObject()->setRotationResponsiveness(")
cpp = cpp.replace("part.setTargetRotationEulerDegrees(", "part.getPrimaryObject()->setTargetRotationEulerDegrees(")

# Replace faceColors references in Serialization.cpp
cpp = cpp.replace("part.faceColors", "part.getPrimaryObject()->faceColors")

# Replace Object::ShapeKind
cpp = cpp.replace("static_cast<Object::ShapeKind>", "static_cast<ObjectTypes::ShapeKind>")

with open(CPP_PATH, 'w') as f:
    f.write(cpp)
