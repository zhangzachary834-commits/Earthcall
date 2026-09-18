import re

with open("src/ConstructedBeing/Singular/Property/PropertyPath.cpp", "r") as f:
    content = f.read()

content = content.replace('Earthcall::StringId id = Earthcall::StringInterner::intern(joined);', """Earthcall::StringId id = Earthcall::StringInterner::intern(joined); printf("intern %s -> %u\\n", joined.c_str(), id.value);""")
content = content.replace('if (slot.owner->setDynamicProperty(slot.dynamicKey, coerced)) {', """printf("setDynamicProperty key=%s\\n", slot.dynamicKey.c_str()); if (slot.owner->setDynamicProperty(slot.dynamicKey, coerced)) {""")

with open("src/ConstructedBeing/Singular/Property/PropertyPath.cpp", "w") as f:
    f.write(content)
