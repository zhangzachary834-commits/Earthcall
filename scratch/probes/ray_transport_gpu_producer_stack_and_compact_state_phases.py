"""Isolated component census for §6co producer (stack forwarding + compact private memory) on 32784 distinct native rays.
Sparkly Guy / Gemini 3.8 Flash High / Antigravity / 2026-10-22.
Runs Mode 0 (Current), Mode 1 (Bulk Connected), Mode 3 (Producer Only), Mode 4 (Consumer Only + Fallback)
in a 65536-capacity single partial chunk allocation on Apple Silicon M5 Metal.
"""
import argparse, importlib.util
from pathlib import Path

def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / f'{name}.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m

def source(base, root):
    q = load(root, 'ray_transport_gpu_pose_qualification')
    s = load(root, 'ray_transport_gpu_producer_stack_and_compact_state').source(base, root)
    s = q.once(s, 'for(uint32_t capacity:{8192u,16384u,32768u})', 'for(uint32_t capacity:{65536u})')
    s = q.once(s, 'else for(const auto& tile:tiles){', r'''
                else if(mode==3)dispatch(cpProducer,bgProducer,count);
                else if(mode==4){dispatch(cpUnfused,bgUnfused,count);dispatch(fallbackCP,fallbackBG,count);}
                else for(const auto& tile:tiles){''')
    s = q.once(s, '        J series=J::array();', r'''
        if(tiles.size()!=1)throw std::runtime_error("phase reuse precondition refused");
        {
            execute(4,1,false);auto got=readResults();
            for(size_t i=0;i<count;++i)for(int k=0;k<4;++k){
                const float expected=resUnfused[i].rgb_T[k];
                if(std::bit_cast<uint32_t>(got[i].rgb_T[k])!=std::bit_cast<uint32_t>(expected))throw std::runtime_error("phase consumer control changed output");
            }
        }
        J series=J::array();''')
    s = q.once(s, 'block%2?std::vector<int>{2,1,0,0,1,2}:std::vector<int>{0,1,2,2,1,0}',
                  'block%2?std::vector<int>{4,3,1,0,0,1,3,4}:std::vector<int>{0,1,3,4,4,3,1,0}')
    meta = r'''
    report["streamed_schedule"]["phase_only"]=true;
    report["streamed_schedule"]["phase_consumer_output_checks"]=true;
    report["streamed_schedule"]["mode_names"]={{"0","current"},{"1","bulk connected"},{"3","§6co producer only"},{"4","bulk consumer plus fallback"}};
    report["streamed_schedule"]["component_clocks_not_additive"]=true;
    report["streamed_schedule"]["scope"]="32784 distinct queries in 65536-capacity single partial chunk; paired 1/2/4-repetition complete host clocks; no full viewport, no occupancy diagnosis or native frame gain";
'''
    return q.once(s, '    std::cout << report.dump(2) << std::endl;', meta + '\n    std::cout << report.dump(2) << std::endl;')

def main():
    p = argparse.ArgumentParser(add_help=False)
    p.add_argument('--root', type=Path, required=True)
    a, _ = p.parse_known_args()
    root = a.root.resolve()
    q = load(root, 'ray_transport_gpu_pose_qualification')
    wide = load(root, 'ray_transport_gpu_wide_stream')
    q.source = lambda base, r: source(base, r)
    q.verify_case = lambda r, p, z: wide.verify_case(r, p, z)
    q.independent_check = wide.independent_check
    q.main()

if __name__ == '__main__':
    main()
