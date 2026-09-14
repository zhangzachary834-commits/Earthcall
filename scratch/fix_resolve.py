with open('src/ConstructedBeing/Singular/Property/PropertyPath.cpp', 'r') as f:
    content = f.read()

import re

old_loop = """            for (std::size_t runLength = 1; runLength <= idsFromHere.size(); ++runLength) {
                Earthcall::StringId id = idsFromHere[runLength - 1];
                if (Property* candidate = currentOwner->findProperty(id)) {
                    foundReg = candidate;
                    consumed = runLength;
                }
            }

            PropertyValue* foundDyn = nullptr;
            if (!foundReg) {
                for (std::size_t runLength = 1; runLength <= idsFromHere.size(); ++runLength) {
                    Earthcall::StringId id = idsFromHere[runLength - 1];
                    if (PropertyValue* candidate = currentOwner->getDynamicPropertyPtr(id)) {
                        foundDyn = candidate;
                        consumed = runLength;
                    }
                }
            }"""

new_loop = """            PropertyValue* foundDyn = nullptr;
            for (std::size_t runLength = 1; runLength <= idsFromHere.size(); ++runLength) {
                Earthcall::StringId id = idsFromHere[runLength - 1];
                if (PropertyValue* candidate = currentOwner->getDynamicPropertyPtr(id)) {
                    foundDyn = candidate;
                    consumed = runLength;
                }
            }

            if (!foundDyn) {
                for (std::size_t runLength = 1; runLength <= idsFromHere.size(); ++runLength) {
                    Earthcall::StringId id = idsFromHere[runLength - 1];
                    if (Property* candidate = currentOwner->findProperty(id)) {
                        foundReg = candidate;
                        consumed = runLength;
                    }
                }
            }"""

if old_loop in content:
    content = content.replace(old_loop, new_loop)
    with open('src/ConstructedBeing/Singular/Property/PropertyPath.cpp', 'w') as f:
        f.write(content)
    print("Replaced!")
else:
    print("Could not find old loop")
