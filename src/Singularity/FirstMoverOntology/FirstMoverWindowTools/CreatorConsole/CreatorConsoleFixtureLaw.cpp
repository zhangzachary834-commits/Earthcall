#include "CreatorConsoleFixtureLaw.hpp"

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

namespace Rendering {

std::shared_ptr<Law> createCreatorConsoleFixtureLaw(Singular& author,
                                                    bool initiallyVisible) {
    auto law = std::make_shared<FirstMoverLaw>("Fixture: Creator Console Visibility");
    law->setLawIdentifier(kCreatorConsoleFixtureLawId);
    law->addAuthor(author);
    law->setEnabled(initiallyVisible);
    return law;
}

Law* syncRegisterCreatorConsoleFixtureLaw(LawManager& laws,
                                          Singular& author,
                                          bool initiallyVisible) {
    if (Law* existing = laws.find(kCreatorConsoleFixtureLawId)) return existing;

    auto law = createCreatorConsoleFixtureLaw(author, initiallyVisible);
    Law* raw = law.get();
    laws.add(std::move(law));
    return raw;
}

void reconcileCreatorConsoleFixtureVisibility(Law& fixtureLaw,
                                              bool& consoleOpen,
                                              bool& lastSyncedVisible) {
    const bool fixtureVisible = fixtureLaw.isEnabled();

    // If the law changed since our last agreement, that is an authored change:
    // Law Author (or another law) gets to move the native fixture.
    if (fixtureVisible != lastSyncedVisible) {
        consoleOpen = fixtureVisible;
        lastSyncedVisible = fixtureVisible;
        return;
    }

    // Otherwise the native chrome changed (F8/menu/dock/tool entry point), so
    // carry that gesture back into the same legible law rather than letting a
    // second hidden visibility truth diverge from it.
    if (consoleOpen != lastSyncedVisible) {
        fixtureLaw.setEnabled(consoleOpen);
        lastSyncedVisible = consoleOpen;
    }
}

} // namespace Rendering
