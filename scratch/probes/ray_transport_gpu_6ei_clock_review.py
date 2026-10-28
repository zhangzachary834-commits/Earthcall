"""§6ei clock/parity correction; shaders frozen, timestamps disabled.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-27.
Blocking and active nonblocking queue-empty waits are different HOST observers,
not hardware timestamps. Active polling can perturb CPU/power economics.
"""
import argparse,hashlib,importlib.util,json,os,re,subprocess,sys
from pathlib import Path
def load(root,name):
 s=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def generate(root):
 old=load(root,'ray_transport_gpu_6eh_clean_timing');cpp=old.clean_cpp(root);frozen=cpp
 start=cpp.index('        auto execute=');end=cpp.index('        auto correctness=',start);execute=cpp[start:end]
 hook='wgpuDevicePoll(gpu.device,true,nullptr);auto end=std::chrono::steady_clock::now();'
 if execute.count(hook)!=1:raise ValueError('completion observer hook')
 execute=execute.replace(hook,'''lastPolls=0;
            if(waitStyle==0){wgpuDevicePoll(gpu.device,true,nullptr);lastPolls=1;}
            else {while(!wgpuDevicePoll(gpu.device,false,nullptr)){++lastPolls;if(std::chrono::steady_clock::now()-submitted>std::chrono::seconds(30))throw std::runtime_error("active completion timeout");}++lastPolls;}
            auto end=std::chrono::steady_clock::now();''')
 cpp=cpp[:start]+'        int waitStyle=0;uint64_t lastPolls=0;\n'+execute+cpp[end:]
 hook='        // 4 Warmup cycles across all 5 active modes: {0, 1, 2, 3, 5}'
 if cpp.count(hook)!=1:raise ValueError('clean sample hook')
 addition=r'''
        J expected;{std::ifstream f(std::getenv("EC_CLOCK_EXPECTED"));f>>expected;}
        if(expected["outputs"].size()!=count||expected["packets"].size()!=count)throw std::runtime_error("expected fixture coverage");
        auto actualSubBytes=readBackBuffer(sBuf,subpanelBytes);auto actualOverBytes=readBackBuffer(oBuf,overlapBytes);
        auto* actualSubs=reinterpret_cast<const GpuSubpanel*>(actualSubBytes.data());auto* actualOvers=reinterpret_cast<const GpuOverlap*>(actualOverBytes.data());
        uint64_t wrongOutputs=0,wrongHeaders=0,wrongPackets=0;J bits=J::array(),headers=J::array();
        for(size_t i=0;i<count;++i){J words=J::array();for(int k=0;k<4;++k)words.push_back(std::bit_cast<uint32_t>(candResults[i].rgb_T[k]));bits.push_back(words);wrongOutputs+=words!=expected["outputs"][i];
          auto& h=candHdrs[i];auto& p=expected["packets"][i];headers.push_back({h.subpanelCount,h.overlapCount});
          if(h.overlapCount!=p[1].get<uint32_t>()||h.subpanelCount!=p[2].size())++wrongHeaders;
          if((h.overlapCount&0xc0000000u)==0u){
            for(uint32_t k=0;k<h.subpanelCount;++k){if(k>=p[2].size()){++wrongPackets;continue;}auto& q=actualSubs[h.subpanelOffset+k];if(J{std::bit_cast<uint32_t>(q.s0),std::bit_cast<uint32_t>(q.s1),q.mediumIndex}!=p[2][k])++wrongPackets;}
            for(uint32_t k=0;k<(h.overlapCount&0x00ffffffu);++k){if(k>=p[3].size()){++wrongPackets;continue;}auto& q=actualOvers[h.overlapOffset+k];if(J{std::bit_cast<uint32_t>(q.s0),std::bit_cast<uint32_t>(q.s1),q.activeMask}!=p[3][k])++wrongPackets;}
          }
        }
        const bool admitted=wrongOutputs==0&&wrongHeaders==0&&wrongPackets==0;
        J clockReport={{"scope","6ei unchanged-shader HOST queue-completion calibration; no hardware GPU timestamps or FPS"},{"candidate_admitted",admitted},{"candidate_vs_validated",{{"wrong_outputs",wrongOutputs},{"wrong_headers",wrongHeaders},{"wrong_nonterminal_packets",wrongPackets},{"terminal_endpoints_observed",false}}},{"candidate_bits",bits},{"candidate_headers",headers}};
        // Verify that changing the HOST wait strategy does not change outputs.
        waitStyle=1;execute(5,1,false);auto activeResults=readResults();uint64_t waitOutputDiffs=0;
        for(size_t i=0;i<count;++i)for(int k=0;k<4;++k)waitOutputDiffs+=std::bit_cast<uint32_t>(activeResults[i].rgb_T[k])!=std::bit_cast<uint32_t>(candResults[i].rgb_T[k]);
        clockReport["wait_strategy_output_component_differences"]=waitOutputDiffs;if(waitOutputDiffs)throw std::runtime_error("wait observer changed results");
        const std::vector<int> modes=admitted?std::vector<int>{0,1,2,3,5}:std::vector<int>{0,1,2,3};
        for(int style:{0,1}){waitStyle=style;for(int m:modes)execute(m,1,false);}
        J observations=J::array();
        for(int rep:{0,1,2,4,8,16})for(int block=0;block<4;++block){
          auto order=modes;if(block%2)std::reverse(order.begin(),order.end());
          for(int m:order){if(rep==0&&m!=0)continue;int styles[4]={0,1,1,0};if(block%2){styles[0]=1;styles[1]=0;styles[2]=0;styles[3]=1;}
            for(int k=0;k<4;++k){waitStyle=styles[k];auto t=execute(m,rep,false);observations.push_back({{"repeats",rep},{"mode",m},{"wait_style",waitStyle},{"block",block},{"position",k},{"encode_ms",t[0]},{"completion_ms",t[1]},{"total_ms",t[2]},{"poll_calls",lastPolls}});}
          }
        }
        clockReport["active_modes"]=modes;clockReport["samples"]=observations;
        clockReport["wait_styles"]={{"0","one blocking devicePoll(true)"},{"1","active devicePoll(false) until documented queue-empty return; CPU spinning included"}};
        clockReport["timing_scope"]="one native invocation, ordered crossover; host encode+submit-to-queue-empty, not hardware GPU duration; repeats0 is empty submission calibration";
        std::ofstream(outputPath)<<clockReport.dump(2)<<"\n";std::cerr<<"6ei parity outputs="<<wrongOutputs<<" headers="<<wrongHeaders<<" packets="<<wrongPackets<<"; calibration samples="<<observations.size()<<std::endl;
        return 0;
'''
 cpp=cpp.replace(hook,addition+'\n'+hook)
 a=dict(re.findall(r'static const char\* (\w+)\s*=\s*R"WGSL\((.*?)\)WGSL";',frozen,re.S));b=dict(re.findall(r'static const char\* (\w+)\s*=\s*R"WGSL\((.*?)\)WGSL";',cpp,re.S))
 if a!=b:raise ValueError('a shader changed during host observer calibration')
 return cpp
def main():
 p=argparse.ArgumentParser(description=__doc__)
 for n in ['root','build-root','work','expected']:p.add_argument('--'+n,type=Path,required=True)
 a=p.parse_args();root=a.root.resolve();work=a.work.resolve()
 if work.exists() or root==work or root in work.parents:raise ValueError('fresh external work required')
 work.mkdir();source=work/'probe.cpp';source.write_text(generate(root));zone=root/'saves/zones/Northern Veil/zone.ecform';digest=lambda p:hashlib.sha256(p.read_bytes()).hexdigest();before=digest(zone)
 (work/'source.json').write_text(json.dumps({'scope':'6ei frozen-shader clock/parity calibration','cpp_sha256':digest(source),'expected_sha256':digest(a.expected),'fixture_sha256':before},indent=2)+'\n')
 subprocess.run([sys.executable,'-B',str(root/'scratch/probes/ray_transport_probe_build.py'),'--root',str(root),'--build-root',str(a.build_root),'--source',str(source),'--work',str(work/'build')],check=True)
 wide=load(root,'ray_transport_gpu_wide_stream');meta,rays,_=wide.verify_case(root,Path('/private/tmp/earthcall-wide-case/manifest.json'),before)
 env=dict(os.environ,EC_CLOCK_EXPECTED=str(a.expected.resolve()),EC_POSE_RAYS=meta['rays'],EC_POSE_REFERENCE=meta['reference'],EC_POSE_CLOCK=str(rays['time']),EC_MATCH_REPEAT='1',EC_GPU_BUDGET='1024',EC_GPU_SUB_CAP='64',EC_GPU_OVER_CAP='16',EC_GPU_MARGIN='.001');env.pop('EC_GPU_GEOMETRY_CASE',None);env.pop('METAL_CAPTURE_ENABLED',None)
 with (work/'native.log').open('xb') as f:r=subprocess.run([str(work/'build/probe'),str(work/'review.json')],cwd=root,env=env,stdout=f,stderr=subprocess.STDOUT)
 if digest(zone)!=before:raise ValueError('fixture drift')
 raise SystemExit(r.returncode)
if __name__=='__main__':main()
