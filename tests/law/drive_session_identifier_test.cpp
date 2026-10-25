// Headless behavioral witness for the LawManager drive-session identifier paths.
// Exercises the real event/Rete/tick pipeline for Absorb and Restart.
// Unlike time_flow_test, no graphical context is needed.
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <utility>
#include <vector>

namespace {
OntoMath::Piecewise boundedInT(OntoMath::ScalarForm expression, double lo, double hi) {
    OntoMath::Piecewise result;
    result.inputVariable = "t";
    OntoMath::Piecewise::Piece piece;
    piece.hasLo = true;
    piece.lo = lo;
    piece.hasHi = true;
    piece.hi = hi;
    piece.mathNode = OntoMath::MathNode::fromLegacyExpression(std::move(expression));
    result.pieces.push_back(piece);
    return result;
}

bool near(float lhs, float rhs) {
    return std::fabs(lhs - rhs) < 1e-4f;
}
} // namespace

int main() {
    Object author;
    Object subject;
    subject.setPosition(glm::vec3(3.0f, 0.0f, 0.0f));
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&subject);
    });

    LawManager manager;
    manager.connectToEventBus();
    const MathBindings timeBinding{{"t", PropertyPath::parse("time.sinceApplied")}};
    const std::size_t kickAlpha = manager.rete().addAlphaNode(
        "type == kick", [](const FactPtr& fact) { return fact->type == "kick"; });
    const auto kick = [&] {
        Core::EventBus::instance().publish(
            ECA::Event{"kick", &subject, nullptr, std::time(nullptr)});
    };

    // Absorb must reuse one session and integrate only once per tick, even
    // when the triggering event is published repeatedly.
    auto absorb = manager.createLaw("drive-absorb-identifier", {&author});
    absorb->setDrives(true);
    absorb->setActionModel(ActionNode::flow(
        "position.y", boundedInT(OntoMath::ScalarForm::constant(1.0), 0.0, 10.0),
        timeBinding));
    manager.rete().bindLawToAlpha(absorb->getIdentifier(), kickAlpha);

    double now = 100.0;
    Universe::instance().setClock(now, 1.0);
    kick();
    manager.tick();
    for (int i = 0; i < 3; ++i) {
        now += 1.0;
        Universe::instance().setClock(now, 1.0);
        kick();
        manager.tick();
    }
    assert(near(subject.getPosition().y, 4.0f));
    assert(manager.driveSessions().size() == 1);
    absorb->setEnabled(false);
    Universe::instance().setClock(now + 1.0, 1.0);
    manager.tick();
    assert(manager.driveSessions().empty());

    // Restart must preserve one session but reset its onset to the new event.
    subject.setPosition(glm::vec3(3.0f, 9.0f, 0.0f));
    auto restart = manager.createLaw("drive-restart-identifier", {&author});
    restart->setDrives(true);
    restart->setRetrigger(Law::Retrigger::Restart);
    restart->setActionModel(ActionNode::map(
        "position.y", boundedInT(OntoMath::ScalarForm::variable("t", 1.0, 2.0),
                                  0.0, 5.0),
        timeBinding));
    manager.rete().bindLawToAlpha(restart->getIdentifier(), kickAlpha);

    Universe::instance().setClock(200.0, 1.0);
    kick();
    manager.tick();
    Universe::instance().setClock(202.0, 1.0);
    manager.tick();
    assert(near(subject.getPosition().y, 4.0f));
    Universe::instance().setClock(203.0, 1.0);
    kick();
    manager.tick();
    assert(near(subject.getPosition().y, 0.0f));
    assert(manager.driveSessions().size() == 1);

    restart->setEnabled(false);
    Universe::instance().setClock(204.0, 1.0);
    manager.tick();
    assert(manager.driveSessions().empty());
    Universe::instance().setProvider({});
    std::puts("drive_session_identifier_test: Absorb and Restart PASS");
    return 0;
}
