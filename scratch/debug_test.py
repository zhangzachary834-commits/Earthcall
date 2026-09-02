import sys
with open("tests/singularity/substrate_split_test.cpp", "r") as f:
    text = f.read()

text = text.replace("check(!o.contains(\"transform\")", """std::cout << "transform in o? " << o.contains("transform") << std::endl;
if (o.contains("transform")) { std::cout << o.dump(2) << std::endl; }
check(!o.contains("transform")""")

with open("tests/singularity/substrate_split_test.cpp", "w") as f:
    f.write(text)
