// Independent converged CPU reference for Northern Veil's projected four-medium
// transport (scratch/probes; never in CMake). Claude Code / Claude Opus 5.5 /
// f3af84cf / 2026-10-21. For Codex's exported actual f32 rays: the continuum
//   T = exp(-∫σ), L = ∫ T·S ds,  σ = Σ_j max(σ_ext,j, 1e-6)·[D_j>0],
//   S = Σ_j max(E_j, 0)·[D_j>0]  (+ in-scatter, exactly 0 under the authored zero source),
// over each ray's clip interval, members by box containment. Authored ASTs are
// evaluated by OntoMath (MathNode::evaluate), not the WGSL emitter. Cumulative
// 4th-order Simpson on one finest grid, read back at nested resolutions to give
// every ray its own convergence estimate.
#define main unused_ray_witness_main
#include "../../tests/singularity/webgpu_line_quadrature_test.cpp"
#undef main
#include <cstdio>
#include <thread>
#include <atomic>
#include <iomanip>
using MathNodeC = OntoMath::MathNode;
struct Member { std::string id; glm::dvec3 origin, half; const MathNodeC *D=nullptr,*X=nullptr,*E=nullptr; };
static const MathNodeC* single(const OntoMath::Piecewise* p){
    if(!p||p->pieces.size()!=1)return nullptr;const auto& x=p->pieces[0];
    return !x.guard&&!x.call&&!x.fold&&!x.hasLo&&!x.hasHi&&!x.whereLEZero?x.mathNode.get():nullptr;}
int main(int argc,char**argv){
    const char* raysPath=argc>1?argv[1]:"docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json";
    const int log2N=argc>2?std::atoi(argv[2]):15;const int limit=argc>3?std::atoi(argv[3]):1<<30;
    const int threads=argc>4?std::atoi(argv[4]):4;const char* outPath=argc>5?argv[5]:nullptr;
    namespace fs=std::filesystem;
    std::ifstream rin(raysPath);auto R=nlohmann::json::parse(rin);
    const double clock=R.at("time").get<double>();
    const glm::dvec3 ro(R["origin"][0].get<double>(),R["origin"][1].get<double>(),R["origin"][2].get<double>());
    fs::path dir="saves/zones/Northern Veil";nlohmann::json zone;
    {std::ifstream in(dir/"zone.ecform",std::ios::binary);std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)),{});
     zone=nlohmann::json::parse(nlohmann::json::from_msgpack(b).at("MigrationRoot").get<std::string>());}
    // The authored root source must be exactly zero radiance for in-scatter to vanish.
    auto root=geom::FieldNode::fromJson(zone.at("spatialRoot"));
    bool zeroSource=false;
    if(root && root->field && root->field->mode==OntoMath::ScalarField::EvaluationMode::AST){
        const auto* rad=single(&root->field->astDefinition);
        zeroSource=rad && rad->op==MathNodeC::Op::ScalarLeaf;
        if(zeroSource)for(const auto& term:rad->scalarForm.terms)zeroSource=zeroSource&&term.coefficient==0.0;   // every term exactly 0
        if(rad)std::printf("SOURCE radiance expression: %s\n",rad->print().c_str());
    }
    std::printf("SOURCE authored radiance identically zero: %s\n",zeroSource?"yes":"NO (refusing: in-scatter not modelled)");
    if(!zeroSource)return 3;
    std::vector<std::shared_ptr<geom::FieldNode>> keep;std::vector<Member> media;
    for(const auto& json:zone.at("spatialFields")){
        auto field=geom::FieldNode::fromJson(json);Rendering::VolumeDensityBinding b;
        if(!field||!Rendering::readVolumeDensity(*field,0,0,b)){continue;}
        Member m{b.producerId,glm::dvec3(b.origin),glm::abs(glm::dvec3(b.scale)),single(b.densityExpr),single(b.extinctionExpr),single(b.emissionExpr)};
        if(!m.D||!m.X||!m.E){std::printf("REFUSE %s: a channel is not a single unguarded piece\n",m.id.c_str());return 4;}
        keep.push_back(field);media.push_back(m);
    }
    std::printf("MEDIA %zu, clock %.3f, eye (%g,%g,%g)\n",media.size(),clock,ro.x,ro.y,ro.z);
    const auto& rays=R.at("rays");const int count=std::min<int>(rays.size(),limit);
    struct Out{double T[4],L[4][3];};std::vector<Out> out(count);
    std::atomic<int> next{0};std::atomic<long long> evals{0};
    auto work=[&]{
        for(int i;(i=next++)<count;){
            const auto& r=rays[i];
            const glm::dvec3 d(r[2].get<double>(),r[3].get<double>(),r[4].get<double>());
            const double a=r[5].get<double>(),bEnd=r[6].get<double>();
            const int N=1<<log2N;const double h=(bEnd-a)/N;
            std::vector<double> sig(N+1);std::vector<glm::dvec3> src(N+1);long long ev=0;
            for(int k=0;k<=N;++k){
                const double s=a+k*h;const glm::dvec3 x=ro+s*d;
                const glm::dvec3 woD=ro-x;const double woL=glm::length(woD);const glm::vec3 wo(woL>1e-8?woD/woL:glm::dvec3(0));
                double S=0;glm::dvec3 E(0);
                for(const auto& m:media){
                    const glm::dvec3 p=x-m.origin;
                    if(std::abs(p.x)>m.half.x||std::abs(p.y)>m.half.y||std::abs(p.z)>m.half.z)continue;
                    const glm::vec3 pf(p);
                    const std::map<std::string,PropertyValue> vars{{"p",PropertyValue(pf)},{"x",PropertyValue(double(pf.x))},{"y",PropertyValue(double(pf.y))},{"z",PropertyValue(double(pf.z))},
                        {OntoMath::kTimeVar,PropertyValue(clock)},{OntoMath::kOmegaXVar,PropertyValue(double(wo.x))},{OntoMath::kOmegaYVar,PropertyValue(double(wo.y))},{OntoMath::kOmegaZVar,PropertyValue(double(wo.z))}};
                    double D=0;{const auto v=m.D->evaluate(vars);if(v)propertyValueToNumber(*v,D);}++ev;
                    if(!(D>0))continue;
                    double X=0;{const auto v=m.X->evaluate(vars);if(v)propertyValueToNumber(*v,X);}
                    const auto e=m.E->evaluate(vars);ev+=2;
                    glm::dvec3 ev3(0);if(e&&std::holds_alternative<glm::vec3>(*e))ev3=glm::dvec3(std::get<glm::vec3>(*e));
                    S+=std::max(X,1e-6);E+=glm::max(ev3,glm::dvec3(0));
                }
                sig[k]=S;src[k]=E;
            }
            // Diagnostic: EC_DUMP_PIXEL="x,y" prints the ray's positive-σ spans per member.
            if(const char* dp=std::getenv("EC_DUMP_PIXEL")){int qx=-1,qy=-1;std::sscanf(dp,"%d,%d",&qx,&qy);
                if(r[0].get<int>()==qx&&r[1].get<int>()==qy){
                    for(const auto& m:media){bool in=false;double s0=0,peak=0;
                        for(int k=0;k<=N;++k){const double s=a+k*h;const glm::dvec3 x=ro+s*d,p=x-m.origin;bool pos=false;double X=0;
                            if(std::abs(p.x)<=m.half.x&&std::abs(p.y)<=m.half.y&&std::abs(p.z)<=m.half.z){const glm::vec3 pf(p);
                                const std::map<std::string,PropertyValue> vars{{"p",PropertyValue(pf)},{"x",PropertyValue(double(pf.x))},{"y",PropertyValue(double(pf.y))},{"z",PropertyValue(double(pf.z))},{OntoMath::kTimeVar,PropertyValue(clock)},{OntoMath::kOmegaXVar,PropertyValue(0.0)},{OntoMath::kOmegaYVar,PropertyValue(0.0)},{OntoMath::kOmegaZVar,PropertyValue(0.0)}};
                                double D=0;const auto v=m.D->evaluate(vars);if(v)propertyValueToNumber(*v,D);
                                if(D>0){pos=true;const auto xv=m.X->evaluate(vars);if(xv)propertyValueToNumber(*xv,X);}}
                            if(pos&&!in){in=true;s0=s;peak=0;} if(pos)peak=std::max(peak,X);
                            if(!pos&&in){in=false;std::printf("  SPAN %s s [%.3f, %.3f] width %.3f peak σ %.4g\n",m.id.c_str(),s0,s,s-s0,peak);}}
                        if(in)std::printf("  SPAN %s s [%.3f, end] peak σ %.4g\n",m.id.c_str(),s0,peak);}
                    std::printf("  clip [%.3f, %.3f]\n",a,bEnd);}}
            evals+=ev;
            // Nested resolutions: stride 1,2,4,8 of the finest grid.
            for(int lev=0;lev<4;++lev){
                const int st=1<<lev;const double H=h*st;const int n=N/st;
                long double tau=0;long double L[3]={0,0,0};
                auto g=[&](int k,long double t,int c){return std::exp(-(double)t)*src[k*st][c];};
                long double ga[3];for(int c=0;c<3;++c)ga[c]=g(0,0,c);
                for(int j=0;j<n;j+=2){
                    const double s0=sig[j*st],s1=sig[(j+1)*st],s2=sig[(j+2)*st];
                    const long double tm=tau+H*(5*s0+8*s1-s2)/12,tb=tau+H*(s0+4*s1+s2)/3;
                    for(int c=0;c<3;++c){const long double gm=g(j+1,tm,c),gb=g(j+2,tb,c);L[c]+=H*(ga[c]+4*gm+gb)/3;ga[c]=gb;}
                    tau=tb;
                }
                out[i].T[lev]=std::exp(-(double)tau);for(int c=0;c<3;++c)out[i].L[lev][c]=L[c];
            }
        }};
    const auto t0=std::chrono::steady_clock::now();
    std::vector<std::thread> pool;for(int t=0;t<threads;++t)pool.emplace_back(work);for(auto& t:pool)t.join();
    const double sec=std::chrono::duration<double>(std::chrono::steady_clock::now()-t0).count();
    std::printf("RAYS %d, finest 2^%d intervals, %.1f s on %d threads, %.3g field evaluations\n",count,log2N,sec,threads,double(evals.load()));
    // Convergence: per-ray spread between the two finest levels (and the next pair, for order).
    double maxT01=0,maxL01=0,maxT12=0,maxL12=0,meanL01=0;
    for(int i=0;i<count;++i){maxT01=std::max(maxT01,std::abs(out[i].T[0]-out[i].T[1]));maxT12=std::max(maxT12,std::abs(out[i].T[1]-out[i].T[2]));
        for(int c=0;c<3;++c){const double d01=std::abs(out[i].L[0][c]-out[i].L[1][c]);maxL01=std::max(maxL01,d01);meanL01+=d01;maxL12=std::max(maxL12,std::abs(out[i].L[1][c]-out[i].L[2][c]));}}
    std::printf("CONVERGENCE finest vs half: max |ΔT| %.3g, max |ΔL| %.3g, mean |ΔL| %.3g;  half vs quarter: max |ΔT| %.3g, max |ΔL| %.3g\n",maxT01,maxL01,meanL01/(3.0*count),maxT12,maxL12);
    // Native outputs against the reference: columns 7..22 = current, CF4 group4, midpoint4096, midpoint8192 (T,Lr,Lg,Lb).
    const char* names[4]={"native current","CF4 group4","native midpoint 4096","native midpoint 8192"};
    for(int q=0;q<4;++q){double mT=0,mL=0;std::vector<double> eT,eL;
        for(int i=0;i<count;++i){const auto& r=rays[i];const int base=7+4*q;
            const double dT=std::abs(r[base].get<double>()-out[i].T[0]);mT+=dT;eT.push_back(dT);
            for(int c=0;c<3;++c){const double dL=std::abs(r[base+1+c].get<double>()-out[i].L[0][c]);mL+=dL;eL.push_back(dL);}}
        std::sort(eT.begin(),eT.end());std::sort(eL.begin(),eL.end());
        std::printf("VS REFERENCE %-22s T mean %.3g p99 %.3g max %.3g | RGB mean %.3g p99 %.3g max %.3g (max %.2f 8-bit levels)\n",names[q],
            mT/count,eT[eT.size()*99/100],eT.back(),mL/(3.0*count),eL[eL.size()*99/100],eL.back(),255*eL.back());}
    if(outPath){std::ofstream o(outPath);o<<std::setprecision(17)<<"{\"columns\":[\"T\",\"Lr\",\"Lg\",\"Lb\",\"T_half\",\"Lr_half\",\"Lg_half\",\"Lb_half\"],\"rows\":[";
        for(int i=0;i<count;++i){o<<(i?",":"")<<"["<<out[i].T[0]<<","<<out[i].L[0][0]<<","<<out[i].L[0][1]<<","<<out[i].L[0][2]<<","<<out[i].T[1]<<","<<out[i].L[1][0]<<","<<out[i].L[1][1]<<","<<out[i].L[1][2]<<"]";}
        o<<"]}\n";}
    return 0;
}
