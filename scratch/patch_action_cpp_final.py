import re

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
if "ActionNode ActionNode::fileRead(" not in text:
    text = text.replace("ActionNode ActionNode::writePixel", factories)


# 2. compile()
compiles = """        case Kind::FileRead: {
            const PropertyPath sourcePath = input;
            const PropertyPath destPath = path;
            return [sourcePath, destPath](const ECA::Event&, Singular& subject) {
                PropertyValue pvFile;
                if (lawGetValue(subject, sourcePath, pvFile)) {
                    lawSetValue(subject, PropertyPath::parse("file-channel.path"), pvFile, "FileRead");
                    lawSetValue(subject, PropertyPath::parse("file-channel.readTrigger"), true, "FileRead");
                    PropertyValue content;
                    if (lawGetValue(subject, PropertyPath::parse("file-channel.content"), content)) {
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
                    lawSetValue(subject, PropertyPath::parse("file-channel.path"), pvFile, "FileWrite");
                    lawSetValue(subject, PropertyPath::parse("file-channel.content"), pvContent, "FileWrite");
                    lawSetValue(subject, PropertyPath::parse("file-channel.writeTrigger"), true, "FileWrite");
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
                    lawSetValue(subject, PropertyPath::parse("codec.input"), cv, "CodecTransform");
                    PropertyValue result;
                    if (lawGetValue(subject, PropertyPath::parse("codec." + op), result)) {
                        lawSetValue(subject, destPath, result, "CodecTransform");
                    }
                }
            };
        }
        case Kind::WritePixel:"""

if "case Kind::FileRead:" not in text:
    text = text.replace("        case Kind::WritePixel:", compiles)


# 3. describe()
describes = """        case Kind::FileRead:
            return "read file at " + input.toString() + " into " + path.toString();
        case Kind::FileWrite:
            return "write " + input.toString() + " to file at " + path.toString();
        case Kind::CodecTransform:
            return "transform " + input.toString() + " via " + propertyName + " into " + path.toString();
        case Kind::WritePixel:"""
if "case Kind::FileRead:\n            return \"read file" not in text:
    text = text.replace("        case Kind::WritePixel:", describes)

with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "w") as f:
    f.write(text)

