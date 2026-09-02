#pragma once

#include "ConstructedBeing/Singular/Singular.hpp"
#include <vector>
#include <cstdint>

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
// Because it does not rely on unguarded machine code, it handles dynamic
// shape changes organically, albeit at the standard VM overhead cost.
// ============================================================================
class NativeBytecodeVM {
public:
    // Opcodes defining the Earthcall Bytecode Instruction Set
    enum class Opcode : uint8_t {
        NoOp = 0,
        ReadProperty,    // Reads a property (checks dynamic shape)
        WriteProperty,   // Writes a property (checks dynamic shape)
        CompareEq,
        CompareGt,
        // ... (math, logic, and state transitions)
        Halt = 255
    };

    struct Instruction {
        Opcode op;
        uint32_t operand1; // StringId or register
        uint32_t operand2;
    };

    using Bytecode = std::vector<Instruction>;

    virtual ~NativeBytecodeVM() = default;

    // Compiles a Law AST into execution-ready bytecode.
    // Extremely fast (microseconds), zero W^X memory requirements.
    virtual Bytecode emit(const class Law& law) = 0;

    // Executes the bytecode against a target Singular.
    virtual bool execute(const Bytecode& code, Singular& target) = 0;
};

} // namespace Execution
} // namespace Earthcall
