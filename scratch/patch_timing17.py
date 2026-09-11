import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

find_str = """bool LawManager::propheticHears(const std::string& propertyName) const {
    ++_propheticCounters.asked;
    static int prop_hears_count = 0;
    static int prop_hears_true_count = 0;"""

replace_str = """
int global_prop_hears_count = 0;
int global_prop_hears_true_count = 0;

bool LawManager::propheticHears(const std::string& propertyName) const {
    ++_propheticCounters.asked;"""

find_str2 = """    prop_hears_count++;
    if (_propheticRevision != Law::textRevision()) { prop_hears_true_count++; return true; }"""

replace_str2 = """    global_prop_hears_count++;
    if (_propheticRevision != Law::textRevision()) { global_prop_hears_true_count++; return true; }"""

content = content.replace("prop_hears_count", "global_prop_hears_count")
content = content.replace("prop_hears_true_count", "global_prop_hears_true_count")
content = content.replace("extern int global_prop_hears_count;", "")
content = content.replace("extern int global_prop_hears_true_count;", "")

with open(file_path, "w") as f:
    f.write(content)
print("Patched Law.cpp timing 17!")
