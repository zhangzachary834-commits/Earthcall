// Generic source ownership, symbol admission, original-order and runtime refusal
// witnesses. GPU/backend rounding authority is deliberately not inferred here.
// Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
#include "Singularity/OntoMath/CommonScalarFactor.hpp"
#include <iostream>
#include <stdexcept>
using namespace OntoMath;
using O = MathNode::Op;
int checks=0;
void check(bool b, const char* why) { ++checks; if(!b) throw std::runtime_error(why); }
std::unique_ptr<MathNode> num(double c) { auto n=std::make_unique<MathNode>();n->scalarForm=ScalarForm::constant(c);return n; }
std::unique_ptr<MathNode> var(const char* name) { auto n=num(1);n->scalarForm.terms[0].factors[name]=1;return n; }
std::unique_ptr<MathNode> binary(O op,std::unique_ptr<MathNode> a,std::unique_ptr<MathNode> b) {
 auto n=std::make_unique<MathNode>();n->op=op;n->children.push_back(std::move(a));n->children.push_back(std::move(b));return n;
}
std::unique_ptr<MathNode> vec() { auto n=std::make_unique<MathNode>();n->op=O::VectorConstruct;
 n->children.push_back(var("theta"));n->children.push_back(num(2));n->children.push_back(num(3));return n; }
int main() {
 TypeEnv env={{"x",ValueKind::Scalar},{"theta",ValueKind::Scalar},{"p",ValueKind::Vector}};
 auto a=binary(O::Scale,num(2),var("x"));
 auto b=binary(O::Scale,var("x"),num(-3));
 auto c=binary(O::Scale,var("x"),vec());
 std::vector<const MathNode*> src={a.get(),b.get(),c.get()};std::string why;
 auto derive=[&]{return factorCommonScalar(src,env,{"theta"},&why);};
 auto r=derive();check(bool(r),"generic factor admission");
 check(r->factorChild==std::vector<unsigned>({1,0,0}),"original multiplication order");
 check(r->originalExpressions[0].toJson()==a->toJson(),"original expression snapshot");
 auto values=evaluateFactorCoefficients(*r,{{"theta",4.0}},&why);
 check(values&&std::get<double>((*values)[0])==2&&std::get<double>((*values)[1])==-3,"negative coefficients mathematical not policy");
 check(std::get<glm::vec3>((*values)[2])==glm::vec3(4,2,3),"explicit nonclock symbol");
 for(double x:{-2.,0.,3.}) {auto v=c->evaluate({{"x",x},{"theta",4.}});check(v&&std::get<glm::vec3>(*v)==glm::vec3(float(x))*std::get<glm::vec3>((*values)[2]),"original evaluator independent algebra path");}
 check(!evaluateFactorCoefficients(*r,{}),"missing symbol refuses");
 check(!evaluateFactorCoefficients(*r,{{"theta",INFINITY}}),"infinite binding refuses");
 check(!evaluateFactorCoefficients(*r,{{"theta",NAN}}),"nan binding refuses");
 check(!evaluateFactorCoefficients(*r,{{"theta",glm::vec3(1)}}),"binding type mismatch refuses");
 auto v2=evaluateFactorCoefficients(*r,{{"theta",9.}});check(v2&&std::get<glm::vec3>((*v2)[2]).x==9,"changed binding updates snapshot coefficient");
 auto original=r->originalExpressions[0].toJson();a->children[1]=var("theta");
 check(!derive(),"source edit requires new admission");
 check(r->originalExpressions[0].toJson()==original,"source edit cannot mutate historical source");
 a.reset();b.reset();c.reset();check(bool(evaluateFactorCoefficients(*r,{{"theta",5.}})),"source destruction preserves owned recipe");
 a=binary(O::Scale,num(2),var("x"));b=binary(O::Scale,var("x"),num(3));c=binary(O::Scale,var("x"),vec());src={a.get(),b.get(),c.get()};
 check(bool(derive()),"fresh revision admitted");
 c->children[1]->children[0]=var("x");check(!derive(),"unadmitted coefficient dependency");c->children[1]=vec();
 b->children[0]=binary(O::Add,var("x"),num(0));check(!derive(),"real equality without original structure refuses");b->children[0]=var("x");
 b->children[0]->unsupported=std::make_shared<nlohmann::json>(nlohmann::json{{"op",999}});check(!derive(),"unknown source payload refuses");b->children[0]=var("x");
 b->children[1]=num(INFINITY);check(!derive(),"nonfinite source scalar refuses");b->children[1]=num(3);
 b->children[1]->children.push_back(num(0));check(!derive(),"malformed literal refuses");b->children[1]=num(3);
 b->children[0]->op=O::Stochastic;check(!derive(),"random shared source is not pure");b->children[0]=var("x");
 b->children[0]=nullptr;check(!derive(),"null operand refuses");b->children[0]=var("x");
 env.erase("x");check(!derive(),"undeclared scalarform coordinate refuses");env["x"]=ValueKind::Vector;check(!derive(),"scalarform coordinate type mismatch refuses");env["x"]=ValueKind::Scalar;
 check(!factorCommonScalar({a.get()},env,{"theta"}),"single expression budget refuses");
 check(!factorCommonScalar(std::vector<const MathNode*>(65,a.get()),env,{"theta"}),"member budget refuses");
 auto deep=var("x");for(int i=0;i<100;++i)deep=binary(O::Add,std::move(deep),num(0));a->children[1]=std::move(deep);check(!derive(),"depth budget refuses before type/JSON recursion");
 // Computed scalar coefficients are legal mathematics; Screen's literal-only
 // transport contract still refuses them before GPU execution.
 a=binary(O::Scale,binary(O::Add,num(1),num(1)),var("x"));src[0]=a.get();
 check(bool(derive()),"generic computed coefficient is not renderer policy");
 auto big=num(1);big->scalarForm.terms[0].trans.emplace_back(TransFactor::Kind::Exp,"theta",1,0);
 c->children[1]->children[0]=std::move(big);auto overflow=derive();check(bool(overflow),"symbolic exp admitted independently of binding");
 check(!evaluateFactorCoefficients(*overflow,{{"theta",1e6}},&why),"nonfinite evaluated coefficient refuses");
 check(!why.empty(),"refusal reason legible");
 std::cout<<"PASS "<<checks<<" shared-factor ownership/type/refusal checks\n";
}
