// Scratch fourth-order preview probe (scratch/probes; never in CMake): Claude Code / Opus 5.5 /
// f3af84cf / 2026-10-21. UNCERTIFIED estimate of the σ samples per inside-view pixel a
// certified Simpson march would need if its bound used sup|σ⁗| (fourth order) instead
// of sup|σ″| (second order), per smooth run (kinks and Perlin lattice planes split),
// with derivative constants either along the ray ("tight") or worst over each crossed
// 4-unit cell ("cell"). Depth budget 0.003, optimally allocated across runs.
#define main unused_ray_witness_main
#include "../../tests/singularity/webgpu_line_quadrature_test.cpp"
#undef main
#include <cstdio>
#include <random>
int main(int argc,char**argv){
    const int grid=argc>1?std::atoi(argv[1]):8;const double eps=0.003;
    namespace fs=std::filesystem;fs::path dir="saves/zones/Northern Veil";nlohmann::json zone;
    if(fs::exists(dir/"zone.json")){std::ifstream in(dir/"zone.json");zone=nlohmann::json::parse(in);}
    else{std::ifstream in(dir/"zone.ecform",std::ios::binary);std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)),{});zone=nlohmann::json::parse(nlohmann::json::from_msgpack(b).at("MigrationRoot").get<std::string>());}
    const glm::dvec3 eye(0,50,75),target(40,62,110);
    const glm::dvec3 fwd=glm::normalize(target-eye),right=glm::normalize(glm::cross(fwd,glm::dvec3(0,1,0))),up=glm::cross(right,fwd);
    const double ty=std::tan(glm::radians(30.0)),tx=ty*2560.0/1440.0;
    const int P=grid*grid;
    // per pixel sums: [0] 4th tight [1] 4th cell [2] 2nd tight [3] 2nd cell; runs
    std::vector<std::array<double,4>> perPixel(P,{0,0,0,0});std::vector<double> runsPerPixel(P,0);std::vector<int> touched(P,0);
    std::mt19937 rng(7);std::uniform_real_distribution<double> un(0,1);
    for(const auto& json:zone.at("spatialFields")){
        auto field=geom::FieldNode::fromJson(json);Rendering::VolumeDensityBinding b;
        if(!field || !Rendering::readVolumeDensity(*field,0,0,b) || !b.extinctionExpr)continue;
        const auto* sigma=b.extinctionExpr->pieces[0].mathNode.get();
        const glm::dvec3 half=glm::abs(glm::dvec3(b.scale)),o=eye-glm::dvec3(b.origin);
        auto f=[&](const glm::dvec3& q){const glm::vec3 p(q);
            const std::map<std::string,PropertyValue> vars{{"p",PropertyValue(p)},{"x",PropertyValue(double(p.x))},{"y",PropertyValue(double(p.y))},{"z",PropertyValue(double(p.z))},
                {OntoMath::kTimeVar,PropertyValue(0.0)},{OntoMath::kOmegaXVar,PropertyValue(0.0)},{OntoMath::kOmegaYVar,PropertyValue(0.0)},{OntoMath::kOmegaZVar,PropertyValue(-1.0)}};
            const auto v=sigma->evaluate(vars);double r=0;if(v)propertyValueToNumber(*v,r);return r;};
        // 4th and 2nd directional differences at two scales; smooth when the scales agree.
        struct D{double d4,d2;bool smooth;};
        auto diff=[&](const glm::dvec3& x,const glm::dvec3& d)->D{
            auto at=[&](double h,double*g){for(int k=-2;k<=2;++k)g[k+2]=f(x+double(k)*h*d);};
            double a[5],c[5];const double H=0.25,h=0.125;at(H,a);at(h,c);
            const double d4a=(a[0]-4*a[1]+6*a[2]-4*a[3]+a[4])/(H*H*H*H),d4c=(c[0]-4*c[1]+6*c[2]-4*c[3]+c[4])/(h*h*h*h);
            const double d2a=(a[1]-2*a[2]+a[3])/(H*H),d2c=(c[1]-2*c[2]+c[3])/(h*h);
            const bool smooth=std::abs(d4c-d4a)<=0.5*std::max(std::abs(d4a),std::abs(d4c))+2e-4 && std::abs(d2c-d2a)<=0.25*std::max(std::abs(d2a),std::abs(d2c))+2e-5;
            return {std::abs(d4c),std::abs(d2c),smooth};};
        std::map<std::array<long long,3>,std::pair<double,double>> cellMax;   // per (cell, direction pixel) cache is per pixel; reset per pixel
        for(int py=0;py<grid;++py)for(int px=0;px<grid;++px){
            const int pix=py*grid+px;
            const double x=2.0*(px+0.5)/grid-1,y=1-2.0*(py+0.5)/grid;
            const glm::dvec3 d=glm::normalize(fwd+x*tx*right+y*ty*up);
            double t0=0,t1=1e30;
            for(int i=0;i<3;++i){double a=(-half[i]-o[i])/d[i],c=(half[i]-o[i])/d[i];t0=std::max(t0,std::min(a,c));t1=std::min(t1,std::max(a,c));}
            if(!(t1>t0))continue;
            // Walk the span: runs of positive, smooth points.
            const double step=0.05;std::vector<std::array<double,4>> runs;   // {start,end,M4,M2}
            // A non-smooth stretch (stencils straddling a kink or lattice plane) is ONE
            // boundary at its midpoint; neighbouring smooth runs extend to it and the
            // boundary costs one located sliver (2 samples), as the device march does.
            bool inRun=false,gap=false;double gapStart=0;std::array<double,4> cur{};int slivers=0;
            cellMax.clear();
            for(double t=t0;t<=t1;t+=step){
                const glm::dvec3 q=o+t*d;
                if(f(q)<=0){if(inRun){runs.push_back(cur);inRun=false;}gap=false;continue;}
                const D dd=diff(q,d);
                if(!dd.smooth){if(!gap){gap=true;gapStart=t;}continue;}
                if(gap){const double mid=0.5*(gapStart+t);if(inRun){cur[1]=mid;runs.push_back(cur);}cur={mid,t,0,0};inRun=true;gap=false;++slivers;}
                if(!inRun){cur={t,t,0,0};inRun=true;}
                cur[1]=t;cur[2]=std::max(cur[2],dd.d4);cur[3]=std::max(cur[3],dd.d2);
            }
            if(inRun)runs.push_back(cur);
            if(runs.empty())continue;
            ++touched[pix];
            // Cell-wide constants: worst smooth directional differences at 24 random points per crossed cell.
            auto cellConst=[&](double ta,double tb){double m4=0,m2=0;
                for(double t=ta;t<=tb+1e-9;t+=1.0){const glm::dvec3 q=o+t*d;
                    const std::array<long long,3> c{(long long)std::floor(q.x/4),(long long)std::floor(q.y/4),(long long)std::floor(q.z/4)};
                    auto it=cellMax.find(c);
                    if(it==cellMax.end()){double a4=0,a2=0;
                        for(int k=0;k<24;++k){const glm::dvec3 r(4.0*(c[0]+un(rng)),4.0*(c[1]+un(rng)),4.0*(c[2]+un(rng)));
                            if(f(r)<=0)continue;const D dd=diff(r,d);if(!dd.smooth)continue;a4=std::max(a4,dd.d4);a2=std::max(a2,dd.d2);}
                        it=cellMax.emplace(c,std::make_pair(a4,a2)).first;}
                    m4=std::max(m4,it->second.first);m2=std::max(m2,it->second.second);}
                return std::make_pair(m4,m2);};
            // Optimal allocation. 4th: E_i=L^5 M/(2880 n^4); 2nd (the march's form): E_i=2.25 M L^3/(8 n^2).
            auto samples4=[&](const std::vector<std::pair<double,double>>& lm){double A=0;for(auto& [L,M]:lm)A+=std::pow(std::pow(L,5)*M/2880.0,0.2);
                const double k=std::pow(A/eps,0.25);double s=0;for(auto& [L,M]:lm){const double n=std::max(1.0,std::ceil(k*std::pow(std::pow(L,5)*M/2880.0,0.2)));s+=2*n+1;}return s;};
            auto samples2=[&](const std::vector<std::pair<double,double>>& lm){double A=0;for(auto& [L,M]:lm)A+=std::pow(2.25*M*L*L*L/8.0,1.0/3);
                const double k=std::sqrt(A/eps);double s=0;for(auto& [L,M]:lm){const double n=std::max(1.0,std::ceil(k*std::pow(2.25*M*L*L*L/8.0,1.0/3)));s+=2*n+1;}return s;};
            std::vector<std::pair<double,double>> t4,c4,t2,c2;
            for(auto& r:runs){const double L=std::max(r[1]-r[0],step);const auto cc=cellConst(r[0],r[1]);
                t4.push_back({L,r[2]});t2.push_back({L,r[3]});c4.push_back({L,std::max(cc.first,r[2])});c2.push_back({L,std::max(cc.second,r[3])});}
            perPixel[pix][0]+=samples4(t4)+2*slivers;perPixel[pix][1]+=samples4(c4)+2*slivers;
            perPixel[pix][2]+=samples2(t2)+2*slivers;perPixel[pix][3]+=samples2(c2)+2*slivers;runsPerPixel[pix]+=runs.size();
        }
        std::fprintf(stderr,"done %s\n",b.producerId.c_str());
    }
    std::vector<double> col[4];double runs=0;int n=0;
    for(int i=0;i<P;++i)if(touched[i]){for(int k=0;k<4;++k)col[k].push_back(perPixel[i][k]);runs+=runsPerPixel[i];++n;}
    const char* names[4]={"4th order, constants along the ray (tight)","4th order, worst over each crossed 4-unit cell","2nd order, tight","2nd order, cell-wide (the march's form, no interval overestimation)"};
    for(int k=0;k<4;++k){std::sort(col[k].begin(),col[k].end());double s=0;for(double v:col[k])s+=v;
        std::printf("%-72s mean %7.1f  median %7.1f  p90 %7.1f σ samples/pixel\n",names[k],s/n,col[k][n/2],col[k][n*9/10]);}
    std::printf("%d pixels; mean %.1f smooth runs per pixel (all curtains). Reference points: oracle ~8.5, native ~29, measured certified march ~1000.\n",n,runs/n);
}
