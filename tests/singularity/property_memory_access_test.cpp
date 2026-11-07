// Zach's PropertyPath/memory micromastery direction: containers remain
// predicates, shared writes reach shared storage, and paths retain no IDs.
// Codex / GPT-6.1 Sol / 01a0e64f-5853-7d30-8196-995b4fd16b89 / 2026-10-01.
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ConstructedBeing/Singular/Property/PropertyCell.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "ConstructedBeing/Singular/SingularPersistence.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "Person/Person.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "Singularity/Terminal/LawSentence.hpp"
#include "Singularity/OntoMath/Field.hpp"

#include <cassert>
#include <chrono>
#include <iostream>

namespace {
static_assert(!is_property_value_alternative<Singular*>);
static_assert(!is_property_value_alternative<Object*>);
static_assert(!is_property_value_alternative<Relation*>);
static_assert(!is_property_value_alternative<Formation*>);

using Result = PropertyPath::PathResult;

class MemorySubject : public Singular {
public:
    std::shared_ptr<PropertyDict> live = std::make_shared<PropertyDict>();
    int setterCalls = 0;
    Singular* child = nullptr;
    mutable int childReads = 0;
    std::shared_ptr<OntoMath::VectorField> field = std::make_shared<OntoMath::VectorField>();
    int fieldSetterCalls = 0;
    std::shared_ptr<OntoMath::VectorField> readField() const { return field; }
    void replaceField(const std::shared_ptr<OntoMath::VectorField>& v) { ++fieldSetterCalls; field=v; }
    std::string getIdentifier() const override { return "memory-subject"; }
    std::shared_ptr<PropertyDict> readLive() const { return live; }
    void replaceLive(const std::shared_ptr<PropertyDict>& value) { ++setterCalls; live = value; }
    Singular* readChild() const { ++childReads; return child; }
    std::shared_ptr<PropertyDict> temporary() const {
        auto dict = std::make_shared<PropertyDict>();
        dict->elements["value"] = 27;
        return dict;
    }
protected:
    void buildProperties() override {
        registerProperty(std::make_unique<PropertyRef<MemorySubject, std::shared_ptr<OntoMath::VectorField>>>(
            "field", this, &MemorySubject::field));
        registerProperty(std::make_unique<ComputedProperty<MemorySubject, std::shared_ptr<OntoMath::VectorField>>>(
            "field-derived", this, &MemorySubject::readField));
        registerProperty(std::make_unique<ComputedProperty<MemorySubject, std::shared_ptr<OntoMath::VectorField>>>(
            "field-setter", this, &MemorySubject::readField, &MemorySubject::replaceField));
        registerProperty(std::make_unique<PropertyRef<MemorySubject, std::shared_ptr<PropertyDict>>>(
            "live", this, &MemorySubject::live));
        registerProperty(std::make_unique<ComputedProperty<MemorySubject, std::shared_ptr<PropertyDict>>>(
            "derived", this, &MemorySubject::readLive));
        registerProperty(std::make_unique<ComputedProperty<MemorySubject, std::shared_ptr<PropertyDict>>>(
            "through-setter", this, &MemorySubject::readLive, &MemorySubject::replaceLive));
        registerProperty(std::make_unique<ComputedProperty<MemorySubject, std::shared_ptr<PropertyDict>>>(
            "temporary", this, &MemorySubject::temporary));
        registerProperty(std::make_unique<ComputedProperty<MemorySubject, Singular*>>(
            "child", this, &MemorySubject::readChild));
    }
};

PropertyValue read(Singular& root, const std::string& path) {
    PropertyValue value;
    assert(PropertyPath::parse(path).getValue(root, value) == Result::Ok);
    return value;
}

std::shared_ptr<PropertyDict> paint(float green) {
    auto layer = std::make_shared<PropertyDict>();
    layer->elements["color"] = glm::vec3(1.0f, green, 0.0f);
    auto layers = std::make_shared<PropertyList>();
    layers->elements.push_back(layer);
    auto dict = std::make_shared<PropertyDict>();
    dict->elements["layers"] = layers;
    return dict;
}
}

int main() {
    MemorySubject subject;
    subject.live->elements["value"] = 4;
    PropertyValue value;
    assert(std::get<int>(read(subject, "live.value")) == 4);
    assert(std::get<int>(read(subject, "temporary.value")) == 27);
    MemorySubject child;
    child.live->elements["value"] = 18;
    subject.child = &child;
    assert(std::get<int>(read(subject, "child.live.value")) == 18);
    assert(subject.childReads == 1);
    // Even an equal-value attempt cannot mutate through a read-only getter.
    assert(PropertyPath::parse("derived.value").setValue(subject, 4) == Result::ReadOnly);
    assert(PropertyPath::parse("derived.value").setValue(subject, 8) == Result::ReadOnly);
    assert(PropertyPath::parse("through-setter.value").setValue(subject, 8) == Result::Unsupported);
    assert(subject.setterCalls == 0);
    assert(std::get<int>(read(subject, "live.value")) == 4);
    auto replacement = std::make_shared<PropertyDict>();
    replacement->elements["value"] = 6;
    assert(PropertyPath::parse("through-setter").setValue(subject, replacement) == Result::Ok);
    assert(subject.setterCalls == 1);
    assert(std::get<int>(read(subject, "live.value")) == 6);

    std::string changed;
    Singular::setPropertyChangeCallback([&](Singular* owner, const std::string& name) {
        assert(owner == &subject);
        changed = name;
    });
    assert(PropertyPath::parse("live.value").setValue(subject, 8) == Result::Ok);
    assert(changed == "live");
    assert(std::get<int>(read(subject, "live.value")) == 8);
    Singular::setPropertyChangeCallback(nullptr);

    // Direct Screen region mathematics is an ordinary typed predicate. A
    // nested write replaces validated canonical field data, wakes its root,
    // and cannot tunnel through a read-only getter or a structural setter.
    // Codex / GPT-6.1 Sol / 01a10992-828e-7e80-890c-c64b09141e18 / 2026-10-07.
    auto vector = std::make_shared<OntoMath::MathNode>();
    vector->op = OntoMath::MathNode::Op::VectorConstruct;
    for (double component : {0.2,0.3,0.4}) {
        auto leaf=std::make_unique<OntoMath::MathNode>();
        leaf->scalarForm=OntoMath::ScalarForm::constant(component);
        vector->children.push_back(std::move(leaf));
    }
    subject.field->mode=OntoMath::VectorField::EvaluationMode::AST;
    subject.field->astDefinition=OntoMath::Piecewise::continuous(vector);
    const std::string coefficient=".astDefinition.pieces.0.mathNode.children.0.scalarForm.terms.0.c";
    double number=0;
    assert(propertyValueToNumber(read(subject,"field"+coefficient),number) && number==0.2);
    const auto oldField=subject.field;
    changed.clear();
    Singular::setPropertyChangeCallback([&](Singular* owner,const std::string& name){assert(owner==&subject);changed=name;});
    assert(PropertyPath::parse("field"+coefficient).setValue(subject,0.8)==Result::Ok);
    assert(changed=="field" && subject.field!=oldField);
    auto sampled=subject.field->astDefinition.evaluate({});
    assert(sampled && std::get<glm::vec3>(*sampled).x==0.8f);
    assert(PropertyPath::parse("field-derived"+coefficient).setValue(subject,0.8)==Result::ReadOnly);
    assert(PropertyPath::parse("field-setter"+coefficient).setValue(subject,0.6)==Result::Ok);
    assert(subject.fieldSetterCalls==1 && changed=="field-setter");
    const auto valid=subject.field->toJson();
    assert(PropertyPath::parse("field.astDefinition.pieces.0.mathNode.op").setValue(subject,999)==Result::TypeMismatch);
    assert(PropertyPath::parse("field.astDefinition.pieces.0.mathNode.op").setValue(subject,2.5)==Result::TypeMismatch);
    assert(PropertyPath::parse("field"+coefficient).setValue(subject,std::numeric_limits<double>::infinity())==Result::TypeMismatch);
    assert(PropertyPath::parse("field"+coefficient).setValue(subject,std::string("bad"))==Result::TypeMismatch);
    assert(subject.field->toJson()==valid);
    assert(PropertyPath::parse("field.frequency").setValue(subject,0.2)==Result::Ok);
    assert(subject.field->frequency==0.2f);
    assert(PropertyPath::parse("field"+coefficient).setValue(subject,0.60000001)==Result::Ok);
    assert(propertyValueToNumber(read(subject,"field"+coefficient),number) && number==0.60000001);
    assert(PropertyPath::parse("field.astDefinition.pieces.999.mathNode").setValue(subject,1)==Result::NoSuchProperty);
    Singular::setPropertyChangeCallback(nullptr);
    subject.setDynamicProperty("region",std::make_shared<PropertyDict>());
    auto region=std::get<std::shared_ptr<PropertyDict>>(read(subject,"region"));
    region->elements["color"]=subject.field;
    changed.clear();
    Singular::setPropertyChangeCallback([&](Singular* owner,const std::string& name){assert(owner==&subject);changed=name;});
    assert(PropertyPath::parse("region.color"+coefficient).setValue(subject,0.9)==Result::Ok);
    assert(changed=="region");
    assert(propertyValueToJson(propertyValueFromJson(propertyValueToJson(read(subject,"region"))))==propertyValueToJson(read(subject,"region")));
    Singular::setPropertyChangeCallback(nullptr);

    subject.setDynamicProperty("paint", paint(0.2f));
    assert(PropertyPath::parse("paint.layers.0.color.g").setValue(subject, 0.5f) == Result::Ok);
    assert(std::get<float>(read(subject, "paint.layers.0.color.g")) == 0.5f);
    for (const std::string& bad : {"paint.layers.0junk", "paint.layers.-0", "paint.layers.+0",
                                  "paint.layers.999999999999999999999999999999", "paint.layers.1",
                                  "paint.missing", "live.value.extra"}) {
        value = 999;
        auto path = PropertyPath::parse(bad);
        assert(path.getValue(subject, value) == Result::NoSuchProperty);
        assert(std::holds_alternative<std::monostate>(value));
        assert(path.setValue(subject, 99) == Result::NoSuchProperty);
        assert(path.resolve(subject).owner == nullptr);
    }
    for (std::size_t offset : {std::size_t(0), std::size_t(2), std::size_t(-1)}) {
        assert(PropertyPath::parse("").getValue(subject, value, offset) == Result::NoSuchProperty);
        assert(PropertyPath::parse("").setValue(subject, 1, offset) == Result::NoSuchProperty);
    }

    // The second path is the actual authored Law/JSON/compiler path. A
    // ValueLeaf carries the typed value; Map binds the existing shared
    // container without a new memory opcode or a special operandPath.
    Object author;
    auto variable = std::make_shared<OntoMath::MathNode>();
    variable->op = OntoMath::MathNode::Op::ValueLeaf;
    variable->variableName = "source";
    auto action = ActionNode::map("alias", OntoMath::Piecewise::continuous(variable),
                                 {{"source", PropertyPath::parse("paint")}});
    Law share("share-paint-memory");
    share.addAuthor(author);
    share.setActionModel(ActionNode::fromJson(action.toJson()));
    // Equal contents in a different container must still bind to the source.
    subject.setDynamicProperty("alias", paint(0.5f));
    assert(share.applyTo(subject) == Law::ApplicationResult::Applied);
    assert(std::get<std::shared_ptr<PropertyDict>>(read(subject, "alias")) ==
           std::get<std::shared_ptr<PropertyDict>>(read(subject, "paint")));
    assert(PropertyPath::parse("alias.layers.0.color.g").setValue(subject, 0.7f) == Result::Ok);
    assert(std::get<float>(read(subject, "paint.layers.0.color.g")) == 0.7f);
    assert(PropertyPath::parse("alias").setValue(subject, paint(0.9f)) == Result::Ok);
    assert(std::get<float>(read(subject, "paint.layers.0.color.g")) == 0.7f);
    assert(std::get<float>(read(subject, "alias.layers.0.color.g")) == 0.9f);
    auto equalReplacement = paint(0.9f);
    auto oldAlias = std::get<std::shared_ptr<PropertyDict>>(read(subject, "alias"));
    assert(propertyValueUnchanged(PropertyValue(oldAlias), PropertyValue(equalReplacement)));
    assert(PropertyPath::parse("alias").setValue(subject, equalReplacement) == Result::Ok);
    assert(std::get<std::shared_ptr<PropertyDict>>(read(subject, "alias")) == equalReplacement);
    assert(std::get<std::shared_ptr<PropertyDict>>(read(subject, "alias")) != oldAlias);
    changed.clear();
    Singular::setPropertyChangeCallback([&](Singular* owner, const std::string& name) {
        assert(owner == &subject);
        changed = name;
    });
    assert(subject.setDynamicProperty("alias", paint(0.9f)));
    assert(changed == "alias");
    Singular::setPropertyChangeCallback(nullptr);
    // A resolved access pins the old container until that access completes.
    // The path itself re-resolves and sees the replacement on its next read.
    {
        auto path = PropertyPath::parse("alias.layers.0.color");
        auto access = path.resolve(subject);
        assert(access.dynamicSlot && !access.containerPins.empty());
        assert(PropertyPath::parse("alias").setValue(subject, paint(0.3f)) == Result::Ok);
        assert(std::get<glm::vec3>(*access.dynamicSlot).g == 0.9f);
        assert(std::get<glm::vec3>(read(subject, "alias.layers.0.color")).g == 0.3f);
    }

    // A non-arithmetic identity binding must preserve integer precision.
    constexpr long exact = sizeof(long) >= 8 ? static_cast<long>(9007199254740993LL) : 16777217L;
    subject.setDynamicProperty("count", exact);
    auto* countStorage = subject.getDynamicPropertyPtr(Earthcall::StringInterner::intern("count"));
    assert(countStorage);
    assert(PropertyPath::parse("count").getValue(subject, *countStorage) == Result::Ok);
    assert(std::get<long>(*countStorage) == exact);
    Law copyScalar("copy-exact-count");
    copyScalar.addAuthor(author);
    auto copy = ActionNode::map("copied-count", OntoMath::Piecewise::continuous(variable),
                               {{"source", PropertyPath::parse("count")}});
    copyScalar.setActionModel(ActionNode::fromJson(copy.toJson()));
    assert(copyScalar.applyTo(subject) == Law::ApplicationResult::Applied);
    assert(std::holds_alternative<long>(read(subject, "copied-count")));
    assert(std::get<long>(read(subject, "copied-count")) == exact);
    assert(PropertyPath::parse("count").setValue(subject, long(5)) == Result::Ok);
    assert(std::get<long>(read(subject, "copied-count")) == exact);

    // Equality is substrate safety, not an authored graph-copy policy.
    // Distinct loops conservatively count as a possible change; acyclic
    // nested graphs can be compared without using the machine call stack.
    auto loopA = std::make_shared<PropertyList>();
    auto loopB = std::make_shared<PropertyList>();
    loopA->elements.push_back(loopA);
    loopB->elements.push_back(loopB);
    assert(!propertyValueUnchanged(PropertyValue(loopA), PropertyValue(loopB)));
    loopA->elements.clear();
    loopB->elements.clear();
    std::vector<std::shared_ptr<PropertyList>> deepA, deepB;
    for (int i = 0; i < 8192; ++i) {
        deepA.push_back(std::make_shared<PropertyList>());
        deepB.push_back(std::make_shared<PropertyList>());
        if (i) {
            deepA[i - 1]->elements.push_back(deepA[i]);
            deepB[i - 1]->elements.push_back(deepB[i]);
        }
    }
    deepA.back()->elements.push_back(7);
    deepB.back()->elements.push_back(7);
    assert(propertyValueUnchanged(PropertyValue(deepA.front()), PropertyValue(deepB.front())));
    deepB.back()->elements.back() = 8;
    assert(!propertyValueUnchanged(PropertyValue(deepA.front()), PropertyValue(deepB.front())));
    for (const auto& list : deepA) list->elements.clear();
    for (const auto& list : deepB) list->elements.clear();

    // -------------------------------------------------------------------------
    // Rung 1: Independent rebinding of one shared path without detaching the other
    // -------------------------------------------------------------------------
    {
        Object rebindA;
        Object rebindB;
        CellId cellAB = PropertyCellTable::instance().allocateCell(
            PropertyValue(100.0), CellOwnershipMode::Shared, &rebindA, Earthcall::StringInterner::intern("val"));
        assert(rebindA.bindPropertyCell("val", cellAB, CellOwnershipMode::Shared));
        assert(rebindB.bindPropertyCell("val", cellAB, CellOwnershipMode::Shared));

        assert(PropertyPath::parse("val").setValue(rebindA, 150.0) == Result::Ok);
        PropertyValue readB;
        assert(PropertyPath::parse("val").getValue(rebindB, readB) == Result::Ok);
        assert(std::get<double>(readB) == 150.0);

        // Explicitly rebind rebindA to a new cell
        CellId newCellA = PropertyCellTable::instance().allocateCell(
            PropertyValue(999.0), CellOwnershipMode::Unique, &rebindA, Earthcall::StringInterner::intern("val"));
        assert(rebindA.bindPropertyCell("val", newCellA, CellOwnershipMode::Unique));

        PropertyValue readA;
        assert(PropertyPath::parse("val").getValue(rebindA, readA) == Result::Ok);
        assert(std::get<double>(readA) == 999.0);

        // rebindB must remain bound to cellAB and unaffected
        assert(PropertyPath::parse("val").getValue(rebindB, readB) == Result::Ok);
        assert(std::get<double>(readB) == 150.0);

        // Mutating rebindB does not affect rebindA
        assert(PropertyPath::parse("val").setValue(rebindB, 200.0) == Result::Ok);
        assert(PropertyPath::parse("val").getValue(rebindA, readA) == Result::Ok);
        assert(std::get<double>(readA) == 999.0);
    }

    // -------------------------------------------------------------------------
    // Rung 1: Unique sharing refusal (UniqueOwnershipRefused) & conversion to Shared
    // -------------------------------------------------------------------------
    {
        Object u1;
        u1.setObjectID("u1-owner");
        Object u2;
        u2.setObjectID("u2-peer");

        CellId uniqueCellId = PropertyCellTable::instance().allocateCell(
            PropertyValue(std::string("confidential")), CellOwnershipMode::Unique, &u1, Earthcall::StringInterner::intern("secret"));
        assert(u1.bindPropertyCell("secret", uniqueCellId, CellOwnershipMode::Unique));

        // Direct attempt to share Unique cell must fail
        assert(!u2.bindPropertyCell("secret", uniqueCellId, CellOwnershipMode::Shared));

        // Law action shareCell must refuse with UniqueOwnershipRefused
        std::vector<Singular*> uniBeings{&u1, &u2, &author};
        Universe::instance().setProvider([&](std::vector<Singular*>& out) {
            for (auto* b : uniBeings) if (b) out.push_back(b);
        });

        ActionNode tryShare = ActionNode::shareCell("secret", "@u1-owner.secret");
        Law refuseShareLaw("refuse-unique-share");
        refuseShareLaw.addAuthor(author);
        refuseShareLaw.setActionModel(tryShare);
        // Evaluating tryShare on u2 must fail to bind
        refuseShareLaw.applyTo(u2);
        assert(!u2.hasPropertyCell("secret"));

        // Explicitly convert unique cell to shared
        assert(u1.convertPropertyCellToShared("secret"));

        // Now shareCell succeeds
        assert(u2.bindPropertyCell("secret", uniqueCellId, CellOwnershipMode::Shared));
        PropertyValue secVal;
        assert(PropertyPath::parse("secret").getValue(u2, secVal) == Result::Ok);
        assert(std::get<std::string>(secVal) == "confidential");

        Universe::instance().setProvider(nullptr);
    }

    // -------------------------------------------------------------------------
    // Rung 1: Weak live observation & ExpiredCell refusal after owner destruction
    // -------------------------------------------------------------------------
    {
        auto tempOwner = std::make_unique<Object>();
        CellId weakTargetCell = PropertyCellTable::instance().allocateCell(
            PropertyValue(42), CellOwnershipMode::Shared, tempOwner.get(), Earthcall::StringInterner::intern("data"));
        assert(tempOwner->bindPropertyCell("data", weakTargetCell, CellOwnershipMode::Shared));

        Object observer;
        assert(observer.bindPropertyCell("weakView", weakTargetCell, CellOwnershipMode::Weak));

        PropertyValue val;
        assert(PropertyPath::parse("weakView").getValue(observer, val) == Result::Ok);
        assert(std::get<int>(val) == 42);

        // Live mutation through owner is observed
        assert(PropertyPath::parse("data").setValue(*tempOwner, 84) == Result::Ok);
        assert(PropertyPath::parse("weakView").getValue(observer, val) == Result::Ok);
        assert(std::get<int>(val) == 84);

        // Destroy owner
        tempOwner.reset();

        // Weak handle must now return ExpiredCell (never 0, stale, or black)
        assert(PropertyPath::parse("weakView").getValue(observer, val) == Result::ExpiredCell);
        assert(PropertyPath::parse("weakView").setValue(observer, 99) == Result::ExpiredCell);
    }

    // -------------------------------------------------------------------------
    // Rung 2: End-to-End "Red Object in Hand -> Building Form" Witness
    // -------------------------------------------------------------------------
    {
        auto objInHand = std::make_unique<Object>();
        objInHand->setObjectID("obj-in-hand");
        assert(PropertyPath::parse("color").setValue(*objInHand, glm::vec3(1.0f, 0.0f, 0.0f)) == Result::Ok); // Red
        objInHand->setDynamicProperty("tempScale", 2.5);

        Object buildingForm;
        buildingForm.setObjectID("building-form");
        assert(PropertyPath::parse("color").setValue(buildingForm, glm::vec3(0.0f, 0.0f, 0.0f)) == Result::Ok); // Black

        std::vector<Singular*> worldBeings{objInHand.get(), &buildingForm, &author};
        Universe::instance().setProvider([&](std::vector<Singular*>& out) {
            for (auto* b : worldBeings) if (b) out.push_back(b);
        });

        // 1. Share color from obj-in-hand to buildingForm
        ActionNode bindColorAction = ActionNode::shareCell("color", "@obj-in-hand.color");
        Law bindColorLaw("bind-color-law");
        bindColorLaw.addAuthor(author);
        bindColorLaw.setActionModel(bindColorAction);
        assert(bindColorLaw.applyTo(buildingForm) == Law::ApplicationResult::Applied);

        // Building now reflects the red color
        PropertyValue bColor;
        assert(PropertyPath::parse("color").getValue(buildingForm, bColor) == Result::Ok);
        assert(std::get<glm::vec3>(bColor) == glm::vec3(1.0f, 0.0f, 0.0f));

        // 2. Weak bind tempScale to buildingForm.observedScale
        ActionNode bindWeakAction = ActionNode::weakCell("observedScale", "@obj-in-hand.tempScale");
        Law bindWeakLaw("bind-weak-law");
        bindWeakLaw.addAuthor(author);
        bindWeakLaw.setActionModel(bindWeakAction);
        assert(bindWeakLaw.applyTo(buildingForm) == Law::ApplicationResult::Applied);

        PropertyValue obsScale;
        assert(PropertyPath::parse("observedScale").getValue(buildingForm, obsScale) == Result::Ok);
        assert(std::get<double>(obsScale) == 2.5);

        // 3. Mutate objInHand.color -> verify buildingForm updates and wakes
        std::string notifiedProp;
        Singular::setPropertyChangeCallback([&](Singular* owner, const std::string& name) {
            if (owner == &buildingForm) notifiedProp = name;
        });

        assert(PropertyPath::parse("color.g").setValue(*objInHand, 0.75f) == Result::Ok);
        assert(notifiedProp == "color");
        PropertyValue bColorG;
        assert(PropertyPath::parse("color.g").getValue(buildingForm, bColorG) == Result::Ok);
        assert(std::get<float>(bColorG) == 0.75f);
        Singular::setPropertyChangeCallback(nullptr);

        // 4. Destroy objInHand -> verify share retains the cell while weak refuses with ExpiredCell
        objInHand.reset();
        worldBeings[0] = nullptr;

        PropertyValue finalColor;
        assert(PropertyPath::parse("color").getValue(buildingForm, finalColor) == Result::Ok);
        assert(std::get<glm::vec3>(finalColor) == glm::vec3(1.0f, 0.75f, 0.0f));

        PropertyValue expiredProbe;
        assert(PropertyPath::parse("observedScale").getValue(buildingForm, expiredProbe) == Result::ExpiredCell);
        assert(PropertyPath::parse("observedScale").setValue(buildingForm, 10.0) == Result::ExpiredCell);

        Universe::instance().setProvider(nullptr);
    }

    // -------------------------------------------------------------------------
    // 7. Pass 2 Bug 1 & Bug 4: Weak write refusal & dynamicProperties synchronization
    // -------------------------------------------------------------------------
    {
        Object strongBearer;
        strongBearer.setObjectID("strong-bearer");
        strongBearer.setDynamicProperty("speed", 50.0);

        Object weakObserver;
        weakObserver.setObjectID("weak-observer");

        CellId speedCell = PropertyCellTable::instance().allocateCell(
            50.0, CellOwnershipMode::Shared, &strongBearer, Earthcall::StringInterner::intern("speed"));
        strongBearer.bindPropertyCell("speed", speedCell, CellOwnershipMode::Shared);

        // Bind weak observer to speedCell
        assert(weakObserver.bindPropertyCell("speed", speedCell, CellOwnershipMode::Weak));

        // Bug 1: Refuse writes through live weak observer
        assert(PropertyPath::parse("speed").setValue(weakObserver, 100.0) == Result::ReadOnly);
        assert(!weakObserver.setDynamicProperty("speed", 100.0));
        assert(weakObserver.getDynamicPropertyPtr(Earthcall::StringInterner::intern("speed")) == nullptr);

        // Value on strong bearer is unmodified
        PropertyValue vStrong;
        assert(PropertyPath::parse("speed").getValue(strongBearer, vStrong) == Result::Ok);
        assert(std::get<double>(vStrong) == 50.0);

        // Bug 4: DynamicProperties synchronization across shared observers
        Object sharedPeer;
        sharedPeer.setObjectID("shared-peer");
        assert(sharedPeer.bindPropertyCell("sharedSpeed", speedCell, CellOwnershipMode::Shared));

        // Mutate through strongBearer
        assert(strongBearer.setDynamicProperty("speed", 75.0));
        // Verify sharedPeer shadow dynamicProperties is synchronized immediately
        auto peerDyn = sharedPeer.dynamicProperties();
        auto itPeer = peerDyn.find(Earthcall::StringInterner::intern("sharedSpeed"));
        assert(itPeer != peerDyn.end());
        assert(std::get<double>(itPeer->second) == 75.0);

        // Move sharedPeer to movedPeer and verify retargeting in PropertyCellTable
        Object movedPeer = std::move(sharedPeer);
        assert(strongBearer.setDynamicProperty("speed", 90.0));
        auto movedDyn = movedPeer.dynamicProperties();
        auto itMoved = movedDyn.find(Earthcall::StringInterner::intern("sharedSpeed"));
        assert(itMoved != movedDyn.end());
        assert(std::get<double>(itMoved->second) == 90.0);
    }

    // -------------------------------------------------------------------------
    // 8. Pass 2 Bug 2: Nested Container Component Access inside a ValueCell
    // -------------------------------------------------------------------------
    {
        Object canvas;
        canvas.setObjectID("canvas");

        // Build a nested container: dict["color"] = glm::vec3(0.1f, 0.2f, 0.3f)
        auto dict = std::make_shared<PropertyDict>();
        dict->elements["color"] = glm::vec3(0.1f, 0.2f, 0.3f);
        PropertyValue containerVal(dict);

        CellId paintCell = PropertyCellTable::instance().allocateCell(
            containerVal, CellOwnershipMode::Shared, &canvas, Earthcall::StringInterner::intern("paint"));
        canvas.bindPropertyCell("paint", paintCell, CellOwnershipMode::Shared);

        // Read nested component paint.color.g
        PropertyValue gVal;
        assert(PropertyPath::parse("paint.color.g").getValue(canvas, gVal) == Result::Ok);
        assert(std::abs(std::get<float>(gVal) - 0.2f) < 1e-5f);

        // Write nested component paint.color.g
        std::string notified;
        Singular::setPropertyChangeCallback([&](Singular* owner, const std::string& name) {
            if (owner == &canvas) notified = name;
        });

        assert(PropertyPath::parse("paint.color.g").setValue(canvas, 0.85f) == Result::Ok);
        assert(notified == "paint");

        // Verify read-back reflects updated lane
        PropertyValue updatedG;
        assert(PropertyPath::parse("paint.color.g").getValue(canvas, updatedG) == Result::Ok);
        assert(std::abs(std::get<float>(updatedG) - 0.85f) < 1e-5f);

        Singular::setPropertyChangeCallback(nullptr);
    }

    // -------------------------------------------------------------------------
    // 9. Pass 2 Bug 3: BindingMode::Derive separate from ShareCell & Reactive Edges
    // -------------------------------------------------------------------------
    {
        Object srcObj;
        srcObj.setObjectID("src-obj");
        srcObj.setDynamicProperty("x", 5.0);

        Object destObj;
        destObj.setObjectID("dest-obj");
        destObj.setDynamicProperty("y", 0.0);

        std::vector<Singular*> beings{&srcObj, &destObj, &author};
        Universe::instance().setProvider([&](std::vector<Singular*>& out) {
            for (auto* b : beings) if (b) out.push_back(b);
        });

        // Map y := x + x with BindingMode::Derive
        auto x1 = std::make_unique<OntoMath::MathNode>();
        x1->op = OntoMath::MathNode::Op::ValueLeaf;
        x1->variableName = "x";

        auto x2 = std::make_unique<OntoMath::MathNode>();
        x2->op = OntoMath::MathNode::Op::ValueLeaf;
        x2->variableName = "x";

        auto addNode = std::make_shared<OntoMath::MathNode>();
        addNode->op = OntoMath::MathNode::Op::Add;
        addNode->children.push_back(std::move(x1));
        addNode->children.push_back(std::move(x2));

        auto doubleF = OntoMath::Piecewise::continuous(addNode);

        MathBindings deriveBinds;
        deriveBinds["x"] = MathBindingEntry{PropertyPath::parse("@src-obj.x"), BindingMode::Derive};

        ActionNode deriveAction = ActionNode::map("y", doubleF, deriveBinds);
        Law deriveLaw("derive-law");
        deriveLaw.addAuthor(author);
        deriveLaw.setActionModel(deriveAction);

        assert(deriveLaw.applyTo(destObj) == Law::ApplicationResult::Applied);

        // Initial derivation: y = 2 * 5.0 = 10.0
        PropertyValue yVal;
        assert(PropertyPath::parse("y").getValue(destObj, yVal) == Result::Ok);
        assert(std::get<double>(yVal) == 10.0);

        // Verify destObj and srcObj do NOT share the same mutable cell
        assert(destObj.getPropertyCell(Earthcall::StringInterner::intern("y")) !=
               srcObj.getPropertyCell(Earthcall::StringInterner::intern("x")));

        // Reactive recomputation: mutate x on srcObj -> y on destObj updates automatically!
        assert(PropertyPath::parse("x").setValue(srcObj, 7.0) == Result::Ok);

        PropertyValue yUpdated;
        assert(PropertyPath::parse("y").getValue(destObj, yUpdated) == Result::Ok);
        assert(std::get<double>(yUpdated) == 14.0);

        Universe::instance().setProvider(nullptr);
    }

    // -------------------------------------------------------------------------
    // 10. Pass 2 Rung 3: Relation-Based Authority DAG Precedence (AD-24, AD-37)
    // -------------------------------------------------------------------------
    {
        Law rootLaw("root-law");
        rootLaw.addAuthor(author);
        rootLaw.grantAuthority(0);

        Law intermediateLaw("intermediate-law");
        intermediateLaw.addAuthor(author);
        intermediateLaw.grantAuthority(0);

        Law leafLaw("leaf-law");
        leafLaw.addAuthor(author);
        leafLaw.grantAuthority(0);

        // Establish authority-over relations: rootLaw -> intermediateLaw -> leafLaw
        Relation rel1("authority-over", rootLaw, intermediateLaw, true);
        Relation rel2("authority-over", intermediateLaw, leafLaw, true);

        std::vector<Relation*> authRelations{&rel1, &rel2};
        Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
            for (auto* r : authRelations) if (r) out.push_back(r);
        });

        assert(rootLaw.hasAuthorityOver(intermediateLaw));
        assert(intermediateLaw.hasAuthorityOver(leafLaw));
        // Transitive reachability along DAG
        assert(rootLaw.hasAuthorityOver(leafLaw));

        assert(!leafLaw.hasAuthorityOver(rootLaw));
        assert(!leafLaw.hasAuthorityOver(intermediateLaw));

        assert(rootLaw.hasEqualOrHigherStandingThan(leafLaw));
        assert(!leafLaw.hasEqualOrHigherStandingThan(rootLaw));

        Universe::instance().setRelationProvider(nullptr);
    }

    // -------------------------------------------------------------------------
    // 11. Pass 2 Rung 3: Policy-Driven Cell Access & AD-38 Self-Elevation Refusal
    // -------------------------------------------------------------------------
    {
        TransferPolicy::instance().clearPolicyRules();

        Object vault;
        vault.setObjectID("vault");
        vault.setDynamicProperty("secret", 1234.0);

        Object vaultMirror;
        vaultMirror.setObjectID("vault-mirror");

        // Cell backing secret
        CellId secretCell = PropertyCellTable::instance().allocateCell(
            1234.0, CellOwnershipMode::Shared, &vault, Earthcall::StringInterner::intern("secret"));
        vault.bindPropertyCell("secret", secretCell, CellOwnershipMode::Shared);
        vaultMirror.bindPropertyCell("secretAlias", secretCell, CellOwnershipMode::Shared);

        // Open-by-default migration baseline (AD-37): revision == 0 allows writes
        assert(TransferPolicy::instance().accessPolicyRevision() == 0);
        assert(PropertyPath::parse("secret").setValue(vault, 5678.0) == Result::Ok);

        // Authority laws
        Law highLaw("high-law");
        highLaw.addAuthor(author);
        highLaw.grantAuthority(10);

        Law lowLaw("low-law");
        lowLaw.addAuthor(author);
        lowLaw.grantAuthority(-5);

        // Add policy rule: secretCell write is DENIED by highLaw
        PolicyRule denySecretWrite;
        denySecretWrite.cellId = secretCell;
        denySecretWrite.op = AccessOp::Write;
        denySecretWrite.allow = false;
        denySecretWrite.governingLaw = &highLaw;
        TransferPolicy::instance().addPolicyRule(denySecretWrite);

        // Effective-cell alias governance: writing to vault.secret OR vaultMirror.secretAlias is AccessDenied!
        assert(PropertyPath::parse("secret").setValue(vault, 9999.0) == Result::AccessDenied);
        assert(PropertyPath::parse("secretAlias").setValue(vaultMirror, 9999.0) == Result::AccessDenied);

        // Conflict resolution: lowLaw tries to allow it
        PolicyRule allowSecretWrite;
        allowSecretWrite.cellId = secretCell;
        allowSecretWrite.op = AccessOp::Write;
        allowSecretWrite.allow = true;
        allowSecretWrite.governingLaw = &lowLaw;
        TransferPolicy::instance().addPolicyRule(allowSecretWrite);

        // HighLaw authority over lowLaw -> highLaw still wins (AccessDenied)
        assert(PropertyPath::parse("secret").setValue(vault, 9999.0) == Result::AccessDenied);

        // AD-38: Self-elevation refusal: lowLaw cannot elevate its authorityLevel or add authority-over over highLaw
        ActionNode elevateSelf = ActionNode::set("authorityLevel", 100);
        lowLaw.setActionModel(elevateSelf);
        assert(lowLaw.applyTo(lowLaw) == Law::ApplicationResult::AuthorityDenied);

        // Firmament Pass 2 Corrective Fix Witness 1: Ambient ExecutionScope on Set/Add
        // Policy rule that allows writes ONLY when authorizingLaw == &highLaw
        TransferPolicy::instance().clearPolicyRules();
        PolicyRule allowHighLawOnly;
        allowHighLawOnly.cellId = secretCell;
        allowHighLawOnly.op = AccessOp::Write;
        allowHighLawOnly.allow = false;
        allowHighLawOnly.governingLaw = &highLaw;
        allowHighLawOnly.filter = [&](const AccessContext& ctx) {
            return ctx.authorizingLaw != &highLaw;
        };
        TransferPolicy::instance().addPolicyRule(allowHighLawOnly);

        Law writerLow("writer-low");
        writerLow.addAuthor(author);
        writerLow.setActionModel(ActionNode::set("secret", 7777.0));
        assert(writerLow.applyTo(vault) == Law::ApplicationResult::Applied);
        assert(!writerLow.applicationLog().back().changedSomething());
        assert(writerLow.applicationLog().back().trace.nodes.front().reason == Result::AccessDenied);
        assert(std::string(ActionNode::reasonName(Result::AccessDenied)) == "Access Denied");

        highLaw.setActionModel(ActionNode::set("secret", 8888.0));
        assert(highLaw.applyTo(vault) == Law::ApplicationResult::Applied);
        assert(highLaw.applicationLog().back().changedSomething());
        PropertyValue afterHigh;
        assert(PropertyPath::parse("secret").getValue(vault, afterHigh) == Result::Ok);
        assert(std::get<double>(afterHigh) == 8888.0);

        // Firmament Pass 2 Corrective Fix Witness 2: UniqueCell custody transfer via ActionNode::uniqueCell
        TransferPolicy::instance().clearPolicyRules();
        Object tokenSrc;
        tokenSrc.setObjectID("token-src");
        tokenSrc.setDynamicProperty("badge", std::string("gold-seal"));
        Object tokenDst;
        tokenDst.setObjectID("token-dst");
        std::vector<Singular*> tokenBeings{&tokenSrc, &tokenDst, &author};
        Universe::instance().setProvider([&](std::vector<Singular*>& out) {
            for (auto* b : tokenBeings) if (b) out.push_back(b);
        });
        Law transferUniqueLaw("transfer-unique-law");
        transferUniqueLaw.addAuthor(author);
        transferUniqueLaw.setActionModel(ActionNode::uniqueCell("badge", "@token-src.badge"));
        assert(transferUniqueLaw.applyTo(tokenDst) == Law::ApplicationResult::Applied);
        assert(transferUniqueLaw.applicationLog().back().changedSomething());
        assert(tokenDst.getPropertyCellMode("badge") == CellOwnershipMode::Unique);
        PropertyValue badgeVal;
        assert(PropertyPath::parse("badge").getValue(tokenDst, badgeVal) == Result::Ok);
        assert(std::get<std::string>(badgeVal) == "gold-seal");
        Universe::instance().setProvider(nullptr);

        // Firmament Pass 2 Corrective Fix Witness 3: Cyclic Derive guard & DerivedEdge subject/law destruction safety
        {
            Object cycleA;
            cycleA.setObjectID("cycle-a");
            cycleA.setDynamicProperty("v", 2.0);
            Object cycleB;
            cycleB.setObjectID("cycle-b");
            cycleB.setDynamicProperty("v", 2.0);
            auto tempController = std::make_unique<Object>();
            tempController->setObjectID("temp-controller");

            std::vector<Singular*> cycleBeings{&cycleA, &cycleB, tempController.get(), &author};
            Universe::instance().setProvider([&](std::vector<Singular*>& out) {
                for (auto* b : cycleBeings) if (b) out.push_back(b);
            });

            auto leafV = std::make_shared<OntoMath::MathNode>();
            leafV->op = OntoMath::MathNode::Op::ValueLeaf;
            leafV->variableName = "in";
            auto idFn = OntoMath::Piecewise::continuous(leafV);

            MathBindings bAtoB;
            bAtoB["in"] = MathBindingEntry{PropertyPath::parse("@cycle-a.v"), BindingMode::Derive};
            Law lawAtoB("law-a-to-b");
            lawAtoB.addAuthor(author);
            lawAtoB.setActionModel(ActionNode::map("@cycle-b.v", idFn, bAtoB));
            assert(lawAtoB.applyTo(*tempController) == Law::ApplicationResult::Applied);

            MathBindings bBtoA;
            bBtoA["in"] = MathBindingEntry{PropertyPath::parse("@cycle-b.v"), BindingMode::Derive};
            Law lawBtoA("law-b-to-a");
            lawBtoA.addAuthor(author);
            lawBtoA.setActionModel(ActionNode::map("@cycle-a.v", idFn, bBtoA));
            assert(lawBtoA.applyTo(*tempController) == Law::ApplicationResult::Applied);

            // Mutating cycleA.v must terminate cleanly without infinite recursion
            assert(PropertyPath::parse("v").setValue(cycleA, 9.0) == Result::Ok);
            PropertyValue bCheck;
            assert(PropertyPath::parse("v").getValue(cycleB, bCheck) == Result::Ok);
            assert(std::get<double>(bCheck) == 9.0);

            // Destroying tempController (the subjectBearer of the DerivedEdge) must detach the edge cleanly
            tempController.reset();
            cycleBeings[2] = nullptr;
            assert(PropertyPath::parse("v").setValue(cycleA, 15.0) == Result::Ok);
            assert(PropertyPath::parse("v").getValue(cycleB, bCheck) == Result::Ok);
            assert(std::get<double>(bCheck) == 9.0);

            Universe::instance().setProvider(nullptr);
        }

        // Firmament Pass 2 Corrective Fix Witness 4: TransferPolicy auto-removes rules when governingLaw dies
        {
            auto ephemeralLaw = std::make_unique<Law>("ephemeral-law");
            ephemeralLaw->addAuthor(author);
            PolicyRule r;
            r.cellId = secretCell;
            r.op = AccessOp::Write;
            r.allow = false;
            r.governingLaw = ephemeralLaw.get();
            TransferPolicy::instance().addPolicyRule(r);
            assert(PropertyPath::parse("secret").setValue(vault, 1111.0) == Result::AccessDenied);

            // Destroying ephemeralLaw automatically purges its rule and restores revision 0
            ephemeralLaw.reset();
            assert(TransferPolicy::instance().accessPolicyRevision() == 0);
            assert(PropertyPath::parse("secret").setValue(vault, 1111.0) == Result::Ok);
        }

        TransferPolicy::instance().clearPolicyRules();
    }

    // -------------------------------------------------------------------------
    // 12. Pass 3 Task 1 (Rung 2): Law Line Grammar & Parser for Cell Bindings
    // -------------------------------------------------------------------------
    {
        using namespace Singularity::Terminal;
        LawSentence::Vocabulary vocab;
        vocab.words = LawSentence::canonicalWords();
        vocab.events = {"trigger_cell"};

        // Form 1: share <destPath> from <srcPath>
        {
            auto p = LawSentence::parse("on trigger_cell then share @dst.color from @src.color", vocab);
            assert(p.ok);
            assert(p.action.has_value());
            assert(p.action->kind == ActionNode::Kind::Map);
            assert(p.action->describe() == "share @dst.color from @src.color");

            Object srcObj; srcObj.setObjectID("src");
            Object dstObj; dstObj.setObjectID("dst");
            srcObj.setDynamicProperty("color", glm::vec3(1.0f, 0.0f, 0.0f));
            dstObj.setDynamicProperty("color", glm::vec3(0.0f, 1.0f, 0.0f));

            std::vector<Singular*> testBeings{&srcObj, &dstObj, &author};
            Universe::instance().setProvider([&](std::vector<Singular*>& out) {
                for (auto* b : testBeings) if (b) out.push_back(b);
            });

            Law shareLaw("test-share-law");
            shareLaw.addAuthor(author);
            shareLaw.setActionModel(*p.action);
            assert(shareLaw.applyTo(dstObj) == Law::ApplicationResult::Applied);

            assert(dstObj.hasPropertyCell("color"));
            assert(srcObj.hasPropertyCell("color"));
            assert(dstObj.getPropertyCell("color") == srcObj.getPropertyCell("color"));
            assert(dstObj.getPropertyCellMode("color") == CellOwnershipMode::Shared);

            // Mutating through dst mutates shared cell
            assert(PropertyPath::parse("color").setValue(dstObj, glm::vec3(0.5f, 0.5f, 0.5f)) == Result::Ok);
            PropertyValue readSrc;
            assert(PropertyPath::parse("color").getValue(srcObj, readSrc) == Result::Ok);
            assert(std::get<glm::vec3>(readSrc) == glm::vec3(0.5f, 0.5f, 0.5f));

            Universe::instance().setProvider(nullptr);
        }

        // Form 2: claim / unique <destPath> from <srcPath>
        {
            auto pClaim = LawSentence::parse("on trigger_cell then claim @dst.badge from @src.badge", vocab);
            assert(pClaim.ok);
            assert(pClaim.action.has_value());
            assert(pClaim.action->describe() == "claim @dst.badge from @src.badge");

            auto pUnique = LawSentence::parse("on trigger_cell then unique @dst.badge from @src.badge", vocab);
            assert(pUnique.ok);
            assert(pUnique.action.has_value());
            assert(pUnique.action->describe() == "claim @dst.badge from @src.badge");

            Object srcObj; srcObj.setObjectID("src");
            Object dstObj; dstObj.setObjectID("dst");
            srcObj.setDynamicProperty("badge", std::string("gold"));
            dstObj.setDynamicProperty("badge", std::string("none"));

            std::vector<Singular*> testBeings{&srcObj, &dstObj, &author};
            Universe::instance().setProvider([&](std::vector<Singular*>& out) {
                for (auto* b : testBeings) if (b) out.push_back(b);
            });

            Law claimLaw("test-claim-law");
            claimLaw.addAuthor(author);
            claimLaw.setActionModel(*pClaim.action);
            assert(claimLaw.applyTo(dstObj) == Law::ApplicationResult::Applied);

            assert(dstObj.hasPropertyCell("badge"));
            assert(dstObj.getPropertyCellMode("badge") == CellOwnershipMode::Unique);
            PropertyValue bVal;
            assert(PropertyPath::parse("badge").getValue(dstObj, bVal) == Result::Ok);
            assert(std::get<std::string>(bVal) == "gold");

            Universe::instance().setProvider(nullptr);
        }

        // Form 3: borrow / observe <destPath> from <srcPath>
        {
            auto pBorrow = LawSentence::parse("on trigger_cell then borrow @dst.stat from @src.stat", vocab);
            assert(pBorrow.ok);
            assert(pBorrow.action.has_value());
            assert(pBorrow.action->describe() == "borrow @dst.stat from @src.stat");

            auto pObserve = LawSentence::parse("on trigger_cell then observe @dst.stat from @src.stat", vocab);
            assert(pObserve.ok);
            assert(pObserve.action.has_value());
            assert(pObserve.action->describe() == "borrow @dst.stat from @src.stat");

            Object srcObj; srcObj.setObjectID("src");
            Object dstObj; dstObj.setObjectID("dst");
            srcObj.setDynamicProperty("stat", 100.0);
            dstObj.setDynamicProperty("stat", 0.0);

            std::vector<Singular*> testBeings{&srcObj, &dstObj, &author};
            Universe::instance().setProvider([&](std::vector<Singular*>& out) {
                for (auto* b : testBeings) if (b) out.push_back(b);
            });

            Law borrowLaw("test-borrow-law");
            borrowLaw.addAuthor(author);
            borrowLaw.setActionModel(*pBorrow.action);
            assert(borrowLaw.applyTo(dstObj) == Law::ApplicationResult::Applied);

            assert(dstObj.hasPropertyCell("stat"));
            assert(dstObj.getPropertyCellMode("stat") == CellOwnershipMode::Weak);
            assert(srcObj.getPropertyCellMode("stat") == CellOwnershipMode::Shared);

            PropertyValue readWeak;
            assert(PropertyPath::parse("stat").getValue(dstObj, readWeak) == Result::Ok);
            assert(std::get<double>(readWeak) == 100.0);

            // Weak observer cannot write directly
            assert(PropertyPath::parse("stat").setValue(dstObj, 200.0) == Result::ReadOnly);

            Universe::instance().setProvider(nullptr);
        }

        // Form 4: derive <destPath> = <mathExpr>
        {
            auto pDerive = LawSentence::parse("on trigger_cell then derive @dst.reading = @src.val * 2", vocab);
            assert(pDerive.ok);
            assert(pDerive.action.has_value());
            assert(pDerive.action->kind == ActionNode::Kind::Map);

            Object srcObj; srcObj.setObjectID("src");
            Object dstObj; dstObj.setObjectID("dst");
            srcObj.setDynamicProperty("val", 15.0);
            dstObj.setDynamicProperty("reading", 0.0);

            std::vector<Singular*> testBeings{&srcObj, &dstObj, &author};
            Universe::instance().setProvider([&](std::vector<Singular*>& out) {
                for (auto* b : testBeings) if (b) out.push_back(b);
            });

            Law deriveLaw("test-derive-law");
            deriveLaw.addAuthor(author);
            deriveLaw.setActionModel(*pDerive.action);
            assert(deriveLaw.applyTo(dstObj) == Law::ApplicationResult::Applied);

            PropertyValue initialReading;
            assert(PropertyPath::parse("reading").getValue(dstObj, initialReading) == Result::Ok);
            assert(std::get<double>(initialReading) == 30.0);

            // Reactive update via DerivedEdge: mutate src.val -> dst.reading updates automatically
            assert(PropertyPath::parse("val").setValue(srcObj, 25.0) == Result::Ok);
            PropertyValue updatedReading;
            assert(PropertyPath::parse("reading").getValue(dstObj, updatedReading) == Result::Ok);
            assert(std::get<double>(updatedReading) == 50.0);

            Universe::instance().setProvider(nullptr);
        }
    }

    // -------------------------------------------------------------------------
    // 13. Pass 3 Task 2 (Rung 4): Durable Alias Topology Save/Load Serialization
    // -------------------------------------------------------------------------
    {
        Object bearer1; bearer1.setObjectID("bearer1");
        Object bearer2; bearer2.setObjectID("bearer2");
        Object bearer3; bearer3.setObjectID("bearer3");

        // Set up: bearer1 & bearer2 share "matrix" cell (Shared), bearer3 observes "matrix" (Weak)
        Earthcall::StringId propMatrix = Earthcall::StringInterner::intern("matrix");
        CellId origCell = PropertyCellTable::instance().allocateCell(
            123.45, CellOwnershipMode::Shared, &bearer1, propMatrix);
        bearer1.bindPropertyCell(propMatrix, origCell, CellOwnershipMode::Shared);
        PropertyCellTable::instance().attachObserver(origCell, &bearer2, propMatrix, CellOwnershipMode::Shared);
        bearer2.bindPropertyCell(propMatrix, origCell, CellOwnershipMode::Shared);
        PropertyCellTable::instance().attachObserver(origCell, &bearer3, propMatrix, CellOwnershipMode::Weak);
        bearer3.bindPropertyCell(propMatrix, origCell, CellOwnershipMode::Weak);

        // Serialize cohort
        auto serializedCohort = SingularPersistence::serializeCohort({&bearer1, &bearer2, &bearer3});
        assert(serializedCohort.is_array());
        assert(serializedCohort.size() == 3);

        // Reconstruct into fresh beings
        Object reload1; reload1.setObjectID("bearer1");
        Object reload2; reload2.setObjectID("bearer2");
        Object reload3; reload3.setObjectID("bearer3");

        SingularPersistence::CohortContext loadCohort;
        SingularPersistence::deserializeCohort({&reload1, &reload2, &reload3}, serializedCohort, loadCohort);

        // Invariant: Reloaded bearers must share the EXACT SAME live CellId
        assert(reload1.hasPropertyCell("matrix"));
        assert(reload2.hasPropertyCell("matrix"));
        assert(reload3.hasPropertyCell("matrix"));
        CellId reloadedCell1 = reload1.getPropertyCell("matrix");
        CellId reloadedCell2 = reload2.getPropertyCell("matrix");
        CellId reloadedCell3 = reload3.getPropertyCell("matrix");
        assert(reloadedCell1.valid());
        assert(reloadedCell1 == reloadedCell2);
        assert(reloadedCell1 == reloadedCell3);
        assert(reload1.getPropertyCellMode("matrix") == CellOwnershipMode::Shared);
        assert(reload2.getPropertyCellMode("matrix") == CellOwnershipMode::Shared);
        assert(reload3.getPropertyCellMode("matrix") == CellOwnershipMode::Weak);

        // Verify initial value
        PropertyValue v1;
        assert(PropertyPath::parse("matrix").getValue(reload1, v1) == Result::Ok);
        assert(std::get<double>(v1) == 123.45);

        // Mutate through reload1 -> reload2 and reload3 immediately read new value
        assert(PropertyPath::parse("matrix").setValue(reload1, 987.65) == Result::Ok);
        PropertyValue v2, v3;
        assert(PropertyPath::parse("matrix").getValue(reload2, v2) == Result::Ok);
        assert(PropertyPath::parse("matrix").getValue(reload3, v3) == Result::Ok);
        assert(std::get<double>(v2) == 987.65);
        assert(std::get<double>(v3) == 987.65);

        // Unbind both strong owners -> cell is reclaimed and weak observer expires
        reload1.unbindPropertyCell("matrix");
        reload2.unbindPropertyCell("matrix");
        PropertyValue vExpired;
        assert(PropertyPath::parse("matrix").getValue(reload3, vExpired) == Result::ExpiredCell);

        // Firmament Pass 3 Corrective Fix Witness 1: Order-independent deserialization
        // (Weak observer serialized BEFORE Shared owners) + orphaned Weak expiry on finalize
        {
            auto reverseCohort = nlohmann::json::array({
                serializedCohort[2], // bearer3 (Weak) FIRST
                serializedCohort[0], // bearer1 (Shared)
                serializedCohort[1]  // bearer2 (Shared)
            });
            Object rev1; rev1.setObjectID("bearer1");
            Object rev2; rev2.setObjectID("bearer2");
            Object rev3; rev3.setObjectID("bearer3");
            SingularPersistence::CohortContext revCtx;
            SingularPersistence::deserializeCohort({&rev1, &rev2, &rev3}, reverseCohort, revCtx);
            assert(rev1.getPropertyCell("matrix") == rev2.getPropertyCell("matrix"));
            assert(rev1.getPropertyCell("matrix") == rev3.getPropertyCell("matrix"));
            assert(rev3.getPropertyCellMode("matrix") == CellOwnershipMode::Weak);
            PropertyValue vRev;
            assert(PropertyPath::parse("matrix").getValue(rev3, vRev) == Result::Ok);
            assert(std::get<double>(vRev) == 123.45);

            // Orphaned Weak-only cohort expires on finalize
            auto orphanCohort = nlohmann::json::array({ serializedCohort[2] });
            Object orphanWeak; orphanWeak.setObjectID("bearer3");
            SingularPersistence::CohortContext orphanCtx;
            SingularPersistence::deserializeCohort({&orphanWeak}, orphanCohort, orphanCtx);
            assert(PropertyPath::parse("matrix").getValue(orphanWeak, vRev) == Result::ExpiredCell);
        }

        // Firmament Pass 3 Corrective Fix Witness 2: Object to_json/from_json & Lexeme classify()
        {
            Object objA; objA.setObjectID("obj-a");
            Object objB; objB.setObjectID("obj-b");
            CellId sharedC = PropertyCellTable::instance().allocateCell(
                glm::vec3(0.2f, 0.4f, 0.8f), CellOwnershipMode::Shared, &objA, Earthcall::StringInterner::intern("tint"));
            objA.bindPropertyCell("tint", sharedC, CellOwnershipMode::Shared);
            objB.bindPropertyCell("tint", sharedC, CellOwnershipMode::Shared);

            nlohmann::json jA = objA;
            nlohmann::json jB = objB;

            Object hydratedA, hydratedB;
            {
                SingularPersistence::CohortScope zoneLoadScope(true);
                from_json(jA, hydratedA);
                from_json(jB, hydratedB);
            }
            assert(hydratedA.getPropertyCell("tint").valid());
            assert(hydratedA.getPropertyCell("tint") == hydratedB.getPropertyCell("tint"));
            assert(PropertyPath::parse("tint.r").setValue(hydratedA, 0.9f) == Result::Ok);
            PropertyValue readTintR;
            assert(PropertyPath::parse("tint.r").getValue(hydratedB, readTintR) == Result::Ok);
            assert(std::abs(std::get<float>(readTintR) - 0.9f) < 1e-5f);

            // Verify open Share/Unique/ObserveWeak/Derive Law classification for denoted Lexemes & definedFor()
            Law openShareLaw("open-share");
            openShareLaw.setActionModel(ActionNode::shareCell("", ""));
            Singularity::Terminal::LawSentence::Preset preset;
            assert(Singularity::Terminal::LawSentence::classify(openShareLaw, {}, preset) == "action.Share");

            ActionNode boundShare = ActionNode::shareCell("tint", "tint");
            assert(boundShare.definedFor(hydratedA));
        }
    }

    // -------------------------------------------------------------------------
    // 14. Pass 3 Task 3 (Rung 5): Compiled PropertyPath Cache Benchmark
    // -------------------------------------------------------------------------
    {
        Object benchSubject;
        benchSubject.setObjectID("bench");
        benchSubject.setDynamicProperty("health", 100.0);
        benchSubject.setDynamicProperty("velocity", glm::vec3(1.0f, 2.0f, 3.0f));

        const std::string pathStr = "velocity.y";
        const auto compiled = PropertyPath::compile(pathStr);

        const int iterations = 10000;
        PropertyValue rawVal;
        PropertyValue compVal;

        // Loop A: Raw string parse + resolve
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            auto res = PropertyPath::parse(pathStr).getValue(benchSubject, rawVal);
            (void)res;
        }
        auto t1 = std::chrono::high_resolution_clock::now();

        // Loop B: CompiledPath resolve
        auto t2 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            auto res = compiled.getValue(benchSubject, compVal);
            (void)res;
        }
        auto t3 = std::chrono::high_resolution_clock::now();

        // Verify zero semantic drift
        assert(std::holds_alternative<float>(rawVal) || std::holds_alternative<double>(rawVal));
        assert(rawVal == compVal);

        auto rawNs = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        auto compNs = std::chrono::duration_cast<std::chrono::microseconds>(t3 - t2).count();
        double speedup = compNs > 0 ? static_cast<double>(rawNs) / compNs : 1.0;
        std::cout << "[Benchmark] Raw string path 10k: " << rawNs << " us, CompiledPath 10k: "
                  << compNs << " us (" << speedup << "x speedup)\n";

        // ActionNode::Kind::Map benchmark: out_val := v + v over 10,000 ticks
        benchSubject.setDynamicProperty("in_val", 21.0);
        benchSubject.setDynamicProperty("out_val", 0.0);

        auto v1 = std::make_unique<OntoMath::MathNode>();
        v1->op = OntoMath::MathNode::Op::ValueLeaf;
        v1->variableName = "v";

        auto v2 = std::make_unique<OntoMath::MathNode>();
        v2->op = OntoMath::MathNode::Op::ValueLeaf;
        v2->variableName = "v";

        auto addNode = std::make_shared<OntoMath::MathNode>();
        addNode->op = OntoMath::MathNode::Op::Add;
        addNode->children.push_back(std::move(v1));
        addNode->children.push_back(std::move(v2));

        auto doubleF = OntoMath::Piecewise::continuous(addNode);

        MathBindings mapBinds;
        mapBinds["v"] = MathBindingEntry{PropertyPath::parse("in_val")};
        auto mapAction = ActionNode::map("out_val", doubleF, mapBinds);
        auto compiledMap = mapAction.compile();

        ECA::Event dummyEvent{"tick", &benchSubject, nullptr};
        auto tMap0 = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < iterations; ++i) {
            compiledMap(dummyEvent, benchSubject);
        }
        auto tMap1 = std::chrono::high_resolution_clock::now();
        auto mapUs = std::chrono::duration_cast<std::chrono::microseconds>(tMap1 - tMap0).count();

        PropertyValue outVal;
        assert(PropertyPath::parse("out_val").getValue(benchSubject, outVal) == Result::Ok);
        assert(std::get<double>(outVal) == 42.0);
        std::cout << "[Benchmark] ActionNode::Kind::Map 10k ticks: " << mapUs << " us ("
                  << (static_cast<double>(mapUs) / iterations) << " us/tick)\n";
    }

    // -------------------------------------------------------------------------
    // 15. Pass 4 Task 2: End-to-End Zone Round-Trip (zoneToJson / makeZoneFromJson)
    // -------------------------------------------------------------------------
    {
        Zone zone("roundtrip-zone", "default");
        auto obj1 = std::make_shared<Object>();
        obj1->setObjectID("obj1");
        obj1->setFaceColor(0, 0.1f, 0.2f, 0.3f);

        auto obj2 = std::make_shared<Object>();
        obj2->setObjectID("obj2");
        obj2->setFaceColor(0, 0.0f, 0.0f, 0.0f);

        auto obj3 = std::make_shared<Object>();
        obj3->setObjectID("obj3");

        auto storedLexeme = std::make_shared<Singularity::Language::Lexeme>("shard", "lex_shard");

        // 1. Shared cell on obj1.color and obj2.color (Shared), observed by obj3.watchedColor (Weak)
        Earthcall::StringId propColor = Earthcall::StringInterner::intern("color");
        CellId colorCell = PropertyCellTable::instance().allocateCell(
            glm::vec3(0.1f, 0.2f, 0.3f), CellOwnershipMode::Shared, obj1.get(), propColor);
        obj1->bindPropertyCell(propColor, colorCell, CellOwnershipMode::Shared);
        PropertyCellTable::instance().attachObserver(colorCell, obj2.get(), propColor, CellOwnershipMode::Shared);
        obj2->bindPropertyCell(propColor, colorCell, CellOwnershipMode::Shared);

        Earthcall::StringId propWatched = Earthcall::StringInterner::intern("watchedColor");
        PropertyCellTable::instance().attachObserver(colorCell, obj3.get(), propWatched, CellOwnershipMode::Weak);
        obj3->bindPropertyCell(propWatched, colorCell, CellOwnershipMode::Weak);

        // 2. Cross-bucket sharing: obj1 (world.objects) and storedLexeme (storedSingulars)
        Earthcall::StringId propSharedTag = Earthcall::StringInterner::intern("sharedTag");
        CellId tagCell = PropertyCellTable::instance().allocateCell(
            std::string("crystal-shard"), CellOwnershipMode::Shared, obj1.get(), propSharedTag);
        obj1->bindPropertyCell(propSharedTag, tagCell, CellOwnershipMode::Shared);
        PropertyCellTable::instance().attachObserver(tagCell, storedLexeme.get(), propSharedTag, CellOwnershipMode::Shared);
        storedLexeme->bindPropertyCell(propSharedTag, tagCell, CellOwnershipMode::Shared);

        zone.addObject(obj1);
        zone.addObject(obj2);
        zone.addObject(obj3);
        zone.retainSingular(storedLexeme);

        // Round-trip serialize & deserialize Zone
        nlohmann::json zj = zoneToJson(zone);
        std::shared_ptr<Zone> reloadedZone = makeZoneFromJson(zj);
        assert(reloadedZone != nullptr);

        const auto& reloadedObjs = reloadedZone->objects();
        assert(reloadedObjs.size() == 3);
        Object* rObj1 = nullptr;
        Object* rObj2 = nullptr;
        Object* rObj3 = nullptr;
        for (const auto& o : reloadedObjs) {
            if (o->getObjectID() == "obj1") rObj1 = o.get();
            else if (o->getObjectID() == "obj2") rObj2 = o.get();
            else if (o->getObjectID() == "obj3") rObj3 = o.get();
        }
        assert(rObj1 && rObj2 && rObj3);

        const auto& reloadedStored = reloadedZone->storedSingulars();
        assert(!reloadedStored.empty());
        Singularity::Language::Lexeme* rLex = nullptr;
        for (const auto& s : reloadedStored) {
            if (auto* lex = dynamic_cast<Singularity::Language::Lexeme*>(s.get())) {
                if (lex->getIdentifier() == "lex_shard") rLex = lex;
            }
        }
        assert(rLex != nullptr);

        // Invariant: Live CellId equality across hydrated objects
        assert(rObj1->hasPropertyCell("color"));
        assert(rObj2->hasPropertyCell("color"));
        assert(rObj3->hasPropertyCell("watchedColor"));
        CellId cell1 = rObj1->getPropertyCell("color");
        CellId cell2 = rObj2->getPropertyCell("color");
        CellId cell3 = rObj3->getPropertyCell("watchedColor");
        assert(cell1.valid());
        assert(cell1 == cell2);
        assert(cell1 == cell3);
        assert(rObj1->getPropertyCellMode("color") == CellOwnershipMode::Shared);
        assert(rObj2->getPropertyCellMode("color") == CellOwnershipMode::Shared);
        assert(rObj3->getPropertyCellMode("watchedColor") == CellOwnershipMode::Weak);

        // Verify initial color value on both ValueCell and underlying C++ faceColors
        PropertyValue readCol;
        assert(PropertyPath::parse("color").getValue(*rObj1, readCol) == Result::Ok);
        assert(std::get<glm::vec3>(readCol) == glm::vec3(0.1f, 0.2f, 0.3f));
        assert(std::abs(rObj2->faceColors[0][0] - 0.1f) < 1e-5f);
        assert(std::abs(rObj2->faceColors[0][1] - 0.2f) < 1e-5f);
        assert(std::abs(rObj2->faceColors[0][2] - 0.3f) < 1e-5f);

        // Mutating rObj1.color propagates to rObj2 and weak observer rObj3
        assert(PropertyPath::parse("color").setValue(*rObj1, glm::vec3(0.7f, 0.8f, 0.9f)) == Result::Ok);
        PropertyValue v2, v3;
        assert(PropertyPath::parse("color").getValue(*rObj2, v2) == Result::Ok);
        assert(PropertyPath::parse("watchedColor").getValue(*rObj3, v3) == Result::Ok);
        assert(std::get<glm::vec3>(v2) == glm::vec3(0.7f, 0.8f, 0.9f));
        assert(std::get<glm::vec3>(v3) == glm::vec3(0.7f, 0.8f, 0.9f));

        // Cross-bucket sharing between world.objects and storedSingulars
        assert(rObj1->hasPropertyCell("sharedTag"));
        assert(rLex->hasPropertyCell("sharedTag"));
        CellId tagCell1 = rObj1->getPropertyCell("sharedTag");
        CellId tagCellLex = rLex->getPropertyCell("sharedTag");
        assert(tagCell1.valid());
        assert(tagCell1 == tagCellLex);

        PropertyValue tagVal;
        assert(PropertyPath::parse("sharedTag").getValue(*rLex, tagVal) == Result::Ok);
        assert(std::get<std::string>(tagVal) == "crystal-shard");

        assert(PropertyPath::parse("sharedTag").setValue(*rObj1, std::string("stellar-diamond")) == Result::Ok);
        PropertyValue newTagVal;
        assert(PropertyPath::parse("sharedTag").getValue(*rLex, newTagVal) == Result::Ok);
        assert(std::get<std::string>(newTagVal) == "stellar-diamond");

        // Weak observer expiration upon unbinding both strong owners
        rObj1->unbindPropertyCell("color");
        rObj2->unbindPropertyCell("color");
        PropertyValue expVal;
        assert(PropertyPath::parse("watchedColor").getValue(*rObj3, expVal) == Result::ExpiredCell);
    }

    // -------------------------------------------------------------------------
    // 16. Pass 5 Quest 3: Zone-Governed Inter-Zone Access Hook on TransferPolicy
    // -------------------------------------------------------------------------
    {
        TransferPolicy::instance().clearPolicyRules();

        Zone zoneA("zoneA", "symA");
        Zone zoneB("zoneB", "symB");

        Object authorA; authorA.setObjectID("authorA");
        Object authorB; authorB.setObjectID("authorB");

        Law lawA("lawA");
        lawA.addAuthor(authorA);

        Law lawB_low("lawB-low");
        lawB_low.addAuthor(authorB);

        Law lawB_high("lawB-high");
        lawB_high.addAuthor(authorB);

        // Relation establishing lawB_high authority-over lawB_low
        Relation relAuthority("authority-over", lawB_high, lawB_low, true);
        std::vector<Relation*> authRelations{&relAuthority};
        Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
            for (auto* r : authRelations) if (r) out.push_back(r);
        });
        assert(lawB_high.hasAuthorityOver(lawB_low));

        Object targetObj;
        targetObj.setObjectID("target-obj");
        targetObj.setDynamicProperty("data", 10.0);

        AccessContext interCtx;
        interCtx.originZone = &zoneA;
        interCtx.targetZone = &zoneB;
        interCtx.op = AccessOp::Write;

        // 1. Initially open-by-default: write succeeds
        assert(PropertyPath::parse("data").setValue(targetObj, 20.0, 0, interCtx) == Result::Ok);

        // 2. Zone A allows outbound writes/shares
        PolicyRule ruleA_outbound;
        ruleA_outbound.originZoneId = "zoneA";
        ruleA_outbound.targetZoneId = "";
        ruleA_outbound.op = AccessOp::Write;
        ruleA_outbound.allow = true;
        ruleA_outbound.governingLaw = &lawA;
        TransferPolicy::instance().addPolicyRule(ruleA_outbound);

        // 3. Zone B denies inbound writes/shares from Zone A (authored by lawB_low)
        PolicyRule ruleB_inbound_deny;
        ruleB_inbound_deny.originZoneId = "zoneA";
        ruleB_inbound_deny.targetZoneId = "zoneB";
        ruleB_inbound_deny.op = AccessOp::Write;
        ruleB_inbound_deny.allow = false;
        ruleB_inbound_deny.governingLaw = &lawB_low;
        TransferPolicy::instance().addPolicyRule(ruleB_inbound_deny);

        // With dual jurisdiction: zoneA allows, but zoneB denies -> write refused with AccessDenied!
        assert(PropertyPath::parse("data").setValue(targetObj, 30.0, 0, interCtx) == Result::AccessDenied);

        // 4. Competing rule in Zone B: lawB_high (higher authority) grants inbound writes from zoneA
        PolicyRule ruleB_inbound_allow;
        ruleB_inbound_allow.originZoneId = "zoneA";
        ruleB_inbound_allow.targetZoneId = "zoneB";
        ruleB_inbound_allow.op = AccessOp::Write;
        ruleB_inbound_allow.allow = true;
        ruleB_inbound_allow.governingLaw = &lawB_high;
        TransferPolicy::instance().addPolicyRule(ruleB_inbound_allow);

        // Conflict resolved via authority-over DAG: lawB_high overrules lawB_low in receiver Zone B
        // Both zoneA and zoneB now allow -> write succeeds!
        assert(PropertyPath::parse("data").setValue(targetObj, 40.0, 0, interCtx) == Result::Ok);

        // 5. If Zone A's outbound rule changes to deny (veto by sender):
        TransferPolicy::instance().clearPolicyRules();
        PolicyRule ruleA_deny;
        ruleA_deny.originZoneId = "zoneA";
        ruleA_deny.targetZoneId = "";
        ruleA_deny.op = AccessOp::Write;
        ruleA_deny.allow = false;
        ruleA_deny.governingLaw = &lawA;
        TransferPolicy::instance().addPolicyRule(ruleA_deny);
        TransferPolicy::instance().addPolicyRule(ruleB_inbound_allow);

        // Sender Zone A denies outbound -> write refused even though Zone B allows!
        assert(PropertyPath::parse("data").setValue(targetObj, 50.0, 0, interCtx) == Result::AccessDenied);

        // 6. Firmament Pass 5 Corrective Fix Witness:
        //    (a) Automatic originZone & targetZone resolution when a real Law executes across Zones
        //    (b) Explicit governingZoneId attribution when an outbound rule specifies both originZoneId and targetZoneId
        Object senderObj;
        senderObj.setObjectID("sender-obj");
        zoneA.addToFormation(&senderObj);
        zoneA.addToFormation(&lawA);
        zoneB.addToFormation(&targetObj);
        zoneB.addToFormation(&lawB_low);
        zoneB.addToFormation(&lawB_high);

        std::vector<Singular*> interZoneBeings{&zoneA, &zoneB, &senderObj, &targetObj, &authorA, &authorB, &lawA, &lawB_low, &lawB_high};
        Universe::instance().setProvider([&](std::vector<Singular*>& out) {
            for (auto* b : interZoneBeings) if (b) out.push_back(b);
        });

        TransferPolicy::instance().clearPolicyRules();
        // Zone A specifically denies outbound writes to Zone B (both originZoneId and targetZoneId set!)
        PolicyRule ruleA_targeted_outbound_deny;
        ruleA_targeted_outbound_deny.governingZoneId = "zoneA";
        ruleA_targeted_outbound_deny.originZoneId = "zoneA";
        ruleA_targeted_outbound_deny.targetZoneId = "zoneB";
        ruleA_targeted_outbound_deny.op = AccessOp::Write;
        ruleA_targeted_outbound_deny.allow = false;
        ruleA_targeted_outbound_deny.governingLaw = &lawA;
        TransferPolicy::instance().addPolicyRule(ruleA_targeted_outbound_deny);
        // Zone B allows inbound via high-authority lawB_high
        TransferPolicy::instance().addPolicyRule(ruleB_inbound_allow);

        // Real Law execution from senderObj in zoneA writing to @target-obj.data in zoneB:
        // TransferPolicy::checkAccess automatically infers originZone=&zoneA and targetZone=&zoneB,
        // and ruleA_targeted_outbound_deny stays in senderRules (cannot be overruled by Zone B's lawB_high).
        lawA.setActionModel(ActionNode::set("@target-obj.data", 77.0));
        assert(lawA.applyTo(senderObj) == Law::ApplicationResult::Applied);
        assert(!lawA.applicationLog().back().changedSomething());
        assert(lawA.applicationLog().back().trace.nodes.front().reason == Result::AccessDenied);

        // Once Zone A allows outbound to Zone B, the same Law execution succeeds automatically!
        TransferPolicy::instance().clearPolicyRules();
        PolicyRule ruleA_targeted_outbound_allow = ruleA_targeted_outbound_deny;
        ruleA_targeted_outbound_allow.allow = true;
        TransferPolicy::instance().addPolicyRule(ruleA_targeted_outbound_allow);
        TransferPolicy::instance().addPolicyRule(ruleB_inbound_allow);

        assert(lawA.applyTo(senderObj) == Law::ApplicationResult::Applied);
        assert(lawA.applicationLog().back().changedSomething());
        PropertyValue autoZoneVal;
        assert(PropertyPath::parse("data").getValue(targetObj, autoZoneVal) == Result::Ok);
        assert(std::get<double>(autoZoneVal) == 77.0);

        TransferPolicy::instance().clearPolicyRules();
        Universe::instance().setProvider(nullptr);
        Universe::instance().setRelationProvider(nullptr);
    }

    // -------------------------------------------------------------------------
    // 17. Legible Property Surface on @transfer-policy for Authoring PolicyRules by Law & Persistence
    // -------------------------------------------------------------------------
    {
        TransferPolicy::instance().clearPolicyRules();

        Zone zoneA("zoneA", "symA");
        Zone zoneB("zoneB", "symB");

        Object authorA; authorA.setObjectID("authorA-17");
        Object authorB; authorB.setObjectID("authorB-17");

        Object senderObj; senderObj.setObjectID("sender-obj-17");
        zoneA.addToFormation(&senderObj);

        Object targetObj; targetObj.setObjectID("target-obj-17");
        zoneB.addToFormation(&targetObj);
        targetObj.setDynamicProperty("data", 10.0);

        Law lawA("lawA-17");
        lawA.setLawIdentifier("lawA-17");
        lawA.addAuthor(authorA);
        lawA.setJurisdiction(std::shared_ptr<Zone>(&zoneA, [](Zone*){}));
        zoneA.addToFormation(&lawA);

        Law lawB_low("lawB-low-17");
        lawB_low.setLawIdentifier("lawB-low-17");
        lawB_low.addAuthor(authorB);
        lawB_low.setJurisdiction(std::shared_ptr<Zone>(&zoneB, [](Zone*){}));
        zoneB.addToFormation(&lawB_low);

        Law lawB_high("lawB-high-17");
        lawB_high.setLawIdentifier("lawB-high-17");
        lawB_high.addAuthor(authorB);
        lawB_high.setJurisdiction(std::shared_ptr<Zone>(&zoneB, [](Zone*){}));
        zoneB.addToFormation(&lawB_high);

        Relation authOver("authority-over", lawB_high, lawB_low, true);
        std::vector<Relation*> rels{&authOver};
        Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
            for (auto* r : rels) if (r) out.push_back(r);
        });

        std::vector<Singular*> allBeings{
            &zoneA, &zoneB, &senderObj, &targetObj, &authorA, &authorB, &lawA, &lawB_low, &lawB_high, &TransferPolicy::instance()
        };
        Universe::instance().setProvider([&](std::vector<Singular*>& out) {
            for (auto* b : allBeings) if (b) out.push_back(b);
        });

        // Step 1: lawB_low authors a deny PolicyRule targeting zoneB purely via PropertyPath writes
        // (zero direct C++ calls to addPolicyRule)
        lawB_low.setActionModel(ActionNode::sequence({
            ActionNode::set("@transfer-policy.rule.pathPattern", PropertyValue(std::string("data"))),
            ActionNode::set("@transfer-policy.rule.op", PropertyValue(std::string("write"))),
            ActionNode::set("@transfer-policy.rule.allow", PropertyValue(false)),
            ActionNode::set("@transfer-policy.rule.targetZoneId", PropertyValue(std::string("zoneB"))),
            ActionNode::set("@transfer-policy.rule.commit", PropertyValue(true))
        }));

        assert(TransferPolicy::instance().ruleCount() == 0);
        assert(lawB_low.applyTo(targetObj) == Law::ApplicationResult::Applied);
        assert(TransferPolicy::instance().ruleCount() == 1);

        // Verify staged fields were reset and ruleCount property reads 1
        PropertyValue rcVal;
        assert(PropertyPath::parse("ruleCount").getValue(TransferPolicy::instance(), rcVal) == Result::Ok);
        assert(std::get<int>(rcVal) == 1);

        // Step 2: Now lawA attempts to write @target-obj-17.data from zoneA.
        // Receiver Zone B has a deny rule from lawB_low. Write is refused!
        lawA.setActionModel(ActionNode::set("@target-obj-17.data", 42.0));
        assert(lawA.applyTo(senderObj) == Law::ApplicationResult::Applied);
        assert(!lawA.applicationLog().back().changedSomething());
        assert(lawA.applicationLog().back().trace.nodes.front().reason == Result::AccessDenied);

        // Step 3: lawB_high authors an allow PolicyRule targeting zoneB purely via PropertyPath writes
        lawB_high.setActionModel(ActionNode::sequence({
            ActionNode::set("@transfer-policy.rule.pathPattern", PropertyValue(std::string("data"))),
            ActionNode::set("@transfer-policy.rule.op", PropertyValue(std::string("write"))),
            ActionNode::set("@transfer-policy.rule.allow", PropertyValue(true)),
            ActionNode::set("@transfer-policy.rule.targetZoneId", PropertyValue(std::string("zoneB"))),
            ActionNode::set("@transfer-policy.rule.commit", PropertyValue(true))
        }));

        assert(lawB_high.applyTo(targetObj) == Law::ApplicationResult::Applied);
        assert(TransferPolicy::instance().ruleCount() == 2);

        // Step 4: lawA attempts to write again.
        // Because lawB_high has authority-over lawB_low, lawB_high's allow rule wins!
        assert(lawA.applyTo(senderObj) == Law::ApplicationResult::Applied);
        assert(lawA.applicationLog().back().changedSomething());
        PropertyValue finalVal;
        assert(PropertyPath::parse("data").getValue(targetObj, finalVal) == Result::Ok);
        assert(std::get<double>(finalVal) == 42.0);

        // Step 5: Persistence round-trip of authored rules
        nlohmann::json savedJson = TransferPolicy::instance().toJson();
        assert(savedJson.contains("rules"));
        assert(savedJson["rules"].size() == 2);
        assert(savedJson["rules"][0]["governingLawId"] == "lawB-low-17");
        assert(savedJson["rules"][1]["governingLawId"] == "lawB-high-17");
        assert(savedJson["rules"][0]["allow"] == false);
        assert(savedJson["rules"][1]["allow"] == true);

        TransferPolicy::instance().clearPolicyRules();
        assert(TransferPolicy::instance().ruleCount() == 0);

        TransferPolicy::instance().loadFromJson(savedJson);
        assert(TransferPolicy::instance().ruleCount() == 2);

        // Verify loaded rules still govern access correctly with authority DAG intact
        targetObj.setDynamicProperty("data", 100.0);
        lawA.setActionModel(ActionNode::set("@target-obj-17.data", 200.0));
        assert(lawA.applyTo(senderObj) == Law::ApplicationResult::Applied);
        assert(lawA.applicationLog().back().changedSomething());
        assert(PropertyPath::parse("data").getValue(targetObj, finalVal) == Result::Ok);
        assert(std::get<double>(finalVal) == 200.0);

        // Step 6: Firmament Pass 6 Corrective Fix Witness:
        //   (a) Lazy governingLawId resolution when TransferPolicy::loadFromJson runs BEFORE Laws are in Universe
        TransferPolicy::instance().clearPolicyRules();
        Universe::instance().setProvider(nullptr);
        TransferPolicy::instance().loadFromJson(savedJson);
        assert(TransferPolicy::instance().ruleCount() == 2);
        // Re-register Universe provider after loadFromJson -> lazy resolution restores governingLaw pointers!
        Universe::instance().setProvider([&](std::vector<Singular*>& out) {
            for (auto* b : allBeings) if (b) out.push_back(b);
        });
        assert(TransferPolicy::instance().toJson()["rules"][0]["governingLawId"] == "lawB-low-17");
        assert(TransferPolicy::instance().toJson()["rules"][1]["governingLawId"] == "lawB-high-17");

        //   (b) Effective-Cell Policy Authoring by Law via @transfer-policy.rule.targetCellPath
        //   (c) Committed-Snapshot Anti-Self-Authorization Guard (createdInScopeId == s_activeScopeId)
        TransferPolicy::instance().clearPolicyRules();
        Object mirrorObj;
        mirrorObj.setObjectID("mirror-obj-17");
        zoneB.addToFormation(&mirrorObj);
        allBeings.push_back(&mirrorObj);

        // Share cell between targetObj.data and mirrorObj.aliasData
        Law shareAliasLaw("share-alias-17");
        shareAliasLaw.addAuthor(authorB);
        shareAliasLaw.setActionModel(ActionNode::shareCell("aliasData", "@target-obj-17.data"));
        assert(shareAliasLaw.applyTo(mirrorObj) == Law::ApplicationResult::Applied);
        assert(mirrorObj.getPropertyCell("aliasData") == targetObj.getPropertyCell("data"));

        // lawB_high authors a cell-specific deny rule via rule.targetCellPath = "@target-obj-17.data"
        lawB_high.setActionModel(ActionNode::sequence({
            ActionNode::set("@transfer-policy.rule.targetCellPath", PropertyValue(std::string("@target-obj-17.data"))),
            ActionNode::set("@transfer-policy.rule.op", PropertyValue(std::string("write"))),
            ActionNode::set("@transfer-policy.rule.allow", PropertyValue(false)),
            ActionNode::set("@transfer-policy.rule.commit", PropertyValue(true))
        }));
        assert(lawB_high.applyTo(targetObj) == Law::ApplicationResult::Applied);

        // Writing through mirrorObj.aliasData is refused by the effective-cell rule!
        lawA.setActionModel(ActionNode::set("@mirror-obj-17.aliasData", 999.0));
        assert(lawA.applyTo(senderObj) == Law::ApplicationResult::Applied);
        assert(!lawA.applicationLog().back().changedSomething());
        assert(lawA.applicationLog().back().trace.nodes.front().reason == Result::AccessDenied);

        // Committed-Snapshot Guard: even if a higher-authority Law commits an allow rule AND tries to write
        // to the protected cell inside the SAME Law application transaction, the uncommitted allow rule
        // cannot authorize the transaction that created it!
        Law superLaw("super-law-17");
        superLaw.setLawIdentifier("super-law-17");
        superLaw.addAuthor(authorB);
        superLaw.setJurisdiction(std::shared_ptr<Zone>(&zoneB, [](Zone*){}));
        zoneB.addToFormation(&superLaw);
        allBeings.push_back(&superLaw);
        Relation superRel("authority-over", superLaw, lawB_high, true);
        rels.push_back(&superRel);

        superLaw.setActionModel(ActionNode::sequence({
            ActionNode::set("@transfer-policy.rule.targetCellPath", PropertyValue(std::string("@target-obj-17.data"))),
            ActionNode::set("@transfer-policy.rule.op", PropertyValue(std::string("write"))),
            ActionNode::set("@transfer-policy.rule.allow", PropertyValue(true)),
            ActionNode::set("@transfer-policy.rule.commit", PropertyValue(true)),
            ActionNode::set("@mirror-obj-17.aliasData", PropertyValue(555.0))
        }));
        // First application: the allow rule is committed, but the write in the SAME transaction is refused!
        assert(superLaw.applyTo(targetObj) == Law::ApplicationResult::Applied);
        assert(superLaw.applicationLog().back().trace.nodes.back().reason == Result::AccessDenied);
        assert(PropertyPath::parse("data").getValue(targetObj, finalVal) == Result::Ok);
        assert(std::get<double>(finalVal) == 200.0);

        // Subsequent application (after the rule transaction committed): write now succeeds!
        lawA.setActionModel(ActionNode::set("@mirror-obj-17.aliasData", 555.0));
        assert(lawA.applyTo(senderObj) == Law::ApplicationResult::Applied);
        assert(lawA.applicationLog().back().changedSomething());
        assert(PropertyPath::parse("data").getValue(targetObj, finalVal) == Result::Ok);
        assert(std::get<double>(finalVal) == 555.0);

        TransferPolicy::instance().clearPolicyRules();
        Universe::instance().setProvider(nullptr);
        Universe::instance().setRelationProvider(nullptr);
    }

    // -------------------------------------------------------------------------
    // 18. Quest 1 & Quest 2: Zone + TransferPolicy Round-Trip Persistence &
    //     Comprehensive AccessOp Governance (Read, Share, BorrowWeak, TransferUnique)
    // -------------------------------------------------------------------------
    {
        TransferPolicy::instance().clearPolicyRules();

        // --- Part A: Zone + TransferPolicy Round-Trip Persistence (Quest 1) ---
        Zone zone18("zone18", "sym18");
        auto obj18 = std::make_shared<Object>();
        obj18->setObjectID("obj18");
        obj18->setDynamicProperty("secret", 123.0);
        obj18->setDynamicProperty("classified", 77.0);
        obj18->setDynamicProperty("nosharer", 100.0);
        obj18->setDynamicProperty("noweak", 200.0);
        obj18->setDynamicProperty("nounique", 300.0);
        zone18.addObject(obj18);

        Object actor18; actor18.setObjectID("actor18");
        zone18.addToFormation(&actor18);

        Law lawAdmin("law-admin-18");
        lawAdmin.setLawIdentifier("law-admin-18");
        lawAdmin.addAuthor(actor18);
        lawAdmin.setJurisdiction(std::shared_ptr<Zone>(&zone18, [](Zone*){}));
        zone18.addToFormation(&lawAdmin);

        std::vector<Singular*> beings18{
            &zone18, obj18.get(), &actor18, &lawAdmin, &TransferPolicy::instance()
        };
        Universe::instance().setProvider([&](std::vector<Singular*>& out) {
            for (auto* b : beings18) if (b) out.push_back(b);
        });

        // Initially no custom policy rules -> hasNonDefaultState() is false!
        assert(!TransferPolicy::instance().hasNonDefaultState());
        nlohmann::json defaultZj = zoneToJson(zone18);
        assert(!defaultZj.contains("transferPolicy")); // Clean zone has zero format churn!

        // Author a PolicyRule denying writes to "secret" purely via PropertyPath writes
        lawAdmin.setActionModel(ActionNode::sequence({
            ActionNode::set("@transfer-policy.rule.pathPattern", PropertyValue(std::string("secret"))),
            ActionNode::set("@transfer-policy.rule.op", PropertyValue(std::string("write"))),
            ActionNode::set("@transfer-policy.rule.allow", PropertyValue(false)),
            ActionNode::set("@transfer-policy.rule.commit", PropertyValue(true))
        }));
        assert(lawAdmin.applyTo(*obj18) == Law::ApplicationResult::Applied);
        assert(TransferPolicy::instance().ruleCount() == 1);
        assert(TransferPolicy::instance().hasNonDefaultState());

        // Serialize Zone -> includes "transferPolicy" additively
        nlohmann::json customZj = zoneToJson(zone18);
        assert(customZj.contains("transferPolicy"));
        assert(customZj["transferPolicy"]["rules"].size() == 1);

        // Clear TransferPolicy rules and verify reload restores them
        TransferPolicy::instance().clearPolicyRules();
        assert(TransferPolicy::instance().ruleCount() == 0);

        std::shared_ptr<Zone> reloadedZone = makeZoneFromJson(customZj);
        assert(reloadedZone != nullptr);
        assert(TransferPolicy::instance().ruleCount() == 1);

        // Verify the restored rule enforces access refusal on writes to "secret"
        Law writerLaw("writer-law-18");
        writerLaw.setLawIdentifier("writer-law-18");
        writerLaw.addAuthor(actor18);
        writerLaw.setActionModel(ActionNode::set("@obj18.secret", 999.0));
        assert(writerLaw.applyTo(actor18) == Law::ApplicationResult::Applied);
        assert(!writerLaw.applicationLog().back().changedSomething());
        assert(writerLaw.applicationLog().back().trace.nodes.front().reason == Result::AccessDenied);
        PropertyValue secretVal;
        assert(PropertyPath::parse("secret").getValue(*obj18, secretVal) == Result::Ok);
        assert(std::get<double>(secretVal) == 123.0);

        // --- Part B: Comprehensive AccessOp Governance (Quest 2) ---
        // 1. AccessOp::Read Governance
        // Author a rule on @transfer-policy denying read access to "classified"
        lawAdmin.setActionModel(ActionNode::sequence({
            ActionNode::set("@transfer-policy.rule.pathPattern", PropertyValue(std::string("classified"))),
            ActionNode::set("@transfer-policy.rule.op", PropertyValue(std::string("read"))),
            ActionNode::set("@transfer-policy.rule.allow", PropertyValue(false)),
            ActionNode::set("@transfer-policy.rule.commit", PropertyValue(true))
        }));
        assert(lawAdmin.applyTo(*obj18) == Law::ApplicationResult::Applied);

        // (a) Condition evaluation: ConditionNode::compare fails because lawGetValue returns false / AccessDenied
        Law conditionReaderLaw("cond-reader-18");
        conditionReaderLaw.setLawIdentifier("cond-reader-18");
        conditionReaderLaw.addAuthor(actor18);
        conditionReaderLaw.setConditionModel(ConditionNode::compare(
            "@obj18.classified", ConditionNode::Op::Eq, PropertyValue(77.0)
        ));
        conditionReaderLaw.setActionModel(ActionNode::set("auditFlag", PropertyValue(true)));
        assert(conditionReaderLaw.applyTo(actor18) == Law::ApplicationResult::ConditionsFailed);
        assert(!actor18.hasDynamicProperty("auditFlag"));

        // (b) CopyValue read governance: ActionNode::copyCell fails because source cannot be read
        Law copyReaderLaw("copy-reader-18");
        copyReaderLaw.setLawIdentifier("copy-reader-18");
        copyReaderLaw.addAuthor(actor18);
        copyReaderLaw.setActionModel(ActionNode::copyCell("leaked", "@obj18.classified"));
        assert(copyReaderLaw.applyTo(actor18) == Law::ApplicationResult::Applied);
        assert(!copyReaderLaw.applicationLog().back().changedSomething());
        assert(copyReaderLaw.applicationLog().back().trace.nodes.front().reason == Result::AccessDenied);
        assert(!actor18.hasDynamicProperty("leaked"));

        // (c) MathBindings read governance: ActionNode::map with bindings reading "classified" fails
        Law mapReaderLaw("map-reader-18");
        mapReaderLaw.setLawIdentifier("map-reader-18");
        mapReaderLaw.addAuthor(actor18);
        MathBindings mb;
        mb["x"] = MathBindingEntry{PropertyPath::parse("@obj18.classified")};
        auto leaf = std::make_shared<OntoMath::MathNode>();
        leaf->op = OntoMath::MathNode::Op::ValueLeaf;
        leaf->variableName = "x";
        mapReaderLaw.setActionModel(ActionNode::map("derived", OntoMath::Piecewise::continuous(std::move(leaf)), std::move(mb)));
        assert(mapReaderLaw.applyTo(actor18) == Law::ApplicationResult::Applied);
        assert(!mapReaderLaw.applicationLog().back().changedSomething());
        assert(mapReaderLaw.applicationLog().back().trace.nodes.front().reason == Result::AccessDenied);
        assert(!actor18.hasDynamicProperty("derived"));

        // 2. AccessOp::Share Governance
        lawAdmin.setActionModel(ActionNode::sequence({
            ActionNode::set("@transfer-policy.rule.pathPattern", PropertyValue(std::string("nosharer"))),
            ActionNode::set("@transfer-policy.rule.op", PropertyValue(std::string("share"))),
            ActionNode::set("@transfer-policy.rule.allow", PropertyValue(false)),
            ActionNode::set("@transfer-policy.rule.commit", PropertyValue(true))
        }));
        assert(lawAdmin.applyTo(*obj18) == Law::ApplicationResult::Applied);

        Law shareAttemptLaw("share-attempt-18");
        shareAttemptLaw.setLawIdentifier("share-attempt-18");
        shareAttemptLaw.addAuthor(actor18);
        shareAttemptLaw.setActionModel(ActionNode::shareCell("sharedAlias", "@obj18.nosharer"));
        assert(shareAttemptLaw.applyTo(actor18) == Law::ApplicationResult::Applied);
        assert(!shareAttemptLaw.applicationLog().back().changedSomething());
        assert(shareAttemptLaw.applicationLog().back().trace.nodes.front().reason == Result::AccessDenied);
        assert(!actor18.hasPropertyCell("sharedAlias"));

        // 3. AccessOp::BorrowWeak Governance
        lawAdmin.setActionModel(ActionNode::sequence({
            ActionNode::set("@transfer-policy.rule.pathPattern", PropertyValue(std::string("noweak"))),
            ActionNode::set("@transfer-policy.rule.op", PropertyValue(std::string("weak"))),
            ActionNode::set("@transfer-policy.rule.allow", PropertyValue(false)),
            ActionNode::set("@transfer-policy.rule.commit", PropertyValue(true))
        }));
        assert(lawAdmin.applyTo(*obj18) == Law::ApplicationResult::Applied);

        Law weakAttemptLaw("weak-attempt-18");
        weakAttemptLaw.setLawIdentifier("weak-attempt-18");
        weakAttemptLaw.addAuthor(actor18);
        weakAttemptLaw.setActionModel(ActionNode::weakCell("weakAlias", "@obj18.noweak"));
        assert(weakAttemptLaw.applyTo(actor18) == Law::ApplicationResult::Applied);
        assert(!weakAttemptLaw.applicationLog().back().changedSomething());
        assert(weakAttemptLaw.applicationLog().back().trace.nodes.front().reason == Result::AccessDenied);
        assert(!actor18.hasPropertyCell("weakAlias"));

        // 4. AccessOp::TransferUnique Governance
        lawAdmin.setActionModel(ActionNode::sequence({
            ActionNode::set("@transfer-policy.rule.pathPattern", PropertyValue(std::string("nounique"))),
            ActionNode::set("@transfer-policy.rule.op", PropertyValue(std::string("unique"))),
            ActionNode::set("@transfer-policy.rule.allow", PropertyValue(false)),
            ActionNode::set("@transfer-policy.rule.commit", PropertyValue(true))
        }));
        assert(lawAdmin.applyTo(*obj18) == Law::ApplicationResult::Applied);

        Law uniqueAttemptLaw("unique-attempt-18");
        uniqueAttemptLaw.setLawIdentifier("unique-attempt-18");
        uniqueAttemptLaw.addAuthor(actor18);
        uniqueAttemptLaw.setActionModel(ActionNode::uniqueCell("uniqueAlias", "@obj18.nounique"));
        assert(uniqueAttemptLaw.applyTo(actor18) == Law::ApplicationResult::Applied);
        assert(!uniqueAttemptLaw.applicationLog().back().changedSomething());
        assert(uniqueAttemptLaw.applicationLog().back().trace.nodes.front().reason == Result::AccessDenied);
        assert(!actor18.hasPropertyCell("uniqueAlias"));

        TransferPolicy::instance().clearPolicyRules();
        Universe::instance().setProvider(nullptr);
        Universe::instance().setRelationProvider(nullptr);
    }

    // -------------------------------------------------------------------------
    // 19. Pass 8 Quests: Rete Reactivity on TransferPolicy Changes,
    //     Legible Rule Inspection & Authority-Checked Revocation, & Constitutional Person-Body Guard
    // -------------------------------------------------------------------------
    {
        TransferPolicy::instance().clearPolicyRules();

        Zone zone19("zone19", "sym19");
        Object authorLow; authorLow.setObjectID("authorLow-19");
        Object authorHigh; authorHigh.setObjectID("authorHigh-19");

        Law lawLow("lawLow-19");
        lawLow.setLawIdentifier("lawLow-19");
        lawLow.addAuthor(authorLow);
        lawLow.setJurisdiction(std::shared_ptr<Zone>(&zone19, [](Zone*){}));
        zone19.addToFormation(&lawLow);

        Law lawHigh("lawHigh-19");
        lawHigh.setLawIdentifier("lawHigh-19");
        lawHigh.addAuthor(authorHigh);
        lawHigh.setJurisdiction(std::shared_ptr<Zone>(&zone19, [](Zone*){}));
        zone19.addToFormation(&lawHigh);

        // Relation establishing lawHigh authority-over lawLow
        Relation authOver("authority-over", lawHigh, lawLow, true);
        std::vector<Relation*> rels19{&authOver};
        Universe::instance().setRelationProvider([&](std::vector<Relation*>& out) {
            for (auto* r : rels19) if (r) out.push_back(r);
        });
        assert(lawHigh.hasAuthorityOver(lawLow));

        Object vault;
        vault.setObjectID("vault");
        vault.setDynamicProperty("classified", 77.0);
        zone19.addObject(std::shared_ptr<Object>(&vault, [](Object*){}));

        std::vector<Singular*> beings19{
            &zone19, &vault, &authorLow, &authorHigh, &lawLow, &lawHigh, &TransferPolicy::instance()
        };
        Universe::instance().setProvider([&](std::vector<Singular*>& out) {
            for (auto* b : beings19) if (b) out.push_back(b);
        });

        // --- Part 19.1: Wake Rete / Continuous Laws on @transfer-policy Changes (Quest 1) ---
        LawManager lm;
        lm.connectToEventBus();
        auto reactiveLaw = lm.createLaw("reactive-law-19", {&authorHigh});
        reactiveLaw->setLawIdentifier("reactive-law-19");
        reactiveLaw->setActivation(Law::Activation::WhileTrue);
        reactiveLaw->addTarget(vault);
        reactiveLaw->setConditionModel(ConditionNode::compare("classified", ConditionNode::Op::Eq, PropertyValue(77.0)));
        reactiveLaw->setActionModel(ActionNode::set("unlocked", PropertyValue(true)));
        beings19.push_back(reactiveLaw.get());

        // Initially: lawLow commits a read deny rule on "classified" via @transfer-policy.rule.*
        lawLow.setActionModel(ActionNode::sequence({
            ActionNode::set("@transfer-policy.rule.pathPattern", PropertyValue(std::string("classified"))),
            ActionNode::set("@transfer-policy.rule.op", PropertyValue(std::string("read"))),
            ActionNode::set("@transfer-policy.rule.allow", PropertyValue(false)),
            ActionNode::set("@transfer-policy.rule.commit", PropertyValue(true))
        }));
        assert(lawLow.applyTo(vault) == Law::ApplicationResult::Applied);
        assert(TransferPolicy::instance().ruleCount() == 1);

        // Tick LawManager: condition evaluation reads "classified" -> denied -> condition fails -> unlocked NOT set!
        lm.tick();
        assert(!vault.hasDynamicProperty("unlocked"));

        // Now: lawHigh commits a read allow rule on "classified" via @transfer-policy.rule.*
        lawHigh.setActionModel(ActionNode::sequence({
            ActionNode::set("@transfer-policy.rule.pathPattern", PropertyValue(std::string("classified"))),
            ActionNode::set("@transfer-policy.rule.op", PropertyValue(std::string("read"))),
            ActionNode::set("@transfer-policy.rule.allow", PropertyValue(true)),
            ActionNode::set("@transfer-policy.rule.commit", PropertyValue(true))
        }));
        assert(lawHigh.applyTo(vault) == Law::ApplicationResult::Applied);
        assert(TransferPolicy::instance().ruleCount() == 2);

        // Crucial invariant: WITHOUT touching vault.classified directly, tick LawManager!
        // Committing the policy rule dirtied vault.classified via notifyRuleChanged -> Rete re-evaluates!
        lm.tick();
        assert(vault.hasDynamicProperty("unlocked"));
        PropertyValue unlockedVal;
        assert(vault.getDynamicProperty("unlocked", unlockedVal));
        assert(std::get<bool>(unlockedVal) == true);

        // --- Part 19.2: Legible Rule Inspection & Authority-Checked Revocation (Quest 2) ---
        // (a) Inspect rules list via @transfer-policy.rules
        PropertyValue pRulesVal;
        assert(PropertyPath::parse("@transfer-policy.rules").getValue(vault, pRulesVal) == Result::Ok);
        auto pList = std::get<std::shared_ptr<PropertyList>>(pRulesVal);
        assert(pList && pList->elements.size() == 2);

        PropertyValue pVal0, pVal1, opVal0;
        assert(PropertyPath::parse("@transfer-policy.rules.0.pathPattern").getValue(vault, pVal0) == Result::Ok);
        assert(std::get<std::string>(pVal0) == "classified");
        assert(PropertyPath::parse("@transfer-policy.rules.0.op").getValue(vault, opVal0) == Result::Ok);
        assert(std::get<std::string>(opVal0) == "read");
        assert(PropertyPath::parse("@transfer-policy.rules.0.governingLawId").getValue(vault, pVal0) == Result::Ok);
        assert(std::get<std::string>(pVal0) == "lawLow-19");

        assert(PropertyPath::parse("@transfer-policy.rules.1.governingLawId").getValue(vault, pVal1) == Result::Ok);
        assert(std::get<std::string>(pVal1) == "lawHigh-19");

        // (b) Subordinate lawLow attempts to revoke superior lawHigh's rules -> refused with ReadOnly!
        lawLow.setActionModel(ActionNode::set(
            "@transfer-policy.rule.revokeGoverningLawId", PropertyValue(std::string("lawHigh-19"))));
        assert(lawLow.applyTo(vault) == Law::ApplicationResult::Applied);
        assert(!lawLow.applicationLog().back().changedSomething());
        assert(lawLow.applicationLog().back().trace.nodes.front().reason == Result::ReadOnly);
        assert(TransferPolicy::instance().ruleCount() == 2); // Rules untouched!

        // (c) Superior lawHigh (having authority-over lawLow) revokes lawLow's rules -> succeeds!
        lawHigh.setActionModel(ActionNode::set(
            "@transfer-policy.rule.revokeGoverningLawId", PropertyValue(std::string("lawLow-19"))));
        assert(lawHigh.applyTo(vault) == Law::ApplicationResult::Applied);
        assert(lawHigh.applicationLog().back().changedSomething());
        assert(TransferPolicy::instance().ruleCount() == 1);
        assert(PropertyPath::parse("@transfer-policy.rules.0.governingLawId").getValue(vault, pVal0) == Result::Ok);
        assert(std::get<std::string>(pVal0) == "lawHigh-19");

        // (d) Superior lawHigh revokes its own rules -> succeeds!
        lawHigh.setActionModel(ActionNode::set(
            "@transfer-policy.rule.revokeGoverningLawId", PropertyValue(std::string("lawHigh-19"))));
        assert(lawHigh.applyTo(vault) == Law::ApplicationResult::Applied);
        assert(lawHigh.applicationLog().back().changedSomething());
        assert(TransferPolicy::instance().ruleCount() == 0);

        // --- Part 19.3: Constitutional Person-Body Guard Invariant Witness ---
        // Even if an authored rule exists with allow := true on "@person.position",
        // the invariant C++ Kernel guard on Person's body unconditionally refuses unconsented positive motion!
        Soul pSoul("Subject19");
        Body pAvatar = Body::createBasicAvatar("Avatar19");
        Person person19(pSoul, std::move(pAvatar), "default");
        beings19.push_back(&person19);

        // Commit an allow rule for person19.position on @transfer-policy
        PolicyRule allowMotionRule;
        allowMotionRule.pathPattern = "position";
        allowMotionRule.op = AccessOp::Write;
        allowMotionRule.allow = true;
        allowMotionRule.governingLaw = &lawHigh;
        TransferPolicy::instance().addPolicyRule(allowMotionRule);

        // A Law applied to vault attempts forced movement of person19 without signed consent Relation
        const std::string personPosPath = "@" + person19.getIdentifier() + ".position.y";
        Law forcedMotionLaw("forced-motion-19");
        forcedMotionLaw.addAuthor(authorHigh);
        forcedMotionLaw.setActionModel(ActionNode::set(personPosPath, PropertyValue(25.0f)));
        // Refuses with AuthorityDenied at the Kernel boundary before TransferPolicy can ever open it
        assert(forcedMotionLaw.applyTo(vault) == Law::ApplicationResult::AuthorityDenied);
        const auto refusalJson = forcedMotionLaw.applicationLog().back().toJson();
        assert(refusalJson.value("refusalReason", std::string()).find("no verified signed actuation consent") != std::string::npos);

        TransferPolicy::instance().clearPolicyRules();
        Universe::instance().setProvider(nullptr);
        Universe::instance().setRelationProvider(nullptr);
    }

    std::cout << "property_memory_access_test: PASS\n";
}
