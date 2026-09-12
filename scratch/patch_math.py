import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """    uint64_t currentRevision = Universe::instance().structuralRevision();
    if (!s_initialized || s_lastRevision != currentRevision) {"""

replace_str = """    uint64_t currentRevision = Universe::instance().structuralRevision();
    if (!s_initialized || s_lastRevision != currentRevision) {
        static int rebuild_count = 0;
        rebuild_count++;
        if (rebuild_count % 100 == 0) {
            printf("MathBinding cache rebuilt %d times!\\n", rebuild_count);
        }"""

if find_str in content:
    content = content.replace(find_str, replace_str)
    with open(file_path, "w") as f:
        f.write(content)
    print("Patched MathBinding.hpp")
else:
    print("Not found")
