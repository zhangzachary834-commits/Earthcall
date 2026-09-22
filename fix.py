with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    code = f.read()

code = code.replace("PropertyValue sel_val;", "{ PropertyValue sel_val;")
code = code.replace('else std::cout << "test_mat found" << std::endl;', 'else std::cout << "test_mat found" << std::endl; }')

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(code)
