import re

with open('src/ZonesOfEarth/AuthorsOfLaw/Law.cpp', 'r') as f:
    cpp = f.read()

bad = """std::vector<Singular*> ReteNetwork::collectTerminalSubjects(
    const std::vector<std::size_t>& terminalIds) const {
    // Collect unique subjects from terminal node memories.
    // Terminal IDs may refer to alpha or beta nodes — we check both.
    std::unordered_set<Singular*> seen;
    std::vector<Singular*> result;

    for (std::size_t termId : terminalIds) {
        // Check alpha nodes first.
        const AlphaNode* alpha = findAlpha(termId);
        if (alpha) {
            for (const auto& fact : alpha->memory) {
                if (fact->subject && seen.insert(fact->subject).second) {
                    result.push_back(fact->subject);
                }
            }
            continue;
        }
        // Check beta nodes.
        for (const auto& beta : _betaNodes) {
            if (beta.id != termId) continue;
            for (const auto& token : beta.memory) {
                for (const auto& fact : token.facts) {
                    if (fact->subject && seen.insert(fact->subject).second) {
                        result.push_back(fact->subject);
                    }
                }
            }
        }
    }
    return result;
}"""

good = """std::vector<Singular*> ReteNetwork::collectTerminalSubjects(
    const std::vector<std::size_t>& terminalIds) const {
    std::vector<Singular*> result;

    for (std::size_t termId : terminalIds) {
        if (const AlphaNode* alpha = findAlpha(termId)) {
            for (const auto& fact : alpha->memory) {
                if (fact->subject) result.push_back(fact->subject);
            }
            continue;
        }
        for (const auto& beta : _betaNodes) {
            if (beta.id != termId) continue;
            for (const auto& token : beta.memory) {
                for (const auto& fact : token.facts) {
                    if (fact->subject) result.push_back(fact->subject);
                }
            }
        }
    }
    
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}"""

cpp = cpp.replace(bad, good)

with open('src/ZonesOfEarth/AuthorsOfLaw/Law.cpp', 'w') as f:
    f.write(cpp)
