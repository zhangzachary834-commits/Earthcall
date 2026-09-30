import re

with open('tests/support/test_save_helper.hpp', 'r') as f:
    content = f.read()

# Add chrono
content = content.replace('#include <cmath>', '#include <cmath>\n#include <chrono>')

struct_str = """
struct TempSaveRoot {
    std::filesystem::path path;
    TempSaveRoot() {
        path = std::filesystem::temp_directory_path() /
               ("earthcall-test-save-" +
                std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        std::filesystem::create_directories(path);
        SaveSystem::setSaveRoot(path.string());
    }
    ~TempSaveRoot() {
        SaveSystem::setSaveRoot("");
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }
};

inline void dump_test_save"""

content = content.replace('inline void dump_test_save', struct_str)

with open('tests/support/test_save_helper.hpp', 'w') as f:
    f.write(content)
