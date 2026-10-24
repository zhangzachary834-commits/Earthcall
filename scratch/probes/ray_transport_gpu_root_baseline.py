"""Matched ray-count current/A* consumer control for the GPU root-proposal experiment.
Changes only batch sizes to 1/4/16; production marching and arithmetic are unchanged.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-21.
"""
import importlib.util,sys
from pathlib import Path
root=Path(sys.argv[sys.argv.index('--root')+1]).resolve()
spec=importlib.util.spec_from_file_location('matched',root/'scratch/probes/ray_transport_native_march_comparison.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
original=module.source
def source(s):
 s=original(s)
 a='std::vector<int>{1,2,4}';b='repeatFactor==1?64:4'
 assert s.count(a)==1 and s.count(b)==1
 return s.replace(a,'std::vector<int>{1,4,16}').replace(b,'repeatFactor==1?64:16').replace('4 at repeat>1','16 at repeat>1')
module.source=source
module.main()
