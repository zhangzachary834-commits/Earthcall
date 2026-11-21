// Common authored-AST consumer witness; host mathematical proofs ONLY.
// Reuses OntoMath's existing range machinery; no renderer math, new geometry
// enum, native skip admission, saved state or default is introduced.
// Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-25.
#define main historical_genericity_main_not_called
#include "ray_transport_gpu_6dg_ontomath_genericity.cpp"
#undef main
#include <limits>

struct FamilyRecipe {
    std::unique_ptr<M> expression;
    std::vector<const M*> zeroFactors;
    std::set<std::string> dependencies;
    bool supported=false;
};
static bool literal(const M* n,double value){
    if(!n||n->op!=M::Op::ScalarLeaf)return false;
    const auto& ts=n->scalarForm.terms;
    if(value==0.0&&ts.empty())return true;
    return ts.size()==1&&ts[0].coefficient==value&&ts[0].factors.empty()&&ts[0].trans.empty();
}
static bool containsNoise(const M* n){if(n->op==M::Op::Noise)return true;for(const auto& c:n->children)if(c&&containsNoise(c.get()))return true;return false;}
static void factorClauses(const M* n,std::vector<const M*>& clauses){
    if(n->op==M::Op::Clamp&&n->children.size()==3&&literal(n->children[1].get(),0)&&literal(n->children[2].get(),1)){if(!containsNoise(n))clauses.push_back(n);return;}
    if(n->op==M::Op::Scale&&n->children.size()==2)for(const auto& c:n->children)if(c)factorClauses(c.get(),clauses);
}
static bool admittedGrammar(const M* n){
    if(!n)return false;
    switch(n->op){
    case M::Op::ScalarLeaf:
        for(const auto& t:n->scalarForm.terms){if(!std::isfinite(t.coefficient))return false;
            for(const auto& [name,power]:t.factors)if(!std::isfinite(power)||power<0||power>8||std::floor(power)!=power)return false;
            for(const auto& f:t.trans)if((f.kind!=OntoMath::TransFactor::Kind::Sin&&f.kind!=OntoMath::TransFactor::Kind::Cos)||!std::isfinite(f.scale)||!std::isfinite(f.shift))return false;}
        return true;
    case M::Op::ValueLeaf:return true;
    case M::Op::Div:{
        if(n->children.size()!=2||!n->children[1]||n->children[1]->op!=M::Op::ScalarLeaf)return false;
        const auto& ts=n->children[1]->scalarForm.terms;
        if(ts.size()!=1||!ts[0].factors.empty()||!ts[0].trans.empty()||!std::isfinite(ts[0].coefficient)||ts[0].coefficient==0)return false;break;}
    case M::Op::Clamp:if(n->children.size()!=3||!literal(n->children[1].get(),0)||!literal(n->children[2].get(),1))return false;break;
    case M::Op::VectorConstruct:case M::Op::Component:case M::Op::Add:case M::Op::Sub:case M::Op::Scale:case M::Op::Length:case M::Op::Abs:case M::Op::Noise:break;
    default:return false;
    }
    for(const auto& c:n->children)if(!admittedGrammar(c.get()))return false;
    return true;
}
static FamilyRecipe analyzeFamily(const M& authored){
    FamilyRecipe r;if(!admittedGrammar(&authored))return r;r.expression=std::make_unique<M>(authored);
    OntoMath::TypeEnv types={{"p",OntoMath::MathType(OntoMath::ValueKind::Vector)},{"x",OntoMath::MathType(OntoMath::ValueKind::Scalar)},{"y",OntoMath::MathType(OntoMath::ValueKind::Scalar)},{"z",OntoMath::MathType(OntoMath::ValueKind::Scalar)},{"t",OntoMath::MathType(OntoMath::ValueKind::Scalar)}};
    auto judged=authored.typeOf(types);if(!judged||judged.kind!=OntoMath::ValueKind::Scalar)return r;r.expression->collectDependencies(r.dependencies);
    for(const auto& dep:r.dependencies)if(dep!="p"&&dep!="x"&&dep!="y"&&dep!="z"&&dep!="t")return r;
    factorClauses(r.expression.get(),r.zeroFactors);r.supported=!r.zeroFactors.empty();return r;
}
struct QueryBox { OntoMath::Interval p[3]; };
static std::map<std::string,M::RangeValue> bindBox(const QueryBox& q){
    return {{"p",M::RangeValue::makeVector(q.p[0],q.p[1],q.p[2])},{"x",M::RangeValue::makeScalar(q.p[0])},
        {"y",M::RangeValue::makeScalar(q.p[1])},{"z",M::RangeValue::makeScalar(q.p[2])},{"t",M::RangeValue::makeScalar(OntoMath::Interval(7.25f))}};
}
static bool certifiedZero(const FamilyRecipe& r,const QueryBox& q){
    if(!r.supported)return false;
    for(const auto& x:q.p)if(!x.bounded()||x.empty())return false;
    const auto env=bindBox(q);
    for(const M* clause:r.zeroFactors){auto range=clause->evalMathematicalRange(env);
        if(range&&range->kind==OntoMath::ValueKind::Scalar&&range->scalar.exactZero())return true;}
    return false;
}
static std::optional<float> authoredPoint(const M& n,glm::vec3 p){
    auto v=n.evaluate({{"p",PropertyValue(p)},{"x",PropertyValue(p.x)},{"y",PropertyValue(p.y)},{"z",PropertyValue(p.z)},{"t",PropertyValue(7.25f)}});
    if(!v)return std::nullopt;
    if(auto f=std::get_if<float>(&*v))return *f;
    if(auto d=std::get_if<double>(&*v))return float(*d);
    return std::nullopt;
}
static bool clipBox(glm::vec3 o,glm::vec3 d,glm::vec3 lo,glm::vec3 hi,float& a,float& b){
    for(int k=0;k<3;++k){if(d[k]==0){if(o[k]<lo[k]||o[k]>hi[k])return false;}
        else {float x=(lo[k]-o[k])/d[k],y=(hi[k]-o[k])/d[k];a=std::max(a,std::min(x,y));b=std::min(b,std::max(x,y));}}
    return b>a;
}
static std::unique_ptr<M> number(double x){auto n=std::make_unique<M>();n->scalarForm=OntoMath::ScalarForm::constant(x);return n;}
static std::unique_ptr<M> point(){auto n=std::make_unique<M>();n->op=M::Op::ValueLeaf;n->variableName="p";return n;}
static std::unique_ptr<M> unary(M::Op op,std::unique_ptr<M> x){auto n=std::make_unique<M>();n->op=op;n->children.push_back(std::move(x));return n;}
static std::unique_ptr<M> binary(M::Op op,std::unique_ptr<M> a,std::unique_ptr<M> b){auto n=unary(op,std::move(a));n->children.push_back(std::move(b));return n;}
static std::unique_ptr<M> authoredShell(glm::vec3 center,double radius,double width,double noiseScale){
 auto c=std::make_unique<M>();c->op=M::Op::VectorConstruct;for(int k=0;k<3;++k)c->children.push_back(number(center[k]));
 auto distance=binary(M::Op::Sub,unary(M::Op::Length,binary(M::Op::Sub,point(),std::move(c))),number(radius));
 auto body=binary(M::Op::Sub,number(1),binary(M::Op::Div,unary(M::Op::Abs,std::move(distance)),number(width)));
 auto support=unary(M::Op::Clamp,std::move(body));support->children.push_back(number(0));support->children.push_back(number(1));
 auto noise=unary(M::Op::Noise,binary(M::Op::Scale,number(noiseScale),point()));
 return binary(M::Op::Scale,std::move(support),binary(M::Op::Add,number(.5),binary(M::Op::Scale,number(.5),std::move(noise))));
}
int main(int argc,char** argv){
    if(argc!=4)return 2;std::ifstream in(argv[1],std::ios::binary);std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(in)),{});
    auto zone=J::parse(J::from_msgpack(bytes).at("MigrationRoot").get<std::string>());J rays;{std::ifstream f(argv[2]);f>>rays;}
    glm::vec3 ro(rays["origin"][0].get<float>(),rays["origin"][1].get<float>(),rays["origin"][2].get<float>());
    struct Input {FamilyRecipe recipe;glm::vec3 origin,lo,hi;std::string label;};std::vector<Input> inputs;
    for(const auto& j:zone.at("spatialFields")){
        auto field=geom::FieldNode::fromJson(j);Rendering::VolumeDensityBinding b;
        if(!field||!Rendering::readVolumeDensity(*field,0,0,b)||!b.densityExpr||b.densityExpr->pieces.size()!=1||!b.densityExpr->pieces[0].mathNode)continue;
        inputs.push_back({analyzeFamily(*b.densityExpr->pieces[0].mathNode),b.origin,-glm::abs(b.scale),glm::abs(b.scale),"authored-field"});
    }
    std::array<glm::vec3,4> centers;
    // Author the second mathematical fixture in the admitted query frustum;
    // the earlier shell fixture had no hits for this captured view.
    for(int k=0;k<4;++k){const auto& ray=rays["rays"][size_t(k+1)*rays["rays"].size()/5];glm::vec3 d(ray[2].get<float>(),ray[3].get<float>(),ray[4].get<float>());centers[k]=ro+d*(.5f*ray[6].get<float>());}

    for(int k=0;k<4;++k){float radius=8.0f+float(k);auto ast=authoredShell(centers[k],radius,1.25f,.1f);
        inputs.push_back({analyzeFamily(*ast),glm::vec3(0),centers[k]-glm::vec3(radius+1.25f),centers[k]+glm::vec3(radius+1.25f),"authored-second-family"});}
    if(inputs.size()!=8)throw std::runtime_error("two-family coverage");
    J results=J::array();uint64_t allZero=0,allSamples=0;
    for(auto& input:inputs){uint64_t queries=0,zeros=0,samples=0;uint64_t refusals=0;
        if(!input.recipe.supported)throw std::runtime_error("fixture analyzer not connected");
        // Same consumer and same existing OntoMath range operation for BOTH
        // families. No wave spec, shell solver or geometry-class switch here.
        for(size_t ri=0;ri<rays["rays"].size();ri+=17){const auto& ray=rays["rays"][ri];glm::vec3 d(ray[2].get<float>(),ray[3].get<float>(),ray[4].get<float>());float a=ray[5],b=ray[6];glm::vec3 o=ro-input.origin;
            if(!clipBox(o,d,input.lo,input.hi,a,b))continue;
            for(int bin=0;bin<8;++bin){float l=a+(b-a)*float(bin)/8,r=a+(b-a)*float(bin+1)/8;QueryBox q;
                for(int axis=0;axis<3;++axis){auto endpoints=OntoMath::Interval::outward(l,r)*OntoMath::Interval(d[axis])+OntoMath::Interval(o[axis]);q.p[axis]=endpoints;}
                ++queries;if(!certifiedZero(input.recipe,q))continue;++zeros;
                for(int k=0;k<9;++k){auto p=o+d*(l+(r-l)*float(k)/8);auto value=authoredPoint(*input.recipe.expression,p);
                    if(!value||!std::isfinite(*value)||std::abs(*value)>1e-7f)throw std::runtime_error("zero proof disagrees with authored evaluator");++samples;}
            }
        }
        // Recipe owns its authored snapshot. A changed/unsupported expression
        // is re-analyzed and retains work; stale support clauses cannot survive.
        M edited;edited.op=M::Op::ScalarLeaf;edited.scalarForm=OntoMath::ScalarForm::constant(1);auto replacement=analyzeFamily(edited);
        QueryBox empty{{OntoMath::Interval(0),OntoMath::Interval(0),OntoMath::Interval(10000)}};
        if(certifiedZero(replacement,empty))throw std::runtime_error("stale recipe survived authored edit");++refusals;
        auto unknown=std::make_unique<M>();unknown->op=M::Op::ValueLeaf;unknown->variableName="unbound";
        auto unknownRecipe=analyzeFamily(*unknown);if(certifiedZero(unknownRecipe,empty))throw std::runtime_error("unbound expression admitted");++refusals;
        QueryBox invalid=empty;invalid.p[0]=OntoMath::Interval::infinite();if(certifiedZero(input.recipe,invalid))throw std::runtime_error("unbounded query admitted");++refusals;
        auto undefined=std::make_unique<M>();undefined->op=M::Op::Scale;undefined->children.push_back(std::make_unique<M>(*input.recipe.expression));
        auto badDivide=std::make_unique<M>();badDivide->op=M::Op::Div;badDivide->children.push_back(number(1));badDivide->children.push_back(number(0));undefined->children.push_back(std::move(badDivide));
        auto undefinedRecipe=analyzeFamily(*undefined);if(certifiedZero(undefinedRecipe,empty))throw std::runtime_error("zero times undefined admitted");++refusals;
        if(!queries||!zeros){std::cerr<<"VACUOUS "<<input.label<<" queries="<<queries<<" zeros="<<zeros<<" clauses="<<input.recipe.zeroFactors.size()<<" expression="<<input.recipe.expression->print()<<"\n";throw std::runtime_error("generic witness vacuous");}
        results.push_back({{"label",input.label},{"queries",queries},{"certified_zero_boxes",zeros},{"independent_point_samples",samples},{"negative_controls",refusals},{"dependencies",input.recipe.dependencies},{"factor_clauses",input.recipe.zeroFactors.size()}});allZero+=zeros;allSamples+=samples;
    }
    J result={{"pass",true},{"shared_analyzer_consumer_used_by_both_families",true},{"results",results},{"certified_zero_boxes",allZero},{"point_samples",allSamples},
       {"native_skip_admission",false},{"generic_arbitrary_AST_admission",false},{"scope","Host mathematical range witness; no finite-execution error enclosure, GPU transfer, performance, or production admission"}};
    std::ofstream(argv[3])<<result.dump(2)<<'\n';std::cout<<"PASS common AST consumer "<<allZero<<" boxes "<<allSamples<<" point samples\n";
}
