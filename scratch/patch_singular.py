with open("src/ConstructedBeing/Singular/Singular.cpp", "r") as f:
    content = f.read()

content = content.replace("""    PropertyValue stored = v;
    if (projected) {
        PropertyValue live;
        if (readAuthoredPropertyProjection(id, live)) stored = std::move(live);
    }""", """    PropertyValue stored = v;""")

with open("src/ConstructedBeing/Singular/Singular.cpp", "w") as f:
    f.write(content)
