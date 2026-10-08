// Direct Screen authoring through the real Terminal -> Metalaw -> Law -> Engine path.
// Codex / GPT-6.1 Sol / 01a10992-828e-7e80-890c-c64b09141e18 / 2026-10-06.
// Isolated first-seed fixture; no inhabited saves or identity keys are written.
#include "Singularity/Core/Engine.hpp"
#include "Person/Person.hpp"
#include "Singularity/Input/Keyboard/KeyboardHandler.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Input/Interaction/InteractionChannel.hpp"
#include "Singularity/Terminal/TerminalChannel.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "Singularity/Screen/ScreenRecorder.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "../../third_party/stb/stb_image.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
#include <thread>
#include <chrono>
extern ZoneManager mgr;
static void require(bool value, const char* message) {
    if (!value) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
    std::cout << "PASS: " << message << '\n';
}
int main(int argc, char** argv) {
    std::cout.setf(std::ios::unitbuf);
    require(argc == 2 || argc == 3, "source repository argument supplied");
    const bool artEditor=argc==3 && std::string(argv[2])=="--art-editor";
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
    // Isolate the mathematical witness from live desktop hotkeys/clicks;
    // Terminal injection still traverses the production authorship path.
    engine.getKeyboardHandler()->disable();
    engine.getMouseHandler()->disable();
    engine.getMainMenu().close();
    auto& laws = *engine.getLawManager();
    auto* terminal = Singularity::Terminal::TerminalChannel::find(laws);
    auto* screen = Singularity::Screen::ScreenChannel::find(laws);
    auto* recorder = Singularity::Screen::ScreenRecorder::find(laws);
    require(terminal && screen && recorder, "Engine exposes all production channels");
    std::map<long,std::string> capturedFrames;
    bool captureSensedFrame=false;
    auto tick=[&](float dt) {
        PropertyValue previousSnapshot;
        if (captureSensedFrame) lawGetValue(*recorder,PropertyPath::parse("lastSnapshotPath"),previousSnapshot);
        if (captureSensedFrame) lawSetValue(*recorder,PropertyPath::parse("snapshot"),true);
        engine.tick(dt);
        require(mgr.active().getIdentifier()=="LawLine","native fixture remains in its isolated authored Zone");
        if (captureSensedFrame) {
            PropertyValue frame,path;
            lawGetValue(*screen,PropertyPath::parse("sample.frame"),frame);
            lawGetValue(*recorder,PropertyPath::parse("lastSnapshotPath"),path);
            require(path!=previousSnapshot && std::holds_alternative<std::string>(path) && std::filesystem::exists(std::get<std::string>(path)),
                    "sensed frame has a native PNG witness");
            capturedFrames[std::get<long>(frame)]=std::get<std::string>(path);
        }
        // Native drawable acquisition is paced by the display. This isolated
        // fixture must not outrun presentation and call a skipped frame proof.
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    };
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
        captureSensedFrame=line.find("ScreenRegion")!=std::string::npos || line.find("Recolour My Region")!=std::string::npos;
        for (int i=0;i<4;++i) tick(.016f);
        captureSensedFrame=false;
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
        {"session","01a10992-828e-7e80-890c-c64b09141e18"},{"date","2026-10-07"},
        {"author",engine.getPerson()->getIdentifier()},{"presence","C++ test seam; no human private key"}};
    auto capture = [&](const char* name, auto expected) {
        lawSetValue(*recorder,PropertyPath::parse("snapshot"),true); tick(.016f);
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
    tick(.1f);screen->getDynamicProperty("output.time",time);propertyValueToNumber(time,after);
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
    // Zach: displayed regions as predicates, compiled by Metalaws. This
    // witness reads actual completed GPU bytes, then derives an aspect of
    // the Person via the second Law in the same authored line.
    speak(file("law_line_screen_region.txt"));
    auto regionResult=[&](const std::string& token,const glm::vec3& expected) {
        PropertyValue value;
        require(lawGetValue(*screen,PropertyPath::parse("sample.result"),value),"read-only Screen observation resolves");
        auto result=std::get<std::shared_ptr<PropertyDict>>(value);
        require(std::get<bool>(result->elements.at("ok")),"completed GPU region readback succeeds");
        require(std::get<std::string>(result->elements.at("token"))==token,"observation identifies the explicit request");
        const auto samples=std::get<std::shared_ptr<PropertyList>>(result->elements.at("samples"));
        const auto captured=capturedFrames.find(std::get<long>(result->elements.at("frame")));
        require(captured!=capturedFrames.end(),"observation frame resolves its same-frame native PNG");
        int rawW=0,rawH=0,rawC=0;
        auto* raw=stbi_load(captured->second.c_str(),&rawW,&rawH,&rawC,4);
        require(raw && rawW==std::get<long>(result->elements.at("width")) && rawH==std::get<long>(result->elements.at("height")),
                "same-frame witness dimensions agree with the observation");
        int rawError=0;
        for (const auto& entry:samples->elements) {
            auto sample=std::get<std::shared_ptr<PropertyDict>>(entry);
            const int x=std::get<int>(sample->elements.at("x")),y=std::get<int>(sample->elements.at("y"));
            auto color=std::get<glm::vec3>(sample->elements.at("color"));
            for (int lane=0;lane<3;++lane) rawError=std::max(rawError,std::abs(int(std::round(color[lane]*255))-int(raw[(y*rawW+x)*4+lane])));
            const auto alpha=std::get<double>(sample->elements.at("alpha"));
            rawError=std::max(rawError,std::abs(int(std::round(alpha*255))-int(raw[(y*rawW+x)*4+3])));
        }
        stbi_image_free(raw);
        require(rawError==0,"all observed RGBA bytes match independently decoded same-frame pixels");
        size_t expectedCount=0;
        bool membership=true;
        int maxError=0;
        size_t mismatches=0;
        for(int y=108;y<=132;++y) for(int x=108;x<=132;++x) {
            if ((x-120)*(x-120)+(y-120)*(y-120)>150.0625) continue;
            if (expectedCount>=samples->elements.size()) { membership=false; continue; }
            auto sample=std::get<std::shared_ptr<PropertyDict>>(samples->elements[expectedCount++]);
            membership &= std::get<int>(sample->elements.at("x"))==x && std::get<int>(sample->elements.at("y"))==y;
            auto color=std::get<glm::vec3>(sample->elements.at("color"));
            bool mismatch=false;
            for(int lane=0;lane<3;++lane) maxError=std::max(maxError,int(std::abs(std::round(color[lane]*255)-std::round(expected[lane]*255))));
            for(int lane=0;lane<3;++lane) mismatch |= std::round(color[lane]*255)!=std::round(expected[lane]*255);
            mismatches += mismatch;
        }
        require(membership,"membership is in physical top-left row-major coordinates");
        std::cout << "REGION " << token << " sampleCount=" << samples->elements.size() << " maxByteError=" << maxError
                  << " mismatches=" << mismatches << " first=" << propertyValueToJson(samples->elements.front()).dump()
                  << " centre=" << propertyValueToJson(samples->elements[samples->elements.size()/2]).dump() << '\n';
        if (maxError) {
            PropertyValue source,binding;
            lawGetValue(*screen,PropertyPath::parse("halo.color"),source);
            screen->getDynamicProperty("output.colorPath",binding);
            std::cout << "BOUND " << propertyValueToJson(binding).dump() << " FIELD " << propertyValueToJson(source).dump() << '\n';
        }
        require(maxError==0,"every selected observation is the independently expected GPU byte");
        require(samples->elements.size()==expectedCount,"no neighbouring sample leaks into named region");
        require(PropertyPath::parse("sample.result.samples.0.color").setValue(*screen,glm::vec3(1))==PropertyPath::PathResult::ReadOnly,
                "nested observations cannot be forged by an authored write");
        PropertyValue carried;
        require(lawGetValue(*engine.getPerson(),PropertyPath::parse("haloReading"),carried),"an authored Law derives the observation onto its Person bearer");
        require(propertyValueToJson(carried)==propertyValueToJson(value),"derived predicate carries the actual observation value");
        require(PropertyPath::parse("haloReading.samples.0.color").setValue(*engine.getPerson(),glm::vec3(.5))==PropertyPath::PathResult::Ok,
                "a carried observation is ordinary editable authored memory");
        PropertyValue protectedColor;lawGetValue(*screen,PropertyPath::parse("sample.result.samples.0.color"),protectedColor);
        require(std::get<glm::vec3>(protectedColor)!=glm::vec3(.5),"editing a carried observation cannot mutate canonical sensor truth through aliasing");
        const auto frame=result->elements.at("frame");
        tick(.016f);
        PropertyValue next;lawGetValue(*screen,PropertyPath::parse("sample.result.frame"),next);
        require(next==frame,"unchanged request token does not recapture every frame");
        evidence[token]={{"samples",expectedCount},{"maxByteError",0},{"sameFrameRgbaByteError",rawError},{"observation",propertyValueToJson(value)}};
    };
    regionResult("gold-halo",glm::vec3(1,.84,0));
    auto haloReference=[](const glm::vec3& color) {
        return [color](int x,int y,int,int)->std::optional<glm::vec3> {
            if ((x-120)*(x-120)+(y-120)*(y-120)<=150.0625) return color;
            return std::nullopt;
        };
    };
    capture("region-gold",haloReference(glm::vec3(1,.84,0)));
    speak(file("law_line_screen_region_edit.txt"));
    regionResult("cyan-halo",glm::vec3(0,1,1));
    capture("region-cyan",haloReference(glm::vec3(0,1,1)));
    lawSetValue(*screen,PropertyPath::parse("sample.request.limit"),1);
    lawSetValue(*screen,PropertyPath::parse("sample.request.token"),std::string("too-many"));
    tick(.016f);
    PropertyValue refused,refusal,empty;
    lawGetValue(*screen,PropertyPath::parse("sample.result.ok"),refused);
    lawGetValue(*screen,PropertyPath::parse("sample.result.refusal"),refusal);
    lawGetValue(*screen,PropertyPath::parse("sample.result.samples"),empty);
    require(refused==PropertyValue(false) && std::get<std::string>(refusal).find("no partial")!=std::string::npos &&
            std::get<std::shared_ptr<PropertyList>>(empty)->elements.empty(),"budget refusal replaces old observation with no partial samples");
    evidence["budgetRefusal"]=std::get<std::string>(refusal);
    // Any Singular can bear this record: use the same ordinary Map/Set path
    // on the Person, then bind the direct output to that explicit source.
    const std::string personRegion="@"+engine.getPerson()->getIdentifier()+".eyeRegion";
    speak("called Person Region becomes true if is a Person then Sequence <children: [add property my.eyeRegion to ScreenRegion <color: VectorField <pieces: [Piece <value: $(cyan)>]>, selector: ScalarField <pieces: [Piece <value: $(-1)>]>>, add property @screen-channel.output.colorPath to \""+personRegion+".color\", add property @screen-channel.sample.request to ScreenSample <region: \""+personRegion+"\", x: 200, y: 200, width: 1, height: 1, limit: 1, token: \"person-pixel\">]>");
    lawGetValue(*screen,PropertyPath::parse("sample.result.ok"),refused);
    require(refused==PropertyValue(true),"Person-owned region resolves without an Object/Material/texture carrier");
    capture("person-region",[](int,int,int,int)->std::optional<glm::vec3>{return glm::vec3(0,1,1);});
    require(mgr.active().getOwnedObjects().size()==objects,"named Screen regions add no Object carriers");
    speak(file("law_line_screen_clear.txt"));
    PropertyValue clearedDrawn,clearedRefusal;
    lawGetValue(*screen,PropertyPath::parse("output.drawn"),clearedDrawn);
    lawGetValue(*screen,PropertyPath::parse("output.lastRefusal"),clearedRefusal);
    require(clearedDrawn==PropertyValue(false) && clearedRefusal==PropertyValue(std::string()) &&
            !screen->hasDynamicProperty("output.color") && !screen->hasDynamicProperty("output.colorPath") &&
            !screen->hasDynamicProperty("output.time"),"repeated authored clear withdraws output without recreating absent slots");
    screen->removeDynamicProperty("sample.request");
    evidence["repeatedClear"]=true;
    if (artEditor) {
        auto* interaction=Singularity::Input::InteractionChannel::find(laws);
        require(interaction,"native Engine has interaction sense");
        engine.ensureCursorUnlocked();
        speak(file("law_line_pixel_art_editor.txt"));
        const auto point=[&](double u,double v,bool held) {
            Singularity::Input::InteractionChannel::Sense sense;
            sense.windowWidth=1280;sense.windowHeight=720;
            sense.pointerX=u*1280;sense.pointerY=v*720;sense.left=held;
            interaction->pointerLocked=false;
            interaction->observePending(sense,{});
            for(int i=0;i<3;++i)laws.tick();
        };
        const auto click=[&](double u,double v) {point(u,v,false);point(u,v,true);point(u,v,false);};
        auto reference=[](glm::vec3 first,glm::vec3 second) {
            return [first,second](int x,int y,int w,int h)->std::optional<glm::vec3> {
                const double u=(x+.5)/w,v=(y+.5)/h;
                if(u<.18 || u>=.8 || v<.12 || v>=.88)return std::nullopt;
                const int col=std::min(15,int((u-.18)/(.62/16))),row=std::min(15,int((v-.12)/(.76/16)));
                const double x0=.18+col*.62/16,y0=.12+row*.76/16;
                // Avoid exact GPU float32 edge parity; test cell interiors and
                // gaps independently with a small uncertainty band excluded.
                const double dx=std::min(u-x0,x0+.62/16-u),dy=std::min(v-y0,y0+.76/16-v);
                if(std::abs(dx-.0007)<.000002 || std::abs(dy-.0007)<.000002)return std::nullopt;
                if(dx<.0007 || dy<.0007)return glm::vec3(.055,.068,.11);
                if(row==0 && col==0)return first;
                if(row==0 && col==1)return second;
                return glm::vec3(1);
            };
        };
        capture("art-blank",reference(glm::vec3(1),glm::vec3(1)));
        click(.199375,.14375);
        capture("art-gold",reference(glm::vec3(1,.84,0),glm::vec3(1)));
        click(.89,.20);
        capture("art-undo",reference(glm::vec3(1),glm::vec3(1)));
        click(.89,.33);
        capture("art-redo",reference(glm::vec3(1,.84,0),glm::vec3(1)));
        click(.08,.12+6*.059+.0215);click(.238125,.14375);
        capture("art-cyan",reference(glm::vec3(1,.84,0),glm::vec3(0,.8,.85)));
        click(.89,.72);click(.238125,.14375);
        capture("art-erase",reference(glm::vec3(1,.84,0),glm::vec3(1)));
        click(.89,.46);
        capture("art-clear",reference(glm::vec3(1),glm::vec3(1)));
        PropertyValue beforeExport,afterExport;
        lawGetValue(*recorder,PropertyPath::parse("lastSnapshotPath"),beforeExport);
        click(.89,.59);tick(.016f);
        lawGetValue(*recorder,PropertyPath::parse("lastSnapshotPath"),afterExport);
        require(afterExport!=beforeExport && std::filesystem::exists(std::get<std::string>(afterExport)),"authored export tile creates a real native PNG");
        click(.89,.85);tick(.016f);
        lawGetValue(*screen,PropertyPath::parse("output.drawn"),clearedDrawn);
        require(clearedDrawn==PropertyValue(false),"authored close tile withdraws direct output");
        evidence["artEditor"]={{"authoredLaws",276},{"canvas","16x16"},{"input","production observePending sense seam; no physical OS click claim"},{"export",true},{"close",true}};
    }
    evidence["timeAdvanced"]=after-before;
    std::ofstream("result.json")<<evidence.dump(2)<<'\n';
    engine.shutdown();
    std::cout << "LAW_LINE_DIRECT_SCREEN_RESULT PASS\n";
}
