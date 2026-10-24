// Headless regression for the LawManager drive-session key index.
// The full time_flow_test requires a GLFW window and cannot execute on
// headless macOS Actions runners; this exercises the actual LawManager
// event/drive lifecycle without creating a graphics context.
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <utility>

namespace {
OntoMath::Piecewise boundedDrive() {
    OntoMath::Piecewise function;
    function.inputVariable = "t";
    OntoMath::Piecewise::Piece piece;
    piece.hasLo = true;
    piece.lo = 0.0;
    piece.hasHi = true;
    piece.hi = 2.0;
    piece.mathNode = OntoMath::MathNode::fromLegacyExpression(
        OntoMath::ScalarForm::variable("t", 1.0, 2.0));
    function.pieces.push_back(std::move(piece));
    return function;
}
void publishKick(Object& target) {
    Core::EventBus::instance().publish(
        ECA::Event{"drive-key-test-kick", &target, nullptr, std::time(nullptr)});
}
} // namespace

int main() {
    Object author;
    Object target;
    target.setPosition(glm::vec3(0.0f, 9.0f, 0.0f));
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&target);
    });

    LawManager manager;
    manager.connectToEventBus();
    auto law = manager.createLaw("drive-key-index", {&author});
    assert(law);
    law->setDrives(true);
    law->setRetrigger(Law::Retrigger::Absorb);
    law->setActionModel(ActionNode::map(
        "position.y", boundedDrive(),
        MathBindings{{"t", PropertyPath::parse("time.sinceApplied")}}));
    const std::size_t alpha = manager.rete().addAlphaNode(
        "type == drive-key-test-kick",
        [](const FactPtr& fact) { return fact->type == "drive-key-test-kick"; });
    manager.rete().bindLawToAlpha(law->getIdentifier(), alpha);

    const std::string lawId = law->getIdentifier();
    const std::string subjectId = target.getIdentifier();
    assert(!manager.hasDriveSession(lawId, subjectId));

    Universe::instance().setClock(100.0, 0.1);
    publishKick(target);
    manager.tick();
    assert(manager.hasDriveSession(lawId, subjectId));
    assert(!manager.hasDriveSession("another-law", subjectId));
    assert(!manager.hasDriveSession(lawId, "another-subject"));
    assert(manager.driveSessions().size() == 1);

    // Repeated events are absorbed, rather than duplicating the index entry.
    Universe::instance().setClock(101.0, 0.1);
    publishKick(target);
    manager.tick();
    assert(manager.driveSessions().size() == 1);
    assert(manager.hasDriveSession(lawId, subjectId));
    assert(std::fabs(target.getPosition().y - 2.0f) < 1e-4f);

    // Leaving the authored time domain removes both session and key.
    Universe::instance().setClock(103.0, 0.1);
    manager.tick();
    assert(manager.driveSessions().empty());
    assert(!manager.hasDriveSession(lawId, subjectId));

    // A new event reuses the same law/subject key after completion.
    Universe::instance().setClock(200.0, 0.1);
    publishKick(target);
    manager.tick();
    assert(manager.hasDriveSession(lawId, subjectId));
    assert(manager.driveSessions().size() == 1);

    // Disabling a law removes its active key.
    law->setEnabled(false);
    Universe::instance().setClock(201.0, 0.1);
    manager.tick();
    assert(manager.driveSessions().empty());
    assert(!manager.hasDriveSession(lawId, subjectId));

    law->setEnabled(true);
    Universe::instance().setClock(300.0, 0.1);
    publishKick(target);
    manager.tick();
    assert(manager.hasDriveSession(lawId, subjectId));

    // Removing a law must remove its drive-session key as well.
    assert(manager.remove(lawId));
    assert(manager.driveSessions().empty());
    assert(!manager.hasDriveSession(lawId, subjectId));

    Universe::instance().setProvider({});
    std::puts("drive_session_key_test: PASS");
    return 0;
}
