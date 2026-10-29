// Read-only original-save verification; runtime writes stay in copied fixtures.
// Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-17.
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Screen/ScreenRecorder.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Input/Keyboard/KeyboardHandler.hpp"
#include "Person/Person.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include <filesystem>
#include <iostream>
#include <cmath>
#include <cstdlib>
extern ZoneManager mgr;
int main(int argc,char** argv) {
    std::cout.setf(std::ios::unitbuf);
    if(argc!=3)return 2;
    auto& engine=Core::Engine::instance();
    if(!engine.init(1,argv))return 3;
    glfwSetWindowSize(engine.window(),960,540);
    glfwSetWindowAttrib(engine.window(),GLFW_FLOATING,GLFW_TRUE);
    glfwSetWindowPos(engine.window(),120,100);
    glfwShowWindow(engine.window());glfwFocusWindow(engine.window());
    engine.ensureCursorUnlocked();
    engine.devToolsWindowOpen()=false;engine.performanceMetricsWindowOpen()=false;
    auto* person=engine.getPerson();auto* camera=engine.getCamera();
    auto* recorder=Singularity::Screen::ScreenRecorder::find(*engine.getLawManager());
    if(!person||!camera||!recorder)return 4;
    const auto raw=std::filesystem::path(argv[1])/"raw";
    std::filesystem::create_directories(raw);
    recorder->findProperty("outputPath")->setValue(PropertyValue(raw.string()));
    recorder->findProperty("recordCursor")->setValue(PropertyValue(false));
    struct View {const char* zone;const char* label;glm::vec3 eye,look;};
    const View views[]={
        {"Prism Cathedral","prism-light",{0,4,166},{0,5,190}},
        {"Prism Cathedral","prism-nebula",{99,8,298},{95,4,285}},
        {"Radiance Gallery","radiance",{-15,4,3},{10,4,0}},
        {"Radiance Gallery","radiance-far",{-15,4,171},{10,4,175}},
        {"Sky-Island Sanctuary","sky",{22,12,26},{0,2,0}}
    };
    for(const auto& v:views){
        if(const char* chosen=std::getenv("EC_CAPTURE_VIEW");chosen&&std::string(chosen)!=v.label)continue;
        int index=mgr.findZoneIndex(v.zone);
        if(index<0||!mgr.switchTo(index)){std::cerr<<"Missing zone "<<v.zone<<'\n';return 5;}
        for(double t:{0.,.75,2.25}){
            const auto rawTime=raw/(std::string(v.label)+"-"+std::to_string(t));
            std::filesystem::create_directories(rawTime);
            recorder->findProperty("outputPath")->setValue(PropertyValue(rawTime.string()));
            auto front=glm::normalize(v.look-v.eye);
            for(int warm=0;warm<8;++warm){
                engine.setWorldTime(t);
                person->position()=v.eye-glm::vec3(0,person->getBody().getEyeHeight(),0);
                camera->setPos(v.eye);camera->setFront(front);
                engine.getMouseHandler()->setYaw(glm::degrees(std::atan2(front.z,front.x)));
                engine.getMouseHandler()->setPitch(glm::degrees(std::asin(front.y)));
                recorder->findProperty("snapshot")->setValue(PropertyValue(true));
                engine.tick(0);
                const auto last=recorder->findProperty("lastSnapshotPath")->value();
                if(std::holds_alternative<std::string>(last)&&std::get<std::string>(last).find(rawTime.string())==0)break;
            }
            const auto p=std::filesystem::path(argv[1])/(std::string(v.label)+"-"+std::to_string(t)+".png");
            const auto captured=recorder->findProperty("lastSnapshotPath")->value();
            if(!std::holds_alternative<std::string>(captured)||!std::filesystem::exists(std::get<std::string>(captured))){std::cerr<<"Capture failed "<<p<<" error="<<std::get<std::string>(recorder->findProperty("lastError")->value())<<" pending="<<recorder->isSnapshotPending()<<'\n';return 6;}
            std::filesystem::copy_file(std::get<std::string>(captured),p,std::filesystem::copy_options::overwrite_existing);
            std::cout<<"CAPTURE lane="<<argv[2]<<" zone="<<v.zone<<" time="<<engine.getWorldTime()
                <<" camera="<<camera->pos.x<<","<<camera->pos.y<<","<<camera->pos.z
                <<" fields="<<mgr.active().additionalSpatialFields().size()<<" path="<<p<<'\n';
        }
    }
    engine.shutdown();return 0;
}
