// First Mover verification/export harness only; no application behavior lives here.
// Zach: authored 2D sidebar, no legacy 3D generator. Codex / GPT-6.1 Sol /
// 01a10992-828e-7e80-890c-c64b09141e18 / 2026-10-20.
#include "support/test_harness.hpp"
#include "Singularity/Terminal/TerminalChannel.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "Singularity/Screen/ScreenRecorder.hpp"
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Input/Keyboard/KeyboardHandler.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/Storage/Serialization/ZonesOfEarth/ZoneSerialization.hpp"
#include "ZonesOfEarth/AuthorsOfLaw/MathBinding.hpp"
#include "ConstructedBeing/Singular/Property/PropertyValueJson.hpp"
#include <GLFW/glfw3.h>
#include <iostream>
#include <thread>
namespace fs=std::filesystem;
using Singularity::Terminal::TerminalChannel;
using Singularity::Input::InteractionChannel;
extern ZoneManager mgr;
static int checks=0;
void require(bool value,const std::string& label){++checks;if(!value)throw std::runtime_error(label);std::cout<<"ok "<<label<<std::endl;}
PropertyValue read(Singular& target,const std::string& path){PropertyValue v;require(lawGetValue(target,PropertyPath::parse(path),v),"read "+path);return v;}
void writeJson(const fs::path& path,const nlohmann::json& j){fs::create_directories(path.parent_path());std::ofstream(path)<<j.dump(2)<<'\n';}
std::string authorId(const fs::path& data){
 const auto app=data/"laws/shape-garden-law-0/law.json";
 const auto source=(fs::exists(app)||fs::exists(app.parent_path()/"law.ecform"))?app:data/"laws/law-line-hear/law.json";
 return SaveSystem::readSaveData(source.string()).at("authors").at(0).get<std::string>();
}
void prepare(const fs::path& source,const fs::path& data){
 auto sourceZone=SaveSystem::readSaveData((source/"zones/LawLine/zone.json").string());
 auto compilerRefs=nlohmann::json::array();
 for(const auto& ref:sourceZone.at("lawRefs")){const auto id=ref.get<std::string>();if(id.rfind("law-line-",0)!=0)continue;compilerRefs.push_back(id);writeJson(data/"laws"/id/"law.json",SaveSystem::readSaveData((source/"laws"/id/"law.json").string()));}
 auto relations=nlohmann::json::array();
 for(const auto& rel:sourceZone.value("formationRelations",nlohmann::json::array()))
  if(rel.value("type",std::string{})=="denotes")relations.push_back(rel);
 nlohmann::json zone{{"identifier","ShapeGarden"},{"name","Shape Garden"},{"kind","zone"},{"scope","strict"},
   {"world",{{"objects",nlohmann::json::array()}}},{"lawRefs",compilerRefs},{"lexemes",sourceZone.value("lexemes",nlohmann::json::array())},
   {"formationRelations",relations},{"owner",authorId(data)},
   {"injected_by","Codex / GPT-6.1 Sol / 01a10992 / 2026-10-20: first-seed Zone envelope; behavior compiled from Law Line"}};
 writeJson(data/"zones/ShapeGarden/zone.json",zone);
}
void checkAuthoredObjects(Zone& zone){for(const auto& object:zone.objects()){
 require(object && object->hasField(),"created solid carries an authored implicit field");
 require(!object->getFieldData().expr.empty(),"distance expression is retained");
 PropertyValue tag;require(object->getDynamicProperty("shapeGardenGenerated",tag)||object->getDynamicProperty("shapeGardenFloor",tag),"created solid retains authored recipe marker");
}}
int main(int argc,char** argv){try{
 require(argc>=5,"arguments: seed root, sentences, isolated data, report root, [--native]");
 const fs::path seed=argv[1],sentences=argv[2],data=argv[3],report=argv[4];
 const bool native=argc>5;
 if(!native){
  prepare(seed,data);SaveSystem::setSaveRoot(data.string());
  TestSupport::BootedEngineHarness h("Zach");h.zones.bindLive();
  const auto author=authorId(data);if(author.rfind("did:earthcall:",0)==0)require(h.player.setPersonId(Identity::SingularId::parse(author)),"fixture uses public submitting Person identity");
  TerminalChannel::syncRegister(h.lawManager);Singularity::Screen::ScreenChannel::syncRegister(h.lawManager);
  require(h.zones.switchTo(h.zones.findZoneIndex("ShapeGarden")),"activate first-seed Zone");
  auto* terminal=TerminalChannel::find(h.lawManager);require(terminal,"Terminal channel exists");terminal->setSink([](const std::string&){});
  h.player.cameraForward=glm::vec3(0,0,-1);
  std::ifstream in(sentences);nlohmann::json lines;in>>lines;
  std::vector<std::string> ids;
  for(size_t i=0;i<lines.size();++i){
   const std::string id="shape-garden-law-"+std::to_string(i);
   const auto r=terminal->authorForeign(h.lawManager,lines[i].get<std::string>(),{&h.player},id);
   if(r.status!="authored") {auto parsed=Singularity::Terminal::LawSentence::parse(lines[i].get<std::string>(),terminal->vocabulary(h.lawManager,author));auto at=parsed.errorOffset;std::cerr<<"PARSE_OFFSET "<<at<<" CONTEXT "<<lines[i].get<std::string>().substr(at>100?at-100:0,220)<<std::endl;}
   require(r.status=="authored","Law Line compiles "+id+": "+r.error);ids.push_back(id);
  }
  auto sense=[&](float u,float v,bool down,bool captured=false){
   InteractionChannel::Sense s;s.windowWidth=1000;s.windowHeight=1000;s.pointerX=u*1000;s.pointerY=v*1000;s.left=down;s.uiCaptured=captured;
   h.interaction->pointerLocked=false;h.interaction->noteMouseButton(down);h.interaction->observePending(s,{});for(int i=0;i<5;++i)h.lawManager.tick();
  };
  auto click=[&](float u,float v){sense(u,v,false);sense(u,v,true);sense(u,v,false);};
  auto& zone=h.zones.active();
  require(zone.objects().empty(),"installation bundle is unexecuted Law data, with no fixture object identifiers");
  require(h.zones.persistActiveZone(),"unexecuted authored Zone persists through storage office");
  auto bundleZone=SaveSystem::readZoneIdentity("ShapeGarden");
  bundleZone["lawRefs"]=ids;bundleZone["lexemes"]=nlohmann::json::array();bundleZone["formationRelations"]=nlohmann::json::array();
  writeJson(report/"installed/zones/ShapeGarden/zone.json",bundleZone);
  for(const auto& id:ids)writeJson(report/"installed/laws"/id/"law.json",SaveSystem::readSaveData((data/"laws"/id/"law.json").string()));
  sense(.5,.5,false);
  require(zone.objects().size()==1,"only the authored floor exists before first Create");
  require(std::get<bool>(read(zone,"forge.installed")),"installation fired through Law");
  click(.1,.82);require(zone.objects().size()==2,"one Create click births one solid");
  for(int i=0;i<8;++i)sense(.1,.82,true);
  require(zone.objects().size()==3,"a new press births once, then holding does not repeat");
  sense(.5,.5,true);sense(.1,.82,true);require(zone.objects().size()==3,"dragging into Create while held does not birth again");sense(.5,.5,false);
  sense(.1,.82,true,true);sense(.1,.82,false);require(zone.objects().size()==3,"foreign panel capture veto prevents birth");
  click(.1,.335);click(.1,.82);require(zone.objects().size()==4,"BOX selection followed by Create births a solid");
  auto box=zone.objects().back();require(box->getFieldData().expr.find("abs(")!=std::string::npos && std::abs(geom::evalSdf(box->getFieldData(),glm::vec3(.65f)))<.02f,"box geometry is authored math, not ShapeKind selection");
  click(.1,.445);click(.16,.61);click(.099,.74);click(.1,.82);
  auto torus=zone.objects().back();require(geom::evalSdf(torus->getFieldData(),glm::vec3(0))>.5f,"torus geometry is authored math");
  require(std::abs(geom::evalSdf(torus->getFieldData(),glm::vec3(1.3f,0,0)))<.02f,"size controls affect the actual distance form");
  require(std::abs(std::get<glm::vec3>(read(*torus,"color"))[0]-.98f)<.01f,"color controls affect newborn color");
  checkAuthoredObjects(zone);
  require(h.zones.persistActiveZone(),"generated solids and state save");
  const auto saved=SaveSystem::readZoneIdentity("ShapeGarden");auto restored=makeZoneFromJson(saved);
  require(restored->objects().size()==zone.objects().size(),"generated geometry survives codec reload");checkAuthoredObjects(*restored);
  double count=0;require(propertyValueToNumber(read(zone,"forge.count"),count)&&count==4,"authored counter follows successful requested births");
  const auto beforeClose=zone.objects().size();click(.187,.12);
  require(!std::get<bool>(read(zone,"forge.enabled")) && zone.objects().size()==beforeClose,"Close hides the sidebar and retains generated geometry");
  const auto reopen=terminal->authorForeign(h.lawManager,"called \"Reopen Shape Garden\" becomes true if is a Person then set @ShapeGarden.forge.enabled to true",{&h.player},"shape-garden-test-reopen");
  require(reopen.status=="authored","reopen uses the Law Line");sense(.5,.5,false);
  auto* screen=Singularity::Screen::ScreenChannel::find(h.lawManager);
  require(std::get<bool>(read(zone,"forge.enabled")) && std::get<std::string>(read(*screen,"output.colorPath"))=="@ShapeGarden.forge.panel" && zone.objects().size()==beforeClose,"reopen restores binding without reinstalling or erasing solids");
  nlohmann::json out{{"checks",checks},{"laws",ids},{"objectsAfterClicks",zone.objects().size()},{"author",author},{"source","TerminalChannel::authorForeign -> authored compiler Metalaws"}};
  writeJson(report/"logic.json",out);
 }else{
  SaveSystem::setSaveRoot(data.string());auto& e=Core::Engine::instance();require(e.init(1,argv),"native Engine init");
  e.getKeyboardHandler()->disable();e.getMouseHandler()->disable();e.getMainMenu().close();Physics::setFlying(true);glfwSetInputMode(e.window(),GLFW_CURSOR,GLFW_CURSOR_NORMAL);
  auto& laws=*e.getLawManager();const auto author=authorId(data);if(author.rfind("did:earthcall:",0)==0)require(e.getPerson()->setPersonId(Identity::SingularId::parse(author)),"native fixture public author identity");
  require(mgr.switchTo(mgr.findZoneIndex("ShapeGarden")),"native Zone activates");
  auto* interaction=InteractionChannel::find(laws);interaction->setPointingPerson(e.getPerson());
  auto* recorder=Singularity::Screen::ScreenRecorder::find(laws);auto* screen=Singularity::Screen::ScreenChannel::find(laws);
  require(recorder&&screen,"Screen and recorder channels exist");
  e.getPerson()->position()=glm::vec3(0,0,6);
  e.getPerson()->cameraForward=glm::vec3(0,0,-1);e.getCamera()->setFront(glm::vec3(0,0,-1));
  lawSetValue(*recorder,PropertyPath::parse("outputPath"),(report/"captures").string());lawSetValue(*recorder,PropertyPath::parse("recordCursor"),false);
  for(int i=0;i<5;++i){e.tick(.016f);std::this_thread::sleep_for(std::chrono::milliseconds(20));}
  auto capture=[&](const std::string& name){
   lawSetValue(*recorder,PropertyPath::parse("snapshot"),true);
   for(int i=0;i<5;++i){e.tick(.016f);std::this_thread::sleep_for(std::chrono::milliseconds(25));}
   require(std::get<bool>(read(*screen,"output.drawn")),"native sidebar is drawn");
   const auto path=std::get<std::string>(read(*recorder,"lastSnapshotPath"));require(fs::exists(path),"native screenshot exists");fs::copy_file(path,report/(name+".png"),fs::copy_options::overwrite_existing);
  };
  capture("sidebar");
  auto click=[&](float u,float v){InteractionChannel::Sense s;s.windowWidth=1000;s.windowHeight=1000;s.pointerX=u*1000;s.pointerY=v*1000;
   interaction->observe(s,{});laws.tick();s.left=true;interaction->observe(s,{});for(int i=0;i<5;++i)laws.tick();s.left=false;interaction->observe(s,{});laws.tick();};
  click(.1,.82);click(.1,.335);click(.099,.74);click(.1,.82);click(.1,.445);click(.137,.74);click(.1,.82);
  require(mgr.active().objects().size()==4,"native sensed controls generate three real solids");
  capture("generated");const auto width=std::get<int>(read(*screen,"output.width"));writeJson(report/"native.json",{{"checks",checks},{"objects",mgr.active().objects().size()},{"width",width}});
  e.shutdown();
 }
 std::cout<<"SHAPE_GARDEN_PASS "<<checks<<std::endl;return 0;
}catch(const std::exception& e){std::cerr<<"SHAPE_GARDEN_FAIL "<<e.what()<<std::endl;return 1;}}
