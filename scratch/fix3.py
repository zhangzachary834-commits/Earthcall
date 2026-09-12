import re

with open('src/ZonesOfEarth/AuthorsOfLaw/Law.cpp', 'r') as f:
    cpp = f.read()

bad_find_2 = """        Singular* subject = nullptr;
        for (Singular* being : Universe::instance().beings()) {
            if (being && being->getIdentifier() == subjectId) { subject = being; break; }
        }
        if (subject) {
        law.rememberOnset(subject, onset);
    }"""
good_find_2 = """    law.rememberOnset(&subject, onset);"""

cpp = cpp.replace(bad_find_2, good_find_2)

with open('src/ZonesOfEarth/AuthorsOfLaw/Law.cpp', 'w') as f:
    f.write(cpp)
