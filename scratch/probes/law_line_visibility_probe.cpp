// Zach's continuous Create visibility report. Test-only saves and Person.
// Codex / GPT-6.1 Sol / 01a10992-828e-7e80-890c-c64b09141e18 / 2026-10-05.
// Run with law_line_visibility_probe.py; this never edits inhabited saves.
#include "../../tests/support/test_harness.hpp"
#include "Singularity/Terminal/TerminalChannel.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/WgpuDevice.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Screen/ScreenRecorder.hpp"
#include "../../third_party/stb/stb_image.h"
#include <glm/gtc/matrix_transform.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <chrono>
#include <array>

extern ZoneManager mgr; // same production register used by EngineRender.cpp

void require(bool yes, const char* why) {
    if (!yes) { std::cerr << "FAIL: " << why << '\n'; std::exit(1); }
    std::cout << "PASS: " << why << '\n';
}
struct Mapping { bool done=false, ok=false; };
void mapped(WGPUMapAsyncStatus status, WGPUStringView, void* data, void*) {
    auto& m=*static_cast<Mapping*>(data); m.done=true; m.ok=status==WGPUMapAsyncStatus_Success;
}
int main(int argc,char** argv) {
    std::cout.setf(std::ios::unitbuf);
    const bool skyStairway=argc>1 && std::string(argv[1])=="--stairway";
    const bool fullEngine=skyStairway || (argc>1 && std::string(argv[1])=="--engine");
    const auto sourceRoot=fullEngine?std::filesystem::path(argv[2]):std::filesystem::current_path();
    const auto saves=sourceRoot/"saves";
    const auto stage=std::filesystem::temp_directory_path()/
        ("earthcall-law-visibility-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::filesystem::create_directories(stage/"zones/LawLine");
    const auto doc=SaveSystem::readSaveData((saves/"zones/LawLine/zone.json").string());
    std::filesystem::copy_file(saves/"zones/LawLine/zone.json",stage/"zones/LawLine/zone.json");
    std::filesystem::create_directories(stage/"zones/World");
    std::filesystem::copy_file(saves/"zones/World/zone.json",stage/"zones/World/zone.json");
    for (const auto& ref:doc["lawRefs"]) {
        const auto id=ref.get<std::string>();
        std::filesystem::create_directories(stage/"laws"/id);
        std::filesystem::copy_file(saves/"laws"/id/"law.json",stage/"laws"/id/"law.json");
    }
    SaveSystem::setSaveRoot(stage.string());
    if(fullEngine) {
        // First seed of an uninhabited fixture; never copy Person keys.
        std::filesystem::create_directories(stage/"persons");
        std::ofstream(stage/"persons/Zach.json")<<nlohmann::json{
            {"displayName","Zach"},{"soulName","Zach"},{"position",{0,5,3}},
            {"velocity",{0,0,0}},{"body",{{"shape","humanoid"},{"artStyle","default"},{"height",1.8}}},
            {"injected_by","Codex / GPT-6.1 Sol / test-only visibility fixture"}}.dump();
        auto& engine=Core::Engine::instance();
        require(engine.init(1,argv),"isolated full production Engine initializes");
        auto* person=engine.getPerson();auto* camera=engine.getCamera();auto* laws=engine.getLawManager();
        auto* terminal=Singularity::Terminal::TerminalChannel::find(*laws);
        auto* recorder=Singularity::Screen::ScreenRecorder::find(*laws);
        require(person && camera && terminal && recorder,"full Engine exposes Person, Terminal and recorder");
        require(mgr.switchTo(mgr.findZoneIndex("LawLine")),"full Engine enters visible LawLine");
        Physics::setFlying(true);
        person->position()=glm::vec3(0,5,3);
        camera->setPos(person->position()+glm::vec3(0,person->getBody().getEyeHeight(),0));
        camera->setFront(glm::vec3(0,0,-1));
        if (skyStairway) {
            // Only the copied test seed is repositioned. Use an oblique camera
            // to witness the whole authored tower, rather than a single face.
            person->position()=glm::vec3(8,8,11);
            camera->setPos(person->position()+glm::vec3(0,person->getBody().getEyeHeight(),0));
            const auto front=glm::normalize(glm::vec3(1,5,1)-camera->getPos());
            camera->setFront(front);
            // Engine update derives its camera from MouseHandler's actual
            // yaw/pitch. Set the fixture's sensed orientation too; a direct
            // Camera front alone is replaced on the next native frame.
            engine.getMouseHandler()->setYaw(glm::degrees(std::atan2(front.z,front.x)));
            engine.getMouseHandler()->setPitch(glm::degrees(std::asin(front.y)));
        }
        terminal->setSink([](const std::string& s){std::cout<<s<<'\n';});
        engine.tick(.016f);
        lawSetValue(*recorder,PropertyPath::parse("outputPath"),(stage/"capture").string());
        lawSetValue(*recorder,PropertyPath::parse("recordCursor"),false);
        auto capture=[&](const char* label) {
            lawSetValue(*recorder,PropertyPath::parse("snapshot"),true);engine.tick(.016f);
            PropertyValue path;lawGetValue(*recorder,PropertyPath::parse("lastSnapshotPath"),path);
            require(std::holds_alternative<std::string>(path),"full Engine viewport capture returns a path");
            int w=0,h=0,c=0;auto* bytes=stbi_load(std::get<std::string>(path).c_str(),&w,&h,&c,4);
            require(bytes && w>0 && h>0,"full Engine captured PNG decodes");
            size_t gold=0;
            for(int y=h/4;y<3*h/4;++y)for(int x=w/4;x<3*w/4;++x) {
                const auto* p=bytes+(y*w+x)*4;
                gold+=p[0]>60 && p[1]>40 && p[2]<p[1]*.5;
            }
            stbi_image_free(bytes);
            std::filesystem::copy_file(std::get<std::string>(path),stage/(std::string(label)+".png"),std::filesystem::copy_options::overwrite_existing);
            std::cout<<"ENGINE_CAPTURE "<<label<<" width="<<w<<" height="<<h<<" centre_gold_pixels="<<gold<<'\n';
            return gold;
        };
        const auto baseline=capture("engine-before");
        const auto before=mgr.active().getOwnedObjects().size();
        if (skyStairway) {
            Object* launcher=nullptr;
            for (const auto& object:mgr.active().getOwnedObjects())
                if(object->getIdentifier()=="law-line-cube") launcher=object.get();
            require(launcher,"copied Stairmaker seed exists in the rendered Zone");
            auto submit=[&](const char* file) {
                std::ifstream input(sourceRoot/"examples"/file);std::string line;std::getline(input,line);
                require(!line.empty(),"actual one-line Stairmaker example is read");
                terminal->inject(line);engine.tick(.016f);engine.tick(.016f);
            };
            auto* interaction=Singularity::Input::InteractionChannel::find(*laws);
            require(interaction,"full Engine exposes the production pointer channel");
            auto click=[&](Object* object) {
                Singularity::Input::InteractionChannel::Sense pointer;
                pointer.rayOrigin=object->getPosition()+glm::vec3(0,0,5);
                pointer.rayDirection=glm::vec3(0,0,-1);
                for(bool pressed:{false,true,false}) {
                    pointer.left=pressed;interaction->observePending(pointer,{object});laws->tick();
                }
            };
            submit("law_line_stairway.txt");click(launcher);
            require(mgr.active().getOwnedObjects().size()==before+3,"original Stairmaker makes its three launch steps");
            auto* first=mgr.active().getOwnedObjects()[before].get();
            submit("law_line_sky_stairway.txt");click(first);
            require(mgr.active().getOwnedObjects().size()==before+11,"sky add-on creates eight jewels in the full Engine");
            auto* crown=mgr.active().getOwnedObjects().back().get();click(crown);
            require(mgr.active().getOwnedObjects().size()==before+19,"a crown creates the next tower turn in the full Engine");
            auto* jewel=mgr.active().getOwnedObjects()[before+3].get();
            const auto restRotation=jewel->getRotationEulerDegrees();
            Singularity::Input::InteractionChannel::Sense hover;
            hover.rayOrigin=jewel->getPosition()+glm::vec3(0,0,5);
            hover.rayDirection=glm::vec3(0,0,-1);
            interaction->observePending(hover,{jewel});laws->tick();
            require(std::abs(jewel->getRotationEulerDegrees().y-restRotation.y-25)<.001f,
                    "native pointer hover turns a jewel by 25 degrees");
            hover.rayOrigin=glm::vec3(100,100,100);
            interaction->observePending(hover,{jewel});laws->tick();
            require(glm::length(jewel->getRotationEulerDegrees()-restRotation)<.001f,
                    "native pointer departure restores the jewel orientation");
            const auto after=capture("engine-after");
            require(after>baseline+200,"native Engine viewport contains the new gold highlights");
            auto* world=mgr.zones()[mgr.findZoneIndex("World")].get();
            require(world->getOwnedObjects().empty(),"sky births remain in the rendered Zone");
            std::cout<<"SKY_STAIRWAY_ENGINE_RESULT PASS created=19 before_gold="<<baseline<<" after_gold="<<after
                     <<" evidence="<<stage<<'\n';
            engine.shutdown();SaveSystem::setSaveRoot("");return 0;
        }
        std::ifstream input(sourceRoot/"examples/law_line_visible_probe.txt");std::string line;std::getline(input,line);
        terminal->inject(line);engine.tick(.016f);engine.tick(.016f);
        require(mgr.active().getOwnedObjects().size()==before+1,"full Engine creates the probe in its rendered Zone");
        auto* world=mgr.zones()[mgr.findZoneIndex("World")].get();
        require(world->getOwnedObjects().empty(),"full Engine leaves inactive World empty");
        const auto after=capture("engine-after");
        require(after>baseline+200,"full Engine viewport really contains the new gold cube");
        std::cout<<"LAW_LINE_ENGINE_VISIBILITY_RESULT PASS before_gold="<<baseline<<" after_gold="<<after
                 <<" evidence="<<stage<<'\n';
        engine.shutdown();SaveSystem::setSaveRoot("");return 0;
    }
    TestSupport::BootedEngineHarness h("Zach");
    h.interaction->setPointingPerson(&h.player);
    Singularity::Terminal::TerminalChannel::syncRegister(h.lawManager);
    auto* terminal=Singularity::Terminal::TerminalChannel::find(h.lawManager);
    require(terminal && h.zones.switchTo(h.zones.findZoneIndex("LawLine")),"isolated LawLine boots");
    terminal->setSink([](const std::string& s){std::cout << s << '\n';});
    // Hold the seeded legacy-author closure before assigning a fixture key,
    // matching the established Terminal presence in law_line_zone_test.
    terminal->sense(h.lawManager);for(int i=0;i<3;++i)h.lawManager.tick();terminal->act(h.lawManager);
    std::array<uint8_t,32> key{}; key.fill(42);
    require(h.player.setPersonId(Identity::SingularId::fromPublicKey(key)),"test Person has keyed identity");
    h.player.position()=glm::vec3(0,0,3);
    std::ifstream input("examples/law_line_cubes_below.txt");
    std::string line; std::getline(input,line);
    terminal->inject(line); terminal->sense(h.lawManager);
    for(int i=0;i<3;++i)h.lawManager.tick(); terminal->act(h.lawManager);
    auto& objects=h.zones.active().getOwnedObjects();
    const auto count=objects.size(); h.lawManager.tick();
    auto* inactiveWorld=h.zones.zones()[h.zones.findZoneIndex("World")].get();
    std::cout<<"birth routing active="<<h.zones.active().getIdentifier()<<" active_count="<<objects.size()
             <<" inactive_World_count="<<inactiveWorld->getOwnedObjects().size()<<'\n';
    require(objects.size()==count+1,"Terminal-authored continuous Law creates a cube");
    auto* cube=objects.back().get();
    require(glm::length(cube->getPosition()-glm::vec3(0,-3,3))<.001f,"created cube is three units beneath the feet");
    for(const auto& law:h.lawManager.getAll()) if(law->name()=="Cubes Below Me") law->setEnabled(false);

    wgpu::Device gpu; require(gpu.init(),"native WebGPU device acquired");
    WebGpuRenderer renderer; require(renderer.init(gpu,WGPUTextureFormat_RGBA8Unorm),"production renderer initialized");
    setCurrentRenderer(&renderer);
    constexpr uint32_t W=64,H=64;
    WGPUTextureDescriptor td={}; td.usage=WGPUTextureUsage_RenderAttachment|WGPUTextureUsage_CopySrc;
    td.dimension=WGPUTextureDimension_2D; td.size={W,H,1}; td.format=WGPUTextureFormat_RGBA8Unorm; td.mipLevelCount=td.sampleCount=1;
    auto texture=wgpuDeviceCreateTexture(gpu.device,&td); auto view=wgpuTextureCreateView(texture,nullptr);
    WGPUBufferDescriptor bd={}; bd.usage=WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead; bd.size=W*H*4;
    auto buffer=wgpuDeviceCreateBuffer(gpu.device,&bd);
    Object floor("visibility-fixture-floor"); floor.setShape(Object::ShapeKind::Cube);
    floor.setTransform(glm::translate(glm::mat4(1),glm::vec3(0,-.05f,3))*glm::scale(glm::mat4(1),glm::vec3(30,.1f,30)));
    // A camera at normal eye height looks straight down, so the cube is inside
    // the frustum. A physical floor's top is exactly at the Person's feet.
    const glm::vec3 eye(0,h.player.getBody().getEyeHeight(),3);
    glm::vec3 aim(0,-3,3);
    auto render=[&](bool drawCube,bool drawFloor,const char* filename){
        renderer.beginFrameOffscreen(view,W,H,glm::vec4(0,0,0,1));
        const glm::vec3 up = std::abs(aim.y-eye.y)>3 ? glm::vec3(0,0,-1) : glm::vec3(0,1,0);
        renderer.setCamera(glm::lookAt(eye,aim,up),
            glm::perspectiveZO(glm::radians(45.f),1.f,.1f,100.f),eye);
        if(drawFloor){renderer.setModel(floor.getTransform());floor.drawObject();}
        if(drawCube){renderer.setModel(cube->getTransform());cube->drawObject();}
        renderer.endFrame();
        auto encoder=wgpuDeviceCreateCommandEncoder(gpu.device,nullptr);
        WGPUTexelCopyTextureInfo src={};src.texture=texture;src.aspect=WGPUTextureAspect_All;
        WGPUTexelCopyBufferInfo dst={};dst.buffer=buffer;dst.layout.bytesPerRow=W*4;dst.layout.rowsPerImage=H;
        WGPUExtent3D extent={W,H,1};wgpuCommandEncoderCopyTextureToBuffer(encoder,&src,&dst,&extent);
        auto command=wgpuCommandEncoderFinish(encoder,nullptr);wgpuQueueSubmit(gpu.queue,1,&command);
        wgpuCommandBufferRelease(command);wgpuCommandEncoderRelease(encoder);
        Mapping mapping;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowProcessEvents;cb.callback=mapped;cb.userdata1=&mapping;
        wgpuBufferMapAsync(buffer,WGPUMapMode_Read,0,W*H*4,cb);
        while(!mapping.done)wgpuInstanceProcessEvents(gpu.instance);
        require(mapping.ok,"native pixels read back");
        const auto* bytes=static_cast<const uint8_t*>(wgpuBufferGetConstMappedRange(buffer,0,W*H*4));
        std::vector<uint8_t> pixels(bytes,bytes+W*H*4);wgpuBufferUnmap(buffer);
        std::ofstream image(stage/filename,std::ios::binary);image<<"P6\n64 64\n255\n";
        for(size_t i=0;i<pixels.size();i+=4)image.write(reinterpret_cast<const char*>(pixels.data()+i),3);
        return pixels;
    };
    const auto empty=render(false,false,"empty.ppm");
    const auto visible=render(true,false,"cube-without-floor.ppm");
    const auto floorOnly=render(false,true,"floor-only.ppm");
    const auto hidden=render(true,true,"cube-beneath-floor.ppm");
    size_t visibleDifferences=0,hiddenDifferences=0;
    for(size_t i=0;i<empty.size();++i){visibleDifferences+=empty[i]!=visible[i];hiddenDifferences+=floorOnly[i]!=hidden[i];}
    require(visibleDifferences>100,"the real created cube draws visible pixels without the floor");
    require(hiddenDifferences==0,"the floor hides every pixel of the same cube");
    // Zach also tried +3, which is overhead and can still be outside the
    // frustum. Test a separate one-shot, view-relative probe through the same
    // authored compilation path; preserve the requested below-feet Law.
    h.player.cameraPos=eye;h.player.cameraForward=glm::vec3(0,0,-1);
    std::ifstream probeInput("examples/law_line_visible_probe.txt");std::getline(probeInput,line);
    const auto beforeProbe=objects.size();
    terminal->inject(line);terminal->sense(h.lawManager);h.lawManager.tick();terminal->act(h.lawManager);
    h.lawManager.tick();
    require(objects.size()==beforeProbe+1,"view-relative one-shot sentence creates exactly one probe");
    cube=objects.back().get();aim=eye+glm::vec3(0,0,-3);
    require(glm::length(cube->getPosition()-aim)<.001f,"probe centre is exactly three units along the camera forward ray");
    h.lawManager.tick();require(objects.size()==beforeProbe+1,"view probe does not keep accumulating Objects");
    const auto frontFloor=render(false,true,"front-floor-only.ppm");
    const auto frontProbe=render(true,true,"front-probe.ppm");
    size_t frontDifferences=0;for(size_t i=0;i<frontFloor.size();++i)frontDifferences+=frontFloor[i]!=frontProbe[i];
    require(frontDifferences>100,"view-relative authored cube draws visible native pixels above the floor");
    std::cout<<"LAW_LINE_VISIBILITY_RESULT PASS visible_byte_changes="<<visibleDifferences
             <<" occluded_byte_changes="<<hiddenDifferences<<" front_probe_byte_changes="<<frontDifferences
             <<" camera_y="<<eye.y<<" evidence="<<stage<<'\n';
    wgpuBufferRelease(buffer);wgpuTextureViewRelease(view);wgpuTextureRelease(texture);
    renderer.shutdown();setCurrentRenderer(nullptr);SaveSystem::setSaveRoot("");
}
