#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"

#include <cstdio>
#include <cassert>
#include <string>

int main() {
    std::printf("=== Gate B: AST Derivative Proof & Parameter Offset Test ===\n");

    auto num = [](double c) {
        auto n = std::make_unique<OntoMath::MathNode>();
        n->op = OntoMath::MathNode::Op::ScalarLeaf;
        n->scalarForm.terms.push_back(OntoMath::Term(c));
        return n;
    };
    auto varLeaf = [](const std::string& v) {
        auto n = std::make_unique<OntoMath::MathNode>();
        n->op = OntoMath::MathNode::Op::ValueLeaf;
        n->variableName = v;
        return n;
    };
    auto vecConstruct = [&](double x, double y, double z) {
        auto n = std::make_unique<OntoMath::MathNode>();
        n->op = OntoMath::MathNode::Op::VectorConstruct;
        n->children.push_back(num(x));
        n->children.push_back(num(y));
        n->children.push_back(num(z));
        return n;
    };

    auto buildTerrainMath = [&]() {
        auto pPlusOffset = std::make_unique<OntoMath::MathNode>();
        pPlusOffset->op = OntoMath::MathNode::Op::Add;
        pPlusOffset->children.push_back(varLeaf("p"));
        pPlusOffset->children.push_back(vecConstruct(100.0, 0.0, 100.0));

        auto q = std::make_unique<OntoMath::MathNode>();
        q->op = OntoMath::MathNode::Op::Scale;
        q->children.push_back(num(0.008));
        q->children.push_back(std::move(pPlusOffset));

        auto noiseNode = std::make_unique<OntoMath::MathNode>();
        noiseNode->op = OntoMath::MathNode::Op::Noise;
        noiseNode->children.push_back(std::move(q));

        auto scaledNoise = std::make_unique<OntoMath::MathNode>();
        scaledNoise->op = OntoMath::MathNode::Op::Scale;
        scaledNoise->children.push_back(num(40.0));
        scaledNoise->children.push_back(std::move(noiseNode));

        auto rootMath = std::make_unique<OntoMath::MathNode>();
        rootMath->op = OntoMath::MathNode::Op::Sub;
        rootMath->children.push_back(varLeaf("y"));
        rootMath->children.push_back(std::move(scaledNoise));
        return rootMath;
    };

    // Test 1: Zero offset
    {
        geom::SdfNode root;
        root.op = geom::SdfOp::Leaf;
        root.prim = geom::SdfPrim::Expr;
        root.mathNode = buildTerrainMath();
        root.offset = glm::vec3(0.0f);

        sdfwgsl::Program prog = sdfwgsl::compile(root);
        assert(prog.ok);
        assert(prog.params.size() == 8);
        assert(prog.params[0] == 0.0f && prog.params[1] == 0.0f && prog.params[2] == 0.0f);
        assert(prog.params[3] == 40.0f);
        assert(prog.params[4] == 0.008f);
        assert(prog.params[5] == 100.0f && prog.params[6] == 0.0f && prog.params[7] == 100.0f);
        std::printf("[Test 1] Zero offset parameter slots verified: [0, 0, 0, 40, 0.008, 100, 0, 100]\n");
    }

    // Test 2: Non-zero offset (Finding 1 verification)
    {
        geom::SdfNode root;
        root.op = geom::SdfOp::Leaf;
        root.prim = geom::SdfPrim::Expr;
        root.mathNode = buildTerrainMath();
        root.offset = glm::vec3(15.0f, -20.0f, 10.0f);

        sdfwgsl::Program prog = sdfwgsl::compile(root);
        assert(prog.ok);
        assert(prog.params.size() == 8);
        assert(prog.params[0] == 15.0f && prog.params[1] == -20.0f && prog.params[2] == 10.0f);
        assert(prog.params[3] == 40.0f);
        assert(prog.params[4] == 0.008f);
        assert(prog.params[5] == 100.0f && prog.params[6] == 0.0f && prog.params[7] == 100.0f);
        std::printf("[Test 2] Non-zero offset parameter slots verified: [15, -20, 10, 40, 0.008, 100, 0, 100]\n");
    }

    // Test 3: Unsupported implicit expression (Expr(iso)) preserves 4-evaluation fallback
    {
        geom::SdfNode root;
        root.op = geom::SdfOp::Leaf;
        root.prim = geom::SdfPrim::Expr;
        root.rpn = geom::compileExpr("x*x + y*y + z*z - 0.3");
        root.offset = glm::vec3(0.0f);

        sdfwgsl::Program prog = sdfwgsl::compile(root);
        assert(prog.ok);
        // sdfEvalGrad is NOT emitted for RPN / non-noise expressions
        assert(prog.wgsl.find("fn sdfSampleStep") != std::string::npos);
        std::printf("[Test 3] Unsupported implicit AST falls back to 4-evaluation forward-difference step\n");
    }

    std::printf("=== All Gate B Tests Passed Successfully! ===\n");
    return 0;
}
