#!/usr/bin/env python3
"""One-shot branch helper: make shared-Law references target-closure-local first.

This file is temporary patch machinery for the regression branch. It mutates
only src/ZonesOfEarth/ZoneManager.cpp and is deleted before the PR is opened.
"""
from pathlib import Path

path = Path("src/ZonesOfEarth/ZoneManager.cpp")
text = path.read_text()
start_marker = "        const auto resolveReference = [&](const std::string& id,"
end_marker = "\n\n        try {"
start = text.index(start_marker)
end = text.index(end_marker, start)

replacement = r'''        const auto resolveReference = [&](const std::string& id,
                                          bool preferPerson) -> Singular* {
            if (id.empty()) return nullptr;

            const auto appendUnique = [](std::vector<Singular*>& candidates, Singular* being) {
                if (being && std::find(candidates.begin(), candidates.end(), being) == candidates.end()) {
                    candidates.push_back(being);
                }
            };
            const auto identifierMatches = [&](Singular* being) {
                if (!being) return false;
                if (auto* person = dynamic_cast<Person*>(being)) {
                    return person->matchesIdentifier(id);
                }
                return being->getIdentifier() == id;
            };

            // Authorship can name a Person or a declared model-author Object.
            // Prefer an actual Person when one answers the identifier so a
            // legacy Object with the same slug can never impersonate them.
            if (preferPerson) {
                std::vector<Singular*> people;
                for (Singular* being : Universe::instance().beings()) {
                    if (dynamic_cast<Person*>(being) && identifierMatches(being)) {
                        appendUnique(people, being);
                    }
                }
                if (people.size() == 1) return people.front();
                if (people.size() > 1) return nullptr;
            }

            // The Law roots named by targetZone->lawRefs belong to the closure
            // being preflighted. Resolve unqualified references in that closure
            // first. Forked Zones intentionally preserve author-marker identity
            // (for example SynthesisStudio and its Living fork both carry the
            // same historical model-author marker); a sibling copy must not make
            // the target Zone's own referent ambiguous.
            std::vector<Singular*> local;
            if (identifierMatches(targetZone.get())) appendUnique(local, targetZone.get());
            for (const auto& object : targetZone->getOwnedObjects()) {
                if (identifierMatches(object.get())) appendUnique(local, object.get());
            }
            if (local.size() == 1) return local.front();
            if (local.size() > 1) return nullptr;

            // Shared/global beings are the next lexical scope. When authorship
            // already checked Persons above, do not let the same Person appear a
            // second time in this fallback scope.
            std::vector<Singular*> shared;
            for (Singular* being : Universe::instance().beings()) {
                if (preferPerson && dynamic_cast<Person*>(being)) continue;
                if (identifierMatches(being)) appendUnique(shared, being);
            }
            if (shared.size() == 1) return shared.front();
            if (shared.size() > 1) return nullptr;

            // Compatibility fallback for older cross-Zone references. Keep the
            // old reach, but only after the target closure and shared Universe
            // fail to answer; genuinely ambiguous sibling identifiers still fail
            // closed instead of picking one arbitrarily.
            std::vector<Singular*> siblings;
            for (const auto& zone : _zones) {
                if (!zone || zone.get() == targetZone.get()) continue;
                if (identifierMatches(zone.get())) appendUnique(siblings, zone.get());
                for (const auto& object : zone->getOwnedObjects()) {
                    if (identifierMatches(object.get())) appendUnique(siblings, object.get());
                }
            }
            return siblings.size() == 1 ? siblings.front() : nullptr;
        };'''

text = text[:start] + replacement + text[end:]
old_author = 'Singular* author = resolveReference(authorJson.get<std::string>(), false);'
new_author = 'Singular* author = resolveReference(authorJson.get<std::string>(), true);'
if old_author not in text:
    raise SystemExit("expected author-resolution call not found")
text = text.replace(old_author, new_author, 1)
path.write_text(text)
