// Focused contract for LawManager drive-session indexing and cleanup.
//
// This is deliberately an OnEvent test: publishing the same event twice while
// a drive is alive forces the hot duplicate-prevention path to consult
// hasDriveSession(lawId, subjectId). It also checks the two cleanup seams most
// likely to leave the derived key index stale: law removal and subject release.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <cassert>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

namespace {

int gChecks = 0;

void check(bool condition, const std::string& description) {
    ++gChecks;
    if (!condition) {
        std::cerr << "FAILED: " << description << '\n';
        assert(condition);
    }
}

} // namespace

int main() {
    Object author;
    author.setObjectID("drive-index-author");

    Object subject;
    subject.setObjectID("drive-index-subject");
    subject.setDynamicProperty("glow", PropertyValue(0.0));

    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&author);
        beings.push_back(&subject);
    });
    Universe::instance().setClock(10.0, 0.1);

    LawManager mgr;
    mgr.connectToEventBus();

    const std::size_t pulseAlpha = mgr.rete().addAlphaNode(
        "type == drive-index-pulse",
        [](const FactPtr& fact) { return fact && fact->type == "drive-index-pulse"; });

    const std::string lawId = "law-drive-index-01";
    const std::string subjectId = subject.getIdentifier();
    auto law = mgr.createLaw("Drive index contract", lawId, {&author});
    law->setDrives(true);
    law->setActionModel(ActionNode::set("glow", PropertyValue(1.0)));
    mgr.rete().bindLawToAlpha(lawId, pulseAlpha);

    check(!mgr.hasDriveSession(lawId, subjectId),
          "no derived drive key exists before the authored event");

    Core::EventBus::instance().publish(
        ECA::Event{"drive-index-pulse", &subject, nullptr, std::time(nullptr)});
    mgr.tick();
    check(mgr.hasDriveSession(lawId, subjectId),
          "first event creates the indexed drive session");
    check(mgr.driveSessions().size() == 1,
          "first event creates exactly one drive session");

    Universe::instance().setClock(11.0, 0.1);
    Core::EventBus::instance().publish(
        ECA::Event{"drive-index-pulse", &subject, nullptr, std::time(nullptr)});
    mgr.tick();
    check(mgr.hasDriveSession(lawId, subjectId),
          "repeated event finds the existing indexed drive session");
    check(mgr.driveSessions().size() == 1,
          "Absorb retrigger does not duplicate the drive session");

    check(mgr.remove(lawId), "driving law can be removed");
    check(!mgr.hasDriveSession(lawId, subjectId),
          "law removal erases the derived drive key");
    check(mgr.driveSessions().empty(),
          "law removal erases the drive session");

    const std::string lawId2 = "law-drive-index-02";
    auto law2 = mgr.createLaw("Drive subject-release contract", lawId2, {&author});
    law2->setDrives(true);
    law2->setActionModel(ActionNode::set("glow", PropertyValue(2.0)));
    mgr.rete().bindLawToAlpha(lawId2, pulseAlpha);

    Universe::instance().setClock(20.0, 0.1);
    Core::EventBus::instance().publish(
        ECA::Event{"drive-index-pulse", &subject, nullptr, std::time(nullptr)});
    mgr.tick();
    check(mgr.hasDriveSession(lawId2, subjectId),
          "second law creates an indexed drive session");

    Core::EventBus::instance().publish(
        ECA::Event{"object-destroyed", &subject, nullptr, std::time(nullptr)});
    check(!mgr.hasDriveSession(lawId2, subjectId),
          "subject release erases the derived drive key");
    check(mgr.driveSessions().empty(),
          "subject release erases the drive session");

    Universe::instance().setProvider({});
    std::cout << "law_manager_drive_session_test: " << gChecks << " checks passed\n";
    return 0;
}
