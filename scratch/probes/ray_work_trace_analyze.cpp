// §6fe Pass A offline analyzer for ray_transport_6fe_engine 'worktrace' output. Claude Code / Sonnet 5.5 / worktree authorable-gpu-runs-268d29 / 2026-10-29.
// usage: ray_work_trace_analyze <strips> <tiles> trace-N.bin ...   -> JSON on stdout. Pure post-processing of the diagnostic trace; touches no GPU.
// Trace word: work=bits0..15, reason=bits16..21, written=bit22 (0 = lane wrote nothing: idle/early-return). Layout [strip][producer S1,S2,FB][tiles*16 lanes] in dispatch order.
// Model: a SIMD group of W consecutive lanes costs max(work) lockstep units (divergent loop trips). Continuation model with quantum Q: stage cost per group = min(Q, max remaining);
// lanes whose remaining work exceeds Q are compacted in original order into fresh W-lane groups for the next stage. It is an UPPER BOUND on benefit: no state traffic, compaction or
// redispatch cost, and one work unit is not one hardware cycle (units are producer charges: 24 per empty-proof, 1 per subdivision/bisection node).
#include <cstdio>
#include <cstdint>
#include <cstdlib>
#include <vector>
#include <algorithm>
#include <fstream>
#include <string>
#include <map>
static std::vector<uint32_t> load(const char* p){std::ifstream f(p,std::ios::binary|std::ios::ate);size_t n=f.tellg();f.seekg(0);std::vector<uint32_t> v(n/4);f.read(reinterpret_cast<char*>(v.data()),n);return v;}
int main(int argc,char** argv){
 if(argc<4){std::fprintf(stderr,"usage\n");return 2;}
 const uint64_t strips=std::strtoull(argv[1],0,10),tiles=std::strtoull(argv[2],0,10),L=tiles*16;const char* names[3]={"S1","S2","Fallback"};
 std::printf("{\"poses\":[");
 for(int a=3;a<argc;++a){
  auto w=load(argv[a]);if(w.size()!=strips*3*L){std::fprintf(stderr,"size mismatch %s\n",argv[a]);return 3;}
  std::printf("%s{\"file\":\"%s\",\"producers\":{",a>3?",":"",argv[a]);
  for(int p=0;p<3;++p){
   std::vector<uint32_t> work;std::map<uint32_t,uint64_t> reasons;uint64_t active=0,refused=0;std::vector<uint64_t> hist(1100,0);
   struct G{uint64_t groups=0,sumMax=0,sumWork=0;};std::map<int,G> sg;const int Ws[3]={16,32,64};
   std::map<std::pair<int,int>,double> modelCost;std::map<std::pair<int,int>,uint64_t> modelStages;std::map<int,uint64_t> pendingAfter1;
   for(uint64_t s=0;s<strips;++s){
    const uint32_t* base=&w[(s*3+p)*L];
    for(uint64_t i=0;i<L;++i){uint32_t v=base[i];if(!(v>>22&1))continue;++active;uint32_t wk=v&0xFFFF,rs=(v>>16)&63;work.push_back(wk);hist[std::min<uint32_t>(wk,1099)]++;reasons[rs]++;if(rs)++refused;}
    for(int W:Ws){
     for(uint64_t g=0;g<L;g+=W){uint32_t mx=0;uint64_t sm=0;bool any=false;for(int i=0;i<W&&g+i<L;++i){uint32_t v=base[g+i];if(!(v>>22&1))continue;any=true;uint32_t wk=v&0xFFFF;mx=std::max(mx,wk);sm+=wk;}
      if(!any)continue;auto& x=sg[W];++x.groups;x.sumMax+=mx;x.sumWork+=sm;}
    }
    for(int W:{32}){for(uint32_t Q:{32u,64u,128u,256u,512u}){
      std::vector<uint32_t> rem;rem.reserve(L);for(uint64_t i=0;i<L;++i){uint32_t v=base[i];rem.push_back((v>>22&1)?(v&0xFFFF):0xFFFFFFFFu);}
      // stage 1 over the original dispatch order (idle lanes stay in their groups), later stages over compacted pending lanes
      double cost=0;int stages=0;bool first=true;
      while(true){
       std::vector<uint32_t> cur;if(first){cur=rem;}else{cur=rem;}
       uint64_t n=cur.size();double stageCost=0;std::vector<uint32_t> next;
       for(uint64_t g=0;g<n;g+=W){uint32_t mx=0;bool any=false;for(int i=0;i<W&&g+i<n;++i){uint32_t v=cur[g+i];if(v==0xFFFFFFFFu)continue;any=true;mx=std::max(mx,v);}if(!any)continue;stageCost+=std::min<uint32_t>(Q,mx);}
       for(uint64_t i=0;i<n;++i){uint32_t v=cur[i];if(v!=0xFFFFFFFFu&&v>Q)next.push_back(v-Q);}
       cost+=stageCost;++stages;if(first){pendingAfter1[Q]+=next.size();}first=false;
       if(next.empty()||stages>=64)break;rem=next;
      }
      modelCost[{W,Q}]+=cost;modelStages[{W,Q}]=std::max<uint64_t>(modelStages[{W,Q}],stages);
    }}
   }
   std::sort(work.begin(),work.end());auto pct=[&](double q){return work.empty()?0u:work[std::min<size_t>(work.size()-1,size_t(q*work.size()))];};
   uint64_t sumW=0;for(auto v:work)sumW+=v;
   std::printf("%s\"%s\":{\"active_lanes\":%llu,\"refused_lanes\":%llu,\"sum_work\":%llu,\"mean\":%.3f,\"p50\":%u,\"p90\":%u,\"p99\":%u,\"p999\":%u,\"max\":%u,",p?",":"",names[p],(unsigned long long)active,(unsigned long long)refused,(unsigned long long)sumW,active?double(sumW)/active:0.0,pct(.5),pct(.9),pct(.99),pct(.999),work.empty()?0u:work.back());
   std::printf("\"reasons\":{");bool f=true;for(auto& [r,n]:reasons){std::printf("%s\"%u\":%llu",f?"":",",r,(unsigned long long)n);f=false;}std::printf("},");
   // coarse histogram: [0],1-7,8-15,16-31,32-63,64-127,128-255,256-511,512-1023,>=1024
   uint64_t b[10]={0};for(uint32_t v=0;v<1100;++v){int k=v==0?0:v<8?1:v<16?2:v<32?3:v<64?4:v<128?5:v<256?6:v<512?7:v<1024?8:9;b[k]+=hist[v];}
   std::printf("\"hist_log2\":[");for(int k=0;k<10;++k)std::printf("%s%llu",k?",":"",(unsigned long long)b[k]);std::printf("],");
   std::printf("\"simd\":{");f=true;for(auto& [W,x]:sg){std::printf("%s\"%d\":{\"groups\":%llu,\"sum_max\":%llu,\"sum_work\":%llu,\"efficiency\":%.4f}",f?"":",",W,(unsigned long long)x.groups,(unsigned long long)x.sumMax,(unsigned long long)x.sumWork,x.sumMax?double(x.sumWork)/(double(W)*x.sumMax):0.0);f=false;}
   std::printf("},\"model_w32\":{");f=true;for(auto& [key,c]:modelCost){auto& d=sg[32];std::printf("%s\"Q%d\":{\"lockstep_units\":%.0f,\"vs_direct\":%.4f,\"max_stages\":%llu,\"pending_after_stage1\":%llu}",f?"":",",key.second,c,d.sumMax?c/double(d.sumMax):0.0,(unsigned long long)modelStages[key],(unsigned long long)pendingAfter1[key.second]);f=false;}
   std::printf("}}");
  }
  std::printf("}}");
 }
 std::printf("]}\n");return 0;}
