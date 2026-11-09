#include "ConstructedBeing/Singular/Object/Geometry/Sdf.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <memory>
#include <string>
#include <vector>

static int failures=0;
static void check(bool ok,const char* s){std::printf("  %s: %s\n",ok?"ok":"FAILED",s);if(!ok)++failures;}
static bool nearf(float a,float b){return std::fabs(a-b)<=2e-6f;}

static std::unique_ptr<OntoMath::MathNode> number(double v){
    auto n=std::make_unique<OntoMath::MathNode>();
    n->op=OntoMath::MathNode::Op::ScalarLeaf;
    n->scalarForm=OntoMath::ScalarForm::constant(v);
    return n;
}
static std::unique_ptr<OntoMath::MathNode> value(const std::string& name){
    auto n=std::make_unique<OntoMath::MathNode>();
    n->op=OntoMath::MathNode::Op::ValueLeaf;
    n->variableName=name;
    return n;
}
static std::unique_ptr<OntoMath::MathNode> vec3(
    std::unique_ptr<OntoMath::MathNode> x,
    std::unique_ptr<OntoMath::MathNode> y,
    std::unique_ptr<OntoMath::MathNode> z){
    auto n=std::make_unique<OntoMath::MathNode>();
    n->op=OntoMath::MathNode::Op::VectorConstruct;
    n->children.push_back(std::move(x));
    n->children.push_back(std::move(y));
    n->children.push_back(std::move(z));
    return n;
}
static std::unique_ptr<OntoMath::MathNode> binary(
    OntoMath::MathNode::Op op,
    std::unique_ptr<OntoMath::MathNode> a,
    std::unique_ptr<OntoMath::MathNode> b){
    auto n=std::make_unique<OntoMath::MathNode>();
    n->op=op;
    n->children.push_back(std::move(a));
    n->children.push_back(std::move(b));
    return n;
}
static std::shared_ptr<OntoMath::MathNode> terrainExpr(){
    auto shifted=binary(
        OntoMath::MathNode::Op::Add,
        value(OntoMath::kAmbientPointVar),
        vec3(number(100.0),number(0.0),number(100.0)));
    auto scaled=binary(
        OntoMath::MathNode::Op::Scale,number(0.008),std::move(shifted));
    auto noise=std::make_unique<OntoMath::MathNode>();
    noise->op=OntoMath::MathNode::Op::Noise;
    noise->children.push_back(std::move(scaled));
    auto shaped=binary(
        OntoMath::MathNode::Op::Scale,number(40.0),std::move(noise));
    auto root=binary(
        OntoMath::MathNode::Op::Sub,value("y"),std::move(shaped));
    return std::shared_ptr<OntoMath::MathNode>(root.release());
}
static std::shared_ptr<OntoMath::MathNode> gyroidExpr(double k){
    using K=OntoMath::TransFactor::Kind;
    const auto leaf=[&](OntoMath::TransFactor a,OntoMath::TransFactor b){
        auto n=std::make_unique<OntoMath::MathNode>();
        n->op=OntoMath::MathNode::Op::ScalarLeaf;
        OntoMath::Term t(1.0);
        t.trans={std::move(a),std::move(b)};
        std::sort(t.trans.begin(),t.trans.end());
        n->scalarForm.terms.push_back(std::move(t));
        return n;
    };
    auto xy=leaf({K::Sin,"x",k,0.0},{K::Cos,"y",k,0.0});
    auto yz=leaf({K::Sin,"y",k,0.0},{K::Cos,"z",k,0.0});
    auto zx=leaf({K::Sin,"z",k,0.0},{K::Cos,"x",k,0.0});
    auto first=binary(OntoMath::MathNode::Op::Add,std::move(xy),std::move(yz));
    auto root=binary(OntoMath::MathNode::Op::Add,std::move(first),std::move(zx));
    return std::shared_ptr<OntoMath::MathNode>(root.release());
}

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
        leafShape&=vm.ok&&vm.instructions.size()==1&&vm.maxStackDepth==1&&
                   !vm.needsGradientStep;
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
    const std::string bound=sdfwgsl::fieldVmBoundMathModule(0u,5u);
    check(bound.find("@group(0) @binding(5) var<storage, read> fieldVmInstructions")!=std::string::npos,
          "production VM module binds the instruction stream at the requested storage slot");

    auto productionTorus=geom::SdfNode::leaf(geom::SdfPrim::Torus,{.75f,.18f,0});
    auto productionCsg=geom::SdfNode::binary(
        geom::SdfOp::Union,
        geom::SdfNode::leaf(geom::SdfPrim::Sphere,{.5f}),
        productionTorus);
    const auto production=sdfwgsl::compile(productionCsg);
    check(production.ok&&
          production.wgsl.find("@group(0) @binding(5) var<storage, read> fieldVmInstructions")!=std::string::npos&&
          production.wgsl.find("struct SceneFieldVmBlockerData")!=std::string::npos&&
          production.wgsl.find("fn sceneFieldVmVisibility(")!=std::string::npos&&
          production.wgsl.find("fieldVmEval(programBase, instructionCount")!=std::string::npos,
          "production SDF WGSL executes foreign field programs through the bound VM");
    check(production.wgsl.find("sceneImplicitBlockers")==std::string::npos&&
          production.wgsl.find("sceneFieldVmBlockers")!=std::string::npos,
          "production SDF visibility no longer depends on the sphere-box foreign packet");

    // Expr is admitted only through authored OntoMath, never the legacy RPN.
    auto expr=geom::makeImplicit("x*x+y*y+z*z-1");
    auto evm=sdfwgsl::lowerFieldVm(expr);
    bool exprParity=evm.ok&&evm.instructions.size()>1&&evm.needsGradientStep;
    for(const auto& p:pts){
        const auto v=sdfwgsl::evaluateFieldVm(evm,p);
        exprParity&=v&&nearf(*v,geom::evalSdf(expr,p));
    }
    check(exprParity,"algebraic Expr lowers from OntoMath and matches geom::evalSdf");

    auto translatedExpr=geom::makeImplicit("x*x+y*y+z*z-1");
    translatedExpr.offset={.3f,-.2f,.1f};
    const auto translatedVm=sdfwgsl::lowerFieldVm(translatedExpr);
    const glm::vec3 translatedProbe(.3f,-.2f,1.1f);
    const auto translatedValue=sdfwgsl::evaluateFieldVm(translatedVm,translatedProbe);
    check(translatedVm.ok&&translatedValue&&
          nearf(*translatedValue,geom::evalSdf(translatedExpr,translatedProbe))&&
          std::fabs(*translatedValue)<=3e-6f,
          "Expr coordinate bytecode preserves leaf-local offset semantics");

    geom::SdfNode terrain=geom::makeImplicit(terrainExpr());
    const auto terrainVm=sdfwgsl::lowerFieldVm(terrain);
    bool terrainParity=terrainVm.ok;
    const glm::vec3 terrainPts[]={{0,0,0},{12.5f,4.0f,-8.0f},{-17.0f,-2.0f,31.0f}};
    for(const auto& p:terrainPts){
        const auto v=sdfwgsl::evaluateFieldVm(terrainVm,p);
        terrainParity&=v&&std::fabs(*v-geom::evalSdf(terrain,p))<=2e-5f;
    }
    check(terrainParity,"vector arithmetic and exact Perlin Expr lower through native OntoMath bytecode");

    geom::SdfNode gyroid=geom::makeImplicit(gyroidExpr(1.37));
    const auto gyroidVm=sdfwgsl::lowerFieldVm(gyroid);
    bool gyroidParity=gyroidVm.ok;
    const glm::vec3 gyroidPts[]={{.2f,.4f,-.3f},{1.1f,-.7f,.6f},{-.8f,.25f,1.4f}};
    for(const auto& p:gyroidPts){
        const auto v=sdfwgsl::evaluateFieldVm(gyroidVm,p);
        gyroidParity&=v&&std::fabs(*v-geom::evalSdf(gyroid,p))<=2e-5f;
    }
    check(gyroidParity,"ScalarForm sin/cos products retain authored gyroid semantics in the VM");

    // Canonical OntoMath primitive expansions are also legal Expr trees. This
    // exercises vector Abs/Sub/Intersection widening, Length, Component, Sqrt
    // and Pow rather than relying on the VM's primitive shorthand opcodes.
    const auto canonicalExprParity=[&](std::unique_ptr<OntoMath::MathNode> math){
        if(!math)return false;
        auto root=std::shared_ptr<OntoMath::MathNode>(math.release());
        auto n=geom::makeImplicit(root);
        const auto vm=sdfwgsl::lowerFieldVm(n);
        if(!vm.ok)return false;
        const glm::vec3 probes[]={{0,0,0},{.3f,.2f,-.1f},{.8f,.7f,.6f},{-.6f,1.2f,.25f}};
        for(const auto& p:probes){
            const auto v=sdfwgsl::evaluateFieldVm(vm,p);
            if(!v||std::fabs(*v-geom::evalSdf(n,p))>2e-5f)return false;
        }
        return true;
    };
    check(canonicalExprParity(OntoMath::MathNode::box(glm::vec3(.5f,.4f,.7f),"p"))&&
          canonicalExprParity(OntoMath::MathNode::cylinder(.5, .8, "p"))&&
          canonicalExprParity(OntoMath::MathNode::torus(.6, .15, "p")),
          "canonical box/cylinder/torus OntoMath expansions lower as Expr without primitive aliases");

    geom::SdfNode rpnOnly;
    rpnOnly.op=geom::SdfOp::Leaf;
    rpnOnly.prim=geom::SdfPrim::Expr;
    rpnOnly.expr="x*x-1";
    rpnOnly.rpn=geom::compileExpr(rpnOnly.expr);
    const auto rpnOnlyVm=sdfwgsl::lowerFieldVm(rpnOnly);
    check(!rpnOnlyVm.ok&&rpnOnlyVm.error.find("legacy SdfToken RPN")!=std::string::npos,
          "legacy RPN without OntoMath authority refuses instead of becoming a second VM language");

    auto bound=std::make_shared<OntoMath::MathNode>();
    bound->op=OntoMath::MathNode::Op::Bind;
    bound->propertyPath="radius";
    const auto boundVm=sdfwgsl::lowerFieldVm(geom::makeImplicit(bound));
    check(!boundVm.ok,"Expr requiring a subject PropertyPath refuses in scene field VM");

    const auto pieceConstant=[](double v){
        auto n=std::make_shared<OntoMath::MathNode>();
        n->op=OntoMath::MathNode::Op::ScalarLeaf;
        n->scalarForm=OntoMath::ScalarForm::constant(v);
        return n;
    };

    // Ordered interval Piecewise: overlapping pieces prove FIRST applicable
    // wins, open/closed endpoints remain distinct, and leaf offset is applied
    // before testing the authored input variable.
    auto pw=std::make_shared<OntoMath::Piecewise>();
    pw->inputVariable="x";
    OntoMath::Piecewise::Piece pa;
    pa.mathNode=pieceConstant(-1.0);
    pa.hasLo=true;pa.lo=-1.0;pa.includeLo=true;
    pa.hasHi=true;pa.hi=.5;pa.includeHi=false;
    OntoMath::Piecewise::Piece pb;
    pb.mathNode=pieceConstant(1.0);
    pb.hasLo=true;pb.lo=0.0;pb.includeLo=false;
    pb.hasHi=true;pb.hi=1.0;pb.includeHi=true;
    OntoMath::Piecewise::Piece pc;
    pc.mathNode=pieceConstant(2.0); // permanent catch-all
    OntoMath::Piecewise::Piece dead;
    dead.mathNode=pieceConstant(99.0);
    dead.whereLEZero=pieceConstant(-1.0); // unreachable after catch-all
    pw->pieces.push_back(std::move(pa));
    pw->pieces.push_back(std::move(pb));
    pw->pieces.push_back(std::move(pc));
    pw->pieces.push_back(std::move(dead));
    auto pieceNode=geom::makeImplicit(pw);
    pieceNode.offset={.3f,0,0};

    const auto pieceVm=sdfwgsl::lowerFieldVm(pieceNode);
    bool pieceParity=pieceVm.ok&&pieceVm.needsGradientStep;
    const float localX[]={-1.1f,-1.0f,-.25f,.25f,.5f,1.0f,1.1f};
    for(float x:localX){
        const glm::vec3 p(x+.3f,0,0);
        const auto v=sdfwgsl::evaluateFieldVm(pieceVm,p);
        pieceParity&=v&&std::fabs(*v-geom::evalSdf(pieceNode,p))<=2e-6f;
    }
    const auto overlap=sdfwgsl::evaluateFieldVm(pieceVm,{.55f,0,0}); // local x=.25
    const auto openBoundary=sdfwgsl::evaluateFieldVm(pieceVm,{.8f,0,0}); // local x=.5
    check(pieceParity&&overlap&&openBoundary&&
          nearf(*overlap,-1.0f)&&nearf(*openBoundary,1.0f),
          "interval Piecewise VM preserves offset, endpoint openness, overlap order and catch-all");

    bool hasBranch=false,hasJump=false;
    for(const auto& inst:pieceVm.instructions){
        const auto op=static_cast<sdfwgsl::FieldVmOpcode>(inst.meta.x);
        hasBranch|=op==sdfwgsl::FieldVmOpcode::BranchInterval;
        hasJump|=op==sdfwgsl::FieldVmOpcode::Jump;
    }
    check(hasBranch&&hasJump&&
          w.find("op == 66u")!=std::string::npos&&
          w.find("op == 67u")!=std::string::npos,
          "Piecewise lowers to bounded forward control in CPU and WGSL VM");

    // Piecewise may be one child of larger CSG. Its alternatives must preserve
    // any already-live sibling operands on the VM stack.
    auto nestedPiecewise=geom::SdfNode::binary(
        geom::SdfOp::Union,
        geom::SdfNode::leaf(geom::SdfPrim::Sphere,{.35f}),
        pieceNode);
    const auto nestedPiecewiseVm=sdfwgsl::lowerFieldVm(nestedPiecewise);
    bool nestedPiecewiseParity=nestedPiecewiseVm.ok;
    const glm::vec3 nestedPts[]={{0,0,0},{.55f,0,0},{1.3f,0,0},{-.8f,.2f,0}};
    for(const auto& p:nestedPts){
        const auto v=sdfwgsl::evaluateFieldVm(nestedPiecewiseVm,p);
        nestedPiecewiseParity&=v&&
            std::fabs(*v-geom::evalSdf(nestedPiecewise,p))<=2e-6f;
    }
    check(nestedPiecewiseParity,
          "Piecewise VM preserves enclosing CSG stack operands across alternative branches");

    // Control records are forward-only derived IR. Corrupt targets must fail
    // closed rather than looping or executing an unintended authored branch.
    auto malformedBranch=pieceVm;
    for(std::size_t ip=0;ip<malformedBranch.instructions.size();++ip){
        auto& inst=malformedBranch.instructions[ip];
        if(static_cast<sdfwgsl::FieldVmOpcode>(inst.meta.x)==
           sdfwgsl::FieldVmOpcode::BranchInterval){
            inst.meta.w=static_cast<uint32_t>(ip); // backward/self target
            break;
        }
    }
    check(!sdfwgsl::evaluateFieldVm(malformedBranch,{0,0,0}),
          "malformed Piecewise branch target refuses instead of looping");

    // No catch-all follows current geom::evalSdf rendering compatibility:
    // Piecewise undefined falls through empty legacy RPN -> 1e9.
    auto boundedPw=std::make_shared<OntoMath::Piecewise>();
    boundedPw->inputVariable="x";
    OntoMath::Piecewise::Piece bounded;
    bounded.mathNode=pieceConstant(-.25);
    bounded.hasLo=true;bounded.lo=-.5;
    bounded.hasHi=true;bounded.hi=.5;
    boundedPw->pieces.push_back(std::move(bounded));
    auto boundedNode=geom::makeImplicit(boundedPw);
    const auto boundedVm=sdfwgsl::lowerFieldVm(boundedNode);
    const auto outside=sdfwgsl::evaluateFieldVm(boundedVm,{2,0,0});
    check(boundedVm.ok&&outside&&*outside>1e8f&&
          std::fabs(*outside-geom::evalSdf(boundedNode,{2,0,0}))<1.0f,
          "Piecewise no-match preserves current SDF empty-space fallback");

    // PURE whereLEZero now has predicate-local undefinedness. CPU semantics
    // test the pure guard first and return immediately when it produces a value,
    // so interval bounds on the same piece are intentionally ignored.
    auto wherePw=std::make_shared<OntoMath::Piecewise>();
    OntoMath::Piecewise::Piece wherePiece;
    wherePiece.mathNode=pieceConstant(-1.0);
    wherePiece.whereLEZero=
        std::shared_ptr<OntoMath::MathNode>(value("x").release());
    wherePiece.hasLo=true;wherePiece.lo=100.0;   // semantically ignored
    wherePiece.hasHi=true;wherePiece.hi=101.0;  // semantically ignored
    OntoMath::Piecewise::Piece whereFallback;
    whereFallback.mathNode=pieceConstant(1.0);
    wherePw->pieces.push_back(std::move(wherePiece));
    wherePw->pieces.push_back(std::move(whereFallback));
    auto whereNode=geom::makeImplicit(wherePw);
    whereNode.offset={.25f,0,0};
    const auto whereVm=sdfwgsl::lowerFieldVm(whereNode);

    bool whereParity=whereVm.ok;
    const glm::vec3 wherePts[]={{-.25f,0,0},{.05f,0,0},{.25f,0,0},{.45f,0,0}};
    for(const auto& p:wherePts){
        const auto v=sdfwgsl::evaluateFieldVm(whereVm,p);
        whereParity&=v&&std::fabs(*v-geom::evalSdf(whereNode,p))<=2e-6f;
    }
    const auto whereNegative=sdfwgsl::evaluateFieldVm(whereVm,{.05f,0,0});
    const auto wherePositive=sdfwgsl::evaluateFieldVm(whereVm,{.45f,0,0});
    check(whereParity&&whereNegative&&wherePositive&&
          nearf(*whereNegative,-1.0f)&&nearf(*wherePositive,1.0f),
          "pure where guard preserves local offset and supersedes impossible interval bounds");

    // ln(x) is the decisive undefinedness witness:
    //   x<=0   -> predicate undefined -> later piece
    //   0<x<=1 -> ln(x)<=0 -> guarded piece
    //   x>1    -> predicate defined false -> later piece
    auto lnPredicate=std::make_shared<OntoMath::MathNode>();
    lnPredicate->op=OntoMath::MathNode::Op::ScalarLeaf;
    lnPredicate->scalarForm=OntoMath::ScalarForm::transcendental(
        OntoMath::TransFactor::Kind::Ln,"x");
    auto lnPw=std::make_shared<OntoMath::Piecewise>();
    OntoMath::Piecewise::Piece lnPiece;
    lnPiece.mathNode=pieceConstant(-2.0);
    lnPiece.whereLEZero=lnPredicate;
    OntoMath::Piecewise::Piece lnFallback;
    lnFallback.mathNode=pieceConstant(2.0);
    lnPw->pieces.push_back(std::move(lnPiece));
    lnPw->pieces.push_back(std::move(lnFallback));
    auto lnNode=geom::makeImplicit(lnPw);
    const auto lnVm=sdfwgsl::lowerFieldVm(lnNode);
    bool lnParity=lnVm.ok;
    const float lnXs[]={-1.0f,0.0f,.5f,1.0f,2.0f};
    for(float x:lnXs){
        const glm::vec3 p(x,0,0);
        const auto v=sdfwgsl::evaluateFieldVm(lnVm,p);
        lnParity&=v&&std::fabs(*v-geom::evalSdf(lnNode,p))<=2e-6f;
    }
    const auto lnUndefined=sdfwgsl::evaluateFieldVm(lnVm,{-1,0,0});
    const auto lnApplies=sdfwgsl::evaluateFieldVm(lnVm,{.5f,0,0});
    const auto lnFalse=sdfwgsl::evaluateFieldVm(lnVm,{2,0,0});
    check(lnParity&&lnUndefined&&lnApplies&&lnFalse&&
          nearf(*lnUndefined,2.0f)&&nearf(*lnApplies,-2.0f)&&
          nearf(*lnFalse,2.0f),
          "pure where predicate undefinedness skips only that piece and continues");

    bool hasWhereBranch=false;
    for(const auto& inst:lnVm.instructions){
        hasWhereBranch|=
            static_cast<sdfwgsl::FieldVmOpcode>(inst.meta.x)==
            sdfwgsl::FieldVmOpcode::BranchWhereLEZero;
    }
    check(hasWhereBranch&&
          w.find("op == 68u")!=std::string::npos&&
          w.find("fieldVmEvalPredicate")!=std::string::npos,
          "pure where lowers to isolated predicate control in CPU and WGSL VM");

    auto malformedWhere=lnVm;
    for(auto& inst:malformedWhere.instructions){
        if(static_cast<sdfwgsl::FieldVmOpcode>(inst.meta.x)==
           sdfwgsl::FieldVmOpcode::BranchWhereLEZero){
            inst.meta.y=0u; // empty predicate slice is malformed execution
            break;
        }
    }
    check(!sdfwgsl::evaluateFieldVm(malformedWhere,{.5f,0,0}),
          "malformed pure-guard predicate slice makes the whole VM result Unknown");

    const auto primaryWhere=sdfwgsl::compile(lnNode);
    check(primaryWhere.ok&&
          primaryWhere.wgsl.find("<= 0.0")!=std::string::npos&&
          primaryWhere.wgsl.find("log(")!=std::string::npos&&
          primaryWhere.wgsl.find("> 0.0")!=std::string::npos,
          "primary SDF WGSL preserves pure-guard definedness and CPU !(g > 0) selection");

    // The primary compiler knows richer OntoMath than the bounded predicate
    // VM. Do not let that create two notions of an authoritative pure guard.
    // SDF(f,q) is scalar and primary-lowerable, but it is deliberately outside
    // the current straight-line predicate VM subset, so BOTH authority paths
    // must refuse it at this rung.
    auto nestedSdfGuard=std::make_shared<OntoMath::MathNode>();
    nestedSdfGuard->op=OntoMath::MathNode::Op::SDF;
    nestedSdfGuard->children.push_back(
        OntoMath::MathNode::sphere(.5));
    auto nestedPoint=std::make_unique<OntoMath::MathNode>();
    nestedPoint->op=OntoMath::MathNode::Op::ValueLeaf;
    nestedPoint->variableName=OntoMath::kAmbientPointVar;
    nestedSdfGuard->children.push_back(std::move(nestedPoint));

    auto richerWherePw=std::make_shared<OntoMath::Piecewise>();
    OntoMath::Piecewise::Piece richerWherePiece;
    richerWherePiece.mathNode=pieceConstant(-1.0);
    richerWherePiece.whereLEZero=nestedSdfGuard;
    richerWherePw->pieces.push_back(std::move(richerWherePiece));
    auto richerWhereNode=geom::makeImplicit(richerWherePw);

    const auto richerWhereVm=sdfwgsl::lowerFieldVm(richerWhereNode);
    const auto richerWherePrimary=sdfwgsl::compile(richerWhereNode);
    check(!richerWhereVm.ok&&
          !richerWherePrimary.ok&&
          richerWherePrimary.error.find("shared field-VM predicate math")!=
              std::string::npos,
          "pure where authority is the shared primary/VM math intersection, not the wider primary compiler");

    const auto primaryPiece=sdfwgsl::compile(pieceNode);
    check(primaryPiece.ok&&
          primaryPiece.wgsl.find("_selected = false")!=std::string::npos&&
          primaryPiece.wgsl.find(" > ")!=std::string::npos&&
          primaryPiece.wgsl.find(" < ")!=std::string::npos,
          "primary SDF WGSL executes the same first-applicable interval Piecewise truth");

    check(w.find("cnoise3(v)")!=std::string::npos&&
          w.find("ontoPow(a, b)")!=std::string::npos&&
          w.find("ontoRawPow(a, b)")!=std::string::npos&&
          w.find("FIELD_VM_DIV_EPS")==std::string::npos,
          "WGSL VM carries native OntoMath math helpers and resolved shared guard constants");
    check(production.wgsl.find("blocker.program.w & 1u")!=std::string::npos&&
          production.wgsl.find("samplePoint + vec3<f32>(ge, 0.0, 0.0)")!=std::string::npos&&
          production.wgsl.find("var marchDenom = max(blocker.extents.w, 1.0)")!=std::string::npos&&
          production.wgsl.find("marchDenom = max(marchDenom, gradLen)")!=std::string::npos&&
          production.wgsl.find("sample.value / marchDenom")!=std::string::npos,
          "production scene visibility VM combines derived Lipschitz and Expr gradient march policy");

    // Convex payloads lower without a Convex opcode: every authored unit
    // half-space is generic scalar math, folded by MathMax.
    const std::vector<glm::vec4> cubePlanes={
        { 1, 0, 0,.5f},{-1, 0, 0,.5f},
        { 0, 1, 0,.5f},{ 0,-1, 0,.5f},
        { 0, 0, 1,.5f},{ 0, 0,-1,.5f}
    };
    auto convex=geom::SdfNode::convex(cubePlanes);
    convex.offset={.2f,-.1f,.3f};
    const auto cvm=sdfwgsl::lowerFieldVm(convex);
    bool convexParity=cvm.ok&&!cvm.needsGradientStep&&
                      std::fabs(cvm.marchLipschitz-1.0f)<=2e-6f;
    const glm::vec3 convexPts[]={{.2f,-.1f,.3f},{.7f,-.1f,.3f},{1.0f,.4f,-.2f},{-.5f,-.1f,.3f}};
    for(const auto& p:convexPts){
        const auto v=sdfwgsl::evaluateFieldVm(cvm,p);
        convexParity&=v&&std::fabs(*v-geom::evalSdf(convex,p))<=2e-6f;
    }
    check(convexParity,
          "Convex unit-plane payload lowers through generic VM math with local-offset parity");
    bool convexUsesOnlyGeneric=true;
    for(const auto& inst:cvm.instructions){
        const auto op=static_cast<sdfwgsl::FieldVmOpcode>(inst.meta.x);
        convexUsesOnlyGeneric&=
            op==sdfwgsl::FieldVmOpcode::Constant||
            op==sdfwgsl::FieldVmOpcode::CoordX||
            op==sdfwgsl::FieldVmOpcode::CoordY||
            op==sdfwgsl::FieldVmOpcode::CoordZ||
            op==sdfwgsl::FieldVmOpcode::MathMul||
            op==sdfwgsl::FieldVmOpcode::MathAdd||
            op==sdfwgsl::FieldVmOpcode::MathSub||
            op==sdfwgsl::FieldVmOpcode::MathMax;
    }
    check(convexUsesOnlyGeneric,
          "Convex admission introduces no dedicated geometry opcode");

    const auto emptyConvex=sdfwgsl::lowerFieldVm(geom::SdfNode::convex({}));
    check(!emptyConvex.ok&&emptyConvex.error.find("at least one")!=std::string::npos,
          "empty Convex payload remains Unknown/refused");

    auto scaledConvex=geom::SdfNode::convex({
        { 100,   0,   0,50.0f},{-100,   0,   0,50.0f},
        {   0, 100,   0,50.0f},{   0,-100,   0,50.0f},
        {   0,   0, 100,50.0f},{   0,   0,-100,50.0f}
    });
    scaledConvex.offset={-.25f,.15f,.05f};
    const auto scaledConvexVm=sdfwgsl::lowerFieldVm(scaledConvex);
    bool scaledConvexParity=scaledConvexVm.ok&&
        !scaledConvexVm.needsGradientStep&&
        std::fabs(scaledConvexVm.marchLipschitz-100.0f)<=2e-5f;
    const glm::vec3 scaledConvexPts[]={
        {-.25f,.15f,.05f},{.25f,.15f,.05f},{.8f,-.2f,.4f},{-.9f,.7f,-.6f}
    };
    for(const auto& p:scaledConvexPts){
        const auto v=sdfwgsl::evaluateFieldVm(scaledConvexVm,p);
        scaledConvexParity&=v&&
            std::fabs(*v-geom::evalSdf(scaledConvex,p))<=2e-4f;
    }
    check(scaledConvexParity,
          "non-unit Convex planes preserve authored CPU field and derive global Lipschitz march bound");

    const float inf=std::numeric_limits<float>::infinity();
    const auto nonFiniteConvex=sdfwgsl::lowerFieldVm(
        geom::SdfNode::convex({{1,0,0,inf}}));
    check(!nonFiniteConvex.ok&&nonFiniteConvex.error.find("non-finite")!=std::string::npos,
          "non-finite Convex plane refuses instead of manufacturing scene truth");

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
