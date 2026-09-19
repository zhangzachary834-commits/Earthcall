#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/OntoMath/ScalarForm.hpp"
#include <cstdio>
#include <string>

int main() {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    
    // Create a basic SDF tree
    auto sphere = geom::SdfNode::leaf(geom::SdfPrim::Sphere, glm::vec3(1.0f));

    // We want a vec3 color, so we build a VectorConstruct MathNode with three ValueLeafs
    auto v_r = std::make_unique<OntoMath::MathNode>();
    v_r->op = OntoMath::MathNode::Op::ScalarLeaf;
    v_r->scalarForm = OntoMath::ScalarForm::constant(1.0f);

    auto v_g = std::make_unique<OntoMath::MathNode>();
    v_g->op = OntoMath::MathNode::Op::ScalarLeaf;
    v_g->scalarForm = OntoMath::ScalarForm::constant(0.0f);

    auto v_b = std::make_unique<OntoMath::MathNode>();
    v_b->op = OntoMath::MathNode::Op::ScalarLeaf;
    v_b->scalarForm = OntoMath::ScalarForm::constant(0.5f);

    auto vectorNode = std::make_shared<OntoMath::MathNode>();
    vectorNode->op = OntoMath::MathNode::Op::VectorConstruct;
    vectorNode->children.push_back(std::move(v_r));
    vectorNode->children.push_back(std::move(v_g));
    vectorNode->children.push_back(std::move(v_b));

    OntoMath::Piecewise colorExpr;
    colorExpr.pieces.push_back({
        false, false, 0.0, 0.0, true, true, // no bounds
        vectorNode, nullptr, nullptr, nullptr, nullptr, nullptr
    });

    sdfwgsl::Program prog = sdfwgsl::compile(sphere, nullptr, &colorExpr);

    if (!prog.ok) {
        std::printf("FAILED: compilation refused\n");
        return 1;
    }

    bool hasCustomColor = prog.wgsl.find("vec3<f32>((P.v[instances[g_instIdx].paramOffset") != std::string::npos;
    bool hasBaseColorFallback = prog.wgsl.find("return instances[g_instIdx].baseColor.xyz;") != std::string::npos;

    if (!hasCustomColor || hasBaseColorFallback) {
        std::printf("FAILED: WGSL output did not correctly integrate the colorExpr!\n");
        return 1;
    }

    std::printf("PASS: colorExpr successfully compiled into WGSL!\n");
    return 0;
}
