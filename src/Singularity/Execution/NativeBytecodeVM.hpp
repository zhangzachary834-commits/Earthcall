#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValue.hpp"
#include "Singularity/Core/StringId.hpp"
#include <vector>
#include <cstdint>
#include <array>

namespace Earthcall {
namespace Execution {

// ============================================================================
// NativeBytecodeVM
//
// The authoritative, fully portable execution form of Earthcall Laws (Phase 2).
// When the JIT is unavailable (e.g. W^X memory restrictions on consoles),
// or when a structural violation invalidates the JIT cache mid-frame, this
// VM takes over execution seamlessly.
//
// Designed as a Register-Based virtual machine operating on PropertyValues.
// ============================================================================
class NativeBytecodeVM {
public:
    // Earthcall Bytecode Instruction Set
    enum class Opcode : uint8_t {
        NoOp = 0,
        
        // Memory & Properties
        LoadImm,       // R[dst] = constants[src2]
        LoadProp,      // R[dst] = target->getProperty(StringId in src2)
        StoreProp,     // target->setProperty(StringId in src2, R[src1])
        
        // Math Operations
        Add,           // R[dst] = R[src1] + R[src2_reg]
        Lerp,          // R[dst] = R[dst] + (R[src1] - R[dst]) * R[src2]
        Drive,
        Spawn,
        Map,
        Flow,
        Publish,
        Create,
        AddProperty,
        RemoveProperty,
        AddElement,
        RemoveElement,
        Destroy,
        PlayAudio,
        AuthorZone,
        AddRelation,
        Synthesize,
        Sub,           // R[dst] = R[src1] - R[src2_reg]
        Mul,           // R[dst] = R[src1] * R[src2_reg]
        
        // Comparisons
        CmpEq,         // R[dst] = (R[src1] == R[src2_reg])
        CmpGt,         // R[dst] = (R[src1] > R[src2_reg])
        
        // Control Flow
        BranchFalse,   // if (!R[src1]) PC = src2_offset
        Jump,          // PC = src2_offset
        
        Halt = 255
    };

    struct Instruction {
        Opcode op;
        uint8_t dst;   
        uint8_t src1;
        uint32_t src2; // Can be immediate index, StringId.value, or jump offset
    };

    struct Bytecode {
        std::vector<Instruction> instructions;
        std::vector<PropertyValue> constants;
    };

    NativeBytecodeVM() = default;
    ~NativeBytecodeVM() = default;

    // Compiles a Law AST into execution-ready bytecode.
    Bytecode emit(const class Law& law);

    // Executes the bytecode against a target Singular.
    // Returns true if the law applied successfully.
    bool execute(const Bytecode& code, Singular& target);

private:
    // VM Thread Context
    std::array<PropertyValue, 256> _registers;
};

} // namespace Execution
} // namespace Earthcall
