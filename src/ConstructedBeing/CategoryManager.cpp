#include "CategoryManager.hpp"
#include "Person/PersonDatabase.hpp"
#include "Singularity/Storage/Serialization.hpp"

#include <algorithm>
#include <filesystem>
#include <iostream>

namespace {
// Canonical authored categories live in the category.* namespace. Older
// generator saves also smuggled author referents through the categories bag;
// keep those legacy model referents loadable for now, but never let that
// compatibility path counterfeit a registered human Person as an Object.
bool shadowsRegisteredPerson(const std::string& identifier) {
    if (identifier.empty() || identifier.rfind("category.", 0) == 0) return false;

    for (const auto& profilePath : PersonDatabase::getInstance().getAllRegisteredPersons()) {
        if (std::filesystem::path(profilePath).stem().string() == identifier) return true;
    }
    return false;
}

bool refusePersonAsCategoryObject(const std::string& identifier) {
    if (!shadowsRegisteredPerson(identifier)) return false;
    std::cerr << "[CategoryManager] REFUSED category Object '" << identifier
              << "': that identifier belongs to a registered Person. "
              << "Persons are not Objects; reference the Person being instead.\n";
    return true;
}
} // namespace

// A category is a BEING, and a being is addressed by its identifier. The slug
// IS the name here ("category.tool.brush"), so law text can name it: an
// Object's `name` member is protected and unaddressable, while
// `setObjectID` is what `getIdentifier()` reports.
std::shared_ptr<Object> CategoryManager::create(const std::string& name) {
    if (auto existing = get(name)) return existing;
    if (refusePersonAsCategoryObject(name)) return nullptr;
    auto cat = std::make_shared<Object>(name);
    cat->setName(name);
    cat->setPhysicalObject(0);   // a classification is extra-spatial
    _categories.push_back(cat);
    return cat;
}

void CategoryManager::add(const std::shared_ptr<Object>& cat) {
    if (!cat) return;
    if (refusePersonAsCategoryObject(cat->getIdentifier())) return;
    if (!get(cat->getIdentifier())) {
        _categories.push_back(cat);
    }
}

bool CategoryManager::remove(const std::string& identifier) {
    auto it = std::remove_if(_categories.begin(), _categories.end(), [&](const auto& c) {
        return c->getIdentifier() == identifier;
    });
    if (it != _categories.end()) {
        _categories.erase(it, _categories.end());
        return true;
    }
    return false;
}

std::shared_ptr<Object> CategoryManager::get(const std::string& identifier) const {
    for (const auto& c : _categories) {
        if (c && c->getIdentifier() == identifier) {
            return c;
        }
    }
    return nullptr;
}

std::shared_ptr<Object> CategoryManager::resolveOrDefault(const std::string& identifier) const {
    if (auto c = get(identifier)) return c;
    if (auto def = get("category.default")) return def;
    return nullptr;
}

nlohmann::json CategoryManager::toJson() const {
    nlohmann::json arr = nlohmann::json::array();
    for (const auto& c : _categories) {
        if (!c) continue;
        nlohmann::json j;
        to_json(j, *c);
        arr.push_back(std::move(j));
    }
    return arr;
}

void CategoryManager::loadFromJson(const nlohmann::json& j) {
    _categories.clear();
    if (j.is_array()) {
        for (const auto& elem : j) {
            std::string id;
            if (elem.contains("objectID") && elem["objectID"].is_string()) {
                id = elem["objectID"].get<std::string>();
            } else if (elem.contains("id") && elem["id"].is_string()) {
                id = elem["id"].get<std::string>();
            }
            auto cat = std::make_shared<Object>(id);
            from_json(elem, *cat);
            // Check the deserialized identifier too: compatibility inputs may
            // spell identity through a field older than the two probes above.
            if (refusePersonAsCategoryObject(cat->getIdentifier())) continue;
            // Classification is extra-spatial. from_json rebuilds an Object
            // whose physical bit defaults to true, so a loaded category
            // would otherwise re-enter the world as a cube.
            cat->setPhysicalObject(0);
            _categories.push_back(cat);
        }
    }
    ensureDefaults();
}

// REMAINING DEBT: `category.tool.brush` is a domain category authored in C++,
// which AUTHORED_CATEGORIES.md refuses — categories are rooted acyclic
// Formations of AUTHORED beings, not translation-unit constants. It stays only
// because nothing reads this manager yet (`globals.cpp` declares the instance
// and no other file touches it), so removing it would change nothing and
// authoring it properly needs a seed world first.
void CategoryManager::ensureDefaults() {
    create("category.default");

    auto brush = create("category.tool.brush");
    // Typed, not stringly: a law that writes size must update a double, not
    // change the property's type on its first write.
    brush->setDynamicProperty("size", PropertyValue(1.0));
    brush->setDynamicProperty("scale", PropertyValue(glm::vec3(1.0f, 1.0f, 1.0f)));
}
