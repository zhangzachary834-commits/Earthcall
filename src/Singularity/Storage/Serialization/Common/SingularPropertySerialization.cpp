#include "Singularity/Storage/Serialization/Common/SingularPropertySerialization.hpp"

#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Property/Property.hpp"
#include "Singularity/Core/StringId.hpp"

#include <cstdio>
#include <vector>

namespace Singularity::Storage {
namespace {

std::string propertyName(Earthcall::StringId id) {
    return Earthcall::StringInterner::resolve(id);
}

bool applyRegistered(const std::string& name,
                     const nlohmann::json& raw,
                     Singular& being,
                     const PropertyReferenceResolver& resolve) {
    const Earthcall::StringId id = Earthcall::StringInterner::intern(name);
    Property* property = being.findProperty(id);
    if (!property || being.hasDynamicProperty(id) ||
        !property->isSemanticallyWritable()) {
        being.deferRegisteredPropertyJson(id, raw);
        std::fprintf(stderr,
            "[SingularPropertySerialization] DEFERRED registered property '%s' on '%s': "
            "this build cannot write that registered semantic path yet.\n",
            name.c_str(), being.getIdentifier().c_str());
        return false;
    }

    auto decoded = tryPropertyValueFromJson(raw, resolve);
    if (!decoded || !property->setValue(*decoded)) {
        being.deferRegisteredPropertyJson(id, raw);
        std::fprintf(stderr,
            "[SingularPropertySerialization] DEFERRED registered property '%s' on '%s': "
            "value or referenced identity is not resolvable yet.\n",
            name.c_str(), being.getIdentifier().c_str());
        return false;
    }

    being.clearDeferredRegisteredProperty(id);
    return true;
}

bool applyAuthored(const std::string& name,
                   const nlohmann::json& raw,
                   Singular& being,
                   const PropertyReferenceResolver& resolve) {
    const Earthcall::StringId id = Earthcall::StringInterner::intern(name);

    // Authored state may never shadow first-mover vocabulary. A legacy file
    // that attempts it is preserved raw and confessed rather than silently
    // reinterpreted as a registered-property write.
    if (!being.hasDynamicProperty(id)) {
        if (Property* registered = being.findProperty(id)) {
            (void)registered;
            being.deferAuthoredPropertyJson(id, raw);
            std::fprintf(stderr,
                "[SingularPropertySerialization] REFUSED authored property '%s' on '%s': "
                "it collides with this build's registered vocabulary. Raw state preserved.\n",
                name.c_str(), being.getIdentifier().c_str());
            return false;
        }
    }

    auto decoded = tryPropertyValueFromJson(raw, resolve);
    if (!decoded || !being.setDynamicProperty(id, *decoded)) {
        being.deferAuthoredPropertyJson(id, raw);
        std::fprintf(stderr,
            "[SingularPropertySerialization] DEFERRED authored property '%s' on '%s': "
            "value or referenced identity is not resolvable yet.\n",
            name.c_str(), being.getIdentifier().c_str());
        return false;
    }

    being.clearDeferredAuthoredProperty(id);
    return true;
}

} // namespace

void writeSingularProperties(nlohmann::json& j, const Singular& being) {
    nlohmann::json authored = nlohmann::json::object();

    // Resolved live state is written first. Any still-deferred raw value wins
    // LAST: preserve-first means an unresolved future/identity payload may
    // never be overwritten by a default/placeholder live value on re-save.
    for (const auto& [id, stored] : being.dynamicProperties()) {
        PropertyValue live = stored;
        being.getDynamicProperty(id, live);
        authored[propertyName(id)] = propertyValueToJson(live);
    }
    for (const auto& [id, raw] : being.pendingAuthoredPropertyJson()) {
        authored[propertyName(id)] = raw;
    }
    if (!authored.empty()) j["authoredProperties"] = std::move(authored);

    nlohmann::json registered = nlohmann::json::object();

    // listProperties lazily materializes the registry but changes no authored
    // semantic state. The registry cache is intentionally lazy in Singular.
    auto& mutableBeing = const_cast<Singular&>(being);
    for (Property* property : mutableBeing.listProperties()) {
        if (!property || !property->isSemanticallyWritable()) continue;
        if (being.hasDynamicProperty(property->nameId())) continue;

        PropertyValue value = property->value();
        if (std::holds_alternative<std::monostate>(value)) continue;
        registered[property->name()] = propertyValueToJson(value);
    }
    // Same preserve-first rule as authored state: if hydration could not yet
    // bind a saved value, that exact payload outranks whatever default the
    // current in-memory property happens to expose.
    for (const auto& [id, raw] : being.pendingRegisteredPropertyJson()) {
        registered[propertyName(id)] = raw;
    }
    if (!registered.empty()) j["registeredProperties"] = std::move(registered);

    if (!being.getDesignatedZones().empty()) {
        j["designatedZones"] = being.getDesignatedZones();
    }
}

bool readSingularProperties(const nlohmann::json& j,
                            Singular& being,
                            const PropertyReferenceResolver& resolve) {
    bool complete = true;

    if (j.contains("registeredProperties") &&
        j["registeredProperties"].is_object()) {
        for (auto it = j["registeredProperties"].begin();
             it != j["registeredProperties"].end(); ++it) {
            if (!applyRegistered(it.key(), it.value(), being, resolve)) {
                complete = false;
            }
        }
    }

    if (j.contains("authoredProperties") &&
        j["authoredProperties"].is_object()) {
        for (auto it = j["authoredProperties"].begin();
             it != j["authoredProperties"].end(); ++it) {
            if (!applyAuthored(it.key(), it.value(), being, resolve)) {
                complete = false;
            }
        }
    }

    if (j.contains("designatedZones") && j["designatedZones"].is_array()) {
        for (const auto& item : j["designatedZones"]) {
            if (item.is_string()) being.addZoneDesignation(item.get<std::string>());
        }
    }

    return complete;
}

bool resolveDeferredSingularProperties(
    Singular& being,
    const PropertyReferenceResolver& resolve) {
    if (!resolve) return false;
    bool complete = true;

    std::vector<std::pair<std::string, nlohmann::json>> registered;
    registered.reserve(being.pendingRegisteredPropertyJson().size());
    for (const auto& [id, raw] : being.pendingRegisteredPropertyJson()) {
        registered.emplace_back(propertyName(id), raw);
    }
    for (const auto& [name, raw] : registered) {
        if (!applyRegistered(name, raw, being, resolve)) complete = false;
    }

    std::vector<std::pair<std::string, nlohmann::json>> authored;
    authored.reserve(being.pendingAuthoredPropertyJson().size());
    for (const auto& [id, raw] : being.pendingAuthoredPropertyJson()) {
        authored.emplace_back(propertyName(id), raw);
    }
    for (const auto& [name, raw] : authored) {
        if (!applyAuthored(name, raw, being, resolve)) complete = false;
    }

    return complete;
}

} // namespace Singularity::Storage
