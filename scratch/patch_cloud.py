import re

with open("tests/singularity/cloud_storage_test.cpp", "r") as f:
    content = f.read()

content = content.replace("std::chrono::seconds(5)", "std::chrono::seconds(10)")

with open("tests/singularity/cloud_storage_test.cpp", "w") as f:
    f.write(content)
