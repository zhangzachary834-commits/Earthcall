#ifdef EARTHCALL_ENABLE_LLVM

#include "PropheticJIT.hpp"
#include "JITBridge.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/IR/Verifier.h>
#include <iostream>

namespace Earthcall {
namespace Execution {

// ============================================================================
// LLVM Prophetic JIT Backend
//
// This translates the ActionNode AST directly into LLVM IR, emitting fast
// C-ABI calls for mutation, and eventually unrolling property offset logic.
// Because the PropheticIndex shields us from structural memory changes,
// we do NOT emit bailouts or shape checks when disjointness is proven!
// ============================================================================
class PropheticJITImpl : public PropheticJIT {
public:
    PropheticJITImpl() {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        
        auto jitBuilder = llvm::orc::LLJITBuilder();
        auto expectedJit = jitBuilder.create();
        if (expectedJit) {
            _jit = std::move(*expectedJit);
        } else {
            std::cerr << "Failed to create LLJIT instance\n";
        }
    }

    ~PropheticJITImpl() override = default;

    bool isSupportedOnHost() {
        return _jit != nullptr;
    }

    NativeLawClosure compileUnguarded(
        const class Law& law, 
        const PropheticIndex& index) override 
    {
        if (!_jit) return nullptr;

        auto context = std::make_unique<llvm::LLVMContext>();
        auto module = std::make_unique<llvm::Module>("EarthcallJIT", *context);
        llvm::IRBuilder<> builder(*context);

        // Define the JIT function signature: void(Singular* target)
        llvm::Type* singularPtrTy = llvm::PointerType::getUnqual(builder.getInt8Ty());
        llvm::FunctionType* funcType = llvm::FunctionType::get(builder.getVoidTy(), {singularPtrTy}, false);
        
        std::string funcName = "execute_" + law.getIdentifier();
        llvm::Function* function = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, funcName, module.get());
        
        llvm::BasicBlock* entryBlock = llvm::BasicBlock::Create(*context, "entry", function);
        builder.SetInsertPoint(entryBlock);

        llvm::Value* targetArg = function->getArg(0);

        // Define external C ABI functions
        llvm::FunctionType* abiFuncType = llvm::FunctionType::get(
            builder.getVoidTy(), 
            {singularPtrTy, builder.getInt32Ty(), builder.getDoubleTy()}, 
            false
        );
        llvm::FunctionCallee setPropFunc = module->getOrInsertFunction("Earthcall_JIT_SetDoubleProperty", abiFuncType);
        llvm::FunctionCallee addPropFunc = module->getOrInsertFunction("Earthcall_JIT_AddDoubleProperty", abiFuncType);

        // Recursively walk the AST and emit IR
        emitNode(law.actionModel(), builder, targetArg, setPropFunc, addPropFunc);

        builder.CreateRetVoid();

        // Verify the generated IR
        if (llvm::verifyFunction(*function, &llvm::errs())) {
            std::cerr << "LLVM IR verification failed for " << funcName << "\n";
            return nullptr;
        }

        // Add to JIT
        auto tsd = llvm::orc::ThreadSafeModule(std::move(module), std::move(context));
        if (auto err = _jit->addIRModule(std::move(tsd))) {
            std::cerr << "Failed to add module to JIT\n";
            return nullptr;
        }

        // Look up the compiled symbol
        auto sym = _jit->lookup(funcName);
        if (!sym) {
            std::cerr << "Failed to lookup compiled function\n";
            return nullptr;
        }

        return reinterpret_cast<NativeLawClosure>(sym->getValue());
    }

    void flushExecutableCache() override {
        // In ORC V2, dropping modules is complex. For Earthcall's hot-swap,
        // we typically tear down the JIT instance or drop the JITDylib and 
        // recreate it to flush the cache.
        _jit.reset();
        auto expectedJit = llvm::orc::LLJITBuilder().create();
        if (expectedJit) {
            _jit = std::move(*expectedJit);
        }
    }

private:
    std::unique_ptr<llvm::orc::LLJIT> _jit;

    void emitNode(const ActionNode& node, llvm::IRBuilder<>& builder, llvm::Value* targetArg,
                  llvm::FunctionCallee setFunc, llvm::FunctionCallee addFunc) 
    {
        switch (node.kind) {
            case ActionNode::Kind::Set: {
                if (std::holds_alternative<double>(node.operand)) {
                    double val = std::get<double>(node.operand);
                    uint32_t idVal = node.path.fullId().value;
                    builder.CreateCall(setFunc, {
                        targetArg,
                        builder.getInt32(idVal),
                        llvm::ConstantFP::get(builder.getDoubleTy(), val)
                    });
                }
                break;
            }
            case ActionNode::Kind::Add: {
                if (std::holds_alternative<double>(node.operand)) {
                    double val = std::get<double>(node.operand);
                    uint32_t idVal = node.path.fullId().value;
                    builder.CreateCall(addFunc, {
                        targetArg,
                        builder.getInt32(idVal),
                        llvm::ConstantFP::get(builder.getDoubleTy(), val)
                    });
                }
                break;
            }
            case ActionNode::Kind::Scale:
            case ActionNode::Kind::Lerp:
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
            case ActionNode::Kind::PlayAudio:
            case ActionNode::Kind::AuthorZone:
            case ActionNode::Kind::AddRelation:
            case ActionNode::Kind::Synthesize:
                break;
            case ActionNode::Kind::Sequence:
            case ActionNode::Kind::Parallel: {
                for (const auto& child : node.children) {
                    emitNode(child, builder, targetArg, setFunc, addFunc);
                }
                break;
            }
            default:
                // Unsupported in this phase
                break;
        }
    }
};

// Expose the global singleton or factory
bool PropheticJIT::isSupportedOnHost() {
    return true; // We compiled with LLVM
}

} // namespace Execution
} // namespace Earthcall

#endif // EARTHCALL_ENABLE_LLVM
