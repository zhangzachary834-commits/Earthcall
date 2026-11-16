import re

with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "r") as f:
    text = f.read()

# 1. kindName
text = text.replace('case Kind::ElevatePixels: return "ElevatePixels";', 'case Kind::ElevatePixels: return "ElevatePixels";\n        case Kind::FileRead: return "FileRead";\n        case Kind::FileWrite: return "FileWrite";\n        case Kind::CodecTransform: return "CodecTransform";')

# 2. toJson
to_json_cases = """        case Kind::ElevatePixels:
            j["propertyName"] = propertyName;
            j["pixelFacePath"] = pixelFacePath.toString();
            j["selector"] = mapFunction.toJson();
            break;
        case Kind::FileRead:
        case Kind::FileWrite:
        case Kind::CodecTransform:
            if (!path.empty()) j["path"] = path.toString();
            if (!input.empty()) j["input"] = input.toString();
            if (!propertyName.empty()) j["propertyName"] = propertyName;
            break;"""
text = text.replace('        case Kind::ElevatePixels:\n            j["propertyName"] = propertyName;\n            j["pixelFacePath"] = pixelFacePath.toString();\n            j["selector"] = mapFunction.toJson();\n            break;', to_json_cases)

with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "w") as f:
    f.write(text)
