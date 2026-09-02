#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include <stdint.h>

extern "C" {
    // ========================================================================
    // C ABI JIT Bridge
    //
    // LLVM IR has no concept of C++ std::variant, std::string, or vtables.
    // To allow the JIT-compiled assembly to mutate Earthcall's C++ state without
    // emitting hundreds of instructions to manipulate libstdc++ memory layouts,
    // the JIT calls these flat C functions.
    // ========================================================================

    void Earthcall_JIT_SetDoubleProperty(Singular* target, uint32_t stringIdVal, double value);
    void Earthcall_JIT_AddDoubleProperty(Singular* target, uint32_t stringIdVal, double value);
    void Earthcall_JIT_ScaleDoubleProperty(Singular* target, uint32_t stringIdVal, double value);
    
    // Returns 1 if true, 0 if false (e.g., for condition checking)
    int Earthcall_JIT_CheckDoubleGt(Singular* target, uint32_t stringIdVal, double value);
}
