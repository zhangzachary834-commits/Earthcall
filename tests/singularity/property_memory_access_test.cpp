// Zach's PropertyPath/memory micromastery direction: containers remain
// predicates, shared writes reach shared storage, and paths retain no IDs.
// Codex / GPT-6.1 Sol / 01a0e64f-5853-7d30-8196-995b4fd16b89 / 2026-10-01.
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "Singularity/OntoMath/Field.hpp"

#include <cassert>
#include <iostream>

namespace {
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

    std::cout << "property_memory_access_test: PASS\n";
}
