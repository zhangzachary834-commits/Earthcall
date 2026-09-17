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
