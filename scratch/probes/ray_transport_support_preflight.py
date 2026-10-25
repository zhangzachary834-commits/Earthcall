"""CPU-only support-discovery/quadrature screen on exported actual native rays.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-21.
Existing leaf proofs and column envelopes supply executable conservative spans.
No truth-assisted support search. CPU candidate accuracy/counts can reject, never
establish native speed or numerical admission. No production or save mutation.
"""
import argparse,hashlib,json,os,shlex,subprocess
from pathlib import Path

SOURCE=r'''
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/VolumeDensity.hpp"
#include "Singularity/Screen/VolumeZeroProof.hpp"
#include "json.hpp"
#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <memory>
#include <random>
#include <stdexcept>
#include <vector>
using J=nlohmann::json;
using M=OntoMath::MathNode;
struct Span {double a,b;};
struct Input {std::shared_ptr<geom::FieldNode> owner;glm::dvec3 origin,half;Rendering::VolumeZeroProof proof;const M *density,*sigma,*emission;};
struct Counts {long long density=0,positive=0,panels=0,lookups=0,runs=0,negativeStages=0;};
static void require(bool x,const char* why){if(!x)throw std::runtime_error(why);}
static Span box(glm::dvec3 o,glm::dvec3 d,glm::dvec3 lo,glm::dvec3 hi,double a,double b){
 if(glm::any(glm::greaterThan(lo,hi)))return {1,0};
 for(int k=0;k<3;++k){if(std::abs(d[k])<1e-15){if(o[k]<lo[k]||o[k]>hi[k])return {1,0};}
 else{double x=(lo[k]-o[k])/d[k],y=(hi[k]-o[k])/d[k];a=std::max(a,std::min(x,y));b=std::min(b,std::max(x,y));}}
 return {a,b};
}
static std::vector<Span> merged(std::vector<Span> x){
 std::sort(x.begin(),x.end(),[](Span a,Span b){return a.a<b.a;});std::vector<Span> out;
 for(auto p:x){if(!(p.b>p.a))continue;if(!out.empty()&&p.a<=out.back().b+1e-10)out.back().b=std::max(out.back().b,p.b);else out.push_back(p);}return out;
}
static std::vector<Span> leafRuns(const Rendering::VolumeZeroProof& p,glm::dvec3 half,glm::dvec3 o,glm::dvec3 d,Span clip,long long& visits,bool join=true){
 auto cut=box(o,d,-half,half,clip.a,clip.b);if(!(cut.b>cut.a))return {};
 const glm::dvec3 h=2.0*half/glm::dvec3(p.dims);const auto q=(o+d*cut.a+half)/h;
 glm::ivec3 cell;glm::dvec3 next,delta;
 for(int k=0;k<3;++k){cell[k]=std::clamp(int(std::floor(q[k])),0,int(p.dims[k])-1);
  if(d[k]>0){next[k]=(-half[k]+(cell[k]+1)*h[k]-o[k])/d[k];delta[k]=h[k]/d[k];}
  else if(d[k]<0){next[k]=(-half[k]+cell[k]*h[k]-o[k])/d[k];delta[k]=-h[k]/d[k];}else next[k]=delta[k]=INFINITY;}
 std::vector<Span> out;double t=cut.a;unsigned guard=0;
 while(t<cut.b){require(++guard<=p.dims.x+p.dims.y+p.dims.z+12,"DDA bound exceeded");
  if(glm::any(glm::lessThan(cell,glm::ivec3(0)))||glm::any(glm::greaterThanEqual(cell,glm::ivec3(p.dims))))break;
  const double end=std::min({cut.b,next.x,next.y,next.z});++visits;
  if(end>t&&!p.proven(cell.x,cell.y,cell.z))out.push_back({t,end});
  if(end>=cut.b-1e-12)break;bool advanced=false;
  for(int k=0;k<3;++k)if(next[k]<=end+1e-12){cell[k]+=d[k]>0?1:-1;next[k]+=delta[k];advanced=true;}
  require(advanced,"DDA failed to advance");t=std::max(t,end);
 }
 return join?merged(out):out;
}
static std::vector<Span> columnRuns(const Input& m,glm::dvec3 ro,glm::dvec3 d,Span clip,long long& queries,bool join=true){
 const auto o=ro-m.origin;auto cut=box(o,d,-m.half,m.half,clip.a,clip.b);if(!(cut.b>cut.a))return {};
 const auto& p=m.proof;const glm::dvec3 pad=(glm::abs(m.origin)+glm::abs(ro)+m.half)*1e-6;
 cut=box(o,d,glm::dvec3(p.supportMin)-pad,glm::dvec3(p.supportMax)+pad,cut.a,cut.b);if(!(cut.b>cut.a))return {};
 const unsigned axis=p.supportAxis,a=(axis+1)%3,b=(axis+2)%3;require(p.supportBands.size()==p.dims[axis],"invalid column packet");
 const glm::dvec3 h=2.0*m.half/glm::dvec3(p.dims);const double x0=o[axis]+d[axis]*cut.a,x1=o[axis]+d[axis]*cut.b;
 const int first=std::clamp(int(std::floor((std::min(x0,x1)-pad[axis]+m.half[axis])/h[axis])),0,int(p.dims[axis])-1);
 const int last=std::clamp(int(std::floor((std::max(x0,x1)+pad[axis]+m.half[axis])/h[axis])),0,int(p.dims[axis])-1);
 std::vector<Span> out;
 for(int i=first;i<=last;++i){++queries;auto band=p.supportBands[i];if(band.x>band.z||band.y>band.w)continue;
  glm::dvec3 lo=-m.half,hi=m.half;lo[axis]=-m.half[axis]+i*h[axis]-pad[axis];hi[axis]=-m.half[axis]+(i+1)*h[axis]+pad[axis];
  lo[a]=band.x-pad[a];hi[a]=band.z+pad[a];lo[b]=band.y-pad[b];hi[b]=band.w+pad[b];
  auto s=box(o,d,lo,hi,cut.a,cut.b);if(s.b>s.a)out.push_back(s);
 }
 return join?merged(out):out;
}
static bool covers(const std::vector<Span>& wide,Span s){
 double a=s.a;for(auto w:wide){if(w.b<a-1e-9)continue;if(w.a>a+1e-8)return false;a=std::max(a,w.b);if(a>=s.b-1e-8)return true;}return a>=s.b-1e-8;
}
static void ddaWitness(){
 Rendering::VolumeZeroProof p;p.dims={4,3,2};p.totalCells=24;p.words={float(0x5ac39f)};const glm::dvec3 half(2,1.5,1),h=2.0*half/glm::dvec3(p.dims);
 std::mt19937 rng(42);std::uniform_real_distribution<double> u(-4,4);
 for(int i=0;i<256;++i){glm::dvec3 o(u(rng),u(rng),u(rng)),d(u(rng),u(rng),u(rng));if(i<16){o=glm::dvec3(0);d=glm::dvec3(i&1?1:-1,i&2?1:0,i&4?1:0);}
  long long visits=0;auto actual=leafRuns(p,half,o,d,{0,100},visits);std::vector<Span> expected;
  for(unsigned z=0;z<2;++z)for(unsigned y=0;y<3;++y)for(unsigned x=0;x<4;++x)if(!p.proven(x,y,z)){const auto lo=-half+glm::dvec3(x,y,z)*h;auto s=box(o,d,lo,lo+h,0,100);if(s.b>s.a)expected.push_back(s);}
  // Rays exactly on a cell face may touch both closed cells. Every such
  // proven cell has a margin; random non-boundary rays establish equality.
  if(i<16){expected=merged(expected);for(auto span:actual)require(covers(expected,span),"DDA face subset");}
  if(i>=16){expected=merged(expected);require(expected.size()==actual.size(),"DDA/brute run count");for(size_t k=0;k<actual.size();++k)require(std::abs(expected[k].a-actual[k].a)<1e-8&&std::abs(expected[k].b-actual[k].b)<1e-8,"DDA/brute interval");}
 }
}
static const M* single(const OntoMath::Piecewise* p){if(!p||p->pieces.size()!=1)return nullptr;const auto& x=p->pieces[0];return !x.guard&&!x.call&&!x.fold&&!x.hasLo&&!x.hasHi&&!x.whereLEZero?x.mathNode.get():nullptr;}
static std::array<double,4> at(const std::vector<Input>& media,const std::vector<int>& active,glm::dvec3 ro,glm::dvec3 d,double s,double clock,Counts& c){
 auto world=ro+s*d;const auto woD=ro-world;const double n=glm::length(woD);const glm::vec3 wo(n>1e-8?woD/n:glm::dvec3(0));std::array<double,4> out{};
 for(int id:active){const auto& m=media[id];const glm::vec3 pf(world-m.origin);if(glm::any(glm::greaterThan(glm::abs(glm::dvec3(pf)),m.half)))continue;
  const std::map<std::string,PropertyValue> vars{{"p",PropertyValue(pf)},{"x",PropertyValue(double(pf.x))},{"y",PropertyValue(double(pf.y))},{"z",PropertyValue(double(pf.z))},{OntoMath::kTimeVar,PropertyValue(clock)},{OntoMath::kOmegaXVar,PropertyValue(double(wo.x))},{OntoMath::kOmegaYVar,PropertyValue(double(wo.y))},{OntoMath::kOmegaZVar,PropertyValue(double(wo.z))}};
  double D=0,X=0;auto v=m.density->evaluate(vars);require(bool(v)&&propertyValueToNumber(*v,D),"density unavailable");++c.density;if(!(D>0))continue;++c.positive;
  v=m.sigma->evaluate(vars);require(bool(v)&&propertyValueToNumber(*v,X),"sigma unavailable");out[0]+=std::max(X,1e-6);
  v=m.emission->evaluate(vars);require(bool(v)&&std::holds_alternative<glm::vec3>(*v),"emission unavailable");const auto e=glm::max(std::get<glm::vec3>(*v),glm::vec3(0));for(int k=0;k<3;++k)out[k+1]+=e[k];
 }
 return out;
}
static std::array<double,4> rkStep(std::array<double,4> y,const std::array<double,4>& a,const std::array<double,4>& b,const std::array<double,4>& c,double h,Counts& counts){
 const double T=y[0],k1=-a[0]*T,T2=T+0.5*h*k1,k2=-b[0]*T2,T3=T+0.5*h*k2,k3=-b[0]*T3,T4=T+h*k3,k4=-c[0]*T4;
 if(std::min({T,T2,T3,T4})<0)++counts.negativeStages;
 for(int j=1;j<4;++j)y[j]+=h*(a[j]*T+2*b[j]*(T2+T3)+c[j]*T4)/6;
 y[0]+=h*(k1+2*k2+2*k3+k4)/6;return y;
}
static void rkWitness(){
 std::vector<double> error;const double q=0.7,exactT=std::exp(-q),A=(1-exactT)/q,B=(1-(1+q)*exactT)/(q*q),exactL=.2*A+.3*B;
 for(int n:{2,4,8,16}){Counts counts;std::array<double,4> y{1,0,0,0};
  const auto field=[&](double s){return std::array<double,4>{q,.2+.3*s,.1,.4};};
  for(int k=0;k<n;++k)y=rkStep(y,field(double(k)/n),field((k+.5)/n),field(double(k+1)/n),1./n,counts);
  error.push_back(std::max(std::abs(y[0]-exactT),std::abs(y[1]-exactL)));require(counts.negativeStages==0,"RK4 witness negative stage");
 }
 for(size_t i=1;i<error.size();++i)require(error[i-1]/error[i]>14 && error[i-1]/error[i]<20,"RK4 smooth order");
 std::printf("RK4 smooth analytic witness errors %.9g %.9g %.9g %.9g\n",error[0],error[1],error[2],error[3]);
}
static std::array<double,4> integrate(const std::vector<Input>& media,const std::vector<std::vector<Span>>& runs,glm::dvec3 ro,glm::dvec3 d,double clock,int target,Counts& counts,bool rk4){
 std::vector<int> lastActive;double lastEnd=-INFINITY;std::array<double,4> lastValue{};
 std::vector<double> events;for(auto& list:runs)for(auto s:list){events.push_back(s.a);events.push_back(s.b);++counts.runs;}
 std::sort(events.begin(),events.end());events.erase(std::unique(events.begin(),events.end()),events.end());std::array<double,4> y{1,0,0,0};const double a=.25-std::sqrt(3.)/6,b=.5-a;
 for(size_t e=1;e<events.size();++e){const double lo=events[e-1],hi=events[e];if(!(hi>lo))continue;const double mid=.5*(lo+hi);std::vector<int> active;double spacing=INFINITY;
  for(size_t i=0;i<runs.size();++i)for(auto s:runs[i])if(mid>=s.a&&mid<=s.b){active.push_back(int(i));spacing=std::min(spacing,(s.b-s.a)/target);break;}
  if(active.empty())continue;const int n=std::max(1,int(std::ceil((hi-lo)/spacing)));const double h=(hi-lo)/n;
  if(rk4){auto left=(lastEnd==lo&&lastActive==active)?lastValue:at(media,active,ro,d,lo,clock,counts);
   for(int k=0;k<n;++k){++counts.panels;const auto middle=at(media,active,ro,d,lo+(k+.5)*h,clock,counts),right=at(media,active,ro,d,lo+(k+1)*h,clock,counts);y=rkStep(y,left,middle,right,h,counts);left=right;}
   lastActive=active;lastEnd=hi;lastValue=left;continue;
  }
  for(int k=0;k<n;++k){++counts.panels;const auto u=at(media,active,ro,d,lo+(k+.5-std::sqrt(3.)/6)*h,clock,counts),v=at(media,active,ro,d,lo+(k+.5+std::sqrt(3.)/6)*h,clock,counts);
   for(auto w:std::array<std::array<double,2>,2>{{{b,a},{a,b}}}){const double sigma=w[0]*u[0]+w[1]*v[0],x=h*sigma,atten=std::exp(-x),gain=h*(x==0?1:-std::expm1(-x)/x);for(int j=1;j<4;++j)y[j]+=y[0]*(w[0]*u[j]+w[1]*v[j])*gain;y[0]*=atten;}
  }
 }
 return y;
}
static J stats(std::vector<double> e){std::sort(e.begin(),e.end());double sum=0,sq=0;for(double x:e){require(std::isfinite(x),"nonfinite error");sum+=x;sq+=x*x;}return J{{"mean",sum/e.size()},{"rms",std::sqrt(sq/e.size())},{"p99",e[e.size()*99/100]},{"max",e.back()}};}
int main(int argc,char** argv){
 try{require(argc==4,"usage: probe rays.json reference.json output.json");ddaWitness();rkWitness();J rays,ref,zone;{std::ifstream in(argv[1]);in>>rays;}{std::ifstream in(argv[2]);in>>ref;}{std::ifstream in("saves/zones/Northern Veil/zone.ecform",std::ios::binary);std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)),{});zone=J::parse(J::from_msgpack(b).at("MigrationRoot").get<std::string>());}
  auto root=geom::FieldNode::fromJson(zone.at("spatialRoot"));require(root&&root->field,"source root absent");const M* source=single(&root->field->astDefinition);require(source&&source->op==M::Op::ScalarLeaf,"not literal zero source");for(auto& t:source->scalarForm.terms)require(t.coefficient==0,"nonzero source unsupported");
  std::vector<Input> media;J metadata=J::array();for(auto& j:zone.at("spatialFields")){auto field=geom::FieldNode::fromJson(j);Rendering::VolumeDensityBinding b;require(field&&Rendering::readVolumeDensity(*field,0,0,b),"medium binding");Input m{field,glm::dvec3(b.origin),glm::abs(glm::dvec3(b.scale)),{},single(b.densityExpr),single(b.extinctionExpr),single(b.emissionExpr)};require(m.density&&m.sigma&&m.emission,"unsupported channel piece");m.proof=Rendering::buildVolumeZeroProof(*b.densityExpr,glm::vec3(m.half));metadata.push_back({{"id",b.producerId},{"dims",{m.proof.dims.x,m.proof.dims.y,m.proof.dims.z}},{"axis",m.proof.supportAxis},{"proven",m.proof.provenCells},{"total",m.proof.totalCells}});media.push_back(std::move(m));}
  const auto& R=rays.at("rays");require(R.size()==ref.at("rows").size(),"reference ray count");const glm::dvec3 ro(rays["origin"][0],rays["origin"][1],rays["origin"][2]);const double clock=rays.at("time");
  const bool cells=std::getenv("EC_SUPPORT_CELL_SCREEN")!=nullptr;std::vector<std::vector<std::vector<Span>>> leaf(R.size()),columns(R.size()),leafCells(R.size()),columnCells(R.size());long long leafVisits=0,colQueries=0;J widths=J::array();
  for(size_t i=0;i<R.size();++i){auto& r=R[i];auto& q=ref["rows"][i];require(r[0]==q[0]&&r[1]==q[1],"reference row mismatch");const glm::dvec3 d(r[2],r[3],r[4]);const Span clip{r[5],r[6]};leaf[i].resize(media.size());columns[i].resize(media.size());leafCells[i].resize(media.size());columnCells[i].resize(media.size());
   for(size_t j=0;j<media.size();++j){auto& m=media[j];auto global=box(ro-m.origin,d,glm::dvec3(m.proof.supportMin),glm::dvec3(m.proof.supportMax),clip.a,clip.b);if(global.b>global.a)leafCells[i][j]=leafRuns(m.proof,m.half,ro-m.origin,d,global,leafVisits,false);leaf[i][j]=merged(leafCells[i][j]);columnCells[i][j]=columnRuns(m,ro,d,clip,colQueries,false);columns[i][j]=merged(columnCells[i][j]);for(auto s:leaf[i][j])require(covers(columns[i][j],s),"column envelope lost an unknown leaf interval");}
  }
  J report{{"scope","CPU optimistic candidate/count screen, not native cost or numerical qualification"},{"media",metadata},{"rays",R.size()},{"dda_bruteforce_random_witnesses",240},{"tie_axis_witnesses",16},{"all_leaf_intervals_covered_by_columns",true},{"leaf_cell_visits",leafVisits},{"column_packet_queries",colQueries},{"candidates",J::array()}};
  std::printf("DISCOVERY leaf=%lld column=%lld on %zu rays; all leaf intervals enclosed\n",leafVisits,colQueries,R.size());std::fflush(stdout);
  for(const char* method:{"cf4","rk4"})for(const char* kind:(cells?std::vector<const char*>{"leaf_cells","column_cells"}:std::vector<const char*>{"leaf","columns"}))for(int target:(cells?std::vector<int>{1,2,3,4}:std::vector<int>{1,2,3,4,6,8,12,16})){Counts counts;std::vector<double> T,L;J answers=J::array();const auto& runs=std::string(kind)=="leaf"?leaf:std::string(kind)=="columns"?columns:std::string(kind)=="leaf_cells"?leafCells:columnCells;
   for(size_t i=0;i<R.size();++i){auto& r=R[i];const glm::dvec3 d(r[2],r[3],r[4]);auto y=integrate(media,runs[i],ro,d,clock,target,counts,std::string(method)=="rk4");const auto& truth=ref["rows"][i];T.push_back(std::abs(y[0]-truth[2].get<double>()));for(int k=1;k<4;++k)L.push_back(std::abs(y[k]-truth[k+2].get<double>()));answers.push_back(y);}
   J result{{"method",method},{"negative_stages",counts.negativeStages},{"kind",kind},{"panels_per_unknown_run",target},{"density_calls",counts.density},{"positive_calls",counts.positive},{"panels",counts.panels},{"runs",counts.runs},{"T",stats(T)},{"RGB",stats(L)}};report["candidates"].push_back(result);std::printf("%s\n",result.dump().c_str());std::fflush(stdout);
   {std::ofstream out(std::string(argv[3])+"-"+method+"-"+kind+"-"+std::to_string(target)+"-rows.json");out<<answers;}
   {std::ofstream out(argv[3]);out<<report.dump(2)<<"\n";}
  }
  return 0;
 }catch(const std::exception& e){std::fprintf(stderr,"REFUSE: %s\n",e.what());return 2;}
}
'''

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--cells',action='store_true');a=p.parse_args();r=a.root.resolve();s=a.output.resolve();assert r!=s and r not in s.parents
    inp=json.loads((r/'docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json').read_text());assert hashlib.sha256((r/inp['fixture']).read_bytes()).hexdigest()==inp['fixture_sha256']
    s.mkdir(parents=True,exist_ok=False);(s/'preflight.cpp').write_text(SOURCE);b=r/'build';db=json.loads((b/'compile_commands.json').read_text())
    c=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command']);args=shlex.split(c['command']);args[args.index('-c')+1]=str(s/'preflight.cpp');args[args.index('-o')+1]=str(s/'preflight.o');args+=['-O2'];subprocess.run(args,cwd=b,check=True)
    args=shlex.split((b/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());args[next(i for i,x in enumerate(args) if x.endswith('/src/entry.cpp.o'))]=str(s/'preflight.o');args[args.index('-o')+1]=str(s/'preflight');subprocess.run(args,cwd=b,check=True)
    subprocess.run([str(s/'preflight'),str(r/'docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json'),str(r/'docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json'),str(s/'summary.json')],cwd=r,check=True,env=dict(os.environ,**({'EC_SUPPORT_CELL_SCREEN':'1'} if a.cells else {})))
