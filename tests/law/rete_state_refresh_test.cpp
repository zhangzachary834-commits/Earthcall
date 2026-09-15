// Regression witness for the distinction between a persistent state fact
// changing value and a fact structurally ceasing to exist.
//
// Origin: Zach asked us to pursue the post-Formation-Rete Living Instrument
// profile after Opus's Formation work landed.  The measured remaining hot path
// was ReteNetwork::retractFact: dirty persistent state updates were removing a
// fact from the global _facts vector and immediately asserting the SAME FactPtr
// again.  This test specifies the semantic boundary the optimization must keep:
// network consequences may depart and re-enter; persistent fact identity and
// fact-store order may not.

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

namespace {

FactPtr positionFact(Object& object, const std::string& id) {
    Property* position = object.findProperty("position");
    assert(position);

    auto fact = std::make_shared<ReteFact>();
    fact->id = id;
    fact->type = "state";
    fact->subject = &object;
    fact->subjectId = object.getIdentifier();
    fact->attribute = "position";
    fact->value = propertyValueToJson(position->value());
    fact->isState = true;
    fact->dirty = false;
    return fact;
}

FactPtr simpleFact(const std::string& id,
                   const std::string& type,
                   Object* subject,
                   const std::string& attribute,
                   bool isState) {
    auto fact = std::make_shared<ReteFact>();
    fact->id = id;
    fact->type = type;
    fact->subject = subject;
    fact->subjectId = subject ? subject->getIdentifier() : std::string{};
    fact->attribute = attribute;
    fact->value = true;
    fact->isState = isState;
    fact->dirty = false;
    return fact;
}

bool containsSubject(const std::vector<Singular*>& subjects, const Singular* subject) {
    return std::find(subjects.begin(), subjects.end(), subject) != subjects.end();
}

void assertStoredExactlyOnce(const ReteNetwork& rete, const FactPtr& fact) {
    const auto& facts = rete.facts();
    assert(std::count(facts.begin(), facts.end(), fact) == 1);
}

} // namespace

int main() {
    // ---------------------------------------------------------------------
    // A persistent state fact must be able to leave and re-enter the network
    // without changing identity or position in the fact store.
    // ---------------------------------------------------------------------
    ReteNetwork rete;
    Object subject;
    subject.setObjectID("rete-refresh.subject");
    subject.setPosition(glm::vec3(1.0f, 0.0f, 0.0f));

    const std::size_t positionAlpha = rete.addAlphaNode(
        "position.x < 2",
        [](const FactPtr& fact) {
            if (!fact || fact->attribute != "position" || !fact->subject) return false;
            const auto* object = dynamic_cast<const Object*>(fact->subject);
            return object && object->getPosition().x < 2.0f;
        });
    const std::size_t anchorAlpha = rete.addAlphaNode(
        "anchor state",
        [](const FactPtr& fact) {
            return fact && fact->attribute == "anchor";
        });
    const std::size_t terminal = rete.addBetaNode(
        "position-and-anchor", false, positionAlpha, anchorAlpha);
    rete.bindLawToBeta("rete-refresh-law", terminal);

    const FactPtr position = positionFact(subject, "refresh-position");
    const FactPtr anchor = simpleFact(
        "refresh-anchor", "state", &subject, "anchor", true);

    // Position deliberately precedes anchor.  The old dirty path erased the
    // position fact and appended it again, silently changing this order.
    rete.assertFact(position);
    rete.assertFact(anchor);
    assert(rete.facts().size() == 2);
    assert(rete.facts()[0] == position);
    assert(rete.facts()[1] == anchor);
    assertStoredExactlyOnce(rete, position);

    auto initiallyMatching = rete.collectTerminalSubjects({terminal});
    assert(containsSubject(initiallyMatching, &subject));
    assert(!rete.drainAgenda().empty());

    // Matching -> non-matching: old Alpha/Beta/agenda consequences must go,
    // but the persistent fact itself must not die or move.
    subject.setPosition(glm::vec3(3.0f, 0.0f, 0.0f));
    assert(rete.markFactDirty(subject.getIdentifier(), "position"));
    rete.evaluateDirty();

    assert(rete.facts().size() == 2);
    assert(rete.facts()[0] == position);
    assert(rete.facts()[1] == anchor);
    assertStoredExactlyOnce(rete, position);
    assert(position->value == propertyValueToJson(subject.findProperty("position")->value()));
    assert(!containsSubject(rete.collectTerminalSubjects({terminal}), &subject));
    assert(rete.drainAgenda().empty());

    // Non-matching -> matching: the same FactPtr re-enters discrimination and
    // can build a fresh terminal token/activation.
    subject.setPosition(glm::vec3(1.0f, 0.0f, 0.0f));
    assert(rete.markFactDirty(subject.getIdentifier(), "position"));
    rete.evaluateDirty();

    assert(rete.facts().size() == 2);
    assert(rete.facts()[0] == position);
    assert(rete.facts()[1] == anchor);
    assertStoredExactlyOnce(rete, position);
    assert(containsSubject(rete.collectTerminalSubjects({terminal}), &subject));
    assert(!rete.drainAgenda().empty());

    // A dirty notification whose value did not change is a no-op: no detach,
    // no duplicate storage, no new activation.
    assert(rete.markFactDirty(subject.getIdentifier(), "position"));
    rete.evaluateDirty();
    assert(rete.facts()[0] == position);
    assertStoredExactlyOnce(rete, position);
    assert(rete.drainAgenda().empty());

    // Structural retraction is still structural deletion.
    assert(rete.retractFact(position->id));
    assert(rete.facts().size() == 1);
    assert(rete.facts()[0] == anchor);
    assert(!rete.markFactDirty(subject.getIdentifier(), "position"));

    // ---------------------------------------------------------------------
    // _facts order is observable through retractFirst().  Put persistent state
    // BETWEEN two events, refresh it, then consume the first two slots.  A
    // refresh must not turn [event-A, state, event-B] into
    // [event-A, event-B, state], or event-B is consumed a round too early.
    // ---------------------------------------------------------------------
    ReteNetwork ordered;
    Object orderedSubject;
    orderedSubject.setObjectID("rete-refresh.ordered-subject");
    orderedSubject.setPosition(glm::vec3(5.0f, 0.0f, 0.0f));

    const FactPtr eventA = simpleFact(
        "refresh-event-a", "event-a", &orderedSubject, "event-a", false);
    const FactPtr orderedState = positionFact(orderedSubject, "refresh-ordered-position");
    const FactPtr eventB = simpleFact(
        "refresh-event-b", "event-b", &orderedSubject, "event-b", false);

    ordered.assertFact(eventA);
    ordered.assertFact(orderedState);
    ordered.assertFact(eventB);
    assert(ordered.facts().size() == 3);
    assert(ordered.facts()[0] == eventA);
    assert(ordered.facts()[1] == orderedState);
    assert(ordered.facts()[2] == eventB);

    orderedSubject.setPosition(glm::vec3(6.0f, 0.0f, 0.0f));
    assert(ordered.markFactDirty(orderedSubject.getIdentifier(), "position"));
    ordered.evaluateDirty();

    assert(ordered.facts().size() == 3);
    assert(ordered.facts()[0] == eventA);
    assert(ordered.facts()[1] == orderedState);
    assert(ordered.facts()[2] == eventB);
    assertStoredExactlyOnce(ordered, orderedState);

    ordered.retractFirst(2);
    assert(ordered.facts().size() == 2);
    assert(ordered.facts()[0] == orderedState);
    assert(ordered.facts()[1] == eventB);

    std::puts("rete_state_refresh_test: ALL OK");
    return 0;
}
