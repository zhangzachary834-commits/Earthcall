// Direct Screen authoring through the real Terminal -> Metalaw -> Law -> Engine path.
// Codex / GPT-6.1 Sol / 01a10992-828e-7e80-890c-c64b09141e18 / 2026-10-06.
// Isolated first-seed fixture; no inhabited saves or identity keys are written.
#include "Singularity/Core/Engine.hpp"
#include "Person/Person.hpp"
#include "Singularity/Terminal/TerminalChannel.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "Singularity/Screen/ScreenRecorder.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "../../third_party/stb/stb_image.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
extern ZoneManager mgr;
static void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
    std::cout << "PASS: " << message << '\n';
}
int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    require(argc == 2, "source repository argument supplied");
    const std::filesystem::path root = argv[1], saves = root / "saves";
    const auto stage = std::filesystem::current_path() / "saves";
    std::filesystem::create_directories(stage / "zones/LawLine");
    const auto native = saves / "zones/LawLine/zone.ecform";
    auto doc = SaveSystem::readSaveData((std::filesystem::exists(native) ? native : saves / "zones/LawLine/zone.json").string());
    std::ofstream(stage / "zones/LawLine/zone.json") << doc.dump();
    for (const auto& ref : doc["lawRefs"]) {
        const auto id = ref.get<std::string>();
        std::filesystem::create_directories(stage / "laws" / id);
        std::filesystem::copy_file(saves / "laws" / id / "law.json", stage / "laws" / id / "law.json");
    }
    std::filesystem::create_directories(stage / "persons");
    std::ofstream(stage / "persons/Zach.json") << nlohmann::json{
        {"displayName","Zach"},{"soulName","Zach"},{"position",{0,5,3}},
        {"velocity",{0,0,0}},{"body",{{"shape","humanoid"},{"height",1.8}}},
        {"injected_by","Codex / GPT-6.1 Sol / isolated direct Screen CLI fixture"}}.dump();
    SaveSystem::setSaveRoot(stage.string());
    auto& engine = Core::Engine::instance();
    require(engine.init(1,argv), "production WebGPU Engine initializes");
    auto& laws = *engine.getLawManager();
    auto* terminal = Singularity::Terminal::TerminalChannel::find(laws);
    auto* screen = Singularity::Screen::ScreenChannel::find(laws);
    auto* recorder = Singularity::Screen::ScreenRecorder::find(laws);
    require(terminal && screen && recorder, "Engine exposes all production channels");
    // Public identity only, matching the shared seed's author. This test
    // holds no human private key and uses the sanctioned C++ presence seam;
    // interactive key unlock is a separate Person check, never bypassed in UI.
    const auto hearing = SaveSystem::readSaveData((stage / "laws/law-line-hear/law.json").string());
    const auto author = hearing["authors"][0].get<std::string>();
    if (author.rfind("did:earthcall:",0)==0)
        require(engine.getPerson()->setPersonId(Identity::SingularId::parse(author)),"fixture uses the seed's public author identifier");
    terminal->setPresenceCheckForTests([&](const Person& p){ return &p==engine.getPerson(); });
    require(mgr.switchTo(mgr.findZoneIndex("LawLine")), "native LawLine vocabulary loads in Engine");
    terminal->setSink([](const std::string& text){std::cout << text << '\n';});
    Physics::setFlying(true);
    auto speak = [&](const std::string& line) {
        const auto before = laws.getAll().size();
        PropertyValue previous;lawGetValue(*terminal,PropertyPath::parse("lastCreated"),previous);
        terminal->inject(line);
        for (int i=0;i<4;++i) engine.tick(.016f);
        PropertyValue created;lawGetValue(*terminal,PropertyPath::parse("lastCreated"),created);
        require(laws.getAll().size() > before && created != previous && std::holds_alternative<std::string>(created)
                && laws.find(std::get<std::string>(created)), "sentence adopts through actual Terminal/Metalaws");
        PropertyValue drawn, refusal;
        lawGetValue(*screen,PropertyPath::parse("output.drawn"),drawn);
        lawGetValue(*screen,PropertyPath::parse("output.lastRefusal"),refusal);
        std::cout << "Screen refusal: " << std::get<std::string>(refusal) << '\n';
    };
    auto file = [&](const char* name) { std::ifstream in(root / "examples" / name); std::string line; std::getline(in,line); return line; };
    lawSetValue(*recorder,PropertyPath::parse("outputPath"),std::string("captures"));
    lawSetValue(*recorder,PropertyPath::parse("recordCursor"),false);
    nlohmann::json evidence{{"harness","Codex"},{"model","GPT-6.1 Sol"},
        {"session","01a10992-828e-7e80-890c-c64b09141e18"},{"date","2026-10-06"},
        {"author",engine.getPerson()->getIdentifier()},{"presence","C++ test seam; no human private key"}};
    auto capture = [&](const char* name, auto expected) {
        lawSetValue(*recorder,PropertyPath::parse("snapshot"),true); engine.tick(.016f);
        PropertyValue path,drawn;
        lawGetValue(*recorder,PropertyPath::parse("lastSnapshotPath"),path);
        lawGetValue(*screen,PropertyPath::parse("output.drawn"),drawn);
        require(std::get<bool>(drawn),"Engine reports a submitted direct field");
        int w=0,h=0,c=0; auto* bytes=stbi_load(std::get<std::string>(path).c_str(),&w,&h,&c,4);
        require(bytes && w>0 && h>0,"native framebuffer capture decodes");
        int maxError=0; std::size_t pixels=0;
        for(int y=0;y<h;++y) for(int x=0;x<w;++x) {
            auto colour=expected(x,y,w,h);
            if (!colour) continue;
            ++pixels;
            for(int k=0;k<3;++k) maxError=std::max(maxError,
                std::abs(int(bytes[(y*w+x)*4+k])-int(std::round(255.f*std::clamp((*colour)[k],0.f,1.f)))));
        }
        stbi_image_free(bytes);
        std::filesystem::copy_file(std::get<std::string>(path),std::string(name)+".png",std::filesystem::copy_options::overwrite_existing);
        std::cout << "PIXELS " << name << " count=" << pixels << " maxByteError=" << maxError << '\n';
        require(maxError<=1,"decoded pixels match independent mathematical expectation");
        evidence[name]={{"width",w},{"height",h},{"pixels",pixels},{"maxByteError",maxError}};
    };
    speak("called \"CLI Gradient\" becomes true if is a Person then add property @screen-channel.output.color to VectorField <pieces: [Piece <value: $((u, v, 0.25))>]>");
    capture("gradient",[](int x,int y,int w,int h)->std::optional<glm::vec3>{return glm::vec3((x+.5f)/w,(y+.5f)/h,.25f);});
    speak(file("law_line_screen_pixel.txt"));
    capture("pixel",[](int x,int y,int,int)->std::optional<glm::vec3>{
        if (x==7 && y==9) return glm::vec3(1,.84,0); return std::nullopt;});
    const auto objects = mgr.active().getOwnedObjects().size();
    speak(file("law_line_screen_lens.txt"));
    PropertyValue time; screen->getDynamicProperty("output.time",time);
    double before=0,after=0;propertyValueToNumber(time,before);
    engine.tick(.1f);screen->getDynamicProperty("output.time",time);propertyValueToNumber(time,after);
    require(after>before,"the authored Flow advances explicitly admitted temporal state");
    PropertyValue fieldValue; screen->getDynamicProperty("output.color",fieldValue);
    std::ofstream("lens-field.json") << std::get<std::shared_ptr<OntoMath::VectorField>>(fieldValue)->astDefinition.toJson().dump(2);
    for (const auto& law : laws.getAll()) if(law && law->name()=="Lens Time") law->setEnabled(false);
    auto lensReference = [](float t) {
        return [t](int ix,int iy,int w,int h)->std::optional<glm::vec3>{
            const float x=ix+.5f,y=iy+.5f,u=x/w,v=y/h;
            const float dx=x-w*.5f,dy=y-h*.5f,d=std::sqrt(dx*dx+dy*dy);
            if(std::abs(d-h*(.25f+.008f*std::sin(2*t)))<=h*.004f) return glm::vec3(1,.84,0);
            if(std::abs(d-h*.29f)<=h*.002f) return glm::vec3(0,1,1);
            if(std::abs(dx)+std::abs(dy)<=h*.07f) return glm::vec3(1,.92,.55);
            for(const glm::vec2 offset : {glm::vec2(-.29f,0),glm::vec2(.29f,0),glm::vec2(0,-.29f),glm::vec2(0,.29f)}) {
                const float ax=x-(w*.5f+h*offset.x),ay=y-h*(.5f+offset.y);
                if(std::sqrt(ax*ax+ay*ay)<=h*.009f) return glm::vec3(1,.84,0);
            }
            if(d<=h*.23f) return glm::vec3(.025f+u*.12f,.16f+v*.25f,.35f+u*.3f);
            return glm::vec3(.01f+v*.025f,.015f+u*.025f,.05f+v*.08f);
        };
    };
    lawSetValue(*screen,PropertyPath::parse("output.time"),0.0);capture("lens-t0",lensReference(0));
    lawSetValue(*screen,PropertyPath::parse("output.time"),1.0);capture("lens-t1",lensReference(1));
    require(mgr.active().getOwnedObjects().size()==objects,"Screen wizardry creates no Object or texture carriers");
    speak(file("law_line_screen_clear.txt"));
    PropertyValue drawn;lawGetValue(*screen,PropertyPath::parse("output.drawn"),drawn);
    require(!std::get<bool>(drawn),"the CLI clear sentence restores normal scene output");
    evidence["timeAdvanced"]=after-before;
    std::ofstream("result.json")<<evidence.dump(2)<<'\n';
    engine.shutdown();
    std::cout << "LAW_LINE_DIRECT_SCREEN_RESULT PASS\n";
}
