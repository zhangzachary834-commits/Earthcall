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
#include "Singularity/OntoMath/Field.hpp"

#include <cassert>
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

    std::cout << "property_memory_access_test: PASS\n";
}
