with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    code = f.read()

debug_code = """
    PropertyValue sel_val;
    if (!obj->getDynamicProperty("surface.selection.regionA", sel_val)) {
        std::cout << "regionA surface.selection missing" << std::endl;
    } else {
        auto* str = std::get_if<std::string>(&sel_val);
        if (!str) std::cout << "regionA surface.selection not a string" << std::endl;
        else std::cout << "regionA valid" << std::endl;
    }
    auto test_mat = materials.resolveOrDefault(obj->materialId());
    if (!test_mat) std::cout << "test_mat missing" << std::endl;
    else std::cout << "test_mat found" << std::endl;
"""

code = code.replace('PropertyValue readA;', debug_code + 'PropertyValue readA;')

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(code)
