// Zach: one Create/set-to-set operation for every admissible existing Singular,
// with real human/First Mover/machine correspondents excluded.
// Codex / GPT-6 / 01a0e64f-5853-7d30-8196-995b4fd16b89 / 2026-10-02.
#include "ConstructedBeing/Singular/Creation/SingularSetToSetCreation.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Object/Creation/ObjectConcept.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ConstructedBeing/Material/Material.hpp"
#include "Identity/FirstMoverRegister.hpp"
#include "Person/Person.hpp"
#include "Person/Relationship/Community/Community.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/Language/LanguageSystem.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "Time/Moment/Moment.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/PropheticRete.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include <cassert>
#include <algorithm>
#include <iostream>

using namespace SingularSetToSetCreation;

namespace {
struct UnadaptedObject : Object {};
struct UnadaptedLaw : Law { UnadaptedLaw() : Law("unadapted-channel") {} };

Singular* find(Zone& zone, const std::string& id) {
    for (auto* being : zone.formation().getMembers())
        if (being && being->getIdentifier() == id) return being;
    for (const auto& being : zone.objects())
        if (being && being->getIdentifier() == id) return being.get();
    return nullptr;
}
}

int main() {
    // The fixture supplies explicit opening; birth must not open these gates.
    auto& policy = TransferPolicy::instance();
    for (const auto& [name, tier] : policy.gates())
        if (tier != TransferPolicy::Tier::Kernel) policy.setOpen(name, true);
    Object author("creation-author");
    Zone zone("universal-creation-zone", "Christ");
    Zone* visible = &zone;
    auto a = std::make_shared<Object>("member-a");
    auto b = std::make_shared<Object>("member-b");
    zone.addObject(a);
    zone.addObject(b);
    auto lexical = Singularity::Language::LanguageSystem::instance().intern("hope", "lexeme.source-hope");
    lexical->setConceptualWeightValue(0.375);
    lexical->setDynamicProperty("focus", PropertyValue(a.get()));
    lexical->setDynamicProperty("t", PropertyValue(2)); // a user key, not a codec tag
    zone.addToFormation(lexical.get());
    Material material("source-paint");
    material.baseColor = glm::vec3(0.1f, 0.2f, 0.3f);
    Formation formation({a.get(), b.get()});
    formation.setIdentifier("formation.source-pair");
    auto bond = std::make_shared<Relation>("joined", *a, *b, false, 0.75f);
    formation.addRelation(bond);
    const auto bondHistory = bond->toJson()["events"];
    const auto bondWeight = bond->getWeight();
    Community community("community.source");
    Moment moment = Moment::interval(2.0, 8.0);
    geom::FieldNode field("field.source");
    field.origin = glm::vec3(2, 3, 4);
    ObjectConcept concept("source-recipe");
    std::vector<Singular*> prototypes{lexical.get(), &material, &formation, &community, &moment, &field, &concept};
    Universe::instance().setProvider([&](std::vector<Singular*>& beings) {
        beings.push_back(&author);
        if (!visible) return;
        beings.push_back(visible);
        for (const auto& object : visible->objects()) beings.push_back(object.get());
        for (auto* member : visible->formation().getMembers()) beings.push_back(member);
        if (visible == &zone) for (auto* prototype : prototypes) beings.push_back(prototype);
    });
    for (auto* prototype : prototypes) {
        Request request{*prototype, {a.get(), b.get()}, &author, nullptr, &zone, {}, {}, {}};
        auto result = derive(request);
        assert(result);
        assert(typeid(*result.newborn) == typeid(*prototype));
        assert(result.newborn != prototype);
        assert(result.newborn->getIdentifier() != prototype->getIdentifier());
        assert(find(zone, result.newborn->getIdentifier()) == result.newborn);
        bool attributed = false;
        for (const auto& relation : zone.formation().relations().getAll())
            if (relation->type == "authored-by" && relation->a() == result.newborn && relation->b() == &author) attributed = true;
        assert(attributed);
        if (auto* copied = dynamic_cast<Formation*>(result.newborn); copied && typeid(*copied) == typeid(Formation)) {
            assert(copied->hasMember(a.get()));
            assert(copied->relations().getAll().front() == bond);
            assert(bond->toJson()["events"] == bondHistory);
            assert(bond->getWeight() == bondWeight);
        }
    }
    auto* copiedMoment = dynamic_cast<Moment*>(find(zone, moment.getIdentifier() + ".branch-1"));
    assert(copiedMoment);
    const auto temporalIdentity = copiedMoment->getIdentifier();
    copiedMoment->setStart(3.0);
    assert(copiedMoment->getIdentifier() == temporalIdentity);
    assert(copiedMoment->endSeconds() == 8.0);

    Request relationBirth{*bond, {a.get(), b.get()}, &author, nullptr, &zone, {}, {}, {}};
    assert(!derive(relationBirth)); // no invented endpoints or duplicate enduring bond
    relationBirth.endpointA = &author;
    relationBirth.endpointB = a.get();
    auto relationResult = derive(relationBirth);
    assert(relationResult);
    auto* relation = dynamic_cast<Relation*>(relationResult.newborn);
    assert(relation && relation->a() == &author && relation->b() == a.get());
    assert(!derive(relationBirth));
    std::swap(relationBirth.endpointA, relationBirth.endpointB);
    assert(!derive(relationBirth)); // same undirected bond, reversed spelling
    Formation bondContainer({&author, a.get()});
    bondContainer.setIdentifier("formation.bond-container");
    assert(bondContainer.retainRelation(RelationManager::retained(relation)));
    Request containerBirth{bondContainer, {}, &author, nullptr, &zone, "formation.newborn-bond-view", {}, {}};
    assert(derive(containerBirth));

    // Actual authored action/JSON/compiler route; no per-kind Action enum.
    Law create("create-an-authored-lexeme", {&author});
    auto node = ActionNode::createFrom("@lexeme.source-hope", "lexeme.authored-hope",
                                     {ActionNode::addProperty("", "meaning", std::string("gift"))});
    assert(ActionNode::fromJson(node.toJson()).toJson() == node.toJson());
    create.setActionModel(ActionNode::fromJson(node.toJson()));
    assert(create.applyTo(*a) == Law::ApplicationResult::Applied);
    auto* born = dynamic_cast<Singularity::Language::Lexeme*>(find(zone, "lexeme.authored-hope"));
    assert(born && born->getSymbol() == "hope");
    assert(born->getConceptualWeight() == 0.375f);
    PropertyValue focus;
    assert(born->getDynamicProperty("focus", focus));
    assert(std::get<Object*>(focus) == a.get());
    assert(Singularity::Language::LanguageSystem::instance().findById("lexeme.authored-hope").get() == born);
    PropertyValue meaning;
    assert(born->getDynamicProperty("meaning", meaning));
    assert(std::get<std::string>(meaning) == "gift");
    assert(create.applyTo(*a) == Law::ApplicationResult::Applied);
    assert(!create.applicationLog().back().trace.anyWrote()); // collision refusal recorded

    // Kernel refusal and exact subclass preservation, before any construction.
    Identity::FirstMover mover;
    Person human(Soul("human-correspondent-fixture"), Body(), "default");
    UnadaptedObject unknownObject;
    UnadaptedLaw unknownLaw;
    for (auto* forbidden : std::vector<Singular*>{&human, &mover, &TransferPolicy::instance(), &unknownObject, &unknownLaw}) {
        Request request{*forbidden, {}, &author, nullptr, &zone, {}, {}, {}};
        const auto result = derive(request);
        assert(!result && !result.refusal.empty());
    }
    Request unauthored{*lexical, {}, nullptr, nullptr, &zone, {}, {}, {}};
    assert(!derive(unauthored));
    policy.setOpen("conceptualWeight", false);
    Request closed{*lexical, {}, &author, nullptr, &zone, "lexeme.closed-birth", {}, {}};
    assert(!derive(closed));
    assert(!find(zone, "lexeme.closed-birth"));
    assert(!policy.isOpen("conceptualWeight"));
    policy.setOpen("conceptualWeight", true);
    Singularity::Language::Lexeme memorySource("memory", "lexeme.memory-copy-probe");
    auto shared = std::make_shared<PropertyList>();
    shared->elements.emplace_back(1);
    memorySource.setDynamicProperty("left", shared);
    memorySource.setDynamicProperty("right", shared);
    Request ambiguousMemory{memorySource, {}, &author, nullptr, &zone, {}, {}, {}};
    assert(!derive(ambiguousMemory)); // unresolved alias-copy contract stays open
    Prophetic::LawFacts footprint;
    Prophetic::analyzeAction(node, footprint);
    assert(footprint.opaqueReads && footprint.opaqueWrites);

    auto snapshot = zoneToJson(zone);
    assert(snapshot.contains("storedSingulars"));
    // A container appearing before its separately stored Relation must still
    // resolve the actual canonical bond, not mint another same-ID individual.
    std::reverse(snapshot["storedSingulars"].begin(), snapshot["storedSingulars"].end());
    // The old root is no longer the visible world during hydration. References
    // to its Objects must bind to the restored root's actual Object instances.
    visible = nullptr;
    auto restored = makeZoneFromJson(snapshot);
    assert(restored);
    visible = restored.get();
    auto* restoredBorn = dynamic_cast<Singularity::Language::Lexeme*>(find(*restored, "lexeme.authored-hope"));
    assert(restoredBorn == born); // language identity remains one canonical instance
    assert(restoredBorn->getDynamicProperty("meaning", meaning));
    assert(restoredBorn->getDynamicProperty("focus", focus));
    assert(std::get<Object*>(focus) == find(*restored, "member-a"));
    auto* restoredFormation = dynamic_cast<Formation*>(find(*restored, "formation.source-pair.branch-1"));
    assert(restoredFormation);
    assert(restoredFormation->hasMember(find(*restored, "member-a")));
    assert(!restoredFormation->hasMember(a.get()));
    assert(restoredFormation->relations().getAll().front()->toJson()["events"] == bondHistory);
    assert(restoredFormation->relations().getAll().front()->getWeight() == bondWeight);
    auto* restoredRelation = dynamic_cast<Relation*>(find(*restored, relation->getIdentifier()));
    assert(restoredRelation && restoredRelation->b() == find(*restored, "member-a"));
    assert(restoredRelation->a() == &author);
    auto* restoredBondView = dynamic_cast<Formation*>(find(*restored, "formation.newborn-bond-view"));
    assert(restoredBondView && restoredBondView->relations().getAll().front().get() == restoredRelation);
    auto* restoredTime = dynamic_cast<Moment*>(find(*restored, temporalIdentity));
    assert(restoredTime && restoredTime->asSeconds() == 3.0);

    // Missing graph participants refuse rather than quietly dropping members.
    auto malformed = snapshot["storedSingulars"];
    for (auto& record : malformed)
        if (record["codec"] == "formation") record["state"]["members"][0]["id"] = "missing-participant";
    Zone empty("refused-restoration", "Christ");
    bool refused = false;
    try { restoreStored(empty, malformed, true); } catch (const std::exception&) { refused = true; }
    assert(refused && empty.storedSingulars().empty());
    Universe::instance().setProvider(nullptr);
    std::cout << "universal_singular_creation_test: PASS\n";
}
