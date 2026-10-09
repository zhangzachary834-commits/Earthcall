// Rung 10 — Mathematics Gift Shop anti-regrowth guard.
//
// This is a source-level architecture witness, not a numerical test.
// GLM may execute or represent OntoMath mathematics, but production code may
// not independently originate matrix/transform meaning through these entry
// points. Failures are all reported before exit so one reopened aisle does not
// hide the others.
//
// The allowlist is deliberately exact-file and reasoned. Adding an allowance
// is an architecture change: name the substrate/oracle boundary in this file.
//
// Run from the repository root; CMake pins that working directory below.

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

struct SemanticCall {
    const char* token;
    const char* name;
};

constexpr SemanticCall kTargetCalls[] = {
    {"glm::inverse(", "inverse"},
    {"glm::transpose(", "transpose"},
    {"glm::determinant(", "determinant"},
    {"glm::translate(", "translate"},
    {"glm::rotate(", "rotate"},
    {"glm::scale(", "scale"},
    {"glm::lookAt(", "lookAt"},
    {"glm::perspective", "perspective*"},
};

struct AllowedFile {
    const char* path;
    const char* reason;
};

constexpr AllowedFile kAllowedFiles[] = {
    {
        "src/Singularity/OntoMath/LinearAlgebra.cpp",
        "OntoMath numerical backend: GLM executes mathematics OntoMath owns",
    },
    {
        "src/Singularity/OntoMath/Affine.cpp",
        "OntoMath affine decomposition backend: GLM executes/refines authored semantics",
    },
    {
        "src/Singularity/Screen/GL/GluCompat.cpp",
        "GLU compatibility boundary: representation/API compatibility, not domain authorship",
    },
    {
        "src/Singularity/Screen/WebGPU/smoke_renderer.cpp",
        "standalone renderer diagnostic oracle, not production semantic authority",
    },
    {
        "src/Singularity/Screen/WebGPU/smoke_window.mm",
        "standalone renderer diagnostic oracle, not production semantic authority",
    },
};

bool isSourceFile(const fs::path& path) {
    const std::string ext = path.extension().string();
    return ext == ".cpp" || ext == ".hpp" || ext == ".h" ||
           ext == ".cc" || ext == ".cxx" || ext == ".mm" ||
           ext == ".m" || ext == ".inl";
}

const char* allowanceFor(const std::string& path) {
    for (const auto& allowed : kAllowedFiles) {
        if (path == allowed.path) return allowed.reason;
    }
    return nullptr;
}

// Return only lexical code from one line. This removes // comments, block
// comments, string literals and character literals while preserving token
// positions well enough for a source-architecture scan.
std::string codeOnly(const std::string& line, bool& inBlockComment) {
    std::string out(line.size(), ' ');
    bool inString = false;
    bool inChar = false;
    bool escaped = false;

    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        const char n = i + 1 < line.size() ? line[i + 1] : '\0';

        if (inBlockComment) {
            if (c == '*' && n == '/') {
                inBlockComment = false;
                ++i;
            }
            continue;
        }

        if (inString) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }

        if (inChar) {
            if (escaped) {
                escaped = false;
            } else if (c == '\\') {
                escaped = true;
            } else if (c == '\'') {
                inChar = false;
            }
            continue;
        }

        if (c == '/' && n == '*') {
            inBlockComment = true;
            ++i;
            continue;
        }
        if (c == '/' && n == '/') break;
        if (c == '"') {
            inString = true;
            continue;
        }
        if (c == '\'') {
            inChar = true;
            continue;
        }

        out[i] = c;
    }

    return out;
}

} // namespace

int main() {
    const fs::path root = fs::current_path();
    const fs::path src = root / "src";

    if (!fs::exists(src) || !fs::is_directory(src)) {
        std::cerr << "gift_shop_guardrail_test: expected repository src/ at "
                  << src << "\n";
        return 2;
    }

    std::size_t scannedFiles = 0;
    std::size_t allowedHits = 0;
    std::size_t violations = 0;

    for (const auto& entry : fs::recursive_directory_iterator(src)) {
        if (!entry.is_regular_file() || !isSourceFile(entry.path())) continue;

        ++scannedFiles;
        const std::string relative =
            fs::relative(entry.path(), root).generic_string();
        const char* allowedReason = allowanceFor(relative);

        std::ifstream input(entry.path());
        if (!input) {
            std::cerr << "FAILED to read " << relative << "\n";
            ++violations;
            continue;
        }

        bool inBlockComment = false;
        std::string line;
        std::size_t lineNumber = 0;
        while (std::getline(input, line)) {
            ++lineNumber;
            const std::string code = codeOnly(line, inBlockComment);

            for (const auto& call : kTargetCalls) {
                std::size_t pos = 0;
                while ((pos = code.find(call.token, pos)) != std::string::npos) {
                    if (allowedReason) {
                        ++allowedHits;
                    } else {
                        ++violations;
                        std::cout
                            << "GIFT_SHOP_VIOLATION " << relative << ":"
                            << lineNumber << " " << call.name << "\n"
                            << "  " << line << "\n";
                    }
                    pos += std::string(call.token).size();
                }
            }
        }
    }

    std::cout << "gift_shop_guardrail_test: scanned " << scannedFiles
              << " source files; " << allowedHits
              << " explicitly allowed substrate/oracle hits; "
              << violations << " unauthorized semantic-origin hits\n";

    if (violations != 0) {
        std::cout
            << "Rung 10 remains open: migrate these calls through OntoMath or "
               "justify a narrow named substrate boundary.\n";
        return 1;
    }

    std::cout << "Mathematics Gift Shop shutters hold.\n";
    return 0;
}
