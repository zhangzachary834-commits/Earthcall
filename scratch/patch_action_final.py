with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "r") as f:
    text = f.read()

# 1. Factories
factories = """ActionNode ActionNode::fileRead(const std::string& inputProp, const std::string& destProp) {
    ActionNode n;
    n.kind = Kind::FileRead;
    n.input = PropertyPath::parse(inputProp);
    n.path = PropertyPath::parse(destProp);
    return n;
}
ActionNode ActionNode::fileWrite(const std::string& pathProp, const std::string& contentProp) {
    ActionNode n;
    n.kind = Kind::FileWrite;
    n.path = PropertyPath::parse(pathProp);
    n.input = PropertyPath::parse(contentProp);
    return n;
}
ActionNode ActionNode::codecTransform(const std::string& codecOperation, const std::string& inputProp, const std::string& destProp) {
    ActionNode n;
    n.kind = Kind::CodecTransform;
    n.propertyName = codecOperation;
    n.input = PropertyPath::parse(inputProp);
    n.path = PropertyPath::parse(destProp);
    return n;
}

ActionNode ActionNode::writePixel"""
text = text.replace("ActionNode ActionNode::writePixel", factories)


# 2. kindName
text = text.replace('case Kind::ElevatePixels: return "ElevatePixels";', 'case Kind::ElevatePixels: return "ElevatePixels";\n        case Kind::FileRead: return "FileRead";\n        case Kind::FileWrite: return "FileWrite";\n        case Kind::CodecTransform: return "CodecTransform";')


# 3. toJson
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


# 4. compile
compile_cases = """        case Kind::FileRead: {
            const PropertyPath sourcePath = input;
            const PropertyPath destPath = path;
            return [sourcePath, destPath](const ECA::Event&, Singular& subject) {
                PropertyValue pvFile;
                if (lawGetValue(subject, sourcePath, pvFile)) {
                    lawSetValue(subject, PropertyPath::parse("@file-channel.file.path"), pvFile, "FileRead");
                    lawSetValue(subject, PropertyPath::parse("@file-channel.file.read"), PropertyValue(true), "FileRead");
                    PropertyValue content;
                    if (lawGetValue(subject, PropertyPath::parse("@file-channel.file.content"), content)) {
                        lawSetValue(subject, destPath, content, "FileRead");
                    }
                }
            };
        }
        case Kind::FileWrite: {
            const PropertyPath targetPathPath = path;
            const PropertyPath contentPath = input;
            return [targetPathPath, contentPath](const ECA::Event&, Singular& subject) {
                PropertyValue pvFile, pvContent;
                if (lawGetValue(subject, targetPathPath, pvFile) && lawGetValue(subject, contentPath, pvContent)) {
                    lawSetValue(subject, PropertyPath::parse("@file-channel.file.path"), pvFile, "FileWrite");
                    lawSetValue(subject, PropertyPath::parse("@file-channel.file.content"), pvContent, "FileWrite");
                    lawSetValue(subject, PropertyPath::parse("@file-channel.file.write"), PropertyValue(true), "FileWrite");
                }
            };
        }
        case Kind::CodecTransform: {
            const std::string op = propertyName;
            const PropertyPath sourcePath = input;
            const PropertyPath destPath = path;
            return [op, sourcePath, destPath](const ECA::Event&, Singular& subject) {
                PropertyValue cv;
                if (lawGetValue(subject, sourcePath, cv)) {
                    lawSetValue(subject, PropertyPath::parse("@codec.codec.input"), cv, "CodecTransform");
                    PropertyValue result;
                    if (lawGetValue(subject, PropertyPath::parse("@codec.codec." + op), result)) {
                        lawSetValue(subject, destPath, result, "CodecTransform");
                    }
                }
            };
        }
        case Kind::WritePixel: {
            const PropertyPath facePath = pixelFacePath;"""
text = text.replace('        case Kind::WritePixel: {\n            const PropertyPath facePath = pixelFacePath;', compile_cases)


# 5. describe()
describe_cases = """        case Kind::FileRead:
            return "read file at " + input.toString() + " into " + path.toString();
        case Kind::FileWrite:
            return "write " + input.toString() + " to file at " + path.toString();
        case Kind::CodecTransform:
            return "transform " + input.toString() + " via " + propertyName + " into " + path.toString();
        case Kind::WritePixel:
            return "write pixel from " + pixelColorPath.toString();"""
text = text.replace('        case Kind::WritePixel:\n            return "write pixel from " + pixelColorPath.toString();', describe_cases)

with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "w") as f:
    f.write(text)

