#pragma once

#include "Property.hpp"

#include <string>
#include <typeinfo>
#include <utility>

template <typename Owner, typename T>
class PropertyRef : public Property {
public:
    PropertyRef(std::string propertyName, Owner* owner, T Owner::*member)
        : _name(std::move(propertyName)), _owner(owner), _member(member) {}

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
    }

private:
    std::string _name;
    Owner* _owner;
    T Owner::*_member;
};
