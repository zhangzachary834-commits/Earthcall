// Zach's PropertyPath/memory micromastery direction: containers remain
// predicates, shared writes reach shared storage, and paths retain no IDs.
// Codex / GPT-6.1 Sol / 01a0e64f-5853-7d30-8196-995b4fd16b89 / 2026-10-01.
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

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
