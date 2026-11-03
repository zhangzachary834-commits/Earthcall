import re
with open("tests/zones/prism_cathedral_test.cpp", "r") as f:
    c = f.read()
c = c.replace('std::shared_ptr<Zone> cathedral = nullptr;', 'std::shared_ptr<Zone> cathedral = nullptr;\n    for (const auto& z : mgr.zones()) { if (z) std::cout << "Hydrated zone: " << z->getIdentifier() << "\\n"; }')
with open("tests/zones/prism_cathedral_test.cpp", "w") as f:
    f.write(c)
