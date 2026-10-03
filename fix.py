with open("src/Singularity/Terminal/LawSentence.cpp", "r") as f:
    code = f.read()

original = """        it->second.description = "shared spelling · Metalaw decides";
        it->second.detail = "shared spelling · Metalaw decides · " + joined;
        std::string joined;
        for (const auto& denotation : meanings) {
            if (!joined.empty()) joined += "  ·  ";
            joined += denotation;
        }
        it->second.detail = "shared spelling · Metalaw decides · " + joined;
        it->second.snippet.clear();"""

new_code = """        it->second.description = "shared spelling · Metalaw decides";
        std::string joined;
        for (const auto& denotation : meanings) {
            if (!joined.empty()) joined += "  ·  ";
            joined += denotation;
        }
        it->second.detail = "shared spelling · Metalaw decides · " + joined;
        it->second.snippet.clear();"""

code = code.replace(original, new_code)
with open("src/Singularity/Terminal/LawSentence.cpp", "w") as f:
    f.write(code)
