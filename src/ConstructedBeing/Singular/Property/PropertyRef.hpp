#pragma once

#include <type_traits>

#include "Property.hpp"
#include "ConstructedBeing/Singular/Singular.hpp"
#include "Singularity/Core/StringId.hpp"

#include <string>
#include <typeinfo>
#include <utility>

template <typename Owner, typename T>
class PropertyRef : public Property {
public:
    PropertyRef(std::string propertyName, Owner* owner, T Owner::*member, Singular* singularOwner = nullptr)
        : _name(std::move(propertyName)),
          _nameId(Earthcall::StringInterner::intern(_name)),
          _owner(owner),
          _member(member),
          _singularOwner(singularOwner) {
        if constexpr (std::is_base_of_v<Singular, Owner>) {
            if (!_singularOwner) _singularOwner = static_cast<Singular*>(owner);
        }
    }

    Earthcall::StringId nameId() const override {
        return _nameId;
    }

    std::string name() const override {
        return _name;
    }

    std::string typeName() const override {
        return typeid(T).name();
    }

    T& get() {
        return _owner->*_member;
    }

    const T& get() const {
        return _owner->*_member;
    }

    void set(const T& value) {
        // A write that changed nothing must not wake the change feed —
        // markFactDirty scans the whole fact list, and a WhileTrue law
        // re-writes its result every tick by design. The path-addressed
        // writes every law goes through are already guarded upstream
        // (PropertyPath::setValue answers Unchanged); this covers the typed
        // C++ set() calls that never touch a path.
        bool same = false;
        if constexpr (std::is_arithmetic_v<T> || std::is_same_v<T, std::string> ||
                      std::is_same_v<T, glm::vec3> || std::is_same_v<T, glm::mat4>) {
            same = (_owner->*_member == value);
        }
        _owner->*_member = value;
        if (_singularOwner && !same) {
            Singular::notifyPropertyChanged(_singularOwner, _name);
        }
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
                set(*typed);
                return true;
            }
        }
        (void)v;
        return false;
    }

    Singular* asSingular() const override {
        if constexpr (std::is_pointer_v<T> &&
                      std::is_base_of_v<Singular, std::remove_pointer_t<T>>) {
            return get();
        } else if constexpr (std::is_base_of_v<Singular, T>) {
            return const_cast<T*>(&get());
        } else {
            return nullptr;
        }
    }

private:
    std::string _name;
    Earthcall::StringId _nameId;  // Cached at construction, zero cost to access
    Owner* _owner;
    T Owner::*_member;
public:
    Singular* _singularOwner = nullptr;
};
