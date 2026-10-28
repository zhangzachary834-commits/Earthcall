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
#include <chrono>
#include <memory>
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


    // Headless, high-cardinality production-path witness. Unlike time_flow_test,
    // this executable requires no GLFW window or graphical runner.
    check(mgr.remove(lawId2), "retired subject-release law removed before bulk trial");
    constexpr std::size_t kSubjects = 256;
    std::vector<std::unique_ptr<Object>> crowd;
    crowd.reserve(kSubjects);
    for (std::size_t i = 0; i < kSubjects; ++i) {
        auto being = std::make_unique<Object>();
        being->setObjectID("drive-bulk-" + std::to_string(i));
        being->setDynamicProperty("glow", PropertyValue(0.0));
        crowd.push_back(std::move(being));
    }
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&author);
        for (const auto& being : crowd) beings.push_back(being.get());
    });

    const std::string bulkLawId = "law-drive-index-bulk";
    auto bulk = mgr.createLaw("Drive high-cardinality contract", bulkLawId, {&author});
    bulk->setDrives(true);
    bulk->setActionModel(ActionNode::set("glow", PropertyValue(1.0)));
    mgr.rete().bindLawToAlpha(bulkLawId, pulseAlpha);

    auto fireCrowd = [&]() {
        for (const auto& being : crowd) {
            Core::EventBus::instance().publish(
                ECA::Event{"drive-index-pulse", being.get(), nullptr, std::time(nullptr)});
        }
        mgr.tick();
    };

    Universe::instance().setClock(100.0, 0.1);
    fireCrowd();
    check(mgr.driveSessions().size() == kSubjects,
          "256 distinct subjects launch exactly 256 drive sessions");
    for (const auto& being : crowd) {
        check(mgr.hasDriveSession(bulkLawId, being->getIdentifier()),
              "every high-cardinality subject has its indexed key");
    }

    // Same-process diagnostic only: correctness is gated, timings are not.
    constexpr int kRounds = 200;
    std::size_t indexedHits = 0;
    const auto indexedBegin = std::chrono::steady_clock::now();
    for (int round = 0; round < kRounds; ++round) {
        for (const auto& being : crowd) {
            indexedHits += mgr.hasDriveSession(bulkLawId, being->getIdentifier()) ? 1u : 0u;
        }
    }
    const auto indexedEnd = std::chrono::steady_clock::now();
    std::size_t linearHits = 0;
    const auto linearBegin = std::chrono::steady_clock::now();
    for (int round = 0; round < kRounds; ++round) {
        for (const auto& being : crowd) {
            const std::string id = being->getIdentifier();
            bool found = false;
            for (const auto& session : mgr.driveSessions()) {
                if (session.lawId == bulkLawId && session.subjectId == id) {
                    found = true;
                    break;
                }
            }
            linearHits += found ? 1u : 0u;
        }
    }
    const auto linearEnd = std::chrono::steady_clock::now();
    check(indexedHits == linearHits &&
              indexedHits == kSubjects * static_cast<std::size_t>(kRounds),
          "indexed and legacy linear membership return identical hits");
    const double indexedUs = std::chrono::duration<double, std::micro>(
        indexedEnd - indexedBegin).count();
    const double linearUs = std::chrono::duration<double, std::micro>(
        linearEnd - linearBegin).count();
    std::cout << "DRIVE_SESSION_LOOKUP_AB subjects=" << kSubjects
              << " lookups=" << indexedHits
              << " indexed_us=" << indexedUs
              << " linear_us=" << linearUs
              << " linear_over_indexed=" << (indexedUs > 0.0 ? linearUs / indexedUs : 0.0)
              << '\n';

    Universe::instance().setClock(101.0, 0.1);
    fireCrowd();
    check(mgr.driveSessions().size() == kSubjects,
          "Absorb preserves session cardinality under repeated events");
    for (const auto& session : mgr.driveSessions()) {
        check(session.onset == 100.0, "Absorb preserves original session onset");
    }

    bulk->setRetrigger(Law::Retrigger::Restart);
    Universe::instance().setClock(102.0, 0.1);
    fireCrowd();
    check(mgr.driveSessions().size() == kSubjects,
          "Restart reuses the existing session identities");
    for (const auto& session : mgr.driveSessions()) {
        check(session.onset == 102.0, "Restart resets every session onset");
    }

    bulk->setEnabled(false);
    Universe::instance().setClock(103.0, 0.1);
    mgr.tick();
    check(mgr.driveSessions().empty(), "disabling law reaps all 256 sessions");
    for (const auto& being : crowd) {
        check(!mgr.hasDriveSession(bulkLawId, being->getIdentifier()),
              "disabling law clears every indexed key");
    }

    bulk->setRetrigger(Law::Retrigger::Absorb);
    bulk->setEnabled(true);
    Universe::instance().setClock(200.0, 0.1);
    fireCrowd();
    check(mgr.driveSessions().size() == kSubjects,
          "re-enabled law can start 256 fresh sessions");
    check(mgr.remove(bulkLawId), "bulk law can be removed");
    check(mgr.driveSessions().empty(), "bulk law removal reaps all sessions");

    Universe::instance().setProvider({});
    std::cout << "law_manager_drive_session_test: " << gChecks << " checks passed\n";
    return 0;
}
