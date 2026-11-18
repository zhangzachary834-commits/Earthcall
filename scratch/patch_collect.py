with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "r") as f:
    text = f.read()

collect_cases = """        case Kind::FileRead:
        case Kind::FileWrite:
        case Kind::CodecTransform:
            add(input);
            add(path);
            break;
        case Kind::WritePixel:"""

text = text.replace("        case Kind::WritePixel:\n            add(pixelFacePath);", collect_cases + "\n            add(pixelFacePath);")

with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "w") as f:
    f.write(text)
