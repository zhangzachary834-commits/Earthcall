import sys

file_path = "tests/singularity/frame_lag_test.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """int main(int argc, char** argv) {"""
replace_str = """#include <GLFW/glfw3.h>

int main(int argc, char** argv) {
    glfwInit();"""

if find_str in content:
    content = content.replace(find_str, replace_str)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched frame_lag_test.cpp!")
else:
    print("Not found!")
