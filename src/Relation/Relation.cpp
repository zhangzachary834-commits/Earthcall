#include "Relation.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Property/ComputedProperty.hpp"
#include "ConstructedBeing/Singular/Property/PropertyRef.hpp"
#include <iostream>
#include <cstring>
#include <cstdio>

// Specific Implementation Vision: Recursive, custom tool creation
// With a combination of the basic tools here, with a Formation system comprised of relations between things, people can create their own tools on top of that.
// This allows for a recursive, self-creating tool system that can evolve over time.
// For example, person wants to create a tool that spins objects. The user can set it so that relations are created between an Object's 2D form with others, and they use the existing tool system to draw the pattern by which they want the new tools behavior to resmble. So they can draw a spiral for the spin tool. Then they choose how the system actually uses it—here, let's say it uses an existing hypotehtical base tool "warp". A new relation is created that relates this "tool-behavior" drawing by looking at the drawing and "warping" the current drawing according to the pattern of the meta-spiral drawing. 
// User can have the choice to have the tools themselves be integrated under relations. Every act of drawing can call a relation between the tool and the other Singulars involved. (tool isn't Singular yet, so we'll make them Singular in the future.)

using json = nlohmann::json;

namespace {
std::vector<float> mat4ToVector(const glm::mat4& matrix) {
    std::vector<float> values(16);
    const float* raw = glm::value_ptr(matrix);
    for (int i = 0; i < 16; ++i) values[i] = raw[i];
    return values;
}

glm::mat4 vectorToMat4(const std::vector<float>& values) {
    glm::mat4 matrix(1.0f);
    if (values.size() == 16) {
        std::memcpy(glm::value_ptr(matrix), values.data(), sizeof(float) * 16);
    }
    return matrix;
}
} // namespace

json Relation::AttachmentData::toJson() const {
    return json{
        {"enabled", enabled},
        {"localOffset", mat4ToVector(localOffset)},
        {"parentAnchor", {parentAnchor.x, parentAnchor.y, parentAnchor.z}},
        {"childAnchor", {childAnchor.x, childAnchor.y, childAnchor.z}},
        {"inheritTranslation", inheritTranslation},
        {"inheritRotation", inheritRotation},
        {"inheritScale", inheritScale}
    };
}

Relation::AttachmentData Relation::AttachmentData::fromJson(const json& j) {
    AttachmentData data;
    data.enabled = j.value("enabled", false);
    data.localOffset = vectorToMat4(j.value("localOffset", std::vector<float>{}));
    if (j.contains("parentAnchor") && j["parentAnchor"].is_array() && j["parentAnchor"].size() >= 3) {
        data.parentAnchor = glm::vec3(j["parentAnchor"][0].get<float>(),
                                      j["parentAnchor"][1].get<float>(),
                                      j["parentAnchor"][2].get<float>());
    }
    if (j.contains("childAnchor") && j["childAnchor"].is_array() && j["childAnchor"].size() >= 3) {
        data.childAnchor = glm::vec3(j["childAnchor"][0].get<float>(),
                                     j["childAnchor"][1].get<float>(),
                                     j["childAnchor"][2].get<float>());
    }
    data.inheritTranslation = j.value("inheritTranslation", true);
    data.inheritRotation = j.value("inheritRotation", true);
    data.inheritScale = j.value("inheritScale", true);
    return data;
}

Relation::Relation(const std::string& type,
                   Singular& aBeing,
                   Singular& bBeing,
                   bool directed,
                   float initialWeight)
    : type(type), directed(directed), _a(&aBeing), _b(&bBeing) {
    _cachedAId = _a ? _a->getIdentifier() : "";
    _cachedBId = _b ? _b->getIdentifier() : "";
    if (initialWeight != -1.0f) setWeight(initialWeight);
}

Relation::Relation(const std::string& type,
                   const Singular& aBeing,
                   const Singular& bBeing,
                   bool directed,
                   float initialWeight)
    : type(type), directed(directed),
      _a(const_cast<Singular*>(&aBeing)),
      _b(const_cast<Singular*>(&bBeing)) {
    _cachedAId = _a ? _a->getIdentifier() : "";
    _cachedBId = _b ? _b->getIdentifier() : "";
    if (initialWeight != -1.0f) setWeight(initialWeight);
}

void Relation::describe() const {
    std::cout << "Relation [" << type << "] "
              << (directed ? "from " : "between ")
              << aId() << (directed ? " -> " : " and ") << bId()
              << " (strength=" << getWeight() << ")"
              << std::endl;
}

bool Relation::involves(const Singular* being) const {
    return being && (_a == being || _b == being);
}

bool Relation::involves(const Singular& being) const {
    return _a == &being || _b == &being;
}

bool Relation::involves(const std::string& identifier) const {
    if (identifier.empty()) return false;
    return aId() == identifier || bId() == identifier;
}

bool Relation::isBetween(const Singular& aBeing, const Singular& bBeing) const {
    if (directed) {
        return _a == &aBeing && _b == &bBeing;
    }
    return (_a == &aBeing && _b == &bBeing) || (_a == &bBeing && _b == &aBeing);
}

bool Relation::isBetween(const std::string& a, const std::string& b) const {
    if (a.empty() || b.empty()) return false;
    if (directed) {
        return aId() == a && bId() == b;
    }
    return (aId() == a && bId() == b) || (aId() == b && bId() == a);
}

json Relation::toJson() const {
    json evArr = json::array();
    for(const auto& ev : events) evArr.push_back(ev.toJson());

    return json{{"type", type},
                {"entityA", aId()},
                {"entityB", bId()},
                {"directed", directed},
                {"weight", getWeight()},
                {"events", evArr},
                {"attachment", attachment.toJson()}};
}

Relation Relation::fromJson(const json& j, const RelationEndpointResolver& resolve) {
    Relation r;
    r.type = j.at("type").get<std::string>();
    r.directed = j.value("directed", false);
    r.setWeight(j.value("weight", 1.0f));

    if(j.contains("events") && j["events"].is_array()){
        for(const auto& item : j["events"]) {
            r.events.push_back(RelationEvent::fromJson(item));
        }
    }
    if (j.contains("attachment")) {
        r.attachment = AttachmentData::fromJson(j["attachment"]);
    }

    r._savedA = j.value("entityA", std::string{});
    r._savedB = j.value("entityB", std::string{});
    if (resolve) {
        r._a = r._savedA.empty() ? nullptr : resolve(r._savedA);
        r._b = r._savedB.empty() ? nullptr : resolve(r._savedB);
        if (r._a) r._savedA.clear();
        if (r._b) r._savedB.clear();
        if ((!j.value("entityA", std::string{}).empty() && !r._a) ||
            (!j.value("entityB", std::string{}).empty() && !r._b)) {
            std::fprintf(stderr,
                "Relation::fromJson: unbound endpoint(s) type='%s' a='%s' b='%s'. "
                "Identifier properties kept; the relation holds no being until bind.\n",
                r.type.c_str(), j.value("entityA", std::string{}).c_str(),
                j.value("entityB", std::string{}).c_str());
        }
    }
    return r;
}

// A Relation is a legible Singular: type/weight/directed are governable
// state; the endpoints are read-only (they ARE the relation's identity).
void Relation::buildProperties() {
    registerProperty(std::make_unique<PropertyRef<Relation, std::string>>(
        "type", this, &Relation::type));
    registerProperty(std::make_unique<PropertyRef<Relation, bool>>(
        "directed", this, &Relation::directed));
    registerProperty(std::make_unique<ComputedProperty<Relation, float>>(
        "weight", this, &Relation::getWeight, &Relation::setWeight));
    registerProperty(std::make_unique<ComputedProperty<Relation, std::string>>(
        "entityA", this, &Relation::propEntityA));
    registerProperty(std::make_unique<ComputedProperty<Relation, std::string>>(
        "entityB", this, &Relation::propEntityB));

    registerProperty(std::make_unique<ComputedProperty<Relation, bool>>(
        "attachment.enabled", this, &Relation::getAttachmentEnabled, &Relation::setAttachmentEnabled));
    registerProperty(std::make_unique<ComputedProperty<Relation, glm::mat4>>(
        "attachment.localOffset", this, &Relation::getAttachmentLocalOffset, &Relation::setAttachmentLocalOffset));
    registerProperty(std::make_unique<ComputedProperty<Relation, glm::vec3>>(
        "attachment.parentAnchor", this, &Relation::getAttachmentParentAnchor, &Relation::setAttachmentParentAnchor));
    registerProperty(std::make_unique<ComputedProperty<Relation, glm::vec3>>(
        "attachment.childAnchor", this, &Relation::getAttachmentChildAnchor, &Relation::setAttachmentChildAnchor));
    registerProperty(std::make_unique<ComputedProperty<Relation, bool>>(
        "attachment.inheritTranslation", this, &Relation::getAttachmentInheritTranslation, &Relation::setAttachmentInheritTranslation));
    registerProperty(std::make_unique<ComputedProperty<Relation, bool>>(
        "attachment.inheritRotation", this, &Relation::getAttachmentInheritRotation, &Relation::setAttachmentInheritRotation));
    registerProperty(std::make_unique<ComputedProperty<Relation, bool>>(
        "attachment.inheritScale", this, &Relation::getAttachmentInheritScale, &Relation::setAttachmentInheritScale));
    registerProperty(std::make_unique<ComputedProperty<Relation, std::shared_ptr<PropertyList>>>(
        "events", this, &Relation::getEventsList, &Relation::setEventsList));
}


bool Relation::s_developerMode = true; // Default true for developer testing

float Relation::getWeight() const {
    PropertyValue out;
    if (getDynamicProperty("weight", out)) {
        return std::get<float>(out);
    }
    if (s_developerMode) {
        std::cerr << "[Relation] AUDIT WARNING: weight not explicitly settled for Relation " << getIdentifier() << ". Falling back to 1.0f in developer mode." << std::endl;
        return 1.0f;
    }
    throw std::runtime_error("Relation weight not explicitly settled by a Person.");
}

void Relation::setWeight(const float& w) {
    setDynamicProperty("weight", PropertyValue(w));
}

std::shared_ptr<PropertyList> Relation::getEventsList() const {
    auto list = std::make_shared<PropertyList>();
    for (const auto& ev : events) {
        auto dict = std::make_shared<PropertyDict>();
        dict->elements["timestamp"] = PropertyValue(static_cast<long>(ev.timestamp));
        dict->elements["description"] = PropertyValue(ev.description);
        dict->elements["deltaWeight"] = PropertyValue(ev.deltaWeight);
        list->elements.push_back(PropertyValue(dict));
    }
    return list;
}

void Relation::setEventsList(const std::shared_ptr<PropertyList>& list) {
    if (!list) return;
    events.clear();
    for (const auto& item : list->elements) {
        if (auto dict = std::get_if<std::shared_ptr<PropertyDict>>(&item)) {
            RelationEvent ev;
            if ((*dict)->elements.count("timestamp")) {
                if (auto* v = std::get_if<long>(&(*dict)->elements["timestamp"])) ev.timestamp = *v;
            }
            if ((*dict)->elements.count("description")) {
                if (auto* v = std::get_if<std::string>(&(*dict)->elements["description"])) ev.description = *v;
            }
            if ((*dict)->elements.count("deltaWeight")) {
                if (auto* v = std::get_if<float>(&(*dict)->elements["deltaWeight"])) ev.deltaWeight = *v;
                else if (auto* dv = std::get_if<double>(&(*dict)->elements["deltaWeight"])) ev.deltaWeight = static_cast<float>(*dv);
            }
            events.push_back(ev);
        }
    }
}
