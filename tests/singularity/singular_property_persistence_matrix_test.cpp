// Universal Singular semantic-property persistence matrix.
//
// Zach's requirement: the save -> fresh hydration loop must work for every
// Singular and its Properties, not only the Object cases that happened to have
// hand-written authoredProperties support.
//
// Path A: all PropertyValue alternatives through the real Object codec.
// Path B: representative concrete persistence roots through their real codecs.
// The Zone identity-store fresh-manager witness lives separately in
// zone_singular_property_roundtrip_test.cpp.

#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Singular/Lexeme/Lexeme.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Relation/Formation/Formation.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/Storage/Serialization/Common/SingularPropertySerialization.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "Singularity/Storage/Serialization/Relation/RelationSerialization.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/OurverseSerialization.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include "ZonesOfEarth/HomesOfEarth/Home.hpp"
#include "ZonesOfEarth/Ourverse/Ourverse.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"

#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace {

int g_checks = 0;
int g_failures = 0;

void check(bool condition, const std::string& description) {
    ++g_checks;
    if (!condition) {
        ++g_failures;
        std::cout << "  FAILED: " << description << "\n";
    } else {
        std::cout << "  ok: " << description << "\n";
    }
}

bool near(double a, double b, double eps = 1e-5) {
    return std::fabs(a - b) <= eps;
}

void seedSimple(Singular& being, int marker) {
    being.setDynamicProperty("authored.persistence.marker", marker);
    being.setDynamicProperty("authored.persistence.label",
                             std::string("marker-") + std::to_string(marker));
    being.setTelosId("lexeme.persistence." + std::to_string(marker));
    being.addZoneDesignation("PersistenceMatrix");
}

void assertSimple(Singular& being, int marker, const std::string& label) {
    PropertyValue v;
    check(being.getDynamicProperty("authored.persistence.marker", v) &&
              std::holds_alternative<int>(v) && std::get<int>(v) == marker,
          label + ": authored int survived");
    check(being.getDynamicProperty("authored.persistence.label", v) &&
              std::holds_alternative<std::string>(v) &&
              std::get<std::string>(v) == "marker-" + std::to_string(marker),
          label + ": authored string survived");
    check(being.telosId() == "lexeme.persistence." + std::to_string(marker),
          label + ": registered Singular.telos survived");
    check(being.belongsToZone("PersistenceMatrix"),
          label + ": Singular Zone designation survived");
}

} // namespace

int main() {
    std::cout << "============================================================\n";
    std::cout << "Universal Singular property persistence matrix\n";
    std::cout << "============================================================\n";

    // ---------------------------------------------------------------------
    // A. Every PropertyValue alternative through a real Object codec.
    // ---------------------------------------------------------------------
    Object refA("ref-a");
    refA.setObjectID("object.ref-a");
    Object refB("ref-b");
    refB.setObjectID("object.ref-b");
    Relation refRelation("persistence-link", refA, refB, true, 0.5f);
    Formation refFormation;
    refFormation.setIdentifier("formation.persistence-ref");
    Singularity::Language::Lexeme refLexeme(
        "Persistence Reference", "lexeme.persistence-ref");

    Object source("property-matrix-source");
    source.setObjectID("object.property-matrix-source");
    source.setDynamicProperty("pv.none", PropertyValue{});
    source.setDynamicProperty("pv.int", 42);
    source.setDynamicProperty("pv.float", 2.5f);
    source.setDynamicProperty("pv.double", 9.25);
    source.setDynamicProperty("pv.bool", true);
    source.setDynamicProperty("pv.char", static_cast<char>('Z'));
    source.setDynamicProperty("pv.long", static_cast<long>(1234567));
    source.setDynamicProperty("pv.string", std::string("Earthcall"));
    source.setDynamicProperty("pv.vec3", glm::vec3(1.0f, 2.0f, 3.0f));

    glm::mat4 matrix(1.0f);
    matrix[3][0] = 7.0f;
    matrix[3][1] = -2.0f;
    source.setDynamicProperty("pv.mat4", matrix);

    auto list = std::make_shared<PropertyList>();
    list->elements.push_back(PropertyValue(7));
    list->elements.push_back(PropertyValue(std::string("seven")));
    source.setDynamicProperty("pv.list", list);

    auto dict = std::make_shared<PropertyDict>();
    dict->elements["answer"] = PropertyValue(42);
    dict->elements["word"] = PropertyValue(std::string("logos"));
    source.setDynamicProperty("pv.dict", dict);

    source.setDynamicProperty("pv.singular", static_cast<Singular*>(&refLexeme));
    source.setDynamicProperty("pv.object", &refA);
    source.setDynamicProperty("pv.relation", &refRelation);
    source.setDynamicProperty("pv.formation", &refFormation);

    auto scalarField = std::make_shared<OntoMath::ScalarField>();
    scalarField->baseDensity = 3.25f;
    scalarField->frequency = 1.75f;
    source.setDynamicProperty("pv.scalarField", scalarField);

    auto vectorField = std::make_shared<OntoMath::VectorField>();
    vectorField->baseFlowX = 4.0f;
    vectorField->baseFlowY = -3.0f;
    vectorField->amplitude = 6.5f;
    source.setDynamicProperty("pv.vectorField", vectorField);

    source.setTelosId("lexeme.persistence.object");

    nlohmann::json objectJson;
    to_json(objectJson, source);
    check(objectJson.contains("authoredProperties"),
          "Object codec emits universal authoredProperties");
    check(objectJson.contains("registeredProperties"),
          "Object codec emits universal registeredProperties");

    Object restored("property-matrix-restored");
    from_json(objectJson, restored);

    std::unordered_map<std::string, Singular*> refs{
        {refLexeme.getIdentifier(), &refLexeme},
        {refA.getIdentifier(), &refA},
        {refRelation.getIdentifier(), &refRelation},
        {refFormation.getIdentifier(), &refFormation}
    };
    const auto resolve = [&](const std::string& id) -> Singular* {
        auto it = refs.find(id);
        return it == refs.end() ? nullptr : it->second;
    };
    check(Singularity::Storage::resolveDeferredSingularProperties(restored, resolve),
          "Object deferred identity Properties bind after referenced roots exist");

    PropertyValue v;
    check(restored.getDynamicProperty("pv.none", v) &&
              std::holds_alternative<std::monostate>(v), "PropertyValue none survived");
    check(restored.getDynamicProperty("pv.int", v) && std::get<int>(v) == 42,
          "PropertyValue int survived");
    check(restored.getDynamicProperty("pv.float", v) && near(std::get<float>(v), 2.5),
          "PropertyValue float survived");
    check(restored.getDynamicProperty("pv.double", v) && near(std::get<double>(v), 9.25),
          "PropertyValue double survived");
    check(restored.getDynamicProperty("pv.bool", v) && std::get<bool>(v),
          "PropertyValue bool survived");
    check(restored.getDynamicProperty("pv.char", v) && std::get<char>(v) == 'Z',
          "PropertyValue char survived");
    check(restored.getDynamicProperty("pv.long", v) && std::get<long>(v) == 1234567,
          "PropertyValue long survived");
    check(restored.getDynamicProperty("pv.string", v) &&
              std::get<std::string>(v) == "Earthcall", "PropertyValue string survived");
    check(restored.getDynamicProperty("pv.vec3", v) &&
              std::get<glm::vec3>(v) == glm::vec3(1.0f, 2.0f, 3.0f),
          "PropertyValue vec3 survived");
    check(restored.getDynamicProperty("pv.mat4", v) &&
              near(std::get<glm::mat4>(v)[3][0], 7.0) &&
              near(std::get<glm::mat4>(v)[3][1], -2.0),
          "PropertyValue mat4 survived");
    check(restored.getDynamicProperty("pv.list", v) &&
              std::get<std::shared_ptr<PropertyList>>(v)->elements.size() == 2,
          "PropertyValue list survived");
    check(restored.getDynamicProperty("pv.dict", v) &&
              std::get<std::shared_ptr<PropertyDict>>(v)->elements.size() == 2,
          "PropertyValue dict survived");
    check(restored.getDynamicProperty("pv.singular", v) &&
              std::get<Singular*>(v) == &refLexeme,
          "PropertyValue Singular* rebound by identity");
    check(restored.getDynamicProperty("pv.object", v) &&
              std::get<Object*>(v) == &refA,
          "PropertyValue Object* rebound by identity");
    check(restored.getDynamicProperty("pv.relation", v) &&
              std::get<Relation*>(v) == &refRelation,
          "PropertyValue Relation* rebound by identity");
    check(restored.getDynamicProperty("pv.formation", v) &&
              std::get<Formation*>(v) == &refFormation,
          "PropertyValue Formation* rebound by identity");
    check(restored.getDynamicProperty("pv.scalarField", v) &&
              near(std::get<std::shared_ptr<OntoMath::ScalarField>>(v)->baseDensity, 3.25) &&
              near(std::get<std::shared_ptr<OntoMath::ScalarField>>(v)->frequency, 1.75),
          "PropertyValue ScalarField payload survived");
    check(restored.getDynamicProperty("pv.vectorField", v) &&
              near(std::get<std::shared_ptr<OntoMath::VectorField>>(v)->baseFlowX, 4.0) &&
              near(std::get<std::shared_ptr<OntoMath::VectorField>>(v)->baseFlowY, -3.0) &&
              near(std::get<std::shared_ptr<OntoMath::VectorField>>(v)->amplitude, 6.5),
          "PropertyValue VectorField payload survived");
    check(restored.telosId() == "lexeme.persistence.object",
          "Object inherited registered telos survived");

    // ---------------------------------------------------------------------
    // B. Concrete persistence-root matrix.
    // ---------------------------------------------------------------------
    Material material("persistence-matrix");
    seedSimple(material, 2);
    auto materialJson = material.toJson();
    Material restoredMaterial = Material::fromJson(materialJson);
    assertSimple(restoredMaterial, 2, "Material");

    Relation relation("matrix-relation", refA, refB, true, 0.75f);
    seedSimple(relation, 3);
    auto relationJson = relationToJson(relation);
    Relation restoredRelation = relationFromJson(
        relationJson, [&](const std::string& id) -> Singular* {
            if (id == refA.getIdentifier()) return &refA;
            if (id == refB.getIdentifier()) return &refB;
            return nullptr;
        });
    assertSimple(restoredRelation, 3, "Relation");

    Formation formation;
    formation.setIdentifier("formation.persistence-matrix");
    seedSimple(formation, 4);
    auto formationJson = formation.toJson();
    auto restoredFormation = Formation::fromJson(
        formationJson, [&](const std::string& id) -> Singular* {
            return resolve(id);
        });
    check(restoredFormation != nullptr, "Formation codec restored root");
    if (restoredFormation) assertSimple(*restoredFormation, 4, "Formation");

    Zone zone("PersistenceMatrixZone", "strict");
    seedSimple(zone, 5);
    auto zoneJson = zoneToJson(zone);
    auto restoredZone = makeZoneFromJson(zoneJson);
    check(restoredZone != nullptr, "Zone codec restored root");
    if (restoredZone) assertSimple(*restoredZone, 5, "Zone");

    Home home("PersistenceMatrixHome", "strict");
    seedSimple(home, 6);
    auto homeJson = zoneToJson(home);
    auto restoredHomeZone = makeZoneFromJson(homeJson);
    auto restoredHome = std::dynamic_pointer_cast<Home>(restoredHomeZone);
    check(restoredHome != nullptr, "Home codec restored Home identity");
    if (restoredHome) assertSimple(*restoredHome, 6, "Home");

    geom::FieldNode fieldNode("field.persistence-matrix");
    seedSimple(fieldNode, 7);
    fieldNode.field->baseDensity = 8.0f;
    auto fieldJson = fieldNode.toJson();
    auto restoredField = geom::FieldNode::fromJson(fieldJson);
    check(restoredField != nullptr, "FieldNode codec restored root");
    if (restoredField) {
        assertSimple(*restoredField, 7, "FieldNode");
        check(near(restoredField->field->baseDensity, 8.0),
              "FieldNode canonical mathematical payload survived");
    }

    Law law("persistence-matrix-law");
    seedSimple(law, 8);
    auto lawJson = law.toJson();
    auto restoredLaw = Law::fromJson(lawJson);
    check(restoredLaw != nullptr, "Law codec restored root");
    if (restoredLaw) assertSimple(*restoredLaw, 8, "Law");

    Person person(Soul("Persistence Person"), Body("Humanoid", "Voxel"), "");
    person.setDisplayName("Persistence Person");
    seedSimple(person, 9);
    seedSimple(person.soul(), 10);
    seedSimple(person.getBody(), 11);
    auto personJson = person.serialize();

    Person restoredPerson(Soul("Restored Person"), Body("Humanoid", "Voxel"), "");
    restoredPerson.deserialize(personJson);
    assertSimple(restoredPerson, 9, "Person");
    assertSimple(restoredPerson.soul(), 10, "Soul nested in Person");
    assertSimple(restoredPerson.getBody(), 11, "Body nested in Person");

    auto gathering = std::make_shared<Zone>("PersistenceGathering", "strict");
    Ourverse ourverse;
    ourverse.setPrimaryGatheringZone(gathering);
    seedSimple(ourverse, 12);
    auto ourverseJson = ourverseToJson(ourverse);

    Ourverse restoredOurverse;
    const bool ourverseLoaded = ourverseFromJson(
        restoredOurverse, ourverseJson,
        [&](const std::string& id) -> std::shared_ptr<Zone> {
            return id == gathering->getIdentifier() ? gathering : nullptr;
        },
        [&](const std::string& id) -> Singular* {
            if (id == gathering->getIdentifier()) return gathering.get();
            return resolve(id);
        });
    check(ourverseLoaded, "Ourverse codec accepted semantic root");
    assertSimple(restoredOurverse, 12, "Ourverse");

    Singularity::Language::Lexeme lexeme(
        "Persistence Word", "lexeme.persistence-matrix");
    seedSimple(lexeme, 13);
    nlohmann::json lexemeSemantic = nlohmann::json::object();
    Singularity::Storage::writeSingularProperties(lexemeSemantic, lexeme);
    Singularity::Language::Lexeme restoredLexeme(
        "Persistence Word", "lexeme.persistence-matrix");
    check(Singularity::Storage::readSingularProperties(
              lexemeSemantic, restoredLexeme, resolve),
          "Lexeme shared semantic envelope restored without deferral");
    assertSimple(restoredLexeme, 13, "Lexeme");

    std::cout << "============================================================\n";
    std::cout << "Singular persistence matrix: " << g_checks
              << " checks, " << g_failures << " failures\n";
    std::cout << "============================================================\n";
    return g_failures == 0 ? 0 : 1;
}
