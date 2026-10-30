// Scratch oracle probe (scratch/probes; never in CMake): Claude Code / Opus 5.5 / f3af84cf / 2026-10-20.
// Minimal uniform Simpson panels over each curtain's occupied span whose TRUE
// T error is ≤ 0.003, for inside-view pixels. A floor for any sampling method.
#define main unused_ray_witness_main
#include "../../tests/singularity/webgpu_line_quadrature_test.cpp"
#undef main
#include <cstdio>
int main(){
    namespace fs=std::filesystem;fs::path dir="saves/zones/Northern Veil";nlohmann::json zone;
    if(fs::exists(dir/"zone.json")){std::ifstream in(dir/"zone.json");zone=nlohmann::json::parse(in);}
    else{std::ifstream in(dir/"zone.ecform",std::ios::binary);std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)),{});zone=nlohmann::json::parse(nlohmann::json::from_msgpack(b).at("MigrationRoot").get<std::string>());}
    const glm::dvec3 eye(0,50,75),target(40,62,110);
    const glm::dvec3 fwd=glm::normalize(target-eye),right=glm::normalize(glm::cross(fwd,glm::dvec3(0,1,0))),up=glm::cross(right,fwd);
    const double ty=std::tan(glm::radians(30.0)),tx=ty*2560.0/1440.0;
    std::vector<double> perPixel(64,0.0);std::vector<int> occupiedCurtains(64,0);
    for(const auto& json:zone.at("spatialFields")){
        auto field=geom::FieldNode::fromJson(json);Rendering::VolumeDensityBinding b;
        if(!field || !Rendering::readVolumeDensity(*field,0,0,b) || !b.extinctionExpr)continue;
        const auto* sigma=b.extinctionExpr->pieces[0].mathNode.get();
        const glm::dvec3 half=glm::abs(glm::dvec3(b.scale)),o=eye-glm::dvec3(b.origin);
        std::vector<int> need;
        for(int py=0;py<8;++py)for(int px=0;px<8;++px){
            const double x=2.0*(px+0.5)/8-1,y=1-2.0*(py+0.5)/8;
            const glm::dvec3 d=glm::normalize(fwd+x*tx*right+y*ty*up);
            double t0=0,t1=1e30;
            for(int i=0;i<3;++i){double a=(-half[i]-o[i])/d[i],c=(half[i]-o[i])/d[i];t0=std::max(t0,std::min(a,c));t1=std::min(t1,std::max(a,c));}
            if(!(t1>t0))continue;
            auto at=[&](double t){const glm::vec3 q(o+t*d);
                const std::map<std::string,PropertyValue> vars{{"p",PropertyValue(q)},{"x",PropertyValue(double(q.x))},{"y",PropertyValue(double(q.y))},{"z",PropertyValue(double(q.z))},
                    {OntoMath::kTimeVar,PropertyValue(0.0)},{OntoMath::kOmegaXVar,PropertyValue(0.0)},{OntoMath::kOmegaYVar,PropertyValue(0.0)},{OntoMath::kOmegaZVar,PropertyValue(-1.0)}};
                const auto v=sigma->evaluate(vars);double r=0;if(v)propertyValueToNumber(*v,r);return r;};
            // Occupied span and reference from a fine scan.
            const int N=20000;double first=-1,last=-1;std::vector<double> f(N+1);
            for(int k=0;k<=N;++k){f[k]=at(t0+(t1-t0)*k/N);if(f[k]>0){if(first<0)first=t0+(t1-t0)*std::max(0,k-1)/N;last=t0+(t1-t0)*std::min(N,k+1)/N;}}
            if(first<0)continue;
            double tau=0;for(int k=0;k<N;k+=2)tau+=(t1-t0)/N*(f[k]+4*f[k+1]+f[k+2])/3;
            const double Tref=std::exp(-tau);
            int n=2;
            for(;n<=4096;n*=2){const double h=(last-first)/n;double s=0;for(int k=0;k<n;k+=2)s+=h*(at(first+k*h)+4*at(first+(k+1)*h)+at(first+(k+2)*h))/3;
                if(std::abs(std::exp(-s)-Tref)<=0.003)break;}
            const int pix=py*8+px;perPixel[pix]+=n+1;++occupiedCurtains[pix];need.push_back(n+1);
        }
        std::sort(need.begin(),need.end());
        if(!need.empty())std::printf("%s: %zu occupied of 64 pixels; σ samples needed (oracle, uniform Simpson over the occupied span, true |ΔT|≤0.003): median %d, p90 %d, max %d\n",
            b.producerId.c_str(),need.size(),need[need.size()/2],need[need.size()*9/10],need.back());
    }
    double sum=0;int pixels=0;for(int i=0;i<64;++i)if(occupiedCurtains[i]){sum+=perPixel[i];++pixels;}
    std::printf("summed over curtains: mean %.1f σ samples per occupied pixel (%d pixels)\n",pixels?sum/pixels:0.0,pixels);
}
