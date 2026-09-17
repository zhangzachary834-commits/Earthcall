import re
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "r") as f:
    content = f.read()
content = content.replace(
    'std::vector<Singular*> subjects = _rete.collectTerminalSubjects(termIds);',
    'std::vector<Singular*> subjects = _rete.collectTerminalSubjects(termIds);\n            std::cout << "Law " << lawId << " terminal subjects count: " << subjects.size() << "\\n";'
).replace(
    'const Law::ApplicationResult result =\n                        applyAndMaybeDrive(*law, *subject, records);',
    'const Law::ApplicationResult result =\n                        applyAndMaybeDrive(*law, *subject, records);\n                    std::cout << "Law " << lawId << " applied to " << subjectId << ", result: " << (int)result << "\\n";'
).replace(
    'if (law->conditionsSatisfied(*subject)) matching.insert(subject);',
    'if (law->conditionsSatisfied(*subject)) { std::cout << lawId << " conditionsSatisfied!\\n"; matching.insert(subject); } else { std::cout << lawId << " NOT satisfied!\\n"; }'
)
with open("src/ZonesOfEarth/AuthorsOfLaw/Law.cpp", "w") as f:
    f.write(content)
