// Astra's addition: native Zone closure, authored gesture, continuous opening,
// persistence, and actual WebGPU rendering. No write to the real SaveRoot.
// Codex / GPT-6 Astra, 01a07eb3-8ee7-7aa3-8b34-65fea2f4cd44,
// 2026-09-19T12:21:43-07:00. Zach commissioned this separate Cathedral place.
#include "support/test_harness.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/Core/EventBus.hpp"
#include "Singularity/Screen/RenderMaterial.hpp"
#include "Singularity/Screen/AuthorableLight.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/FieldNode.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/Screen/WebGPU/SdfWgsl.hpp"
#include "Singularity/Storage/Serialization/Serialization.hpp"
#include "ConstructedBeing/Singular/Object/Geometry/SdfJson.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ECA.hpp"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <iostream>
#include <map>
#include <cmath>

namespace fs = std::filesystem;
using json = nlohmann::json;
namespace {
const std::string prefix = "cathedral.astra.openhand.";
const std::string zoneId = "Cathedral of the Living Logos";
int failures = 0;
void check(bool ok, const std::string& label) {
    std::cout << (ok ? "PASS " : "FAIL ") << label << std::endl;
    if (!ok) ++failures;
}
double number(Singular& s, const std::string& path) {
    PropertyValue v;
    if (!lawGetValue(s, PropertyPath::parse(path), v)) return -999;
    if (auto d=std::get_if<double>(&v)) return *d;
    if (auto f=std::get_if<float>(&v)) return *f;
    if (auto i=std::get_if<int>(&v)) return *i;
    return -998;
}
json read(const fs::path& p) { std::ifstream f(p); json j; f>>j; return j; }
struct Scratch {
    fs::path path;
    ~Scratch() { SaveSystem::setSaveRoot(""); std::error_code ec; fs::remove_all(path,ec); }
};
struct MapResult { bool done=false; bool ok=false; };
void mapped(WGPUMapAsyncStatus s, WGPUStringView, void* u, void*) {
    auto* r=static_cast<MapResult*>(u);r->done=true;r->ok=s==WGPUMapAsyncStatus_Success;
}

// Render loaded Objects through the production WebGPU SDF path, not a mock-up.
// Output is optional and belongs outside SaveRoot; no changes to the live camera.
bool render(wgpu::Device& gpu, WebGpuRenderer& r, const std::vector<Object*>& objects,
            const fs::path& out, bool closeup=false) {
    constexpr uint32_t W=1024,H=768,ROW=W*4;
    WGPUTextureDescriptor td={};td.usage=WGPUTextureUsage_RenderAttachment|WGPUTextureUsage_CopySrc;
    td.dimension=WGPUTextureDimension_2D;td.size={W,H,1};td.format=WGPUTextureFormat_RGBA8Unorm;
    td.mipLevelCount=1;td.sampleCount=1;
    auto tex=wgpuDeviceCreateTexture(gpu.device,&td);
    auto view=wgpuTextureCreateView(tex,nullptr);
    const glm::vec3 eye=closeup ? glm::vec3(51,5.5,19) : glm::vec3(65,15,38);
    const glm::vec3 aim=closeup ? glm::vec3(44,4,10) : glm::vec3(44,4.8,10);
    r.setCamera(glm::lookAt(eye,aim,glm::vec3(0,1,0)),
                glm::perspectiveZO(glm::radians(46.f),float(W)/H,.1f,160.f),eye);
    r.beginFrameOffscreen(view,W,H,glm::vec4(.026,.039,.060,1));
    bool compiled=true;
    for (Object* o:objects) {
        auto mat=resolveRenderMaterial(o->materialId(),{});
        auto program=sdfwgsl::compile(o->getFieldData(),nullptr,mat.colorExpr.get(),r.radianceExpr());
        if (!program.ok) {std::cerr<<o->getIdentifier()<<": "<<program.error<<'\n';compiled=false;}
        r.setModel(o->getTransform());
        r.drawImplicit(o->getFieldData(),o->getFieldExtent(),mat);
    }
    r.endFrame();
    WGPUBufferDescriptor bd={};bd.usage=WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead;bd.size=ROW*H;
    auto buffer=wgpuDeviceCreateBuffer(gpu.device,&bd);
    auto enc=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);
    WGPUTexelCopyTextureInfo src={};src.texture=tex;src.aspect=WGPUTextureAspect_All;
    WGPUTexelCopyBufferInfo dst={};dst.buffer=buffer;dst.layout.bytesPerRow=ROW;dst.layout.rowsPerImage=H;
    WGPUExtent3D extent={W,H,1};wgpuCommandEncoderCopyTextureToBuffer(enc,&src,&dst,&extent);
    auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuQueueSubmit(gpu.queue,1,&cmd);
    MapResult mr;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;
    cb.callback=mapped;cb.userdata1=&mr;wgpuBufferMapAsync(buffer,WGPUMapMode_Read,0,ROW*H,cb);
    while (!mr.done) wgpuDevicePoll(gpu.device,true,nullptr);
    size_t visible=0;
    if (mr.ok) {
        auto* px=static_cast<const unsigned char*>(wgpuBufferGetConstMappedRange(buffer,0,ROW*H));
        std::ofstream image;
        if (!out.empty()) {image.open(out,std::ios::binary);image<<"P6\n"<<W<<" "<<H<<"\n255\n";}
        for (size_t i=0;i<W*H;++i) {
            if (px[i*4]>30 || px[i*4+1]>30 || px[i*4+2]>30) ++visible;
            if (image) image.write(reinterpret_cast<const char*>(px+i*4),3);
        }
        wgpuBufferUnmap(buffer);
    }
    wgpuCommandBufferRelease(cmd);wgpuCommandEncoderRelease(enc);wgpuBufferRelease(buffer);
    wgpuTextureViewRelease(view);wgpuTextureRelease(tex);
    return compiled && mr.ok && visible>25000;
}
}

int main(int argc,char** argv) {
    std::cout.setf(std::ios::unitbuf);
    fs::path source=argc>1 ? argv[1] : "saves";
    if (!fs::exists(source/"zones"/zoneId/"zone.json")) source="../saves";
    source=fs::absolute(source);
    fs::path captures=argc>2 ? argv[2] : "";
    if (!captures.empty()) fs::create_directories(captures);
    auto original=read(source/"zones"/zoneId/"zone.json");
    Scratch scratch{fs::temp_directory_path()/("earthcall-openhand-test-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
    fs::create_directories(scratch.path/"zones"/zoneId);
    fs::copy(source/"zones"/zoneId,scratch.path/"zones"/zoneId,fs::copy_options::recursive|fs::copy_options::overwrite_existing);
    for (const auto& ref:original["lawRefs"]) {
        auto id=ref.get<std::string>();
        fs::create_directories(scratch.path/"laws"/id);
        fs::copy_file(source/"laws"/id/"law.json",scratch.path/"laws"/id/"law.json");
    }
    SaveSystem::setSaveRoot(scratch.path.string());
    TestSupport::BootedEngineHarness h("Zach");h.zones.bindLive();
    size_t index=0;
    while(index<h.zones.zones().size() && h.zones.zones()[index]->getIdentifier()!=zoneId) ++index;
    check(index<h.zones.zones().size() && h.zones.switchTo(index),"native Cathedral boot without session file");
    if(failures) return 1;
    auto zone=h.zones.zones()[index];
    std::vector<Object*> ours;std::map<std::string,Object*> byId;
    for(auto& o:zone->getOwnedObjects()) if(o) {
        byId[o->getIdentifier()]=o.get();
        if(o->getIdentifier().rfind(prefix,0)==0) ours.push_back(o.get());
    }
    check(ours.size()==94,"all 94 new Objects survive native hydration");
    auto* touch=byId[prefix+"touchstone"];
    check(touch!=nullptr,"touchstone is a real loaded Object");if(!touch)return 1;
    for(const auto* suffix:{"gesture","approach","unfold"}) {
        auto* law=h.lawManager.find(std::string("law-astra-openhand-")+suffix);
        check(law && law->isAuthored() && law->isEnabled(),std::string("authored Law is live: ")+suffix);
        check(law && law->authors().findMemberByIdentifier("Zach")==&h.player,
              std::string("Law author resolves to the actual Person: ")+suffix);
    }
    auto tick=[&](double dt) {h.worldTime+=dt;Universe::instance().setClock(h.worldTime,dt);h.lawManager.tick();};
    tick(1./60);
    wgpu::Device gpu;
    check(gpu.init(),"WebGPU device available");if(failures)return 1;
    WebGpuRenderer renderer;check(renderer.init(gpu),"production renderer starts");setCurrentRenderer(&renderer);
    // Match EngineRender's production light handoff, including the Cathedral's
    // existing source position/chroma and authored scalar radiance definition.
    auto* root=zone->spatialRoot();
    Rendering::AuthorableLightState light;
    check(root && Rendering::readAuthorableLight(*root,light),"Cathedral's authored illumination resolves");
    if(root && Rendering::readAuthorableLight(*root,light)) {
        renderer.setLight(light.position,Rendering::lightAmbientRadiance(light),
                          Rendering::lightDiffuseRadiance(light),Rendering::lightSpecularRadiance(light));
        renderer.setLightingEnabled(light.enabled);
        if(root->field)renderer.setRadianceField(&root->field->astDefinition,1);
    }
    const bool hasCourtLight=original["world"]["objects"].end()!=std::find_if(
        original["world"]["objects"].begin(),original["world"]["objects"].end(),[](const auto& o){
            return o.value("objectID","")==prefix+"touchstone" && o["authoredProperties"].contains("courtLightAuthored");});
    if(hasCourtLight && root && root->field) {
        auto full=root->field->astDefinition;
        auto base=full;
        base.pieces[0].mathNode=std::make_shared<OntoMath::MathNode>(*full.pieces[0].mathNode->children[0]);
        auto sample=[&](const OntoMath::Piecewise& f,glm::vec3 world)->double {
            auto p=world-light.position;
            auto v=f.evaluate({{"p",p},{"x",double(p.x)},{"y",double(p.y)},{"z",double(p.z)}});
            if(!v)return -999;
            if(auto n=std::get_if<double>(&*v))return *n;
            if(auto n=std::get_if<float>(&*v))return *n;
            return -998;
        };
        bool unchanged=true;size_t checked=0;
        for(const auto& o:original["world"]["objects"])if(o.value("objectID","").rfind(prefix,0)!=0) {
            const auto& t=o["transform"];glm::vec3 p(t[12].template get<float>(),t[13].template get<float>(),t[14].template get<float>());
            double a=sample(base,p),b=sample(full,p);
            unchanged &= a>=0 && std::isfinite(a) && a==b;++checked;
        }
        for(auto p:{glm::vec3(31,5,10),glm::vec3(57,5,10),glm::vec3(44,5,-1),glm::vec3(44,5,25),glm::vec3(44,13,10)})
            unchanged &= sample(base,p)==sample(full,p);
        check(unchanged && checked>1000,"light contribution is zero at every prior Object center and outside court bounds");
        check(sample(full,{44,4.7,10})>sample(base,{44,4.7,10})+.5,"authored light strengthens the seed's surrounding region");
    }
    check(render(gpu,renderer,ours,captures.empty()?fs::path{}:captures/"gathered.ppm"),"gathered court renders on GPU");
    auto click=[&](Object* o) {Core::EventBus::instance().publish(ECA::Event{"object-clicked",o,nullptr,std::time(nullptr)});tick(1./60);};
    click(touch);
    check(number(*touch,"intention")==1,"one click expresses opening intention");
    tick(.2);
    check(number(*touch,"aperture")>0 && number(*touch,"aperture")<1,"aperture passes through an intermediate value");
    for(int i=0;i<180;++i)tick(1./60);
    check(number(*touch,"aperture")>.999,"opening reaches its authored destination");
    int leaves=0;
    for(auto* o:ours) if(o->getIdentifier().rfind(prefix+"leaf.",0)==0) {
        auto p=o->getPosition();float radius=glm::length(glm::vec2(p.x-44,p.z-10));
        if(std::abs(radius-4.8f)<.015f && std::abs(p.y-4.15f)<.015f)++leaves;
    }
    check(leaves==12,"all twelve leaves actually move to their open positions");
    check(render(gpu,renderer,ours,captures.empty()?fs::path{}:captures/"open.ppm"),"open court renders on GPU");
    check(render(gpu,renderer,ours,captures.empty()?fs::path{}:captures/"near.ppm",true),"near approach renders real curved surfaces");
    auto unrelated=byId.find("cathedral.floor.main");
    if(unrelated!=byId.end())click(unrelated->second);
    check(number(*touch,"intention")==1,"unrelated Cathedral click does not affect the court");
    check(h.zones.persistActiveZone(),"Save Zone succeeds inside temporary SaveRoot");
    auto saved=SaveSystem::readZoneIdentity(zoneId);
    if(root && root->field) {
        auto restoredRadiance=OntoMath::Piecewise::fromJson(saved["spatialRoot"]["field"]["astDefinition"]);
        check(restoredRadiance.toJson()==root->field->astDefinition.toJson(),
              "complete authored light field survives Save Zone and AST reload");
    }
    size_t count=0;bool shapesPreserved=true,provenancePreserved=true;
    if(!captures.empty()) {std::ofstream f(captures/"saved-zone.json");f<<saved.dump(2);}
    for(const auto& o:saved["world"]["objects"])if(o.value("objectID","").rfind(prefix,0)==0) {
        ++count;
        auto* live=byId.at(o["objectID"].get<std::string>());
        Object restored;from_json(o,restored);
        bool shapeEqual=geom::sdfToJson(restored.getFieldData())==geom::sdfToJson(live->getFieldData());
        if(!shapeEqual && shapesPreserved) {
            std::cerr<<"FIRST FIELD DIFFERENCE "<<live->getIdentifier()<<"\nBEFORE "<<geom::sdfToJson(live->getFieldData())
                     <<"\nAFTER "<<geom::sdfToJson(restored.getFieldData())<<'\n';
        }
        shapesPreserved &= shapeEqual;
        shapesPreserved &= glm::length(restored.getPosition()-live->getPosition())<.001f;
        shapesPreserved &= restored.materialId()==live->materialId();
        provenancePreserved &= o["authoredProperties"].contains("authorizingPerson") &&
            o["authoredProperties"]["authorizingPerson"]["v"]=="Zach";
    }
    check(count==ours.size(),"round-trip preserves all new beings");
    check(shapesPreserved,"reload preserves every new SDF, position, and Material reference");
    check(provenancePreserved,"persistent provenance names the authorizing Person on all new Objects");
    size_t fieldColors=0;
    for(const auto& m:saved["materials"])if(m.value("name","").rfind(prefix,0)==0 && m.contains("colorExpr"))++fieldColors;
    size_t expectedColors=0;
    for(const auto& m:original["materials"])if(m.value("name","").rfind(prefix,0)==0 && m.contains("colorExpr"))++expectedColors;
    check(expectedColors>0 && fieldColors==expectedColors,"all private Material color fields survive Save Zone");
    size_t authoredRelations=0;
    for(const auto& rel:saved["formationRelations"]) {
        auto a=rel.value("entityA","");
        if((a.rfind(prefix,0)==0 || a.rfind("material."+prefix,0)==0) &&
            rel.value("type","")=="commissioned-under" && rel.value("entityB","")=="lexeme.astra.openhand.commission") ++authoredRelations;
    }
    check(authoredRelations==188,"all Objects and Materials retain their named commission ("+std::to_string(authoredRelations)+")");
    auto savedTouch=std::find_if(saved["world"]["objects"].begin(),saved["world"]["objects"].end(),
        [&](const auto& o){return o.value("objectID","")==prefix+"touchstone";});
    check(savedTouch!=saved["world"]["objects"].end() && (*savedTouch)["authoredProperties"]["intention"]["v"]==1,
        "opened intention survives persistence");
    click(touch);tick(10.0);tick(1./60);
    check(number(*touch,"intention")==0 && std::abs(number(*touch,"aperture"))<.001,
        "second click gathers; even a ten-second frame does not overshoot");
    setCurrentRenderer(nullptr);
    check(read(source/"zones"/zoneId/"zone.json")==original,"real source Zone remains untouched by test");
    std::cout<<failures<<" failures\n";
    return failures?1:0;
}
