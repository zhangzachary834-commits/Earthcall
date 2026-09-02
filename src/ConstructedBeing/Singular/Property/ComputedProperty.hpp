#pragma once

#include "Property.hpp"
#include "Singularity/Core/StringId.hpp"

#include <string>
#include <typeinfo>
#include <utility>

// Wrap getter/setter methods instead of raw member variables. Used where the
// truth of a property is derived state (e.g. Object "position" lives in the
// transform matrix, not in a vec3 member). Read-only when setter is null.
template <typename Owner, typename T>
class ComputedProperty : public Property {
public:
    using Getter = T (Owner::*)() const;
    using Setter = void (Owner::*)(const T&);

    ComputedProperty(std::string propertyName, Owner* owner, Getter getter, Setter setter = nullptr)
        : _name(std::move(propertyName)),
          _nameId(Earthcall::StringInterner::intern(_name)),
          _owner(owner),
          _getter(getter),
          _setter(setter) {}

    Earthcall::StringId nameId() const override { return _nameId; }

    std::string name() const override { return _name; }

    std::string typeName() const override { return typeid(T).name(); }

    T get() const { return (_owner->*_getter)(); }

    bool set(const T& v) {
        if (!_setter) return false;
        (_owner->*_setter)(v);
        return true;
    }

    PropertyValue value() const override {
        if constexpr (is_property_value_alternative<T>) {
            return PropertyValue(get());
        } else {
            return PropertyValue{};   // monostate: not legible as a value
        }
    }

    bool setValue(const PropertyValue& v) override {
        if constexpr (is_property_value_alternative<T>) {
            if (const T* typed = std::get_if<T>(&v)) {
                return set(*typed);
            }
        }
        (void)v;
        return false;
    }

    Singular* asSingular() const override {
        // Only the pointer case: the getter returns T by value, so a
        // Singular-derived value type would be the address of a temporary.
        if constexpr (std::is_pointer_v<T> &&
                      std::is_base_of_v<Singular, std::remove_pointer_t<T>>) {
            return get();
        } else {
            return nullptr;
        }
    }

private:
    std::string _name;
    Earthcall::StringId _nameId;  // Cached at construction, zero cost to access
    Owner* _owner;
    Getter _getter;
    Setter _setter;
};
