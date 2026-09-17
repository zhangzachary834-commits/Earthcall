import re
with open("src/ConstructedBeing/Singular/Object/Object.hpp", "r") as f:
    content = f.read()

if "bool readAuthoredPropertyProjectionColors" not in content:
    content = content.replace(
        'bool readAuthoredPropertyProjection(Earthcall::StringId id,',
        'bool readAuthoredPropertyProjection(Earthcall::StringId id,\n                                        PropertyValue& out) const override;\n    bool readAuthoredPropertyProjectionColors(Earthcall::StringId id,'
    )
with open("src/ConstructedBeing/Singular/Object/Object.hpp", "w") as f:
    f.write(content)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    content2 = f.read()

content2 = content2.replace(
    'bool Object::readAuthoredPropertyProjection(Earthcall::StringId id,',
    'bool Object::readAuthoredPropertyProjectionColors(Earthcall::StringId id,'
)

new_func = """
bool Object::readAuthoredPropertyProjection(Earthcall::StringId id,
                                            PropertyValue& out) const {
    const std::string name = Earthcall::StringInterner::resolve(id);
    PixelAddress pixel;
    int face = -1;
    OntoMath::Piecewise selector;
    const bool single = parsePixelAddress(name, pixel);
    if (single) face = pixel.face;
    else if (!selectionDefinition(*this, name, face, selector)) return false;
    auto mat = materials.resolveOrDefault(_materialId);
    if (!mat || face < 0 || face >= static_cast<int>(mat->faceTextures.size())) return false;
    const FaceTexture& ft = mat->faceTextures[static_cast<std::size_t>(face)];
    
    auto dict = std::make_shared<PropertyDict>();
    dict->elements["_type"] = PropertyValue(std::string("projection"));
    dict->elements["target"] = PropertyValue(name);
    dict->elements["revision"] = PropertyValue(static_cast<double>(ft.revision));
    out = PropertyValue(std::move(dict));
    return true;
}

bool Object::readAuthoredPropertyProjectionColors(Earthcall::StringId id,"""

content2 = content2.replace(
    'bool Object::readAuthoredPropertyProjectionColors(Earthcall::StringId id,',
    new_func
)

with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(content2)

with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.hpp", "r") as f:
    content3 = f.read()

if "uint64_t revision = 0;" not in content3:
    content3 = content3.replace(
        'mutable TextureHandle id = 0;',
        'mutable TextureHandle id = 0;\n    mutable uint64_t revision = 0;'
    )

with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.hpp", "w") as f:
    f.write(content3)

with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "r") as f:
    content4 = f.read()

content4 = content4.replace(
    'return writeSamples(coordinates, colors);\n}',
    'bool ok = writeSamples(coordinates, colors);\n    if (ok) revision++;\n    return ok;\n}'
)

content4 = content4.replace(
    'currentRenderer().uploadTextureRegion(id, pixels.data(), width, height, minX, minY, regionW, regionH);\n    }\n    return true;\n}',
    'currentRenderer().uploadTextureRegion(id, pixels.data(), width, height, minX, minY, regionW, regionH);\n    }\n    revision++;\n    return true;\n}'
)

with open("src/ConstructedBeing/Singular/Object/Object/FaceTexture.cpp", "w") as f:
    f.write(content4)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "r") as f:
    content5 = f.read()

helper = """
static void resolveProjectionToken(Singular& target, const PropertyValue& in, PropertyValue& out) {
    out = in;
    const auto* dict = std::get_if<std::shared_ptr<PropertyDict>>(&in);
    if (!dict || !*dict) return;
    auto itType = (*dict)->elements.find("_type");
    if (itType == (*dict)->elements.end() || !std::holds_alternative<std::string>(itType->second) || std::get<std::string>(itType->second) != "projection") return;
    
    auto itTarget = (*dict)->elements.find("target");
    if (itTarget == (*dict)->elements.end() || !std::holds_alternative<std::string>(itTarget->second)) return;
    std::string targetName = std::get<std::string>(itTarget->second);
    
    if (target.isKind(BeingKind::Object)) {
        Object& obj = static_cast<Object&>(target);
        obj.readAuthoredPropertyProjectionColors(Earthcall::StringInterner::intern(targetName), out);
    }
}
"""

if "resolveProjectionToken" not in content5:
    content5 = content5.replace(
        'namespace Earthcall {\n\n',
        'namespace Earthcall {\n\n#include "ConstructedBeing/Singular/Object/Object.hpp"\n' + helper
    )

content5 = content5.replace(
    'PropertyValue rhs = rhsLiteral;',
    'resolveProjectionToken(t, lhs, lhs);\n                PropertyValue rhs = rhsLiteral;'
)

content5 = content5.replace(
    '                        });\n                    }\n                    return false;\n                }\n                return [evaluate](lhs, rhs);',
    '                        });\n                    }\n                    return false;\n                }\n                resolveProjectionToken(t, rhs, rhs);\n                return [evaluate](lhs, rhs);'
)

with open("src/ZonesOfEarth/AuthorsOfLaw/ConditionModel.cpp", "w") as f:
    f.write(content5)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content6 = f.read()

if "law->setActivation" not in content6:
    content6 = content6.replace(
        'law->setActionModel(act);',
        'law->setActionModel(act);\n    law->setActivation(Law::Activation::WhileTrue);'
    )

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content6)
