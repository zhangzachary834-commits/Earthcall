import re

with open('src/ZonesOfEarth/AuthorsOfLaw/Law.cpp', 'r') as f:
    content = f.read()

content = content.replace('law.rememberOnset(subjectId, now);', 'if (Singular* subject = findBeing(subjectId)) law.rememberOnset(subject, now);')
content = content.replace('law.rememberOnset(subjectId, onset);', 'if (Singular* subject = findBeing(subjectId)) law.rememberOnset(subject, onset);')

content = content.replace('if (law) law->forgetOnset(it->subjectId);', 'if (law && subject) law->forgetOnset(subject);')
content = content.replace('law->forgetOnset(it->subjectId);', 'law->forgetOnset(subject);')
content = content.replace('law->rememberOnset(it->subjectId, it->onset);', 'law->rememberOnset(subject, it->onset);')

with open('src/ZonesOfEarth/AuthorsOfLaw/Law.cpp', 'w') as f:
    f.write(content)
