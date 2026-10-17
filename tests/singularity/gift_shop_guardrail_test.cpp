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

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <regex>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

// Match C++ call syntax rather than literal substrings: spaces, newlines,
// stripped comments, and optional explicit template arguments must not reopen
// the GLM gift shop. A prefix-family match still requires an actual call.
const std::regex kSemanticCallPattern(
    R"(\bglm\s*::\s*(inverse|transpose|determinant|translate|rotate|scale|lookAt|perspective[A-Za-z0-9_]*|frustum[A-Za-z0-9_]*|ortho[A-Za-z0-9_]*)\s*(?:<[^();]*>)?\s*\()"
);

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
    // Guard the guard: whitespace and template spelling must not evade it,
    // while longer unrelated identifiers must not be mistaken for calls.
    for (const char* sample : {
             "glm::inverse (M)", "glm :: transpose\n (M)",
             "glm::inverse<glm::mat4>(M)", "glm::perspectiveFov (fov)",
             "glm::frustumRH_ZO (bounds)", "glm::orthoZO (bounds)"}) {
        if (!std::regex_search(sample, kSemanticCallPattern)) {
            std::cerr << "gift_shop_guardrail_test: matcher missed: "
                      << sample << "\n";
            return 2;
        }
    }
    for (const char* sample : {
             "glm::inverseness(M)", "xglm::inverse(M)",
             "glm::normalize(M)"}) {
        if (std::regex_search(sample, kSemanticCallPattern)) {
            std::cerr << "gift_shop_guardrail_test: false match: "
                      << sample << "\n";
            return 2;
        }
    }
    bool probeBlockComment = false;
    if (!std::regex_search(codeOnly("glm::inverse /* gap */ (M)",
                                    probeBlockComment), kSemanticCallPattern) ||
        std::regex_search(codeOnly("\"glm::inverse (M)\"",
                                        probeBlockComment), kSemanticCallPattern)) {
        std::cerr << "gift_shop_guardrail_test: lexical stripping regression\n";
        return 2;
    }

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
        std::string code;
        std::vector<std::string> originalLines;
        while (std::getline(input, line)) {
            originalLines.push_back(line);
            code += codeOnly(line, inBlockComment);
            code += '\n';
        }

        for (std::sregex_iterator hit(code.begin(), code.end(),
                                      kSemanticCallPattern), end;
             hit != end; ++hit) {
            if (allowedReason) {
                ++allowedHits;
                continue;
            }

            ++violations;
            const std::size_t lineNumber = 1 + std::count(
                code.cbegin(), code.cbegin() + hit->position(), '\n');
            std::cout << "GIFT_SHOP_VIOLATION " << relative << ":"
                      << lineNumber << " " << (*hit)[1].str() << "\n"
                      << "  " << originalLines.at(lineNumber - 1) << "\n";
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
