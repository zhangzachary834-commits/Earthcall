// Synthetic CPU adversary; included after the repository CPU helper prefix.
// Post-P12 Repair-Verification Mode.

int main(){
 std::array<KernelRoutedSpec,4> specs{};
 for(int m=0;m<4;++m){
  specs[m].origin=glm::vec4(m<2?0.f:1000.f,m==0?5.f:13.f,m==0?5.f:13.f,0.f);
  specs[m].half=glm::vec4(100,5,5,0);
  specs[m].limits=glm::vec4(100,5,0,5);
  specs[m].curv=glm::vec4(2e-9f,.001f,0,0);
  specs[m].amps=glm::vec4(1e-6f,1e-6f,1e-6f,0);
  specs[m].freqs=glm::vec4(.01f,.02f,.03f,0);
 }
 Ray rays[64]{};
 const double step=2*std::tan(3.14159265358979323846/8)/1440;
 const double angle=std::atan(1.0001)-std::atan(-.5*step);
 const glm::dvec3 forward(0,std::sin(angle),std::cos(angle));
 const glm::dvec3 up(0,std::cos(angle),-std::sin(angle));
 for(int row=0;row<4;++row)for(int col=0;col<4;++col){
  const auto d=glm::normalize(forward+up*((-.5-row)*step)+glm::dvec3(1,0,0)*(col*step));
  rays[row*4+col]={glm::vec4(0,-38,-40,0),glm::vec4(glm::vec3(d),100),glm::uvec4(0)};
 }
 const auto serial=classifyTileSerial(0,rays,specs);
 const auto legacySerial=classifyTileSerialLegacyBug(0,rays,specs);
 TileRouteResult coop[4]{};classifyWorkgroupCooperative(0,4,rays,specs,coop);
 int overlapRays=0,acceptedOverlapRays=0;
 Json cases=Json::array();
 uint32_t forcedS2Word = (0u << 28u) | (1u << 24u) | 0u;
 for(int i=0;i<16;++i){
  auto s2=runS2Producer(rays[i],forcedS2Word,specs);
  auto fb=runFallbackProducer(rays[i],specs);
  bool backwards=false;
  for(size_t j=1;j<s2.subpanels.size();++j)backwards|=s2.subpanels[j].span.x<s2.subpanels[j-1].span.y;
  overlapRays+=fb.overs>0;
  acceptedOverlapRays+=(fb.overs>0&&s2.reason==0);
  cases.push_back(Json{{"ray",i},{"dx",rays[i].rd_clipB.x},{"dy_over_dz",rays[i].rd_clipB.y/rays[i].rd_clipB.z},{"s2_reason",s2.reason},{"s2_subpanels",s2.subpanels.size()},{"fallback_overlap_count",fb.overs},{"s2_chronology_violation",backwards}});
 }
 float at=(8.08f-rays[12].ro_clipA.z)/rays[12].rd_clipB.z;
 const auto world=glm::vec3(rays[12].ro_clipA)+at*glm::vec3(rays[12].rd_clipB);
 auto density=[&](int m){auto p=world-glm::vec3(specs[m].origin);auto clamp=[](float a){return std::max(0.f,std::min(1.f,a));};return clamp(1-std::abs(p.x)/100.f)*clamp(.2f*(p.y+5))*clamp(1-(p.y+5)/10.f)*clamp(1-std::abs(p.z+evalWave(specs[m],p.x))/5.f);};
 std::array<KernelRoutedSpec,4> emptySpecs{};
 for(auto& m:emptySpecs){m.origin=glm::vec4(0);m.half=glm::vec4(20,20,20,0);m.limits=glm::vec4(20,20,0,1);m.curv=glm::vec4(.001f,.001f,0,0);m.amps=glm::vec4(.1f,.1f,.1f,0);m.freqs=glm::vec4(.01f,.02f,.03f,0);}
 Ray emptyRays[16]{};
 for(auto& r:emptyRays)r={glm::vec4(0,0,10,0),glm::vec4(0,1,0,20),glm::uvec4(0)};
 const auto emptyTile=classifyTileSerial(0,emptyRays,emptySpecs);
 Json result={{"legacy_serial_cohort",legacySerial.cohort},{"independent_world_point",{world.x,world.y,world.z}},{"independent_density_A",density(0)},{"independent_density_B",density(1)},{"scope","CPU witness using the repository classifier/producer mirror functions plus independent synthetic rectilinear camera/box geometry; NOT a GPU execution"},{"viewport",{1281,1440}},{"tile_pixel_origin",{640,720}},{"camera",{0,-38,-40}},{"serial_cohort",serial.cohort},{"cooperative_cohort",coop[0].cohort},{"serial_packed",serial.packedWord},{"overlapping_rays",overlapRays},{"accepted_overlap_rays",acceptedOverlapRays},{"rays",cases}};
 result["empty_tile_cohort"]=emptyTile.cohort;
 std::cout<<result.dump(2)<<'\n';
 return (serial.cohort==3&&coop[0].cohort==3&&legacySerial.cohort==2&&emptyTile.cohort==0&&overlapRays==12&&acceptedOverlapRays==0&&density(0)>0&&density(1)>0)?0:42;
}
