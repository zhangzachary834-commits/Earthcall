"""Build one standalone probe against an explicitly source-matching engine build.
No build-tree source/header/object grafts; fresh external output only.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-25.
"""
import argparse,subprocess,shlex,json,hashlib
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--build-root',type=Path,required=True);p.add_argument('--source',type=Path,required=True);p.add_argument('--work',type=Path,required=True);a=p.parse_args()
root=a.root.resolve();build=a.build_root.resolve();src=a.source.resolve();work=a.work.resolve()
if work.exists() or root==work or root in work.parents:raise ValueError('fresh external work required')
trees=[]
for r in [root,build]:
 if subprocess.check_output(['git','-c','core.fsmonitor=false','status','--porcelain','--','src'],cwd=r,text=True).strip():raise ValueError('dirty engine source')
 trees.append(subprocess.check_output(['git','rev-parse','HEAD:src'],cwd=r,text=True).strip())
if trees[0]!=trees[1]:raise ValueError('engine source trees differ')
work.mkdir(parents=True,exist_ok=False);b=build/'build';db=json.loads((b/'compile_commands.json').read_text());e=next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command']);cmd=shlex.split(e['command']);cmd[cmd.index('-c')+1]=str(src);cmd[cmd.index('-o')+1]=str(work/'probe.o');cmd+=['-std=c++20','-O3','-I'+str(root/'scratch/probes')];subprocess.run(cmd,cwd=b,check=True)
cmd=shlex.split((b/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());cmd[next(i for i,x in enumerate(cmd) if x.endswith('/src/entry.cpp.o'))]=str(work/'probe.o');cmd[cmd.index('-o')+1]=str(work/'probe');subprocess.run(cmd,cwd=b,check=True)
(work/'source.json').write_text(json.dumps({'source':str(src),'cpp_sha256':hashlib.sha256(src.read_bytes()).hexdigest(),'build_root':str(build),'engine_src_tree':trees[0]},indent=2)+'\n');print(work/'probe')
