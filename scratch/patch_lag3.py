import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/LawManager.cpp" # wait, it's Law.cpp
file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

# I want to print how many WhileTrue laws have terminals and how many don't
patch = """
        auto termIt = _reteTerminals.find(lawId);
        const bool hasTerminals =
            _connected && termIt != _reteTerminals.end() && !termIt->second.empty();
        
        static int print_count = 0;
        if (print_count++ < 21 && law->activation() == Law::Activation::WhileTrue) {
            printf("WhileTrue Law %s: hasTerminals=%d\\n", lawId.c_str(), hasTerminals);
        }
"""
content = content.replace("""        auto termIt = _reteTerminals.find(lawId);
        // Terminals alone do not license the reactive path: it answers from""", patch + """        // Terminals alone do not license the reactive path: it answers from""")

with open(file_path, "w") as f:
    f.write(content)
