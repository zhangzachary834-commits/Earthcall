import re

with open('src/ZonesOfEarth/AuthorsOfLaw/Law.cpp', 'r') as f:
    cpp = f.read()

bad = "        // _applicationLog disabled for test"
good = """        ApplicationRecord record;
        record.worldTime = onsetScope ? onsetScope->onset : 0.0;
        record.subjectId = target.getIdentifier();
        record.targetId = target.getIdentifier();
        record.actions = std::move(*traceScope.trace);
        
        _applicationLog.push_back(std::move(record));
        if (_applicationLog.size() > 50) {
            _applicationLog.erase(_applicationLog.begin(), _applicationLog.begin() + 10);
        }"""

cpp = cpp.replace(bad, good)

with open('src/ZonesOfEarth/AuthorsOfLaw/Law.cpp', 'w') as f:
    f.write(cpp)
