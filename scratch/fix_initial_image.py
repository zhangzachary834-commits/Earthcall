with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'img[i] = 255; img[i+1] = 0; img[i+2] = 0; img[i+3] = 255;',
    'img[i] = 0; img[i+1] = 0; img[i+2] = 0; img[i+3] = 255;'
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
