#include "NativeBytecodeVM.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

namespace Earthcall {
namespace Execution {

// Helper to traverse ActionModel and emit bytecode
void emitNode(const ActionNode& node, NativeBytecodeVM::Bytecode& code) {
    switch (node.kind) {
        case ActionNode::Kind::Set: {
            Earthcall::StringId pathId = node.path.fullId();
            uint32_t valIdx = code.constants.size();
            code.constants.push_back(node.operand);
            
            // R[0] = constant
            code.instructions.push_back({NativeBytecodeVM::Opcode::LoadImm, 0, 0, valIdx});
            // prop = R[0]
            code.instructions.push_back({NativeBytecodeVM::Opcode::StoreProp, 0, 0, pathId.value});
            break;
        }
        case ActionNode::Kind::Add: {
            Earthcall::StringId pathId = node.path.fullId();
            uint32_t valIdx = code.constants.size();
            code.constants.push_back(node.operand);
            
            // R[0] = loadProp
            code.instructions.push_back({NativeBytecodeVM::Opcode::LoadProp, 0, 0, pathId.value});
            // R[1] = constant
            code.instructions.push_back({NativeBytecodeVM::Opcode::LoadImm, 1, 0, valIdx});
            // R[0] = R[0] + R[1]
            code.instructions.push_back({NativeBytecodeVM::Opcode::Add, 0, 0, 1});
            // StoreProp
            code.instructions.push_back({NativeBytecodeVM::Opcode::StoreProp, 0, 0, pathId.value});
            break;
        }
        case ActionNode::Kind::Scale: {
            Earthcall::StringId pathId = node.path.fullId();
            uint32_t valIdx = code.constants.size();
            code.constants.push_back(node.operand);
            
            // R[0] = loadProp
            code.instructions.push_back({NativeBytecodeVM::Opcode::LoadProp, 0, 0, pathId.value});
            // R[1] = constant
            code.instructions.push_back({NativeBytecodeVM::Opcode::LoadImm, 1, 0, valIdx});
            // R[0] = R[0] * R[1]
            code.instructions.push_back({NativeBytecodeVM::Opcode::Mul, 0, 0, 1});
            // StoreProp
            code.instructions.push_back({NativeBytecodeVM::Opcode::StoreProp, 0, 0, pathId.value});
            break;
        }
        case ActionNode::Kind::Sequence:
        case ActionNode::Kind::Parallel: {
            for (const auto& child : node.children) {
                emitNode(child, code);
            }
            break;
        }
        default:
            // Unimplemented for now in Phase 1 (Bytecode compiler skeleton)
            // Complex nodes fallback to C++ execution or NoOp
            break;
    }
}

NativeBytecodeVM::Bytecode NativeBytecodeVM::emit(const class Law& law) {
    Bytecode code;
    if (law.actionModel()) {
        emitNode(*law.actionModel(), code);
    }
    code.instructions.push_back({Opcode::Halt, 0, 0, 0});
    return code;
}

bool NativeBytecodeVM::execute(const Bytecode& code, Singular& target) {
    const Instruction* ip = code.instructions.data();
    const Instruction* end = ip + code.instructions.size();

    while (ip < end) {
        switch (ip->op) {
            case Opcode::NoOp:
                break;

            case Opcode::LoadImm:
                if (ip->src2 < code.constants.size()) {
                    _registers[ip->dst] = code.constants[ip->src2];
                }
                break;

            case Opcode::LoadProp: {
                Earthcall::StringId id(ip->src2);
                Property* prop = target.findProperty(id);
                if (prop) {
                    _registers[ip->dst] = prop->value();
                } else {
                    _registers[ip->dst] = PropertyValue(); // Null/Unset
                }
                break;
            }

            case Opcode::StoreProp: {
                Earthcall::StringId id(ip->src2);
                Property* prop = target.findProperty(id);
                if (prop) {
                    prop->setValue(_registers[ip->src1]);
                } else {
                    target.setDynamicProperty(Earthcall::StringInterner::resolve(id), _registers[ip->src1]);
                }
                break;
            }

            case Opcode::Add: {
                double lhs = 0.0, rhs = 0.0;
                propertyValueToNumber(_registers[ip->src1], lhs);
                propertyValueToNumber(_registers[ip->src2], rhs);
                _registers[ip->dst] = PropertyValue(lhs + rhs);
                break;
            }

            case Opcode::Sub: {
                double lhs = 0.0, rhs = 0.0;
                propertyValueToNumber(_registers[ip->src1], lhs);
                propertyValueToNumber(_registers[ip->src2], rhs);
                _registers[ip->dst] = PropertyValue(lhs - rhs);
                break;
            }

            case Opcode::Mul: {
                double lhs = 0.0, rhs = 0.0;
                propertyValueToNumber(_registers[ip->src1], lhs);
                propertyValueToNumber(_registers[ip->src2], rhs);
                _registers[ip->dst] = PropertyValue(lhs * rhs);
                break;
            }

            case Opcode::CmpEq: {
                double lhs = 0.0, rhs = 0.0;
                bool eq = false;
                if (propertyValueToNumber(_registers[ip->src1], lhs) &&
                    propertyValueToNumber(_registers[ip->src2], rhs)) {
                    eq = (lhs == rhs);
                } else {
                    eq = propertyValueUnchanged(_registers[ip->src1], _registers[ip->src2]);
                }
                _registers[ip->dst] = PropertyValue(eq ? 1.0 : 0.0);
                break;
            }

            case Opcode::CmpGt: {
                double lhs = 0.0, rhs = 0.0;
                propertyValueToNumber(_registers[ip->src1], lhs);
                propertyValueToNumber(_registers[ip->src2], rhs);
                _registers[ip->dst] = PropertyValue(lhs > rhs ? 1.0 : 0.0);
                break;
            }

            case Opcode::BranchFalse: {
                bool isTrue = false;
                if (std::holds_alternative<bool>(_registers[ip->src1])) {
                    isTrue = std::get<bool>(_registers[ip->src1]);
                } else {
                    double val = 0.0;
                    if (propertyValueToNumber(_registers[ip->src1], val)) {
                        isTrue = (val != 0.0);
                    }
                }
                if (!isTrue) {
                    if (ip->src2 < code.instructions.size()) {
                        ip = code.instructions.data() + ip->src2;
                        continue;
                    }
                }
                break;
            }

            case Opcode::Jump: {
                if (ip->src2 < code.instructions.size()) {
                    ip = code.instructions.data() + ip->src2;
                    continue;
                }
                break;
            }

            case Opcode::Halt:
                return true;
        }
        ++ip;
    }

    return true;
}

} // namespace Execution
} // namespace Earthcall
