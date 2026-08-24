// Probe: does a base Singular have the innate capacity to take an arbitrary
// number of properties of any kind — gated or not?
//
// Claims this executes (not just reads):
//   1. The authored map lives on Singular itself (Relation/Material/Person/Object).
//   2. There is no engine cap on count.
//   3. Every PropertyValue alternative can be stored at runtime.
//   4. ActionNode::AddProperty grants without consulting TransferPolicy.
//   5. TransferPolicy closing a name does not stop AddProperty.
//   6. Persistence, enumeration, copy, and JSON kind round-trip are the
//      actual bounds — not existence of the slot.

#include "ConstructedBeing/Material/Material.hpp"
#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "Person/Body/Body.hpp"
#include "Person/Person.hpp"
#include "Person/Soul/Soul.hpp"
#include "Relation/Relation.hpp"
#include "Singularity/OntoMath/Field.hpp"
#include "Singularity/Storage/Serialization.hpp"
#include "Singularity/TransferPolicy.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Universe.hpp"

#include <GLFW/glfw3.h>
#include <cstdio>
#include <string>
#include <vector>

namespace {

int g_pass = 0;
int g_fail = 0;

void check(bool ok, const char* what) {
    if (ok) {
        ++g_pass;
        std::printf("  ok: %s\n", what);
    } else {
        ++g_fail;
        std::printf("  FAIL: %s\n", what);
    }
}

bool hasName(Singular& being, const std::string& name) {
    for (Property* p : being.listProperties()) {
        if (p && p->name() == name) return true;
    }
    return false;
}

} // namespace

int main() {
    if (!glfwInit()) {
        std::fprintf(stderr, "glfwInit failed\n");
        return 1;
    }
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
    GLFWwindow* window = glfwCreateWindow(64, 64, "singular_authored_properties_probe",
                                          nullptr, nullptr);
    if (!window) {
        std::fprintf(stderr, "no GL context\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);

    ECA::Event event{"probe", nullptr, nullptr, 0};

    // ------------------------------------------------------------------
    // 1. The map is on Singular. A Relation is as close as the tree gets
    //    to a "base Singular" you can actually construct.
    // ------------------------------------------------------------------
    std::printf("\n[1] Innate slot on every instantiable Singular\n");
    {
        Relation rel("bond", "a", "b");
        Material mat("probe-clay");
        Object obj;
        Person person(Soul("Probe"), Body::createBasicAvatar("Voxel"), "default");

        rel.setDynamicProperty("mood", PropertyValue(std::string("bright")));
        mat.setDynamicProperty("grain", PropertyValue(0.3));
        obj.setDynamicProperty("warmth", PropertyValue(0.75));
        person.setDynamicProperty("temper", PropertyValue(true));

        PropertyValue v;
        check(rel.getDynamicProperty("mood", v) &&
                  std::holds_alternative<std::string>(v) &&
                  std::get<std::string>(v) == "bright",
              "Relation holds an authored string");
        check(mat.getDynamicProperty("grain", v) &&
                  std::holds_alternative<double>(v) &&
                  std::get<double>(v) == 0.3,
              "Material holds an authored double");
        check(obj.getDynamicProperty("warmth", v) &&
                  std::holds_alternative<double>(v) &&
                  std::get<double>(v) == 0.75,
              "Object holds an authored double");
        check(person.getDynamicProperty("temper", v) &&
                  std::holds_alternative<bool>(v) &&
                  std::get<bool>(v),
              "Person holds an authored bool");
    }

    // ------------------------------------------------------------------
    // 2. Arbitrary number: no engine bound. 500 is enough to show the map
    //    is not a fixed table.
    // ------------------------------------------------------------------
    std::printf("\n[2] Arbitrary count\n");
    {
        Relation rel("count", "a", "b");
        for (int i = 0; i < 500; ++i) {
            rel.setDynamicProperty("p" + std::to_string(i), PropertyValue(i));
        }
        check(rel.dynamicProperties().size() == 500,
              "500 distinct authored names sit on one Relation");
        PropertyValue v;
        check(rel.getDynamicProperty("p0", v) && std::get<int>(v) == 0 &&
                  rel.getDynamicProperty("p499", v) && std::get<int>(v) == 499,
              "first and last of the 500 still read");
    }

    // ------------------------------------------------------------------
    // 3. Every PropertyValue kind at runtime.
    // ------------------------------------------------------------------
    std::printf("\n[3] Every PropertyValue alternative stores\n");
    {
        Relation rel("kinds", "a", "b");
        Object host;
        auto list = std::make_shared<PropertyList>();
        list->elements.push_back(PropertyValue(1));
        auto dict = std::make_shared<PropertyDict>();
        dict->elements["k"] = PropertyValue(std::string("v"));
        auto scalar = std::make_shared<OntoMath::ScalarField>();
        auto vector = std::make_shared<OntoMath::VectorField>();

        rel.setDynamicProperty("none", PropertyValue{});
        rel.setDynamicProperty("i", PropertyValue(7));
        rel.setDynamicProperty("f", PropertyValue(1.5f));
        rel.setDynamicProperty("d", PropertyValue(2.5));
        rel.setDynamicProperty("b", PropertyValue(true));
        rel.setDynamicProperty("c", PropertyValue('Q'));
        rel.setDynamicProperty("l", PropertyValue(static_cast<long>(99)));
        rel.setDynamicProperty("s", PropertyValue(std::string("hello")));
        rel.setDynamicProperty("v3", PropertyValue(glm::vec3(1, 2, 3)));
        rel.setDynamicProperty("m4", PropertyValue(glm::mat4(1.0f)));
        rel.setDynamicProperty("sing", PropertyValue(static_cast<Singular*>(&rel)));
        rel.setDynamicProperty("obj", PropertyValue(&host));
        rel.setDynamicProperty("relp", PropertyValue(&rel));
        rel.setDynamicProperty("form", PropertyValue(static_cast<Formation*>(nullptr)));
        rel.setDynamicProperty("list", PropertyValue(list));
        rel.setDynamicProperty("dict", PropertyValue(dict));
        rel.setDynamicProperty("sf", PropertyValue(scalar));
        rel.setDynamicProperty("vf", PropertyValue(vector));

        const auto& m = rel.dynamicProperties();
        check(std::holds_alternative<std::monostate>(m.at("none")), "monostate");
        check(std::holds_alternative<int>(m.at("i")), "int");
        check(std::holds_alternative<float>(m.at("f")), "float");
        check(std::holds_alternative<double>(m.at("d")), "double");
        check(std::holds_alternative<bool>(m.at("b")), "bool");
        check(std::holds_alternative<char>(m.at("c")), "char");
        check(std::holds_alternative<long>(m.at("l")), "long");
        check(std::holds_alternative<std::string>(m.at("s")), "string");
        check(std::holds_alternative<glm::vec3>(m.at("v3")), "vec3");
        check(std::holds_alternative<glm::mat4>(m.at("m4")), "mat4");
        check(std::holds_alternative<Singular*>(m.at("sing")), "Singular*");
        check(std::holds_alternative<Object*>(m.at("obj")), "Object*");
        check(std::holds_alternative<Relation*>(m.at("relp")), "Relation*");
        check(std::holds_alternative<Formation*>(m.at("form")), "Formation*");
        check(std::holds_alternative<std::shared_ptr<PropertyList>>(m.at("list")),
              "PropertyList");
        check(std::holds_alternative<std::shared_ptr<PropertyDict>>(m.at("dict")),
              "PropertyDict");
        check(std::holds_alternative<std::shared_ptr<OntoMath::ScalarField>>(m.at("sf")),
              "ScalarField");
        check(std::holds_alternative<std::shared_ptr<OntoMath::VectorField>>(m.at("vf")),
              "VectorField");
        check(m.size() == 18, "all 18 alternatives coexist on one being");
    }

    // ------------------------------------------------------------------
    // 4. Law grant (AddProperty) vs mint-by-Set vs shadowing.
    // ------------------------------------------------------------------
    std::printf("\n[4] Law grant, Set-mint, shadow refuse\n");
    {
        Relation rel("law", "a", "b");
        ActionNode::addProperty("", "warmth", PropertyValue(0.75)).compile()(event, rel);
        PropertyValue v;
        check(rel.getDynamicProperty("warmth", v) && std::get<double>(v) == 0.75,
              "AddProperty grants onto a Relation");

        ActionNode::set("minted", PropertyValue(std::string("by-set"))).compile()(event, rel);
        check(rel.getDynamicProperty("minted", v) &&
                  std::get<std::string>(v) == "by-set",
              "Set of a missing single-segment name mints a dynamic property");

        ActionNode::addProperty("", "type", PropertyValue(std::string("shadow"))).compile()(
            event, rel);
        check(!rel.hasDynamicProperty("type"),
              "AddProperty refuses to shadow a first-mover name");

        ActionNode::set("nested.leaf", PropertyValue(1.0)).compile()(event, rel);
        check(!rel.hasDynamicProperty("nested.leaf") && !rel.hasDynamicProperty("nested"),
              "Set of a multi-segment missing path does not mint");

        ActionNode::addProperty("", "acoustic.amplitude", PropertyValue(0.5)).compile()(
            event, rel);
        check(rel.hasDynamicProperty("acoustic.amplitude"),
              "AddProperty may use a dotted string as a single flat key");
        check(PropertyPath::parse("acoustic.amplitude").getValue(rel, v) ==
                  PropertyPath::PathResult::Ok,
              "a dotted flat key still reads through PropertyPath longest-match");
    }

    // ------------------------------------------------------------------
    // 5. TransferPolicy does not gate authoring. Closing "warmth" still
    //    lets AddProperty write it. The gate is for set-to-set capture.
    // ------------------------------------------------------------------
    std::printf("\n[5] TransferPolicy does not gate AddProperty\n");
    {
        Relation rel("gated", "a", "b");
        TransferPolicy& policy = TransferPolicy::instance();
        const bool wasOpen = policy.isOpen("warmth");
        policy.setOpen("warmth", false);
        check(!policy.canTransfer(PropertyPath::parse("warmth")),
              "warmth transfer is closed");
        ActionNode::addProperty("", "warmth", PropertyValue(1.0)).compile()(event, rel);
        check(rel.hasDynamicProperty("warmth"),
              "AddProperty still grants while the transfer gate is closed");
        policy.setOpen("warmth", wasOpen);
    }

    // ------------------------------------------------------------------
    // 6. Nested dict/list exist as values; PropertyPath does not walk them.
    // ------------------------------------------------------------------
    std::printf("\n[6] Nested kinds vs path walk\n");
    {
        Relation rel("nested", "a", "b");
        auto dict = std::make_shared<PropertyDict>();
        dict->elements["leaf"] = PropertyValue(3.0);
        rel.setDynamicProperty("bag", PropertyValue(dict));
        PropertyValue v;
        check(PropertyPath::parse("bag").getValue(rel, v) == PropertyPath::PathResult::Ok,
              "the dict as a whole is a property");
        check(PropertyPath::parse("bag.leaf").getValue(rel, v) !=
                  PropertyPath::PathResult::Ok,
              "PropertyPath does not descend into an authored dict");
    }

    // ------------------------------------------------------------------
    // 7. Enumeration gap: listProperties does not offer an un-looked-up
    //    authored name. After findProperty, it does.
    // ------------------------------------------------------------------
    std::printf("\n[7] Enumeration\n");
    {
        Relation rel("enum", "a", "b");
        rel.setDynamicProperty("secret", PropertyValue(1));
        check(!hasName(rel, "secret"),
              "listProperties misses an authored name nobody has resolved");
        check(rel.findProperty("secret") != nullptr, "findProperty still finds it");
        check(hasName(rel, "secret"),
              "after findProperty, listProperties offers the bridge");
    }

    // ------------------------------------------------------------------
    // 8. Stale bridge: revoke after a lookup, then re-grant.
    // ------------------------------------------------------------------
    std::printf("\n[8] Revoke after lookup\n");
    {
        Relation rel("revoke", "a", "b");
        ActionNode::addProperty("", "glow", PropertyValue(1.5)).compile()(event, rel);
        PropertyValue v;
        PropertyPath::parse("glow").getValue(rel, v);   // caches a DynamicPropertyBridge
        ActionNode::removeProperty("", "glow").compile()(event, rel);
        check(!rel.hasDynamicProperty("glow"), "RemoveProperty erased the map entry");
        check(rel.findProperty("glow") != nullptr,
              "stale DynamicPropertyBridge remains in the registry");
        ActionNode::addProperty("", "glow", PropertyValue(2.0)).compile()(event, rel);
        check(!rel.hasDynamicProperty("glow"),
              "re-grant is refused as first-mover shadow because of the stale bridge");
    }

    // ------------------------------------------------------------------
    // 9. Copy drops authored properties (Singular copy only takes telos).
    // ------------------------------------------------------------------
    std::printf("\n[9] Copy\n");
    {
        Relation src("copy", "a", "b");
        src.setDynamicProperty("kept", PropertyValue(true));
        src.setTelosId("lexeme.christ");
        Relation dst = src;
        check(dst.telosId() == "lexeme.christ", "copy preserves telos");
        check(!dst.hasDynamicProperty("kept"),
              "copy drops the authored map (Singular copy is telos-only)");
    }

    // ------------------------------------------------------------------
    // 10. Persistence: Object keeps authoredProperties; Relation, Material,
    //     Person do not write the map.
    // ------------------------------------------------------------------
    std::printf("\n[10] Persistence\n");
    {
        Object obj;
        obj.setDynamicProperty("jointAngle", PropertyValue(0.25));
        auto list = std::make_shared<PropertyList>();
        list->elements.push_back(PropertyValue(1));
        obj.setDynamicProperty("items", PropertyValue(list));
        obj.setDynamicProperty("label", PropertyValue(std::string("pillar")));
        nlohmann::json j;
        to_json(j, obj);
        check(j.contains("authoredProperties") &&
                  j["authoredProperties"].contains("jointAngle"),
              "Object JSON carries authoredProperties");

        Object loaded;
        from_json(j, loaded);
        PropertyValue v;
        check(loaded.getDynamicProperty("jointAngle", v) &&
                  std::holds_alternative<double>(v) &&
                  std::get<double>(v) == 0.25,
              "Object reload restores a double");
        check(loaded.getDynamicProperty("label", v) &&
                  std::get<std::string>(v) == "pillar",
              "Object reload restores a string");
        check(loaded.getDynamicProperty("items", v) &&
                  std::holds_alternative<std::shared_ptr<PropertyList>>(v),
              "Object reload restores a list");

        Relation rel("persist", "a", "b");
        rel.setDynamicProperty("extra", PropertyValue(1.0));
        auto rj = rel.toJson();
        check(!rj.contains("authoredProperties") && !rj.contains("extra"),
              "Relation JSON does not persist authored properties");

        Material mat("persist-clay");
        mat.setDynamicProperty("grain", PropertyValue(0.2));
        auto mj = mat.toJson();
        check(!mj.contains("authoredProperties") && !mj.contains("grain"),
              "Material JSON does not persist authored properties");

        Person person(Soul("Probe"), Body::createBasicAvatar("Voxel"), "default");
        person.setDynamicProperty("temper", PropertyValue(true));
        auto pj = person.serialize();
        check(!pj.contains("authoredProperties") && !pj.contains("temper"),
              "Person JSON does not persist authored properties");
    }

    // ------------------------------------------------------------------
    // 11. JSON kind round-trip of a PropertyValue itself. Refs and fields
    //     do not come back as themselves.
    // ------------------------------------------------------------------
    std::printf("\n[11] PropertyValue JSON kinds\n");
    {
        auto round = [](const PropertyValue& v) {
            return propertyValueFromJson(propertyValueToJson(v));
        };
        check(std::holds_alternative<int>(round(PropertyValue(3))), "int round-trips");
        check(std::holds_alternative<double>(round(PropertyValue(1.25))),
              "double round-trips");
        check(std::holds_alternative<std::string>(
                  round(PropertyValue(std::string("x")))),
              "string round-trips");
        check(std::holds_alternative<glm::vec3>(round(PropertyValue(glm::vec3(1, 2, 3)))),
              "vec3 round-trips");
        check(std::holds_alternative<std::shared_ptr<PropertyDict>>(
                  round(PropertyValue(std::make_shared<PropertyDict>()))),
              "dict round-trips");

        Relation rel("ref", "a", "b");
        PropertyValue back = round(PropertyValue(static_cast<Singular*>(&rel)));
        check(std::holds_alternative<std::monostate>(back),
              "Singular* serializes as a ref tag and loads as monostate");

        PropertyValue fieldBack =
            round(PropertyValue(std::make_shared<OntoMath::ScalarField>()));
        check(std::holds_alternative<std::monostate>(fieldBack),
              "ScalarField serializes as a type tag with no payload and loads empty");
    }

    // ------------------------------------------------------------------
    // 12. DataStructure::writeBounds exists and is never consulted by grant.
    // ------------------------------------------------------------------
    std::printf("\n[12] Unused bounds object\n");
    {
        Relation rel("bounds", "a", "b");
        DataStructure ds("bag", PropertyValue(0.0));
        rel.addDataStructure(ds);
        check(rel.getDataStructure("bag") != nullptr,
              "a DataStructure can be attached");
        rel.setDynamicProperty("unbounded", PropertyValue(1));
        check(rel.hasDynamicProperty("unbounded"),
              "setDynamicProperty ignores DataStructure writeBounds");
    }

    std::printf("\n%s — %d passed, %d failed\n",
                g_fail == 0 ? "SUCCESS" : "FAILURE", g_pass, g_fail);

    glfwDestroyWindow(window);
    glfwTerminate();
    return g_fail == 0 ? 0 : 1;
}
