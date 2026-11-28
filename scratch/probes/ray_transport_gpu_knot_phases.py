"""Same-query nonadditive component clocks for the numerical event stream.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
One32784-ray query in a65536-capacity allocation, not65536 distinct inputs.
No fallback fixture permitted for component-only schedule reuse.
"""
import argparse,importlib.util
from pathlib import Path

def load(root,name):
 spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m

def source(base,root):
 q=load(root,'ray_transport_gpu_pose_qualification');s=load(root,'ray_transport_gpu_knot_numerical').source(base,root)
 s=q.once(s,'for(uint32_t capacity:{8192u,32768u})','for(uint32_t capacity:{65536u})')
 s=q.once(s,'else for(const auto& tile:tiles){',r'''
                else if(mode==3)dispatch(cpProducer,bgProducer,count);
                else if(mode==4){dispatch(cpUnfused,bgUnfused,count);dispatch(fallbackCP,fallbackBG,count);}
                else if(mode==5)for(const auto& tile:tiles)dispatch(knotProducer,tile.producer,tile.n);
                else if(mode==6)for(const auto& tile:tiles){dispatch(knotConsumer,tile.consumer,tile.n);dispatch(fallbackCP,tile.fallback,tile.n);}
                else for(const auto& tile:tiles){''')
 s=q.once(s,'        J series=J::array();',r'''
        if(tiles.size()!=1||normal["fallback_rays"]!=0||normal["capacity_fallback_rays"]!=0)throw std::runtime_error("phase reuse precondition refused");
        for(int mode:{4,6}){
            execute(mode,1,false);auto got=readResults();
            for(size_t i=0;i<count;++i)for(int k=0;k<4;++k){
                const float expected=mode==4?resUnfused[i].rgb_T[k]:normalRows[i][k+2].get<float>();
                if(std::bit_cast<uint32_t>(got[i].rgb_T[k])!=std::bit_cast<uint32_t>(expected))throw std::runtime_error("phase consumer control changed output");
            }
        }
        J series=J::array();''')
 s=q.once(s,'block%2?std::vector<int>{2,1,0,0,1,2}:std::vector<int>{0,1,2,2,1,0}', 'block%2?std::vector<int>{6,5,4,3,2,1,0,0,1,2,3,4,5,6}:std::vector<int>{0,1,2,3,4,5,6,6,5,4,3,2,1,0}')
 meta=r'''
    report["streamed_schedule"]["phase_only"]=true;
    report["streamed_schedule"]["phase_consumer_output_checks"]=true;
    report["streamed_schedule"]["mode_names"]={{"0","current"},{"1","bulk connected"},{"2","compact connected"},{"3","bulk producer only"},{"4","bulk consumer plus fallback"},{"5","compact producer only"},{"6","compact consumer plus fallback"}};
    report["streamed_schedule"]["component_clocks_not_additive"]=true;
    report["streamed_schedule"]["scope"]="32784 distinct queries in65536-capacity single partial chunk; paired1/2/4-repetition complete host clocks; no full viewport, no occupancy diagnosis or native frame gain";
    report["connected_measured_scope"]["scope"]="32784 distinct native rays; original bulk connected control; older2304 label superseded";
'''
 return q.once(s,'    std::cout << report.dump(2) << std::endl;',meta+'\n    std::cout << report.dump(2) << std::endl;')

def main():
 p=argparse.ArgumentParser(add_help=False);p.add_argument('--root',type=Path,required=True);a,_=p.parse_known_args();root=a.root.resolve();q=load(root,'ray_transport_gpu_pose_qualification');wide=load(root,'ray_transport_gpu_wide_stream');q.source=lambda base,r:source(base,r);q.verify_case=lambda r,p,z:wide.verify_case(r,p,z);q.independent_check=wide.independent_check;q.main()
 import sys
 if '--build-only' not in sys.argv:load(root,'ray_transport_knot_evidence').finish(root)
if __name__=='__main__':main()
