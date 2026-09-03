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
        case ActionNode::Kind::Lerp: {
            Earthcall::StringId pathId = node.path.fullId();
            uint32_t valIdx = code.constants.size();
            code.constants.push_back(node.operand);
            uint32_t factorIdx = code.constants.size();
            code.constants.push_back(PropertyValue(node.factor));

            // R[0] = loadProp
            code.instructions.push_back({NativeBytecodeVM::Opcode::LoadProp, 0, 0, pathId.value});
            // R[1] = constant (target)
            code.instructions.push_back({NativeBytecodeVM::Opcode::LoadImm, 1, 0, valIdx});
            // R[2] = constant (factor)
            code.instructions.push_back({NativeBytecodeVM::Opcode::LoadImm, 2, 0, factorIdx});
            // R[0] = Lerp(R[0], R[1], R[2])
            code.instructions.push_back({NativeBytecodeVM::Opcode::Lerp, 0, 1, 2});
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

        case ActionNode::Kind::Drive:
        case ActionNode::Kind::Spawn:
        case ActionNode::Kind::Map:
        case ActionNode::Kind::Flow:
        case ActionNode::Kind::Publish:
        case ActionNode::Kind::Create:
        case ActionNode::Kind::AddProperty:
        case ActionNode::Kind::RemoveProperty:
        case ActionNode::Kind::AddElement:
        case ActionNode::Kind::RemoveElement:
        case ActionNode::Kind::Destroy:
        case ActionNode::Kind::Synthesize:
        case ActionNode::Kind::PlayAudio:
        case ActionNode::Kind::AuthorZone:
        case ActionNode::Kind::AddRelation:
            // Unimplemented for now in Phase 1 (Bytecode compiler skeleton)
            code.instructions.push_back({NativeBytecodeVM::Opcode::NoOp, 0, 0, 0});
            break;

        default:
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
                if (std::holds_alternative<double>(_registers[ip->src1])) {
                    lhs = std::get<double>(_registers[ip->src1]);
                }
                if (std::holds_alternative<double>(_registers[ip->src2])) {
                    rhs = std::get<double>(_registers[ip->src2]);
                }
                _registers[ip->dst] = PropertyValue(lhs + rhs);
                break;
            }

            case Opcode::Mul: {
                double lhs = 0.0, rhs = 0.0;
                if (std::holds_alternative<double>(_registers[ip->src1])) {
                    lhs = std::get<double>(_registers[ip->src1]);
                }
                if (std::holds_alternative<double>(_registers[ip->src2])) {
                    rhs = std::get<double>(_registers[ip->src2]);
                }
                _registers[ip->dst] = PropertyValue(lhs * rhs);
                break;
            }

            case Opcode::Lerp: {
                double tgt = 0.0, current = 0.0, factor = 0.0;
                if (std::holds_alternative<double>(_registers[ip->dst])) {
                    current = std::get<double>(_registers[ip->dst]);
                }
                if (std::holds_alternative<double>(_registers[ip->src1])) {
                    tgt = std::get<double>(_registers[ip->src1]);
                }
                if (std::holds_alternative<double>(_registers[ip->src2])) {
                    factor = std::get<double>(_registers[ip->src2]);
                }
                _registers[ip->dst] = PropertyValue(current + (tgt - current) * factor);
                break;
            }

            case Opcode::Drive:
            case Opcode::Spawn:
            case Opcode::Map:
            case Opcode::Flow:
            case Opcode::Publish:
            case Opcode::Create:
            case Opcode::AddProperty:
            case Opcode::RemoveProperty:
            case Opcode::AddElement:
            case Opcode::RemoveElement:
            case Opcode::Destroy:
            case Opcode::PlayAudio:
            case Opcode::AuthorZone:
            case Opcode::AddRelation:
            case Opcode::Synthesize:
            case Opcode::Sub:
            case Opcode::CmpEq:
            case Opcode::CmpGt:
            case Opcode::BranchFalse:
            case Opcode::Jump:
                break;

            case Opcode::Halt:
                return true;
        }
        ++ip;
    }

    return true;
}

} // namespace Execution
} // namespace Earthcall
