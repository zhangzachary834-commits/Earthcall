with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "r") as f:
    text = f.read()

bad_compile = """        case Kind::FileRead:
            return "read file at " + input.toString() + " into " + path.toString();
        case Kind::FileWrite:
            return "write " + input.toString() + " to file at " + path.toString();
        case Kind::CodecTransform:
            return "transform " + input.toString() + " via " + propertyName + " into " + path.toString();"""

good_compile = """        case Kind::FileRead: {
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
        }"""

text = text.replace(bad_compile, good_compile)

with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "w") as f:
    f.write(text)
