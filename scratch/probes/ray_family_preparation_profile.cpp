// Scratch profiling probe: Codex/GPT-6.1 Sol/session 01a148a7-bd85-7550-a416-0e859a729c1a/2026-10-20.
#define main unused_ray_witness_main
#include "../../tests/singularity/webgpu_line_quadrature_test.cpp"
#undef main
#include <unistd.h>
int main(){
    namespace fs=std::filesystem;fs::path dir="saves/zones/Northern Veil";nlohmann::json zone;
    if(fs::exists(dir/"zone.json")){std::ifstream in(dir/"zone.json");zone=nlohmann::json::parse(in);}
    else{std::ifstream in(dir/"zone.ecform",std::ios::binary);std::vector<uint8_t> b((std::istreambuf_iterator<char>(in)),{});zone=nlohmann::json::parse(nlohmann::json::from_msgpack(b).at("MigrationRoot").get<std::string>());}
    for(const auto& json:zone.at("spatialFields")){
        auto field=geom::FieldNode::fromJson(json);Rendering::VolumeDensityBinding b;
        if(!field || !Rendering::readVolumeDensity(*field,0,0,b) || !b.extinctionExpr)continue;
        const auto* sigma=b.extinctionExpr->pieces[0].mathNode.get();auto zero=constant(0);
        const std::map<std::string,std::pair<double,double>> symbols{{OntoMath::kTimeVar,{0,3}},{OntoMath::kOmegaXVar,{-1,1}},{OntoMath::kOmegaYVar,{-1,1}},{OntoMath::kOmegaZVar,{-1,1}}};
        OntoMath::RegionCertificate sc(*sigma,Rendering::kVolumeExecutionModel,4,symbols),zc(*zero,Rendering::kVolumeExecutionModel,4,symbols);
        const OntoMath::LineRayDomain domain{{-0.015625,-0.015625,-0.015625},{0.015625,0.015625,0.015625},{0,0,0.99609375},{0,0,1.00390625}};
        std::cout<<"PID "<<getpid()<<std::endl;const auto start=std::chrono::steady_clock::now();
        for(int i=0;i<100;++i){const auto plan=OntoMath::prepareRegionalLineTransport(sc,zc,domain,double(float(-std::abs(b.scale.z))),double(float(std::abs(b.scale.z))),128);if(!plan)return 2;}
        std::cout<<"100 preparations "<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count()<<" ms"<<std::endl;return 0;
    }return 3;
}
