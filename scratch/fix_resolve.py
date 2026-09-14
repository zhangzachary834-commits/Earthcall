with open('src/ConstructedBeing/Singular/Property/PropertyPath.cpp', 'r') as f:
    content = f.read()

old_code = """
            if (!foundReg && !foundDyn) return slot;

            i += consumed;
            slot.owner = currentOwner;
            currentRegistered = foundReg;
            currentDynamic = foundDyn;
            currentOwner = nullptr;
"""

new_code = """
            if (!foundReg && !foundDyn) return slot;

            i += consumed;
            slot.owner = currentOwner;
            if (foundDyn) {
                slot.dynamicKey = Earthcall::StringInterner::resolve(_joinedIds[i - consumed][consumed - 1]);
            }
            currentRegistered = foundReg;
            currentDynamic = foundDyn;
            currentOwner = nullptr;
"""

if old_code in content:
    content = content.replace(old_code, new_code)
    with open('src/ConstructedBeing/Singular/Property/PropertyPath.cpp', 'w') as f:
        f.write(content)
    print("Replaced resolve!")
else:
    print("Could not find old code in resolve")
