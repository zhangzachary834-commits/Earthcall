// Actual Engine fixture: isolated copied data, scripted camera, full native window.
// Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
#include "Singularity/Core/Engine.hpp"
#include "Singularity/Network/WebSocketServer.hpp"
#include "Singularity/Storage/SaveSystem.hpp"
#include "Singularity/Screen/WebGPU/WebGpuRenderer.hpp"
#include "Singularity/Screen/WebGPU/VolumeRoutedTransport.hpp"
#include "Singularity/Screen/ScreenChannel.hpp"
#include "Singularity/Screen/ScreenRecorder.hpp"
#include "Singularity/Screen/Camera.hpp"
#include "Singularity/Input/Mouse/MouseHandler.hpp"
#include "Singularity/FirstMoverOntology/FirstMoverWindowTools/PerformanceMetricsWindow.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Person/Person.hpp"
#include "ZonesOfEarth/ZoneManager.hpp"
#include "ZonesOfEarth/Zone/Zone.hpp"
#include "ZonesOfEarth/Physics/Physics.hpp"
#include <webgpu/wgpu.h>
#include <json.hpp>
#include <imgui.h>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <thread>
#include <cmath>
#include <cstring>
#include <vector>
#include <string>
using Clock=std::chrono::steady_clock;
extern ZoneManager mgr;
namespace fs=std::filesystem;
using Json=nlohmann::json;
static double elapsed(Clock::time_point t){return std::chrono::duration<double,std::milli>(Clock::now()-t).count();}
template<class T> static void need(const T& b,const char* why){if(!b)throw std::runtime_error(why);}
static void need(PropertyPath::PathResult r,const char* why){need(r==PropertyPath::PathResult::Ok||r==PropertyPath::PathResult::Unchanged,why);}
static void write(const fs::path& p,const Json& j){std::ofstream(p)<<j.dump(2)<<'\n';}
static Json vec(glm::vec3 p){return Json::array({p.x,p.y,p.z});}
static std::pair<glm::vec3,glm::vec3> pose(int i){
 const double a=6.2831853071795864769*i/64.0;
 glm::vec3 eye(float(8*std::sin(a)),float(50+2*std::sin(a*.5)),float(75+8*(1-std::cos(a))));
 glm::vec3 aim(float(40+8*std::sin(a*.5)),float(62+2*std::cos(a)),110);
 return {eye,glm::normalize(aim-eye)};
}
static void fence(const KernelRoutedTransport* k){
 need(k&&k->device&&k->queue,"missing native device/queue");
 struct State{std::atomic<bool> done{false};bool ok=false;} s;
 WGPUQueueWorkDoneCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowSpontaneous;cb.userdata1=&s;
 cb.callback=[](WGPUQueueWorkDoneStatus status,WGPUStringView,void* p,void*){auto& s=*static_cast<State*>(p);s.ok=status==WGPUQueueWorkDoneStatus_Success;s.done.store(true,std::memory_order_release);};
 wgpuQueueOnSubmittedWorkDone(k->queue,cb);wgpuDevicePoll(k->device,true,nullptr);
 auto start=Clock::now();while(!s.done.load(std::memory_order_acquire)){if(elapsed(start)>10000)std::abort();std::this_thread::yield();}
 need(s.ok,"queue completion failed");
}
static Json publication(const KernelRoutedTransport* k){
 need(k&&k->publication&&k->width==2560&&k->height==1440,"full publication extent");
 const uint64_t bytes=uint64_t(k->width)*k->height*16;
 WGPUBufferDescriptor bd={};bd.usage=WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead;bd.size=bytes;
 auto dst=wgpuDeviceCreateBuffer(k->device,&bd);auto enc=wgpuDeviceCreateCommandEncoder(k->device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(enc,k->publication,0,dst,0,bytes);auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuQueueSubmit(k->queue,1,&cmd);wgpuCommandBufferRelease(cmd);wgpuCommandEncoderRelease(enc);
 struct State{std::atomic<bool>done{false};bool ok=false;} s;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowSpontaneous;cb.userdata1=&s;cb.callback=[](WGPUMapAsyncStatus status,WGPUStringView,void*p,void*){auto& s=*static_cast<State*>(p);s.ok=status==WGPUMapAsyncStatus_Success;s.done.store(true,std::memory_order_release);};
 wgpuBufferMapAsync(dst,WGPUMapMode_Read,0,bytes,cb);wgpuDevicePoll(k->device,true,nullptr);auto start=Clock::now();while(!s.done.load(std::memory_order_acquire)){if(elapsed(start)>10000)std::abort();std::this_thread::yield();}need(s.ok,"publication map failed");auto* f=static_cast<const float*>(wgpuBufferGetConstMappedRange(dst,0,bytes));need(f,"mapped publication");uint64_t accepted=0,refused=0,nonempty=0,nonfinite=0;
 for(uint64_t i=0;i<bytes/16;++i){auto* v=f+i*4;if(v[3]<0){++refused;continue;}++accepted;nonempty+=v[3]>0||v[0]!=0||v[1]!=0||v[2]!=0;for(int j=0;j<4;++j)nonfinite+=!std::isfinite(v[j]);}
 wgpuBufferUnmap(dst);wgpuBufferRelease(dst);need(nonfinite==0,"nonfinite accepted publication");return Json{{"accepted",accepted},{"refused",refused},{"nonempty",nonempty},{"nonfinite",nonfinite},{"bytes",bytes}};
}
static std::vector<float> rawPublication(const KernelRoutedTransport* k){
 const uint64_t bytes=uint64_t(k->width)*k->height*16;WGPUBufferDescriptor bd={};bd.usage=WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead;bd.size=bytes;
 auto dst=wgpuDeviceCreateBuffer(k->device,&bd);auto enc=wgpuDeviceCreateCommandEncoder(k->device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(enc,k->publication,0,dst,0,bytes);auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuQueueSubmit(k->queue,1,&cmd);wgpuCommandBufferRelease(cmd);wgpuCommandEncoderRelease(enc);
 struct State{std::atomic<bool>done{false};bool ok=false;} s;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowSpontaneous;cb.userdata1=&s;cb.callback=[](WGPUMapAsyncStatus status,WGPUStringView,void*p,void*){auto& s=*static_cast<State*>(p);s.ok=status==WGPUMapAsyncStatus_Success;s.done.store(true,std::memory_order_release);};
 wgpuBufferMapAsync(dst,WGPUMapMode_Read,0,bytes,cb);wgpuDevicePoll(k->device,true,nullptr);auto start=Clock::now();while(!s.done.load(std::memory_order_acquire)){if(elapsed(start)>10000)std::abort();std::this_thread::yield();}need(s.ok,"publication map failed");
 auto* f=static_cast<const float*>(wgpuBufferGetConstMappedRange(dst,0,bytes));need(f,"mapped publication");std::vector<float> out(f,f+bytes/4);wgpuBufferUnmap(dst);wgpuBufferRelease(dst);return out;
}
static uint64_t fnv(const void* d,size_t n){uint64_t h=0xcbf29ce484222325ull;auto* b=static_cast<const unsigned char*>(d);for(size_t i=0;i<n;++i)h=(h^b[i])*0x100000001b3ull;return h;}
static std::string hex(uint64_t v){char b[24];std::snprintf(b,sizeof b,"%016llx",(unsigned long long)v);return b;}
// Poison every scratch/output buffer encode() must rewrite (offsets/indices are the constant tile permutation and are left intact).
static void poisonScratch(const KernelRoutedTransport* k){
 std::vector<unsigned char> v;for(auto b:{k->publication,k->rays,k->singles,k->overlaps,k->counts,k->lists[0],k->lists[1],k->lists[2],k->args}){const uint64_t n=wgpuBufferGetSize(b);if(v.size()<n)v.assign(n,0xFF);wgpuQueueWriteBuffer(k->queue,b,0,v.data(),n);}
}
int main(int argc,char**argv){
 try{
  need(argc==3,"usage engine <preflight|timing|negative> <result.json>");std::cout.setf(std::ios::unitbuf);const std::string mode=argv[1];const fs::path output=fs::absolute(argv[2]);const float dt=1.f/60;
  auto& e=Core::Engine::instance();auto initStart=Clock::now();need(e.init(1,argv),"actual Engine init");double initMs=elapsed(initStart);
  // Isolate this copied fixture from auto-attaching local clients. The rest of
  // the production Engine tick remains intact; network service is excluded.
  Singularity::Network::WebSocketServer::instance().stop();
  need(SaveSystem::rootPath()==fs::current_path()/"saves","isolated data root refused");need(mgr.switchTo(mgr.findZoneIndex("Northern Veil")),"actual Northern Veil hydration");
  auto* p=e.getPerson();auto* c=e.getCamera();auto* sc=Singularity::Screen::ScreenChannel::find(*e.getLawManager());auto* rec=Singularity::Screen::ScreenRecorder::find(*e.getLawManager());auto* r=dynamic_cast<WebGpuRenderer*>(&currentRenderer());need(p&&c&&sc&&rec&&r,"production channels");need(mgr.active().getOwnedObjects().size()==24,"all24 scene objects admitted");need(mgr.active().additionalSpatialFields().size()==4,"four authored fields admitted");
  Physics::setFlying(true);e.getMainMenu().close();e.ensureCursorUnlocked();ImGui::GetIO().IniFilename=nullptr;
  bool requireRoute=false;bool requireFocus=false;bool freezePose=mode=="negative";
  auto frame=[&](int i,bool on){
   const auto expected=pose(i);auto actual=pose(freezePose?0:i);p->position()=actual.first-glm::vec3(0,p->getBody().getEyeHeight(),0);p->velocity()=glm::vec3(0);c->setPos(actual.first);c->setFront(actual.second);e.getMouseHandler()->setYaw(glm::degrees(std::atan2(actual.second.z,actual.second.x)));e.getMouseHandler()->setPitch(glm::degrees(std::asin(actual.second.y)));e.setWorldTime(7.25+double(i)/60);sc->volumeRoutedTransportEnabled=on;
   auto begin=Clock::now();e.tick(dt);double tickMs=elapsed(begin);int w=0,h=0;glfwGetFramebufferSize(e.window(),&w,&h);need(w==2560&&h==1440,"native framebuffer must be2560x1440");need(glm::length(c->getPos()-expected.first)<1e-4f&&glm::length(c->getFront()-expected.second)<1e-4f,"scripted camera pose was not executed");if(requireFocus)need(glfwGetWindowAttrib(e.window(),GLFW_FOCUSED)==GLFW_TRUE,"native benchmark lost focus");const auto& s=r->frameStats();need(s.volumeProgramRefusals==0,"production volume program refusal");need(!requireRoute||(on?s.volumeRoutedSets>0:s.volumeRoutedSets==0),"lane dispatch mismatch");
   return Json{{"index",i},{"focused",glfwGetWindowAttrib(e.window(),GLFW_FOCUSED)==GLFW_TRUE},{"eye",vec(c->getPos())},{"front",vec(c->getFront())},{"world_time",e.getWorldTime()},{"tick_ms",tickMs},{"host_total_ms",g_frameTimings.total_ms},{"render3d_ms",g_frameTimings.render3d_ms},{"surface_ms",g_frameTimings.wait_surface_ms},{"submit_ms",g_frameTimings.wait_submit_ms},{"gui_ms",g_frameTimings.imgui_ms},{"zone_ms",g_frameTimings.zone_ms},{"laws_ms",g_frameTimings.laws_ms},{"routed_sets",s.volumeRoutedSets},{"routed_pixels",s.volumeRoutedPixels},{"volume_compiles",s.volumeProgramCompiles},{"draw_calls",s.drawCalls},{"triangles",s.trianglesDrawn},{"working_set_bytes",s.volumeRoutedWorkingSetBytes}};
  };
  Json result={{"schema","6ey-actual-engine-release-scripted-path-1"},{"mode",mode},{"init_ms",initMs},{"framebuffer",{2560,1440}},{"objects",24},{"media",4},{"foreign_network_service",false},{"fixed_step",dt},{"snapshot_scope","3D scene before ImGui overlay, through actual ScreenRecorder"}};
  const auto gate=output.parent_path()/"foreground-ready";auto gateStart=Clock::now();
  std::cout<<"WAIT_FOREGROUND "<<gate<<'\n';
  while(!fs::exists(gate)||glfwGetWindowAttrib(e.window(),GLFW_FOCUSED)!=GLFW_TRUE){need(elapsed(gateStart)<120000,"foreground handoff timeout");glfwPollEvents();std::this_thread::sleep_for(std::chrono::milliseconds(10));}
  need(glfwGetWindowAttrib(e.window(),GLFW_FOCUSED)==GLFW_TRUE,"foreground handoff did not focus native window");
  requireFocus=true;result["foreground_handoff_ms"]=elapsed(gateStart);result["foreground_required"]=true;
  Json startup=Json::array();const KernelRoutedTransport* k=nullptr;
  for(int attempt=0;attempt<16;++attempt){
   auto row=frame(0,true);startup.push_back(row);k=r->routedTransportKernelForDiagnostics();
   if(k&&row["routed_sets"].get<uint32_t>()>0)break;
   std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
  need(k&&startup.back()["routed_sets"].get<uint32_t>()>0,"native window did not become ready");
  result["startup_attempts"]=startup;requireRoute=true;fence(k);
  if(mode=="negative"){frame(1,true);throw std::runtime_error("negative failed to reject");}
  if(mode=="parity"){
   // §6fb: original routed (separate compute passes) vs coalesced (one pass), same process/device/camera/clock.
   need(lawSetValue(*rec,PropertyPath::parse("outputPath"),(fs::current_path()/"captures").string()),"recorder output");need(lawSetValue(*rec,PropertyPath::parse("recordCursor"),false),"recorder cursor");
   const uint32_t strips=(k->height+k->stripRows-1)/k->stripRows,expectDispatch=8*strips;Json caps=Json::array();std::map<std::string,uint64_t> pubHash,snapHash;std::map<std::string,uint64_t> traceHash;
   auto capture=[&](const char* name,int index,bool on,bool coalesce,uint32_t limit,bool poisoned){
    r->setRoutedWorkLimitForDiagnostics(limit);r->setRoutedCoalescedPassesForDiagnostics(coalesce,true);Json row={{"name",name},{"index",index},{"routed",on},{"coalesced",coalesce},{"work_limit",limit},{"poisoned",poisoned}};
    if(poisoned){fence(k);poisonScratch(k);fence(k);auto raw=rawPublication(k);uint64_t nan=0;for(float f:raw)nan+=std::isnan(f);need(nan==raw.size(),"poison control did not reach the publication buffer");row["poison_nan_words"]=nan;}
    need(lawSetValue(*rec,PropertyPath::parse("snapshot"),true),"snapshot request");auto facts=frame(index,on);fence(k);
    PropertyValue path;need(lawGetValue(*rec,PropertyPath::parse("lastSnapshotPath"),path)&&std::holds_alternative<std::string>(path),"actual viewport snapshot");const auto file=output.parent_path()/(std::string("scene-")+name+"-"+std::to_string(index)+fs::path(std::get<std::string>(path)).extension().string());fs::copy_file(std::get<std::string>(path),file,fs::copy_options::overwrite_existing);
    std::ifstream in(file,std::ios::binary);std::vector<char> bytes((std::istreambuf_iterator<char>(in)),{});need(!bytes.empty(),"snapshot bytes");row["snapshot"]=file.filename().string();row["snapshot_fnv"]=hex(fnv(bytes.data(),bytes.size()));snapHash[std::string(name)+std::to_string(index)]=fnv(bytes.data(),bytes.size());
    row["frame"]=facts;if(on){need(facts["routed_sets"].get<uint32_t>()==1,"exactly one routed set per frame");row["encoded_passes"]=k->encodedPasses;row["encoded_dispatches"]=k->encodedDispatches;row["dispatch_trace"]=hex(k->dispatchTrace);traceHash[std::string(name)+std::to_string(index)]=k->dispatchTrace;
     need(k->encodedDispatches==expectDispatch,"logical dispatch count changed");need(k->encodedPasses==(coalesce?1u:expectDispatch),"compute-pass count not as declared");
     auto raw=rawPublication(k);uint64_t accepted=0,refused=0,nonempty=0,bad=0;for(uint64_t i=0;i<raw.size()/4;++i){auto* v=&raw[i*4];if(v[3]<0){++refused;continue;}++accepted;nonempty+=v[3]>0||v[0]!=0||v[1]!=0||v[2]!=0;for(int j=0;j<4;++j)bad+=!std::isfinite(v[j]);}
     need(bad==0,"nonfinite or stale poison word in publication");row["publication"]={{"accepted",accepted},{"refused",refused},{"nonempty",nonempty},{"nonfinite",bad}};row["publication_fnv"]=hex(fnv(raw.data(),raw.size()*4));pubHash[std::string(name)+std::to_string(index)]=fnv(raw.data(),raw.size()*4);
     if(limit==0)need(nonempty==0,"forced refusal must not publish partial transport");else need(nonempty>100,"normal transport must contribute");}
    caps.push_back(row);
   };
   for(int index:{0,32}){
    const std::string I=std::to_string(index);
    capture("current",index,false,false,1024,false);
    capture("orig",index,true,false,1024,false);capture("orig2",index,true,false,1024,false);capture("coal",index,true,true,1024,false);
    capture("origP",index,true,false,1024,true);capture("coalP",index,true,true,1024,true);
    capture("origR",index,true,false,0,false);capture("coalR",index,true,true,0,false);capture("coalRP",index,true,true,0,true);
    auto eq=[&](std::map<std::string,uint64_t>& m,const char* a,const char* b,const char* why){need(m.at(a+I)==m.at(b+I),why);};
    eq(pubHash,"orig","orig2","control: original not deterministic");eq(pubHash,"orig","coal","coalesced publication differs");eq(pubHash,"orig","origP","poisoned original differs");eq(pubHash,"orig","coalP","poisoned coalesced differs");eq(pubHash,"origR","coalR","refusal publication differs");eq(pubHash,"origR","coalRP","poisoned refusal publication differs");
    eq(snapHash,"orig","orig2","control: original snapshot not deterministic");eq(snapHash,"orig","coal","coalesced snapshot differs");eq(snapHash,"orig","origP","poisoned original snapshot differs");eq(snapHash,"orig","coalP","poisoned coalesced snapshot differs");eq(snapHash,"current","origR","forced refusal must preserve Current output (original)");eq(snapHash,"current","coalR","forced refusal must preserve Current output (coalesced)");eq(snapHash,"current","coalRP","forced refusal must preserve Current output (coalesced, poisoned)");
    need(snapHash.at("orig"+I)!=snapHash.at("current"+I),"routed output must differ from Current somewhere (transport actually contributes)");
    eq(traceHash,"orig","coal","dispatch order/workgroup trace differs");eq(traceHash,"orig","coalP","dispatch order trace differs (poisoned)");
   }
   r->setRoutedWorkLimitForDiagnostics(1024);r->setRoutedCoalescedPassesForDiagnostics(false);result["captures"]=caps;result["expected_dispatches"]=expectDispatch;result["strips"]=strips;
  }
  if(mode=="packaging"){
   // Lanes: 0 Current, 1 original routed, 2 coalesced routed. Separately paid warmup and activation; queued and completion-fenced series.
   auto lane=[&](int l,int i){r->setRoutedCoalescedPassesForDiagnostics(l==2);return frame(i,l!=0);};
   Json warm=Json::array();for(int l:{0,1,2})for(int i=0;i<64;++i)warm.push_back(lane(l,i));fence(k);result["warmup"]=warm;Json samples=Json::array();const int orders[4][3]={{0,1,2},{1,2,0},{2,0,1},{2,1,0}};
   for(bool serial:{false,true})for(int block=0;block<4;++block)for(int pos=0;pos<3;++pos){const int l=orders[block][pos];fence(k);Json activation=Json::array();auto activationStart=Clock::now();for(int w=0;w<2;++w)activation.push_back(lane(l,0));fence(k);double activationMs=elapsed(activationStart);
    Json frames=Json::array();auto begin=Clock::now();for(int i=0;i<64;++i){auto start=Clock::now();auto row=lane(l,i);need(row["volume_compiles"].get<uint32_t>()==0,"cold volume compile in timed interval");if(l!=0){row["encoded_passes"]=k->encodedPasses;row["encoded_dispatches"]=k->encodedDispatches;need(k->encodedPasses==(l==2?1u:8*((k->height+k->stripRows-1)/k->stripRows)),"lane compute-pass count");}if(serial)fence(k);row["completion_inclusive_ms"]=elapsed(start);frames.push_back(row);}
    auto drain=Clock::now();fence(k);double drainMs=elapsed(drain),total=elapsed(begin);samples.push_back(Json{{"serial",serial},{"block",block},{"position",pos},{"lane",l},{"mode_activation_ms",activationMs},{"mode_activation_frames",activation},{"frames",frames},{"terminal_drain_ms",drainMs},{"total_ms",total}});result["samples"]=samples;write(output.parent_path()/"progress.json",result);std::cout<<"SAMPLE serial="<<serial<<" block="<<block<<" lane="<<l<<" total_ms="<<total<<'\n';}
   result["samples"]=samples;result["final_publication"]=publication(k);
  }
  if(mode=="preflight"||mode=="session"){
   need(lawSetValue(*rec,PropertyPath::parse("outputPath"),(fs::current_path()/"captures").string()),"recorder output");need(lawSetValue(*rec,PropertyPath::parse("recordCursor"),false),"recorder cursor");Json captures=Json::array();
   for(int index:{0,32})for(int variant:{0,1,2}){
    r->setRoutedWorkLimitForDiagnostics(variant==2?0:1024);need(lawSetValue(*rec,PropertyPath::parse("snapshot"),true),"snapshot request");auto facts=frame(index,variant!=0);fence(k);PropertyValue path;need(lawGetValue(*rec,PropertyPath::parse("lastSnapshotPath"),path)&&std::holds_alternative<std::string>(path),"actual viewport snapshot");auto file=output.parent_path()/(std::string("scene-")+std::to_string(index)+"-"+std::to_string(variant)+fs::path(std::get<std::string>(path)).extension().string());fs::copy_file(std::get<std::string>(path),file,fs::copy_options::overwrite_existing);facts["variant"]=variant;facts["snapshot"]=file.filename().string();if(variant!=0){facts["publication"]=publication(k);if(variant==1)need(facts["publication"]["nonempty"].get<uint64_t>()>100,"normal transport must contribute");else need(facts["publication"]["nonempty"].get<uint64_t>()==0,"forced refusal must not publish partial transport");}captures.push_back(facts);
   }
   result["captures"]=captures;r->setRoutedWorkLimitForDiagnostics(1024);
  }
  if(mode=="timing"||mode=="session"){
   if(mode=="session"){
    freezePose=true;bool rejected=false;
    try{frame(1,true);}catch(const std::exception& error){rejected=std::string(error.what())=="scripted camera pose was not executed";}
    freezePose=false;need(rejected,"same-process missing movement must reject");fence(k);
    result["same_process_movement_negative"]="rejected at scripted camera pose assertion";
    write(output.parent_path()/"progress.json",result);
   }
   Json warm=Json::array();for(bool on:{false,true})for(int i=0;i<64;++i)warm.push_back(frame(i,on));fence(k);result["warmup"]=warm;Json samples=Json::array();
   for(bool serial:{false,true})for(int block=0;block<4;++block)for(int order=0;order<4;++order){bool on=order==1||order==2;fence(k);Json activation=Json::array();auto activationStart=Clock::now();for(int w=0;w<2;++w)activation.push_back(frame(0,on));fence(k);double activationMs=elapsed(activationStart);Json frames=Json::array();auto begin=Clock::now();for(int i=0;i<64;++i){auto start=Clock::now();auto row=frame(i,on);need(row["volume_compiles"].get<uint32_t>()==0,"cold volume compile in timed interval");if(serial)fence(k);row["completion_inclusive_ms"]=elapsed(start);frames.push_back(row);}auto drain=Clock::now();fence(k);double drainMs=elapsed(drain),total=elapsed(begin);samples.push_back(Json{{"serial",serial},{"block",block},{"order",order},{"on",on},{"mode_activation_ms",activationMs},{"mode_activation_frames",activation},{"frames",frames},{"terminal_drain_ms",drainMs},{"total_ms",total}});result["samples"]=samples;write(output.parent_path()/"progress.json",result);std::cout<<"SAMPLE serial="<<serial<<" block="<<block<<" order="<<order<<" on="<<on<<" total_ms="<<total<<'\n';}
   result["samples"]=samples;result["final_publication"]=publication(k);
  }
  write(output,result);e.shutdown();std::cout<<"PASS actual Engine "<<mode<<'\n';return 0;
 }catch(const std::exception& x){std::cerr<<"FAIL: "<<x.what()<<'\n';return 42;}
}
