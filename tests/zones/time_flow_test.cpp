// Change-over-time milestone test.
//
// "User sets change over time": a Person authors an exact OntoMath model of
// HOW a property evolves — position form p := f(t) (Map) or rate form
// dp/dt = f(...) (Flow) — binds t to substrate time, and the law runs it.
// This test proves the world clock is legible (reserved "time" paths), that
// onset memory gives each law-and-subject its own t=0, that Flow integrates
// the authored rate, and that an OnEvent law reading time.sinceApplied
// becomes a DRIVE: it keeps applying after its event, for exactly as long as
// the authored Piecewise bounds allow, then announces "law-drive-finished".

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <cassert>
#include <cmath>
#include <cstdio>

namespace {

bool nearf(float a, float b, float eps = 1e-4f) { return std::fabs(a - b) < eps; }

OntoMath::Piecewise boundedInT(OntoMath::ScalarForm e, double lo, double hi) {
    OntoMath::Piecewise f;
    f.inputVariable = "t";
    OntoMath::Piecewise::Piece piece;
    piece.hasLo = true;
    piece.lo = lo;
    piece.hasHi = true;
    piece.hi = hi;
    piece.mathNode = OntoMath::MathNode::fromLegacyExpression(std::move(e));
    f.pieces.push_back(piece);
    return f;
}

OntoMath::Piecewise everywhereInT(OntoMath::ScalarForm e) {
    OntoMath::Piecewise f = OntoMath::Piecewise::continuous(OntoMath::MathNode::fromLegacyExpression(std::move(e)));
    f.inputVariable = "t";
    return f;
}

} // namespace

int main() {
    // CPU-only LawManager and OntoMath witness. Do not require a GLFW window:
    // hosted macOS CI runners have no interactive display/GL context, and
    // none of the assertions below exercise rendering.

    {
        Object author;
        Object a, b;
        a.setPosition(glm::vec3(0.0f, 5.0f, 0.0f));
        b.setPosition(glm::vec3(3.0f, 5.0f, 0.0f));

        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&a);
            beings.push_back(&b);
        });

        const MathBindings tBinding{{"t", PropertyPath::parse("time.sinceApplied")}};

        // ------------------------------------------------------------------
        // 1. The world clock is legible — and nobody writes time.
        // ------------------------------------------------------------------
        Universe::instance().setClock(42.5, 0.25);
        PropertyValue v;
        double x = 0.0;
        assert(lawGetValue(a, PropertyPath::parse("time"), v));
        assert(propertyValueToNumber(v, x) && nearf(static_cast<float>(x), 42.5f));
        assert(lawGetValue(a, PropertyPath::parse("time.delta"), v));
        assert(propertyValueToNumber(v, x) && nearf(static_cast<float>(x), 0.25f));
        // The clock is read-only, and now SAYS SO rather than just failing.
        assert(lawSetValue(a, PropertyPath::parse("time"), PropertyValue(0.0)) ==
               PropertyPath::PathResult::ReadOnly);
        // sinceApplied is defined only INSIDE a law application.
        assert(!lawGetValue(a, PropertyPath::parse("time.sinceApplied"), v));

        LawManager mgr;

        // ------------------------------------------------------------------
        // 2. Position form under WhileTrue: y := t^2 from the moment the
        //    condition takes hold — onset is t=0, release re-arms it.
        // ------------------------------------------------------------------
        auto parabola = mgr.createLaw("y-follows-t-squared", {&author});
        parabola->setActivation(Law::Activation::WhileTrue);
        parabola->addTarget(a);
        parabola->setConditionModel(ConditionNode::compare(
            "position.x", ConditionNode::Op::Gt, PropertyValue(1.0)));
        parabola->setActionModel(ActionNode::map(
            "position.y", everywhereInT(OntoMath::ScalarForm::variable("t", 2.0)),
            tBinding));

        Universe::instance().setClock(10.0, 0.1);
        mgr.tick();                                        // x=0: condition false
        assert(nearf(a.getPosition().y, 5.0f));            // untouched

        a.setPosition(glm::vec3(5.0f, 5.0f, 0.0f));        // condition takes hold
        Universe::instance().setClock(11.0, 0.1);
        mgr.tick();                                        // onset: t = 0
        assert(nearf(a.getPosition().y, 0.0f));
        Universe::instance().setClock(13.0, 0.1);
        mgr.tick();                                        // t = 2
        assert(nearf(a.getPosition().y, 4.0f));

        a.setPosition(glm::vec3(0.0f, a.getPosition().y, 0.0f));   // release
        Universe::instance().setClock(14.0, 0.1);
        mgr.tick();
        assert(nearf(a.getPosition().y, 4.0f));            // no writes while released

        a.setPosition(glm::vec3(5.0f, a.getPosition().y, 0.0f));   // re-hold
        Universe::instance().setClock(20.0, 0.1);
        mgr.tick();                                        // onset RESET: t = 0 again
        assert(nearf(a.getPosition().y, 0.0f));
        parabola->setEnabled(false);

        // ------------------------------------------------------------------
        // 3. Rate form: Flow integrates the authored dp/dt each tick.
        // ------------------------------------------------------------------
        b.setPosition(glm::vec3(3.0f, 0.0f, 0.0f));
        auto steady = mgr.createLaw("rise-three-per-second", {&author});
        steady->setActivation(Law::Activation::WhileTrue);
        steady->addTarget(b);
        steady->setConditionModel(ConditionNode::compare(
            "position.y", ConditionNode::Op::Lt, PropertyValue(1e9)));
        steady->setActionModel(ActionNode::flow(
            "position.y", everywhereInT(OntoMath::ScalarForm::constant(3.0)),
            tBinding));

        double now = 20.0;
        for (int i = 0; i < 4; ++i) {
            now += 0.5;
            Universe::instance().setClock(now, 0.5);
            mgr.tick();
        }
        assert(nearf(b.getPosition().y, 6.0f));            // 3 * 0.5 * 4, exact
        steady->setEnabled(false);

        // dp/dt = 2t sums to ~t^2 (Euler steps around the exact answer).
        b.setPosition(glm::vec3(3.0f, 0.0f, 0.0f));
        auto quadratic = mgr.createLaw("accelerate", {&author});
        quadratic->setActivation(Law::Activation::WhileTrue);
        quadratic->addTarget(b);
        quadratic->setConditionModel(ConditionNode::compare(
            "position.y", ConditionNode::Op::Lt, PropertyValue(1e9)));
        quadratic->setActionModel(ActionNode::flow(
            "position.y", everywhereInT(OntoMath::ScalarForm::variable("t", 1.0, 2.0)),
            tBinding));

        now = 100.0;
        Universe::instance().setClock(now, 0.1);
        mgr.tick();                                        // onset here: t = 0
        for (int i = 0; i < 10; ++i) {
            now += 0.1;
            Universe::instance().setClock(now, 0.1);
            mgr.tick();
        }
        // exact integral over [0,1] is 1; forward steps of 0.1 land nearby
        assert(std::fabs(b.getPosition().y - 1.0) < 0.15);
        quadratic->setEnabled(false);

        // ------------------------------------------------------------------
        // 4. The drive: ONE event, motion that OUTLIVES it, ending exactly
        //    where the authored bounds end — then "law-drive-finished".
        // ------------------------------------------------------------------
        mgr.connectToEventBus();

        bool driveFinished = false;
        Core::EventBus::instance().subscribe<ECA::Event>([&](const ECA::Event& e) {
            if (e.type == "law-drive-finished") driveFinished = true;
        });

        a.setPosition(glm::vec3(5.0f, 9.0f, 0.0f));
        auto arc = mgr.createLaw("kick-arc", {&author});   // OnEvent (default)
        arc->setDrives(true);                              // the AUTHORED choice
        arc->setActionModel(ActionNode::map(
            "position.y",
            boundedInT(OntoMath::ScalarForm::variable("t", 1.0, 2.0), 0.0, 2.0),
            tBinding));                                    // y := 2t for t in [0,2]
        const std::size_t alphaKick = mgr.rete().addAlphaNode(
            "type == kick", [](const FactPtr& f) { return f->type == "kick"; });
        mgr.rete().bindLawToAlpha(arc->getIdentifier(), alphaKick);

        Universe::instance().setClock(200.0, 0.1);
        Core::EventBus::instance().publish(
            ECA::Event{"kick", &a, nullptr, std::time(nullptr)});
        mgr.tick();                                        // the event: t = 0
        assert(nearf(a.getPosition().y, 0.0f));
        assert(mgr.driveSessions().size() == 1);

        Universe::instance().setClock(201.0, 0.1);
        mgr.tick();                                        // NO event — still moving
        assert(nearf(a.getPosition().y, 2.0f));
        Universe::instance().setClock(202.0, 0.1);
        mgr.tick();                                        // t = 2: the last moment
        assert(nearf(a.getPosition().y, 4.0f));
        assert(!driveFinished);

        Universe::instance().setClock(203.0, 0.1);
        mgr.tick();                                        // t = 3: beyond the bounds
        assert(nearf(a.getPosition().y, 4.0f));            // motion ENDED, value kept
        assert(driveFinished);                             // and the world was told
        assert(mgr.driveSessions().empty());

        // A second kick starts a fresh drive from t = 0.
        driveFinished = false;
        Universe::instance().setClock(300.0, 0.1);
        Core::EventBus::instance().publish(
            ECA::Event{"kick", &a, nullptr, std::time(nullptr)});
        mgr.tick();
        assert(nearf(a.getPosition().y, 0.0f));
        assert(mgr.driveSessions().size() == 1);
        arc->setEnabled(false);
        Universe::instance().setClock(301.0, 0.1);
        mgr.tick();                                        // disabled law: session ends
        assert(mgr.driveSessions().empty());

        // ------------------------------------------------------------------
        // 5. ANY variable can be the drive's domain — time is one input
        //    among the rest. Here the drive follows ANOTHER BEING's x, and
        //    ends when THAT leaves the authored bounds.
        // ------------------------------------------------------------------
        driveFinished = false;
        a.setPosition(glm::vec3(2.0f, 0.0f, 0.0f));
        b.setPosition(glm::vec3(3.0f, 0.0f, 0.0f));

        OntoMath::Piecewise followF;                       // f(x) = x for x in [0,10]
        followF.inputVariable = "x";
        {
            OntoMath::Piecewise::Piece piece;
            piece.hasLo = true;
            piece.lo = 0.0;
            piece.hasHi = true;
            piece.hi = 10.0;
            piece.mathNode = OntoMath::MathNode::fromLegacyExpression(OntoMath::ScalarForm::variable("x"));
            followF.pieces.push_back(piece);
        }
        auto follow = mgr.createLaw("follow-the-other", {&author});
        follow->setDrives(true);
        follow->setActionModel(ActionNode::map(
            "position.y", followF,
            MathBindings{{"x", PropertyPath::parse(
                "@" + a.getIdentifier() + ".position.x")}}));
        mgr.rete().bindLawToAlpha(follow->getIdentifier(), alphaKick);

        Universe::instance().setClock(400.0, 0.1);
        Core::EventBus::instance().publish(
            ECA::Event{"kick", &b, nullptr, std::time(nullptr)});
        mgr.tick();
        assert(nearf(b.getPosition().y, 2.0f));            // y := a.x at the event
        assert(mgr.driveSessions().size() == 1);

        a.setPosition(glm::vec3(7.0f, 0.0f, 0.0f));        // the INPUT moves
        Universe::instance().setClock(401.0, 0.1);
        mgr.tick();                                        // no event — still tracking
        assert(nearf(b.getPosition().y, 7.0f));

        a.setPosition(glm::vec3(20.0f, 0.0f, 0.0f));       // input leaves the bounds
        Universe::instance().setClock(402.0, 0.1);
        mgr.tick();
        assert(nearf(b.getPosition().y, 7.0f));            // drive ENDED, value kept
        assert(driveFinished);                             // announced, same as time
        assert(mgr.driveSessions().empty());
        follow->setEnabled(false);

        // ------------------------------------------------------------------
        // 6. Re-triggering while a drive runs is ABSORBED: one process, one
        //    clock, per law-and-subject. A constantly-publishing event (a
        //    block resting in collision) can neither stack independent
        //    copies of the process nor double-integrate a Flow.
        // ------------------------------------------------------------------
        b.setPosition(glm::vec3(3.0f, 0.0f, 0.0f));
        auto climb = mgr.createLaw("climb-once", {&author});
        climb->setDrives(true);
        climb->setActionModel(ActionNode::flow(
            "position.y", boundedInT(OntoMath::ScalarForm::constant(1.0), 0.0, 10.0),
            tBinding));                                    // dy/dt = 1 for t in [0,10]
        mgr.rete().bindLawToAlpha(climb->getIdentifier(), alphaKick);

        double clock6 = 600.0;
        Universe::instance().setClock(clock6, 1.0);
        Core::EventBus::instance().publish(
            ECA::Event{"kick", &b, nullptr, std::time(nullptr)});
        mgr.tick();                                        // launch: one step
        for (int i = 0; i < 3; ++i) {
            clock6 += 1.0;
            Universe::instance().setClock(clock6, 1.0);
            Core::EventBus::instance().publish(            // the event KEEPS firing
                ECA::Event{"kick", &b, nullptr, std::time(nullptr)});
            mgr.tick();                                    // absorbed: ONE step per tick
        }
        assert(nearf(b.getPosition().y, 4.0f));            // 4 single steps, never doubled
        assert(mgr.driveSessions().size() == 1);           // and never a second process
        climb->setEnabled(false);
        Universe::instance().setClock(clock6 + 1.0, 1.0);
        mgr.tick();
        assert(mgr.driveSessions().empty());

        // Retrigger is AUTHORED vocabulary: with Restart, a re-kick mid-arc
        // is a NEW t = 0 — re-kick, re-arc.
        b.setPosition(glm::vec3(3.0f, 9.0f, 0.0f));
        auto rearc = mgr.createLaw("re-kick-re-arc", {&author});
        rearc->setDrives(true);
        rearc->setRetrigger(Law::Retrigger::Restart);
        rearc->setActionModel(ActionNode::map(
            "position.y",
            boundedInT(OntoMath::ScalarForm::variable("t", 1.0, 2.0), 0.0, 5.0),
            tBinding));                                    // y := 2t, t in [0,5]
        mgr.rete().bindLawToAlpha(rearc->getIdentifier(), alphaKick);

        Universe::instance().setClock(700.0, 1.0);
        Core::EventBus::instance().publish(
            ECA::Event{"kick", &b, nullptr, std::time(nullptr)});
        mgr.tick();                                        // t = 0 -> y = 0
        Universe::instance().setClock(702.0, 1.0);
        mgr.tick();                                        // t = 2 -> y = 4
        assert(nearf(b.getPosition().y, 4.0f));
        Core::EventBus::instance().publish(                // re-kick MID-ARC
            ECA::Event{"kick", &b, nullptr, std::time(nullptr)});
        Universe::instance().setClock(703.0, 1.0);
        mgr.tick();                                        // restarted: t = 0 -> y = 0
        assert(nearf(b.getPosition().y, 0.0f));            // re-arc, not t = 3 -> 6
        assert(mgr.driveSessions().size() == 1);           // same ONE process, new clock
        rearc->setEnabled(false);
        Universe::instance().setClock(704.0, 1.0);
        mgr.tick();
        assert(mgr.driveSessions().empty());

        // The choice survives serialization.
        assert(Law::fromJson(rearc->toJson())->retrigger() == Law::Retrigger::Restart);

        // ------------------------------------------------------------------
        // 7. The model survives serialization; sessions are runtime-only.
        // ------------------------------------------------------------------
        const auto j = ActionNode::flow(
            "position.y", boundedInT(OntoMath::ScalarForm::variable("t"), 0.0, 5.0),
            tBinding).toJson();
        const ActionNode reborn = ActionNode::fromJson(j);
        assert(reborn.kind == ActionNode::Kind::Flow);
        assert(reborn.referencesSinceApplied());
        Universe::instance().setClock(500.0, 0.1);
        Universe::instance().setApplicationOnset(495.0);   // t = 5: the last moment
        assert(reborn.definedFor(a));
        Universe::instance().setApplicationOnset(494.5);   // t = 5.5: beyond
        assert(!reborn.definedFor(a));
        Universe::instance().clearApplicationOnset();

        auto rebornLaw = Law::fromJson(arc->toJson());
        assert(rebornLaw->drives());                       // the choice survives
        assert(rebornLaw->hasActionModel());
        assert(rebornLaw->actionModel()->referencesSinceApplied());

        // ------------------------------------------------------------------
        // 8. The event's PARTICIPANTS are addressable BY CHOICE: a collision
        //    has two, and the author may ask about or act on either — the
        //    action phase names its own referents. A drive remembers its
        //    launching participants for its whole life.
        // ------------------------------------------------------------------
        a.setPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        b.setPosition(glm::vec3(3.0f, 0.0f, 0.0f));

        auto onTheOther = mgr.createLaw("act-on-the-other", {&author});
        onTheOther->setConditionModel(ConditionNode::compare(
            "@event.subject.position.y", ConditionNode::Op::Lt, PropertyValue(1.0)));
        onTheOther->setActionModel(ActionNode::set(
            "@event.object.position.y", PropertyValue(33.0)));
        const std::size_t alphaTouch = mgr.rete().addAlphaNode(
            "type == touch", [](const FactPtr& f) { return f->type == "touch"; });
        mgr.rete().bindLawToAlpha(onTheOther->getIdentifier(), alphaTouch);

        Core::EventBus::instance().publish(
            ECA::Event{"touch", &a, &b, std::time(nullptr)});   // a touches b
        mgr.tick();
        assert(nearf(b.getPosition().y, 33.0f));   // the OTHER participant moved
        assert(nearf(a.getPosition().y, 0.0f));    // the subject did not
        onTheOther->setEnabled(false);

        // A drive on "@event.object": the participants stay addressable
        // across event-less ticks, for as long as the drive lives.
        driveFinished = false;
        b.setPosition(glm::vec3(3.0f, 0.0f, 0.0f));
        auto pursue = mgr.createLaw("lift-the-other-over-time", {&author});
        pursue->setDrives(true);
        pursue->setActionModel(ActionNode::map(
            "@event.object.position.y",
            boundedInT(OntoMath::ScalarForm::variable("t", 1.0, 2.0), 0.0, 2.0),
            tBinding));                                    // other.y := 2t, t in [0,2]
        mgr.rete().bindLawToAlpha(pursue->getIdentifier(), alphaTouch);

        Universe::instance().setClock(700.0, 0.1);
        Core::EventBus::instance().publish(
            ECA::Event{"touch", &a, &b, std::time(nullptr)});
        mgr.tick();                                        // t = 0
        assert(nearf(b.getPosition().y, 0.0f));
        Universe::instance().setClock(701.0, 0.1);
        mgr.tick();                                        // NO event — b still lifts
        assert(nearf(b.getPosition().y, 2.0f));
        Universe::instance().setClock(702.0, 0.1);
        mgr.tick();
        assert(nearf(b.getPosition().y, 4.0f));
        Universe::instance().setClock(703.0, 0.1);
        mgr.tick();                                        // bounds ended
        assert(nearf(b.getPosition().y, 4.0f));
        assert(driveFinished);
        assert(mgr.driveSessions().empty());
        pursue->setEnabled(false);


        // ------------------------------------------------------------------
        // 9. An index must not change the first-match identity contract.
        //    A provider can expose two distinct Objects with the same
        //    identifier; the original linear lookup selected the FIRST.
        //    A hash map that overwrites duplicates instead drives the LAST.
        //    Also verify the existing Law-target fallback when the provider
        //    no longer supplies the subject.
        // ------------------------------------------------------------------
        Object firstDuplicate, laterDuplicate;
        firstDuplicate.setObjectID("shared-drive-identifier");
        laterDuplicate.setObjectID("shared-drive-identifier");
        firstDuplicate.setPosition(glm::vec3(0.0f, 5.0f, 0.0f));
        laterDuplicate.setPosition(glm::vec3(0.0f, 99.0f, 0.0f));
        Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
            beings.push_back(&firstDuplicate);
            beings.push_back(&laterDuplicate);
        });

        auto sameIdDrive = mgr.createLaw("first-match-drive", {&author});
        sameIdDrive->setDrives(true);
        sameIdDrive->setActionModel(ActionNode::map(
            "position.y",
            boundedInT(OntoMath::ScalarForm::variable("t", 1.0, 2.0), 0.0, 2.0),
            tBinding));
        const std::size_t alphaDuplicate = mgr.rete().addAlphaNode(
            "type == duplicate-drive",
            [](const FactPtr& f) { return f->type == "duplicate-drive"; });
        mgr.rete().bindLawToAlpha(sameIdDrive->getIdentifier(), alphaDuplicate);

        Universe::instance().setClock(800.0, 0.1);
        Core::EventBus::instance().publish(
            ECA::Event{"duplicate-drive", &firstDuplicate, nullptr, std::time(nullptr)});
        mgr.tick();
        assert(mgr.driveSessions().size() == 1);
        assert(nearf(firstDuplicate.getPosition().y, 0.0f));
        assert(nearf(laterDuplicate.getPosition().y, 99.0f));

        Universe::instance().setClock(801.0, 0.1);
        mgr.tick();
        assert(nearf(firstDuplicate.getPosition().y, 2.0f));
        assert(nearf(laterDuplicate.getPosition().y, 99.0f));

        // The provider no longer has the subject, but the Law explicitly
        // retains it as a target: the original fallback must still work.
        sameIdDrive->addTarget(firstDuplicate);
        Universe::instance().setProvider({});
        Universe::instance().setClock(802.0, 0.1);
        mgr.tick();
        assert(nearf(firstDuplicate.getPosition().y, 4.0f));
        assert(nearf(laterDuplicate.getPosition().y, 99.0f));

        Universe::instance().setClock(803.0, 0.1);
        mgr.tick();
        assert(mgr.driveSessions().empty());

        Universe::instance().setProvider({});              // leave no dangling refs
    }


    // Reproducible, algorithm-level A/B for the drive-session being resolver.
    // Includes per-pass index construction, excludes unrelated Law execution.
    // Timing is observational: correctness is mandatory, speedup is not.
    for (const std::size_t count : {std::size_t{200}, std::size_t{1200}}) {
        std::vector<std::string> identifiers;
        identifiers.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            identifiers.push_back("being-" + std::to_string(i));
        }
        identifiers.back() = identifiers.front(); // duplicate: first wins
        std::vector<std::string> queries;
        for (std::size_t q = 0; q < 600; ++q) {
            queries.push_back("being-" + std::to_string((q * 13) % (count - 1)));
        }
        queries.push_back(identifiers.front());
        queries.push_back("missing-being");

        const auto scan = [&]() -> std::size_t {
            std::size_t checksum = 0;
            for (const auto& query : queries) {
                for (std::size_t i = 0; i < identifiers.size(); ++i) {
                    if (identifiers[i] == query) {
                        checksum += i + 1;
                        break;
                    }
                }
            }
            return checksum;
        };
        const auto indexed = [&]() -> std::size_t {
            std::unordered_map<std::string, std::size_t> index;
            index.reserve(identifiers.size());
            for (std::size_t i = 0; i < identifiers.size(); ++i) {
                index.emplace(identifiers[i], i); // first insertion wins
            }
            assert(index.at(identifiers.front()) == 0);
            std::size_t checksum = 0;
            for (const auto& query : queries) {
                const auto it = index.find(query);
                if (it != index.end()) checksum += it->second + 1;
            }
            return checksum;
        };
        const auto timed = [](const auto& operation) {
            const auto start = std::chrono::steady_clock::now();
            const std::size_t checksum = operation();
            const auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - start).count();
            return std::make_pair(elapsed, checksum);
        };

        std::array<long long, 3> scanUs{}, indexedUs{};
        std::size_t referenceChecksum = 0;
        for (std::size_t trial = 0; trial < 3; ++trial) {
            // Alternate order to reduce systematic warm-cache bias.
            const auto first = (trial % 2 == 0) ? timed(scan) : timed(indexed);
            const auto second = (trial % 2 == 0) ? timed(indexed) : timed(scan);
            const auto scanResult = (trial % 2 == 0) ? first : second;
            const auto indexResult = (trial % 2 == 0) ? second : first;
            if (scanResult.second != indexResult.second) {
                std::fprintf(stderr, "drive index A/B semantic mismatch\n");
                return 1;
            }
            referenceChecksum = scanResult.second;
            scanUs[trial] = scanResult.first;
            indexedUs[trial] = indexResult.first;
        }
        std::sort(scanUs.begin(), scanUs.end());
        std::sort(indexedUs.begin(), indexedUs.end());
        std::printf("LAW_DRIVE_BEING_INDEX_AB nodes=%zu queries=%zu scan_median_us=%lld indexed_median_us=%lld checksum=%zu\n",
                    count, queries.size(), scanUs[1], indexedUs[1], referenceChecksum);
    }

    std::puts("time_flow_test: ALL OK");
    return 0;
}
