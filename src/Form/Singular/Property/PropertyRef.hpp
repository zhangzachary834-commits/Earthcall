#pragma once

#include "Property.hpp"

#include <string>
#include <typeinfo>
#include <utility>

template <typename Owner, typename T>
class PropertyRef : public Property {
public:
    Singular* _singularOwner = nullptr;

    PropertyRef(std::string propertyName, Owner* owner, T Owner::*member, Singular* singularOwner = nullptr)
        : _name(std::move(propertyName)), _owner(owner), _member(member), _singularOwner(singularOwner) {
        if constexpr (std::is_base_of_v<Singular, Owner>) {
            if (!_singularOwner) _singularOwner = static_cast<Singular*>(owner);
        }
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
        _owner->*_member = value;
        if (_singularOwner) {
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
    Owner* _owner;
    T Owner::*_member;
};
