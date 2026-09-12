// Codex (GPT-6 Astra), session 01a07eb3-8ee7-7aa3-8b34-65fea2f4cd44,
// 2026-09-08: Zach's living instrument, through real boot/load/pick/Law/save.
// Every write is confined to a disposable SaveRoot. No real Zone store is read.
#include "support/test_harness.hpp"
#include "Singularity/Input/Interaction/ControlPatterns.hpp"
#include "Singularity/Storage/Serialization/ConstructedBeing/ObjectSerialization.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/ActionModel.hpp"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>

namespace {
int failures = 0;
void check(bool good, const std::string& message) {
    std::printf("%s %s\n", good ? "ok:" : "FAILED:", message.c_str());
    if (!good) ++failures;
}
double number(Singular& s, const char* path) {
    PropertyValue v; double n = -1e9;
    if (lawGetValue(s, PropertyPath::parse(path), v)) propertyValueToNumber(v,n);
    return n;
}
bool near(double a, double b, double tolerance = 0.015) { return std::abs(a-b) < tolerance; }
struct Note { double frequency, amplitude; std::string voice; };
struct Scratch {
    std::filesystem::path path;
    ~Scratch() { std::error_code ec; std::filesystem::remove_all(path, ec); }
};
}

int main() {
    std::setbuf(stdout, nullptr);
    const auto source = std::filesystem::absolute("saves/worlds/synthesis_studio_living.json");
    if (!std::filesystem::exists(source)) return 1;
    Scratch scratch{std::filesystem::temp_directory_path() /
        ("earthcall-living-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))};
    std::filesystem::create_directories(scratch.path / "worlds");
    const auto world = scratch.path / "worlds/living.json";
    std::filesystem::copy_file(source, world);
    SaveSystem::setSaveRoot(scratch.path.string());
    TestSupport::BootedEngineHarness h;
    h.loadWorld(world.string());
    std::shared_ptr<Zone> zone;
    for (size_t i=0;i<h.zones.zones().size();++i) {
        if (h.zones.zones()[i]->getIdentifier() == "SynthesisStudio.LivingInstrument") {
            zone=h.zones.zones()[i]; h.zones.switchTo(i); break;
        }
    }
    check(bool(zone), "clean edition has its own Zone identity after the real load");
    if (!zone) return 1;
    auto find = [&](const std::string& id)->Object* {
        for (auto& o:zone->getOwnedObjects()) if (o && o->getIdentifier()==id) return o.get();
        return nullptr;
    };
    Object* state=find("state.studio");
    check(state && find("studio.author.astra"), "authored state and declared model author exist");
    if (!state) return 1;
    Singular* author=nullptr;
    for (auto* s: Universe::instance().beings()) if (s->getIdentifier()=="Zach") author=s;
    check(author!=nullptr, "original human authorial marker resolves");
    if (!author) return 1;
    Singularity::Input::syncRegisterControlPatterns(h.lawManager, categories, *author);
    bool authored=true;
    for (auto& l:h.lawManager.getAll()) if (!l->isFirstMover() && !l->isAuthored()) authored=false;
    check(authored,"all saved Laws reattach to their actual recorded authors");
    std::vector<Note> sounds;
    registerAudioSink([&](Singular&,double f,double a,const std::string& v){sounds.push_back({f,a,v});});
    double time=20;
    auto tick=[&]{Universe::instance().setClock(time,1.0/60);h.lawManager.tick();};
    auto observe=[&](Object& obj,bool down,double u=0.5,double v=0.5) {
        std::vector<Object*> objects;
        for (auto& o:zone->getOwnedObjects()) objects.push_back(o.get());
        const auto r=obj.getRect2D();
        Singularity::Input::InteractionChannel::Sense s;
        s.pointerX=r.x+(r.z-r.x)*u; s.pointerY=r.y+(r.w-r.y)*v; s.left=down;
        h.interaction->observe(s,objects);
        tick();
    };
    auto click=[&](const std::string& id) {
        auto* o=find(id); check(o!=nullptr,"control exists: "+id);
        if (!o) return;
        observe(*o,true); observe(*o,false);
    };
    tick();
    const size_t population=zone->getOwnedObjects().size();
    const char* slugs[]={"c5","cs5","d5","ds5","e5","f5","fs5","g5","gs5","a5","as5","b5"};
    const double frequencies[]={523.25,554.37,587.33,622.25,659.25,698.46,739.99,783.99,830.61,880,932.33,987.77};
    for(int i=0;i<12;++i) {
        sounds.clear(); click(std::string("hud.pad.")+slugs[i]);
        check(sounds.size()==1 && near(sounds[0].frequency,frequencies[i]),
              std::string("real click sounds exactly the chromatic pitch ")+slugs[i]);
        auto* pad=find(std::string("hud.pad.")+slugs[i]);
        auto* resonator=find(std::string("studio.resonance.")+slugs[i]);
        check(pad && resonator && near(number(*resonator,"struckAt"),time),"note reaches its spatial resonator");
        if (pad) check(number(*pad,"color.r")>number(*pad,"pigmentR"),"pressed pad brightens in its own hue");
        time+=0.2;
    }
    for (int octave: {3,4,5,6}) {
        click("hud.living.octave."+std::to_string(octave));
        sounds.clear();click("hud.pad.cs5");
        check(sounds.size()==1 && near(sounds[0].frequency,554.37*std::pow(2.0,octave-5)),
              "octave selector changes audible pitch: "+std::to_string(octave));
        PropertyValue label;
        find("hud.pad.cs5")->getDynamicProperty("controlLabel",label);
        check(label==PropertyValue(std::string("C#")+std::to_string(octave)),"pad label agrees with selected octave");
    }
    click("hud.living.octave.5");
    for (const auto& mode: {std::string("solo"),std::string("fifth"),std::string("major"),std::string("minor")}) {
        click("hud.living.harmony."+mode);
        for(const char* voice:{"triangle","sine","square"}) {
            click(std::string("hud.resonance.voice.")+voice);
            sounds.clear();click("hud.pad.c5");
            const size_t expected=mode=="solo"?1:mode=="fifth"?2:3;
            check(sounds.size()==expected,"harmony/voice produces exactly its authored number of tones: "+mode+"/"+voice);
            if(sounds.size()==expected) {
                std::sort(sounds.begin(),sounds.end(),[](const Note&a,const Note&b){return a.frequency<b.frequency;});
                check(near(sounds[0].frequency,523.25),"harmony preserves its root pitch");
                if(expected>1) check(near(sounds.back().frequency,523.25*std::pow(2,7.0/12)),"fifth interval is audible");
                if(expected==3) check(near(sounds[1].frequency,523.25*std::pow(2,(mode=="minor"?3.0:4.0)/12)),"major/minor third differs musically");
                for(const auto& n:sounds) check(n.voice==voice && n.amplitude>0 && n.amplitude<=0.31,"voice and bounded dynamics reach the audio channel");
            }
        }
    }
    auto* expression=find("hud.living.expression");
    auto* resonator=find("studio.resonance.b5");
    auto* cursor=find("hud.living.touch");
    check(expression && resonator && cursor,"the expressive surface and its consumers exist");
    if(expression && resonator && cursor) {
        observe(*expression,true,0.05,0.95);
        check(near(number(*state,"bloom"),0.05) && near(number(*state,"motion"),0.05),"lower-left gesture authors intimate, quiet motion");
        const double intimate=number(*resonator,"position.x");
        observe(*expression,true,0.95,0.05);
        check(near(number(*state,"bloom"),0.95) && near(number(*state,"motion"),0.95),"upper-right gesture authors expansion and motion");
        check(number(*resonator,"position.x")>intimate+0.5,"the same gesture actually opens the 3D constellation");
        check(number(*cursor,"x2D")>1200 && number(*cursor,"y2D")<440,"the field cursor shows the authored expression");
        observe(*expression,false,0.95,0.05);
    }
    click("hud.living.harmony.solo");click("hud.resonance.voice.triangle");
    auto* pad=find("hud.pad.c5");
    if(pad) {
        time=100;click("hud.pad.c5");
        time=103;tick();
        check(near(number(*pad,"color.r"),number(*pad,"pigmentR"),0.001),"pad settles back to its exact authored pigment");
    }
    for(int i=0;i<24;++i) {time+=0.1;click(std::string("hud.pad.")+slugs[i%12]);}
    check(zone->getOwnedObjects().size()==population,"repeated music and expression allocate no beings");
    click("hud.living.sound-ink");
    click("hud.pad.a5");
    check(near(number(*state,"inkR"),0.33) && near(number(*state,"inkB"),0.95),
          "sound ink carries the played note's exact pigment into drawing");
    click("hud.btn.draw-stroke");
    auto canvasFrame=[&](float x,float pointerX,bool down) {
        std::vector<Object*> objects;
        for(auto& o:zone->getOwnedObjects()) objects.push_back(o.get());
        Singularity::Input::InteractionChannel::Sense s;
        s.pointerX=pointerX;s.pointerY=-999;s.left=down;
        s.rayOrigin=glm::vec3(0,1.35,-2.6);
        s.rayDirection=glm::normalize(glm::vec3(x,2.2,2.35)-s.rayOrigin);
        h.interaction->observe(s,objects);tick();
    };
    canvasFrame(0.2,-999,false);
    check(h.interaction->hoveredId=="studio.easel.canvas","the drawing ray reaches the actual clean easel");
    canvasFrame(0.2,-999,true);
    const size_t beforeDraw=zone->getOwnedObjects().size();
    canvasFrame(0.5,-970,true);
    check(zone->getOwnedObjects().size()==beforeDraw+1,"a real canvas drag creates one authored mark");
    if(zone->getOwnedObjects().size()==beforeDraw+1) {
        auto* mark=zone->getOwnedObjects().back().get();
        check(near(number(*mark,"acoustic.frequency"),880) && near(number(*mark,"color.b"),0.95),
              "the mark retains the chosen note's pitch and pigment");
        sounds.clear();
        Core::EventBus::instance().publish(ECA::Event{"object-hover-entered",mark,nullptr,1});tick();
        check(!sounds.empty() && near(sounds.back().frequency,880),"the painted mark sounds the note it remembers");
    }
    canvasFrame(0.5,-970,false);
    // Real world save plus the actual Law and Object deserializers.
    const auto saved=scratch.path / "worlds/roundtrip.json";
    h.zones.saveState(saved.string(),h.ctx);
    check(std::filesystem::exists(saved),"real SaveContext writes the new edition");
    auto serialized=h.lawManager.toJson();
    h.lawManager.loadFromJson(serialized);
    sounds.clear();click("hud.pad.fs5");
    check(sounds.size()==1 && near(sounds[0].frequency,739.99),"chromatic playback survives real Law serialization");
    Object decoded;
    nlohmann::json objectJson=*state;
    from_json(objectJson,decoded);
    check(near(number(decoded,"bloom"),number(*state,"bloom")),"expression survives actual Object serialization");
    std::ifstream savedFile(saved);nlohmann::json savedJson;savedFile>>savedJson;
    check(savedJson.dump().find("studio.author.astra")!=std::string::npos,"saved world retains the new author's identity");
    registerAudioSink(nullptr);
    std::printf("Living Studio: %d failures\n",failures);
    return failures?1:0;
}
