// Original/derived evaluator controls; no renderer or numerical reference model.
// Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
#include "Singularity/OntoMath/ScalarForm.hpp"
#include <atomic>
#include <bit>
#include <cassert>
#include <cstdlib>
#include <fstream>
#include <future>
#include <iostream>
#include <limits>
#include <new>
#include <thread>
using namespace OntoMath;
static thread_local bool counting=false;
static thread_local uint64_t allocations=0;
void* operator new(std::size_t n){if(counting)++allocations;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p) noexcept{std::free(p);}
void operator delete[](void* p) noexcept{std::free(p);}
void operator delete(void* p,std::size_t) noexcept{std::free(p);}
void operator delete[](void* p,std::size_t) noexcept{std::free(p);}
using Env=std::map<std::string,PropertyValue>;
static std::unique_ptr<MathNode> leaf(ScalarForm f){auto p=std::make_unique<MathNode>();p->scalarForm=std::move(f);return p;}
static std::unique_ptr<MathNode> binary(MathNode::Op op,std::unique_ptr<MathNode> a,std::unique_ptr<MathNode> b){auto p=std::make_unique<MathNode>();p->op=op;p->children.push_back(std::move(a));p->children.push_back(std::move(b));return p;}
static void append(std::vector<uint8_t>& o,uint64_t u){for(int i=0;i<8;++i)o.push_back(uint8_t(u>>(8*i)));}
static void record(std::vector<uint8_t>& o,const std::optional<PropertyValue>& r){
 if(!r){o.push_back(0);return;}
 double x=0;
 if(propertyValueToNumber(*r,x)){o.push_back(1);append(o,std::bit_cast<uint64_t>(x));return;}
 if(auto v=std::get_if<glm::vec3>(&*r)){o.push_back(2);for(int k=0;k<3;++k)append(o,std::bit_cast<uint32_t>((*v)[k]));return;}
 std::abort();
}
static std::vector<uint8_t> suite(int seed){
 std::vector<uint8_t> out;
 std::vector<PropertyValue> values={double(-0.0),double(0),double(1),double(-3.125),float(0.1),int(7),true,char('a'),long(4294967297L),std::string("not-numeric"),glm::vec3(1,2,3),std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN(),std::numeric_limits<double>::denorm_min()};
 std::vector<ScalarForm> forms={ScalarForm::constant(-0.0),ScalarForm::variable("x"),ScalarForm::variable("missing",0.0,0.0),ScalarForm::variable("x",0.5),ScalarForm::variable("x",-2.0)};
 ScalarForm mixed;mixed.terms={Term(1e16,{{"x",1}}),Term(-1e16,{{"x",1}}),Term(0.25,{{"x",2},{"y",1}}),Term(-0.125)};forms.push_back(mixed);
 for(auto kind:{TransFactor::Kind::Sin,TransFactor::Kind::Cos,TransFactor::Kind::Exp,TransFactor::Kind::Ln})forms.push_back(ScalarForm::transcendental(kind,"x",0.375,-0.125,2.25));
 ScalarForm repeats;Term t(0.75,{{"x",2},{"y",-1}});t.trans.push_back(TransFactor(TransFactor::Kind::Sin,"x",0.5,0.25));t.trans.push_back(TransFactor(TransFactor::Kind::Cos,"x",0.75,-0.25));repeats.terms={t,Term(-2,{{"x",1}})};forms.push_back(repeats);
 for(const auto& f:forms){auto node=leaf(f);for(const auto& x:values){Env vars{{"x",x},{"y",double(2.75)},{"irrelevant",std::string("ignore")}};record(out,node->evaluate(vars));vars.erase("x");record(out,node->evaluate(vars));vars["x"]=double(seed+0.25);record(out,node->evaluate(vars));}}
 for(int i=0;i<1000;++i){
  Env vars{{"x",double(i-500)/37},{"y",double(seed+1)/17},{"p",glm::vec3(float(i)/71,2,-3)}};
  auto body=binary(MathNode::Op::Add,leaf(mixed),leaf(repeats));
  auto let=binary(MathNode::Op::Let,leaf(ScalarForm::variable("x",1,0.5)),std::move(body));let->variableName="x";
  record(out,let->evaluate(vars));record(out,leaf(ScalarForm::variable("x"))->evaluate(vars));
  auto field=leaf(ScalarForm::variable("x",2).plus(ScalarForm::variable("y",1,3)));
  auto point=std::make_unique<MathNode>();point->op=MathNode::Op::ValueLeaf;point->variableName="p";
  auto sdf=binary(MathNode::Op::SDF,std::make_unique<MathNode>(*field),std::make_unique<MathNode>(*point));record(out,sdf->evaluate(vars));
  auto gradient=binary(MathNode::Op::Gradient,std::move(field),std::move(point));record(out,gradient->evaluate(vars));
 }
 return out;
}
int main(int argc,char** argv){
 if(argc!=2)return 2;
 auto bytes=suite(0);std::ofstream f(argv[1],std::ios::binary);f.write((char*)bytes.data(),bytes.size());f.close();
 std::vector<std::future<std::vector<uint8_t>>> tasks;
 for(int i=0;i<8;++i)tasks.push_back(std::async(std::launch::async,[i]{return suite(i);}));
 for(int i=0;i<8;++i){auto a=tasks[i].get();auto b=suite(i);if(a!=b)return 3;}
 auto node=leaf(ScalarForm::variable("x",2,3).plus(ScalarForm::transcendental(TransFactor::Kind::Sin,"y",0.5,0.25,2)));
 Env vars{{"x",double(1.25)},{"y",float(2.5)}};
 node->evaluate(vars);allocations=0;counting=true;for(int i=0;i<10000;++i){auto r=node->evaluate(vars);if(!r)std::abort();}counting=false;
 std::cout<<"{\"allocation_calls_10000_scalar_leaves\":"<<allocations<<",\"serial_bytes\":"<<bytes.size()<<",\"parallel_workers\":8,\"nested_cases_per_suite\":1000,\"binding_cases_per_suite\":"<<495<<"}\n";
 return 0;
}
