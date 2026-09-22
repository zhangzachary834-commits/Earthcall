// ============================================================================
// ObjectComposition.cpp - Composition-related Object implementations
//
// This file contains composition-related method implementations for Object,
// including element membership, attributes, and tags.
// ============================================================================

#include "ConstructedBeing/Singular/Object/Object.hpp"
#include "ConstructedBeing/Singular/Object/Object/ObjectComposition.hpp"

// Element membership methods are in ObjectCore.cpp

// Attributes/Tags implementation

void Object::setAttribute(const std::string& key, const std::string& value) {
    _composition.attributes[key] = value;
}

bool Object::hasAttribute(const std::string& key) const {
    return _composition.attributes.find(key) != _composition.attributes.end();
}

const std::string& Object::getAttribute(const std::string& key) const {
    static const std::string empty;
    auto it = _composition.attributes.find(key);
    return it == _composition.attributes.end() ? empty : it->second;
}

void Object::addTag(const std::string& tag) {
    if (!hasTag(tag)) _composition.tags.push_back(tag);
}

void Object::removeTag(const std::string& tag) {
    _composition.tags.erase(std::remove(_composition.tags.begin(), _composition.tags.end(), tag), _composition.tags.end());
}

bool Object::hasTag(const std::string& tag) const {
    for (const auto& t : _composition.tags) if (t == tag) return true;
    return false;
}
