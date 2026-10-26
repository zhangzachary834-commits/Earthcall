"""Verify/export Shape Garden through a clean ray-transport build.
Codex / GPT-6.1 Sol / 01a10992-828e-7e80-890c-c64b09141e18 / 2026-10-20.
This compiles only a verification harness, never application behavior. Build
ray-root/build-sidebar/earthcall_webgpu first. All writes use isolated fixtures.
"""
from pathlib import Path
import argparse,hashlib,json,os,shlex,shutil,subprocess,tempfile

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--ray-root',type=Path,required=True)
 p.add_argument('--build',type=Path)
 p.add_argument('--native',action='store_true')
 p.add_argument('--output',type=Path)
 args=p.parse_args();repo=Path(__file__).resolve().parents[2];ray=args.ray_root.resolve();build=(args.build or ray/'build-sidebar').resolve()
 head=subprocess.check_output(['git','-c','core.fsmonitor=false','rev-parse','HEAD'],cwd=ray,text=True).strip()
 subprocess.run(['git','merge-base','--is-ancestor',head,'claude/ray-domain-transport-rung0'],cwd=ray,check=True)
 if subprocess.check_output(['git','diff','--name-only','--','src'],cwd=ray,text=True).strip():raise SystemExit('Use a clean pinned ray checkout; do not link changing headers.')
 stage=Path(tempfile.mkdtemp(prefix='shape-garden-proof-'));report=stage/'report';report.mkdir()
 (stage/'AGENTS.md').write_text('Isolated verification fixture; no inhabited saves or keys.\n')
 for name in ['src','imgui']:(stage/name).symlink_to(ray/name,target_is_directory=True)
 flags={}
 for line in (build/'CMakeFiles/earthcall_webgpu.dir/flags.make').read_text().splitlines():
  if ' = ' in line:
   k,v=line.split(' = ',1);flags[k]=shlex.split(v)
 obj=stage/'probe.o';binary=stage/'probe'
 with (report/'build.log').open('w') as log:
  subprocess.run(['/usr/bin/c++',*flags['CXX_DEFINES'],*flags['CXX_INCLUDES'],'-I'+str(ray/'tests'),*flags['CXX_FLAGS'],'-c',str(repo/'scratch/probes/law_line_shape_garden_probe.cpp'),'-o',str(obj)],stdout=log,stderr=subprocess.STDOUT,check=True)
  link=shlex.split((build/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());link[next(i for i,v in enumerate(link) if v.endswith('/src/entry.cpp.o'))]=str(obj);link[link.index('-o')+1]=str(binary)
  subprocess.run(link,cwd=build,stdout=log,stderr=subprocess.STDOUT,check=True)
 env=dict(os.environ,EARTHCALL_DATA=str(stage/'data'))
 command=[str(binary),str(repo/'saves'),str(repo/'examples/law_line_shape_garden_sentences.json'),str(stage/'data'),str(report)]
 with (report/'logic.log').open('w') as log:subprocess.run(command,cwd=stage,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 if args.native:
  data=stage/'native-data';shutil.copytree(report/'installed',data)
  (data/'data-home.json').write_text(json.dumps({'schema':1,'injected_by':'Codex isolated Shape Garden native fixture'}))
  (data/'persons').mkdir();(data/'persons/Zach.json').write_text(json.dumps({'displayName':'Zach','soulName':'Zach','position':[0,0,6],'velocity':[0,0,0],'body':{'shape':'humanoid','height':1.8},'injected_by':'Codex isolated native fixture; no private keys'}))
  command[3]=str(data);env['EARTHCALL_DATA']=str(data)
  with (report/'native.log').open('w') as log:subprocess.run(command+['--native'],cwd=stage,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
 metadata={'branch':'claude/ray-domain-transport-rung0','commit':head,'fixture':str(stage),'sourceSha256':hashlib.sha256((repo/'examples/law_line_shape_garden.txt').read_bytes()).hexdigest(),'logic':json.loads((report/'logic.json').read_text()),'native':args.native}
 (report/'verification.json').write_text(json.dumps(metadata,indent=2)+'\n')
 if args.output:shutil.copytree(report,args.output,dirs_exist_ok=True)
 print(json.dumps({'report':str(args.output or report),'commit':head,'checks':metadata['logic']['checks'],'native':args.native},indent=2))
if __name__=='__main__': main()
