import re

with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp", "r") as f:
    text = f.read()

# Add to enum class Kind
if "FileRead = 23" not in text:
    text = text.replace('ElevatePixels = 22\n    };', 'ElevatePixels = 22,\n        FileRead = 23,\n        FileWrite = 24,\n        CodecTransform = 25\n    };')

# Add factories
if "static ActionNode fileRead(" not in text:
    text = text.replace('static ActionNode writePixel(', 'static ActionNode fileRead(const std::string& pathProp, const std::string& destProp);\n    static ActionNode fileWrite(const std::string& pathProp, const std::string& contentProp);\n    static ActionNode codecTransform(const std::string& codecOperation, const std::string& inputProp, const std::string& destProp);\n    static ActionNode writePixel(')

with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp", "w") as f:
    f.write(text)
