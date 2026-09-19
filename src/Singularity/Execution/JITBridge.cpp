#include "JITBridge.hpp"
#include "Singularity/Core/StringId.hpp"
#include <iostream>

extern "C" {

void Earthcall_JIT_SetDoubleProperty(Singular* target, uint32_t stringIdVal, double value) {
    Earthcall::StringId id(stringIdVal);
    auto* prop = target->findProperty(id);
    if (prop) {
        prop->setValue(value);
    }
}

void Earthcall_JIT_AddDoubleProperty(Singular* target, uint32_t stringIdVal, double value) {
    Earthcall::StringId id(stringIdVal);
    auto* prop = target->findProperty(id);
    if (prop && std::holds_alternative<double>(prop->value())) {
        double current = std::get<double>(prop->value());
        prop->setValue(current + value);
    }
}

void Earthcall_JIT_ScaleDoubleProperty(Singular* target, uint32_t stringIdVal, double value) {
    Earthcall::StringId id(stringIdVal);
    auto* prop = target->findProperty(id);
    if (prop && std::holds_alternative<double>(prop->value())) {
        double current = std::get<double>(prop->value());
        prop->setValue(current * value);
    }
}

int Earthcall_JIT_CheckDoubleGt(Singular* target, uint32_t stringIdVal, double value) {
    Earthcall::StringId id(stringIdVal);
    auto* prop = target->findProperty(id);
    if (prop && std::holds_alternative<double>(prop->value())) {
        return std::get<double>(prop->value()) > value ? 1 : 0;
    }
    return 0;
}

} // extern "C"
