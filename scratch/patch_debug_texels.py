with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "r") as f:
    or_cpp = f.read()

or_cpp = or_cpp.replace(
    '            if (selectorIncludes(selector, u, v, subject)) selected.emplace_back(x, y);',
    '            bool inc = selectorIncludes(selector, u, v, subject);\n            std::cout << "selectedTexels: x=" << x << " y=" << y << " u=" << u << " v=" << v << " inc=" << inc << "\\n";\n            if (inc) selected.emplace_back(x, y);'
)
with open("src/ConstructedBeing/Singular/Object/ObjectRender.cpp", "w") as f:
    f.write(or_cpp)
