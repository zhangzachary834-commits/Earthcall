// Actual Engine fixture: isolated copied data, scripted camera, full native window.
// §6fe derivative (Claude Code / Sonnet 5.5): adds mode 'worktrace' (Rung 0 per-lane work/reason census); all other modes unchanged from 6fb.
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
static bool g_inside=false; // §6fe 'worktrace_in': hard-case population — eye placed at the route's aim point (inside/among the curtains), yaw sweeping 360 degrees.
static std::pair<glm::vec3,glm::vec3> pose(int i){
 const bool inside=g_inside||i>=1000; if(i>=1000)i-=1000; // §6fe parity2b_more: pose index 1000+k is inside-population pose k
 const double a=6.2831853071795864769*i/64.0;
 if(inside){glm::vec3 eye(float(40+6*std::sin(a*.5)),float(62+2*std::cos(a)),float(110+8*std::sin(a)));glm::vec3 f(float(std::cos(a)*std::cos(.17*std::sin(a*2))),float(std::sin(.17*std::sin(a*2))),float(std::sin(a)*std::cos(.17*std::sin(a*2))));return {eye,glm::normalize(f)};}
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
static std::vector<uint32_t> readWords(const KernelRoutedTransport* k,WGPUBuffer src,uint64_t bytes){
 WGPUBufferDescriptor bd={};bd.usage=WGPUBufferUsage_CopyDst|WGPUBufferUsage_MapRead;bd.size=bytes;auto dst=wgpuDeviceCreateBuffer(k->device,&bd);auto enc=wgpuDeviceCreateCommandEncoder(k->device,nullptr);wgpuCommandEncoderCopyBufferToBuffer(enc,src,0,dst,0,bytes);auto cmd=wgpuCommandEncoderFinish(enc,nullptr);wgpuQueueSubmit(k->queue,1,&cmd);wgpuCommandBufferRelease(cmd);wgpuCommandEncoderRelease(enc);
 struct State{std::atomic<bool>done{false};bool ok=false;} s;WGPUBufferMapCallbackInfo cb={};cb.mode=WGPUCallbackMode_AllowSpontaneous;cb.userdata1=&s;cb.callback=[](WGPUMapAsyncStatus status,WGPUStringView,void*p,void*){auto& s=*static_cast<State*>(p);s.ok=status==WGPUMapAsyncStatus_Success;s.done.store(true,std::memory_order_release);};
 wgpuBufferMapAsync(dst,WGPUMapMode_Read,0,bytes,cb);wgpuDevicePoll(k->device,true,nullptr);auto start=Clock::now();while(!s.done.load(std::memory_order_acquire)){if(elapsed(start)>10000)std::abort();std::this_thread::yield();}need(s.ok,"word map failed");
 auto* f=static_cast<const uint32_t*>(wgpuBufferGetConstMappedRange(dst,0,bytes));need(f,"mapped words");std::vector<uint32_t> out(f,f+bytes/4);wgpuBufferUnmap(dst);wgpuBufferRelease(dst);return out;
}
int main(int argc,char**argv){
 try{
  need(argc==3,"usage engine <preflight|timing|negative> <result.json>");std::cout.setf(std::ios::unitbuf);const std::string mode=argv[1];g_inside=mode=="worktrace_in";const fs::path output=fs::absolute(argv[2]);const float dt=1.f/60;
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
  if(mode!="worktrace"&&mode!="worktrace_in"&&mode!="parity2b"&&mode!="parity2b_more"&&mode!="refusal2b"&&mode!="uitoggles"){std::cout<<"WAIT_FOREGROUND "<<gate<<'\n';
  while(!fs::exists(gate)||glfwGetWindowAttrib(e.window(),GLFW_FOCUSED)!=GLFW_TRUE){need(elapsed(gateStart)<120000,"foreground handoff timeout");glfwPollEvents();std::this_thread::sleep_for(std::chrono::milliseconds(10));}
  need(glfwGetWindowAttrib(e.window(),GLFW_FOCUSED)==GLFW_TRUE,"foreground handoff did not focus native window");
  requireFocus=true;result["foreground_handoff_ms"]=elapsed(gateStart);result["foreground_required"]=true;}
  else{result["foreground_required"]=false;}
  Json startup=Json::array();const KernelRoutedTransport* k=nullptr;
  for(int attempt=0;attempt<16;++attempt){
   auto row=frame(0,true);startup.push_back(row);k=r->routedTransportKernelForDiagnostics();
   if(k&&row["routed_sets"].get<uint32_t>()>0)break;
   std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
  need(k&&startup.back()["routed_sets"].get<uint32_t>()>0,"native window did not become ready");
  result["startup_attempts"]=startup;requireRoute=true;fence(k);
  if(mode=="negative"){frame(1,true);throw std::runtime_error("negative failed to reject");}
  if(mode=="parity"||mode=="parity2b"||mode=="parity2b_more"){
   const bool p2b=mode!="parity";
   // §6fb: original routed (separate compute passes) vs coalesced (one pass), same process/device/camera/clock.
   need(lawSetValue(*rec,PropertyPath::parse("outputPath"),(fs::current_path()/"captures").string()),"recorder output");need(lawSetValue(*rec,PropertyPath::parse("recordCursor"),false),"recorder cursor");
   const uint32_t strips=(k->height+k->stripRows-1)/k->stripRows,expectDispatch=8*strips;Json caps=Json::array();std::map<std::string,uint64_t> pubHash,snapHash;std::map<std::string,uint64_t> traceHash;
   auto capture=[&](const char* name,int index,bool on,bool coalesce,uint32_t limit,bool poisoned,bool skip=false,bool one=false){r->setRoutedSkipEmptyTilesForDiagnostics(skip);r->setRoutedSingleMemberS1ForDiagnostics(one);
    r->setRoutedWorkLimitForDiagnostics(limit);r->setRoutedCoalescedPassesForDiagnostics(coalesce,true);Json row={{"name",name},{"index",index},{"routed",on},{"coalesced",coalesce},{"skip_empty",skip},{"single_member_s1",one},{"work_limit",limit},{"poisoned",poisoned}};
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
   const std::vector<int> poseList=mode=="parity2b_more"?std::vector<int>{20,28,36,44,1000,1008,1016,1024,1032,1040,1048,1056}:p2b?std::vector<int>{0,8,16,24,32,40,48,56}:std::vector<int>{0,32};Json eff2b=Json::array(),diffs=Json::array();
   for(int index:poseList){
    const std::string I=std::to_string(index);
    if(p2b){
     capture("current",index,false,false,1024,false);capture("orig",index,true,true,1024,false);auto rawO=rawPublication(k);capture("origR",index,true,true,0,false);
     capture("skip",index,true,true,1024,false,true);capture("one",index,true,true,1024,false,false,true);{auto rawN=rawPublication(k);uint64_t nd=0,refToAcc=0,accToRef=0,other=0;Json ex=Json::array();
   for(uint64_t i=0;i<rawO.size()/4;++i){bool d=false;for(int j=0;j<4;++j)if(std::memcmp(&rawO[i*4+j],&rawN[i*4+j],4))d=true;if(!d)continue;++nd;
    const bool oRef=rawO[i*4+3]<0,nRef=rawN[i*4+3]<0;if(oRef&&!nRef){++refToAcc;if(ex.size()<4)ex.push_back(Json{{"x",i%2560},{"y",i/2560},{"new",Json::array({rawN[i*4],rawN[i*4+1],rawN[i*4+2],rawN[i*4+3]})}});}else if(!oRef&&nRef)++accToRef;else ++other;}
   diffs.push_back(Json{{"index",index},{"differing_pixels",nd},{"orig_refused_2b_accepted",refToAcc},{"orig_accepted_2b_refused",accToRef},{"both_accepted_but_different",other},{"examples",ex}});
   need(accToRef==0&&other==0,"2b changed an accepted ray or refused one the original accepted");}capture("oneP",index,true,true,1024,true,false,true);capture("oneR",index,true,true,0,false,false,true);capture("oneRP",index,true,true,0,true,false,true);
     auto eq2=[&](std::map<std::string,uint64_t>& m,const char* a,const char* b,const char* why){if(m.at(a+I)!=m.at(b+I))throw std::runtime_error(std::string(why)+" @pose "+I+" "+a+"="+hex(m.at(a+I))+" "+b+"="+hex(m.at(b+I)));};
     eq2(pubHash,"orig","skip","2a skip-empty publication differs from original routed");eq2(pubHash,"one","oneP","2b poisoned publication differs from clean 2b");eq2(pubHash,"origR","oneR","2b refusal publication differs");eq2(pubHash,"origR","oneRP","2b poisoned refusal publication differs");
     eq2(snapHash,"one","oneP","2b poisoned snapshot differs from clean 2b");eq2(snapHash,"current","oneR","2b forced refusal must preserve Current");eq2(snapHash,"current","oneRP","2b poisoned forced refusal must preserve Current");
     need(snapHash.at("orig"+I)!=snapHash.at("current"+I),"routed output must differ from Current somewhere");
     // production list counters (observer pass also totals counts): original vs skip vs 2b
     uint32_t fb[3]={0,0,0},s1c[3]={0,0,0},s2c[3]={0,0,0};
     for(int v=0;v<3;++v){need(r->setRoutedRouteObserverForDiagnostics(true),"observer on");r->setRoutedWorkLimitForDiagnostics(1024);r->setRoutedCoalescedPassesForDiagnostics(true);r->setRoutedSkipEmptyTilesForDiagnostics(v==1);r->setRoutedSingleMemberS1ForDiagnostics(v==2);r->resetRoutedRouteObserverForDiagnostics();frame(index,true);fence(k);auto w=readWords(k,k->routeHistogram,1032*4);s1c[v]=w[1024];s2c[v]=w[1025];fb[v]=w[1026];need(r->setRoutedRouteObserverForDiagnostics(false),"observer off");}
     r->setRoutedSkipEmptyTilesForDiagnostics(false);r->setRoutedSingleMemberS1ForDiagnostics(false);
     need(s2c[0]==s2c[2]&&s2c[0]==s2c[1],"S2 count must be untouched");need(s1c[2]+fb[2]<=s1c[0]+fb[0]&&fb[2]<=fb[1]&&fb[1]<=fb[0],"2b must only shrink the fallback list");
     if(index==24||index==32||index==40||index==48)need(fb[2]<fb[1]&&s1c[2]>s1c[0],"2b must move single-curtain tiles to S1 on poses with them");
     if(index==8||index==16||index==56)need(fb[2]==fb[0]&&s1c[2]==s1c[0],"2b must be inert on poses without empties/single-curtain tiles");
     eff2b.push_back(Json{{"index",index},{"s1",{s1c[0],s1c[1],s1c[2]}},{"s2",{s2c[0],s2c[1],s2c[2]}},{"fallback",{fb[0],fb[1],fb[2]}},{"order","original, skip-empty, 2b"}});
     continue;
    }
    capture("current",index,false,false,1024,false);
    capture("orig",index,true,false,1024,false);capture("orig2",index,true,false,1024,false);capture("coal",index,true,true,1024,false);
    capture("origP",index,true,false,1024,true);capture("coalP",index,true,true,1024,true);
    capture("origR",index,true,false,0,false);capture("coalR",index,true,true,0,false);capture("coalRP",index,true,true,0,true);
    capture("skip",index,true,true,1024,false,true);capture("skipP",index,true,true,1024,true,true);capture("skipR",index,true,true,0,false,true);capture("skipRP",index,true,true,0,true,true);
    auto eq=[&](std::map<std::string,uint64_t>& m,const char* a,const char* b,const char* why){need(m.at(a+I)==m.at(b+I),why);};
    eq(pubHash,"orig","orig2","control: original not deterministic");eq(pubHash,"orig","coal","coalesced publication differs");eq(pubHash,"orig","origP","poisoned original differs");eq(pubHash,"orig","coalP","poisoned coalesced differs");eq(pubHash,"origR","coalR","refusal publication differs");eq(pubHash,"origR","coalRP","poisoned refusal publication differs");
    eq(snapHash,"orig","orig2","control: original snapshot not deterministic");eq(snapHash,"orig","coal","coalesced snapshot differs");eq(snapHash,"orig","origP","poisoned original snapshot differs");eq(snapHash,"orig","coalP","poisoned coalesced snapshot differs");eq(snapHash,"current","origR","forced refusal must preserve Current output (original)");eq(snapHash,"current","coalR","forced refusal must preserve Current output (coalesced)");eq(snapHash,"current","coalRP","forced refusal must preserve Current output (coalesced, poisoned)");
    need(snapHash.at("orig"+I)!=snapHash.at("current"+I),"routed output must differ from Current somewhere (transport actually contributes)");
    eq(pubHash,"orig","skip","skip-empty publication differs");eq(pubHash,"orig","skipP","poisoned skip-empty publication differs");eq(pubHash,"origR","skipR","skip-empty refusal publication differs");eq(pubHash,"origR","skipRP","poisoned skip-empty refusal publication differs");eq(snapHash,"orig","skip","skip-empty snapshot differs");eq(snapHash,"orig","skipP","poisoned skip-empty snapshot differs");eq(snapHash,"current","skipR","skip-empty forced refusal must preserve Current output");eq(snapHash,"current","skipRP","skip-empty poisoned forced refusal must preserve Current output");eq(traceHash,"orig","coal","dispatch order/workgroup trace differs");eq(traceHash,"orig","coalP","dispatch order trace differs (poisoned)");
   }
   if(p2b){result["effectiveness_2b"]=eff2b;result["2b_vs_original_diffs"]=diffs;}
   if(!p2b){Json eff=Json::array();uint64_t allEmpties=0;for(int index:{0,32}){need(r->setRoutedRouteObserverForDiagnostics(true),"observer on");r->setRoutedWorkLimitForDiagnostics(1024);r->setRoutedCoalescedPassesForDiagnostics(true);r->setRoutedSkipEmptyTilesForDiagnostics(true);r->resetRoutedRouteObserverForDiagnostics();frame(index,true);fence(k);auto w=readWords(k,k->routeHistogram,1032*4);uint64_t empties=0,obsFb=0;for(uint32_t key=0;key<1024;++key){if(!w[key])continue;const uint32_t mask=key&15,overlap=(key>>8)&1,ordered=(key>>9)&1;const bool s1=mask==1&&!overlap,s2=ordered;if(!s1&&!s2){obsFb+=w[key];if(mask==0)empties+=w[key];}}
     // the observer's own classifier is the original; the production counters come from the skipping classifier
     allEmpties+=empties;need(w[1026]+empties==obsFb,"skip-empty did not remove exactly the wave-empty tiles from the fallback list");eff.push_back(Json{{"index",index},{"observed_fallback",obsFb},{"observed_empty",empties},{"production_fallback_with_skip",w[1026]}});}
    need(allEmpties>0,"checked poses must contain wave-empty tiles (skip not exercised otherwise)");r->setRoutedSkipEmptyTilesForDiagnostics(false);need(r->setRoutedRouteObserverForDiagnostics(false),"observer off");result["skip_effectiveness"]=eff;}
   r->setRoutedWorkLimitForDiagnostics(1024);r->setRoutedCoalescedPassesForDiagnostics(false);result["captures"]=caps;result["expected_dispatches"]=expectDispatch;result["strips"]=strips;
  }
  if(mode=="worktrace"||mode=="worktrace_in"){
   // §6fe Pass A: per-lane (work,reason) trace of the production producers on the frozen route. Diagnostic only; clean timing is a separate mode that never enables the trace.
   // Output-perturbation control: the trace must not change the publication. Trace words: work = bits 0..15, reason = bits 16..21, written = bit 22 (0 = lane wrote nothing).
   const uint32_t strips=(k->height+k->stripRows-1)/k->stripRows;Json caps=Json::array();
   auto pubHash=[&](){auto raw=rawPublication(k);return hex(fnv(raw.data(),raw.size()*4));};
   r->setRoutedCoalescedPassesForDiagnostics(true);r->setRoutedWorkLimitForDiagnostics(1024);r->setRoutedSkipEmptyTilesForDiagnostics(false);
   Json control=Json::array();
   for(int index:{0,32}){need(r->setRoutedWorkTraceForDiagnostics(false),"trace off");frame(index,true);fence(k);auto off=pubHash();need(r->setRoutedWorkTraceForDiagnostics(true)&&k->observeWork,"trace on");frame(index,true);fence(k);auto on=pubHash();need(on==off,"work trace perturbed the publication");control.push_back(Json{{"index",index},{"off",off},{"on",on}});}
   result["trace_output_control"]=control;result["strips"]=strips;result["tiles"]=k->tiles;result["trace_bytes"]=k->workTraceBytes;
   for(int index=0;index<64;index+=4){
    frame(index,true);fence(k);auto w=readWords(k,k->workTrace,k->workTraceBytes);const auto name=std::string(g_inside?"traceIn-":"trace-")+std::to_string(index)+".bin";
    {std::ofstream f(output.parent_path()/name,std::ios::binary);f.write(reinterpret_cast<const char*>(w.data()),w.size()*4);}
    caps.push_back(Json{{"index",index},{"file",name},{"fnv",hex(fnv(w.data(),w.size()*4))},{"publication_fnv",pubHash()}});
   }
   result["traces"]=caps;need(r->setRoutedWorkTraceForDiagnostics(false),"trace off");
  }
  if(mode=="uitoggles"){
   // Developer Tools toggles are ScreenChannel properties applied by the renderer on change: each must reach the Kernel next frame, defaults must restore, and 2b must publish like the diagnostics path.
   frame(32,true);fence(k);need(k->coalescePasses&&!k->skipEmptyTiles&&!k->singleMemberS1,"defaults must be coalesced/no skip/no 2b");
   sc->volumeRoutedSkipEmptyTiles=true;frame(32,true);fence(k);need(k->skipEmptyTiles&&!k->singleMemberS1&&k->coalescePasses,"2a toggle did not reach the Kernel");auto h2a=hex(fnv(rawPublication(k).data(),rawPublication(k).size()*4));
   sc->volumeRoutedSkipEmptyTiles=false;sc->volumeRoutedSingleMemberS1=true;frame(32,true);fence(k);need(k->singleMemberS1&&!k->skipEmptyTiles,"2b toggle did not reach the Kernel");auto h2b=hex(fnv(rawPublication(k).data(),rawPublication(k).size()*4));
   sc->volumeRoutedCoalescedPasses=false;frame(32,true);fence(k);need(!k->coalescePasses,"coalesce toggle did not reach the Kernel");
   sc->volumeRoutedCoalescedPasses=true;sc->volumeRoutedSingleMemberS1=false;frame(32,true);fence(k);need(k->coalescePasses&&!k->skipEmptyTiles&&!k->singleMemberS1,"defaults did not restore");auto h0=hex(fnv(rawPublication(k).data(),rawPublication(k).size()*4));
   need(h2a==h0&&h2b==h0,"UI-path publications at pose 32 must equal the default publication");
   result["ui_toggle_hashes"]=Json{{"default",h0},{"2a",h2a},{"2b",h2b}};
  }
  if(mode=="refusal2b"){
   // §6fe refusal isolation: poses where 2b published a ray the original refused (40: 1 px, 44: 2 px). Fallback trace indexed by ray id with the member at the lane's exit
   // (the refusing member on a refusal). Same ray compared across original / skip-empty (2a) / 2b: a ray absent from the 2a fallback trace was in a wave-empty tile; present in 2a but
   // absent in 2b was in a single-active-member tile (S1). Trace words: work bits 0..15, reason 16..21, written bit 22, member 23..24.
   const uint32_t strips=(k->height+k->stripRows-1)/k->stripRows;const uint64_t stride=uint64_t(k->tiles)*16;Json rows=Json::array();
   r->setRoutedCoalescedPassesForDiagnostics(true);r->setRoutedWorkLimitForDiagnostics(1024);
   need(r->setRoutedWorkTraceForDiagnostics(false),"trace off");r->setRoutedFallbackTraceByRayForDiagnostics(true);need(r->setRoutedWorkTraceForDiagnostics(true)&&k->observeWork&&k->traceFallbackByRay,"by-ray trace on");
   for(int index:{40,44}){
    auto grab=[&](bool skip,bool one){r->setRoutedSkipEmptyTilesForDiagnostics(skip);r->setRoutedSingleMemberS1ForDiagnostics(one);frame(index,true);fence(k);auto w=readWords(k,k->workTrace,k->workTraceBytes);return w;};
    auto t0=grab(false,false);auto t2=grab(true,false);auto t3=grab(false,true);r->setRoutedSkipEmptyTilesForDiagnostics(false);r->setRoutedSingleMemberS1ForDiagnostics(false);
    const auto at=[&](const std::vector<uint32_t>& t,uint64_t s,uint64_t id){return t[s*3*stride+2*stride+id];};
    uint64_t refusals=0,fallbackLanes=0;
    for(uint64_t s=0;s<strips;++s)for(uint64_t id=0;id<stride;++id){const uint32_t v=at(t0,s,id);if(!(v>>22&1))continue;++fallbackLanes;const uint32_t reason=(v>>16)&63u;if(!reason)continue;++refusals;
     const uint32_t v2=at(t2,s,id),v3=at(t3,s,id);
     const char* route=(v2>>22&1)?((v3>>22&1)?"still-fallback-in-2b":"single-active-member S1 in 2b"):"wave-empty tile skipped by 2a";
     rows.push_back(Json{{"index",index},{"strip",s},{"ray_id",id},{"approx_pixel",Json::array({id%k->width,s*k->stripRows+id/k->width})},{"orig_reason",reason},{"orig_work",v&0xFFFF},{"refusing_member",(v>>23)&3u},{"in_2a_fallback",bool(v2>>22&1)},{"in_2b_fallback",bool(v3>>22&1)},{"2b_fallback_reason",(v3>>16)&63u},{"route_under_2b",route}});}
    result["pose_"+std::to_string(index)]=Json{{"fallback_lanes",fallbackLanes},{"refusals",refusals}};
   }
   result["refusals"]=rows;result["strips"]=strips;need(r->setRoutedWorkTraceForDiagnostics(false),"trace off");r->setRoutedFallbackTraceByRayForDiagnostics(false);
  }
  if(mode=="census"){
   // §6fd: actual route masks on the frozen 64-pose route, observer-controlled. Every observer-ON frame is cross-checked against the production list counters.
   const uint32_t strips=(k->height+k->stripRows-1)/k->stripRows;const uint64_t perFrame=uint64_t(k->tiles)*strips;Json rows=Json::array();std::map<uint32_t,uint64_t> total;uint64_t totals[3]={0,0,0};
   auto pubHash=[&](){auto raw=rawPublication(k);return hex(fnv(raw.data(),raw.size()*4));};
   // Output-perturbation control: observer ON must publish exactly what observer OFF publishes.
   Json control=Json::array();for(int index:{0,32}){need(r->setRoutedRouteObserverForDiagnostics(false),"observer off");frame(index,true);fence(k);auto off=pubHash();need(r->setRoutedRouteObserverForDiagnostics(true),"observer on");r->resetRoutedRouteObserverForDiagnostics();frame(index,true);fence(k);auto on=pubHash();need(on==off,"route observer perturbed the publication");control.push_back(Json{{"index",index},{"publication_fnv_off",off},{"publication_fnv_on",on}});}
   result["observer_output_control"]=control;
   for(int index=0;index<64;++index){
    r->resetRoutedRouteObserverForDiagnostics();auto facts=frame(index,true);fence(k);auto w=readWords(k,k->routeHistogram,1032*4);
    uint64_t sum=0,s1=0,s2=0,fb=0;Json bins=Json::object();for(uint32_t key=0;key<1024;++key){if(!w[key])continue;sum+=w[key];total[key]+=w[key];bins[std::to_string(key)]=w[key];const uint32_t mask=key&15,overlap=(key>>8)&1,ordered=(key>>9)&1;if(mask==1&&!overlap)s1+=w[key];else if(ordered)s2+=w[key];else fb+=w[key];}
    need(sum==perFrame,"observer did not classify every tile of every strip");need(w[1027]==strips,"observer strip count");need(s1==w[1024]&&s2==w[1025]&&fb==w[1026],"observer routes differ from production list counters");
    totals[0]+=s1;totals[1]+=s2;totals[2]+=fb;rows.push_back(Json{{"index",index},{"s1",s1},{"s2",s2},{"fallback",fb},{"bins",bins},{"production",Json::array({w[1024],w[1025],w[1026]})}});
   }
   Json agg=Json::object();for(auto& [key,n]:total)agg[std::to_string(key)]=n;result["tiles_per_frame"]=perFrame;result["frames"]=64;result["aggregate_bins"]=agg;result["aggregate_routes"]={{"s1",totals[0]},{"s2",totals[1]},{"fallback",totals[2]}};result["poses"]=rows;
   need(r->setRoutedRouteObserverForDiagnostics(false),"observer off");
  }
  if(mode=="skiptiming"){
   // Lanes: 0 Current, 1 coalesced baseline (production default), 2 coalesced + skip-empty-tiles, 3 coalesced + single-member S1 (step 2b). 4x4 Latin-square block order.
   auto lane=[&](int l,int i){r->setRoutedCoalescedPassesForDiagnostics(l!=0);r->setRoutedSkipEmptyTilesForDiagnostics(l==2);r->setRoutedSingleMemberS1ForDiagnostics(l==3);return frame(i,l!=0);};
   Json warm=Json::array();for(int l:{0,1,2,3})for(int i=0;i<64;++i)warm.push_back(lane(l,i));fence(k);Json samples=Json::array();const int orders[4][4]={{0,1,2,3},{1,2,3,0},{2,3,0,1},{3,0,1,2}};
   for(bool serial:{false,true})for(int block=0;block<4;++block)for(int pos=0;pos<4;++pos){const int l=orders[block][pos];fence(k);Json activation=Json::array();auto activationStart=Clock::now();for(int w=0;w<2;++w)activation.push_back(lane(l,0));fence(k);double activationMs=elapsed(activationStart);
    Json frames=Json::array();auto begin=Clock::now();for(int i=0;i<64;++i){auto start=Clock::now();auto row=lane(l,i);need(row["volume_compiles"].get<uint32_t>()==0,"cold volume compile in timed interval");if(l!=0)need(k->encodedPasses==1u,"lane compute-pass count");if(serial)fence(k);row["completion_inclusive_ms"]=elapsed(start);frames.push_back(row);}
    auto drain=Clock::now();fence(k);double drainMs=elapsed(drain),total=elapsed(begin);samples.push_back(Json{{"serial",serial},{"block",block},{"position",pos},{"lane",l},{"mode_activation_ms",activationMs},{"frames",frames},{"terminal_drain_ms",drainMs},{"total_ms",total}});result["samples"]=samples;write(output.parent_path()/"progress.json",result);std::cout<<"SAMPLE serial="<<serial<<" block="<<block<<" lane="<<l<<" total_ms="<<total<<'\n';}
   r->setRoutedSkipEmptyTilesForDiagnostics(false);r->setRoutedSingleMemberS1ForDiagnostics(false);result["samples"]=samples;result["final_publication"]=publication(k);
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
