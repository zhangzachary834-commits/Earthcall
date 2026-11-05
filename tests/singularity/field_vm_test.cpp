#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

static int failures=0;
static void check(bool ok,const char* s){std::printf("  %s: %s\n",ok?"ok":"FAILED",s);if(!ok)++failures;}
static bool nearf(float a,float b){return std::fabs(a-b)<=2e-6f;}

int main(){
    std::printf("Running native GPU field VM substrate test...\n");
    const glm::vec3 pts[]={{0,0,0},{.2f,-.1f,.3f},{-.7f,.25f,.4f},{1.1f,-.35f,-.2f}};

    struct LeafCase{geom::SdfPrim prim;glm::vec3 dims,off;float p0;};
    const LeafCase cases[]={
        {geom::SdfPrim::Sphere,{.7f},{.15f,-.1f,.05f},0},
        {geom::SdfPrim::Box,{.6f,.4f,.8f},{-.2f,.05f,.1f},0},
        {geom::SdfPrim::RoundBox,{.7f,.5f,.4f},{.05f,.15f,-.1f},.12f},
        {geom::SdfPrim::Ellipsoid,{.8f,.5f,.35f},{-.1f,.1f,.2f},0},
        {geom::SdfPrim::Cylinder,{.45f,.75f,0},{.12f,-.08f,.04f},0},
        {geom::SdfPrim::Cone,{.55f,.8f,0},{-.08f,.03f,-.06f},0},
        {geom::SdfPrim::Torus,{.75f,.18f,0},{.02f,-.12f,.11f},0}
    };
    bool leafShape=true,leafParity=true;
    for(const auto& c:cases){
        auto n=geom::SdfNode::leaf(c.prim,c.dims,c.p0);n.offset=c.off;
        auto vm=sdfwgsl::lowerFieldVm(n);
        leafShape&=vm.ok&&vm.instructions.size()==1&&vm.maxStackDepth==1;
        for(const auto& p:pts){
            auto v=sdfwgsl::evaluateFieldVm(vm,p);
            leafParity&=v&&nearf(*v,geom::evalSdf(n,p));
        }
    }
    check(leafShape,"all admitted primitives lower to one bounded instruction");
    check(leafParity,"primitive VM values match geom::evalSdf");

    const geom::SdfOp ops[]={geom::SdfOp::Morph,geom::SdfOp::Union,geom::SdfOp::Intersect,geom::SdfOp::Subtract,geom::SdfOp::SmoothUnion};
    bool csg=true;
    for(auto op:ops){
        auto a=geom::SdfNode::leaf(geom::SdfPrim::Sphere,{.72f});a.offset={-.18f,0,0};
        auto b=geom::SdfNode::leaf(geom::SdfPrim::Box,{.48f,.62f,.39f});b.offset={.21f,.05f,-.04f};
        auto n=geom::SdfNode::binary(op,a,b,.23f);
        auto vm=sdfwgsl::lowerFieldVm(n);
        csg&=vm.ok&&vm.instructions.size()==3&&vm.maxStackDepth==2;
        for(const auto& p:pts){auto v=sdfwgsl::evaluateFieldVm(vm,p);csg&=v&&nearf(*v,geom::evalSdf(n,p));}
    }
    check(csg,"all CSG operators preserve geom::evalSdf semantics");

    const std::string w=sdfwgsl::fieldVmMathModule();
    check(w.find("fn fieldVmEval(")!=std::string::npos&&w.find("array<f32, 128>")!=std::string::npos&&w.find("instructionCount > 1024u")!=std::string::npos,"WGSL VM carries fixed instruction and stack bounds");
    check(w.find("@group(")==std::string::npos,"WGSL VM module is binding-neutral");

    auto expr=geom::makeImplicit("x*x+y*y+z*z-1");
    auto evm=sdfwgsl::lowerFieldVm(expr);
    check(!evm.ok&&evm.error.find("native OntoMath bytecode")!=std::string::npos,"Expr refuses until native OntoMath bytecode exists");
    auto cvm=sdfwgsl::lowerFieldVm(geom::SdfNode::convex({{1,0,0,.5f},{-1,0,0,.5f}}));
    check(!cvm.ok&&cvm.error.find("plane payload table")!=std::string::npos,"Convex refuses until plane payload exists");

    auto svm=sdfwgsl::lowerFieldVm(geom::SdfNode::leaf(geom::SdfPrim::Sphere,{.5f}));
    svm.instructions[0].meta.x=0xffffffffu;
    check(!sdfwgsl::evaluateFieldVm(svm,{0,0,0}),"unknown opcode refuses instead of fabricating zero");

    auto deep=geom::SdfNode::leaf(geom::SdfPrim::Sphere,{.2f});
    for(uint32_t i=0;i<sdfwgsl::kFieldVmMaxStackDepth;++i)
        deep=geom::SdfNode::binary(geom::SdfOp::Union,geom::SdfNode::leaf(geom::SdfPrim::Sphere,{.2f}),deep);
    auto dvm=sdfwgsl::lowerFieldVm(deep);
    check(!dvm.ok&&dvm.error.find("stack budget exceeded")!=std::string::npos,"128-slot stack overflow refuses");

    auto wide=geom::SdfNode::leaf(geom::SdfPrim::Sphere,{.2f});
    for(uint32_t i=0;i<520;++i)
        wide=geom::SdfNode::binary(geom::SdfOp::Union,wide,geom::SdfNode::leaf(geom::SdfPrim::Sphere,{.25f}));
    auto wvm=sdfwgsl::lowerFieldVm(wide);
    check(!wvm.ok&&wvm.error.find("instruction budget exceeded")!=std::string::npos,"1024-instruction overflow refuses");

    if(failures){std::printf("field_vm_test: %d failure(s)\n",failures);return 1;}
    std::printf("field_vm_test: PASS\n");return 0;
}
