"""Fresh Release app objects, real Engine/window, isolated frozen scene and path.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28; §6fe derivative: Claude Code / Sonnet 5.5 / worktree authorable-gpu-runs-268d29 / 2026-10-29.
"""
import argparse,datetime as dt,hashlib,json,os,plistlib,shlex,shutil,subprocess,time
from pathlib import Path
import ray_transport_6ew_native as N
R=N.R
ROOT=Path(__file__).resolve().parents[2]
BUILD=Path('/private/tmp/earthcall-6fe-release-build')
CPP=Path(__file__).with_suffix('.cpp')

def need(v,msg):
 if not v:raise ValueError(msg)

def tree_hash(path):return {str(p.relative_to(path)):R.sha(p) for p in path.rglob('*') if p.is_file()}

def compile(work):
 need(not work.exists(),'fresh engine harness');work.mkdir();before=R.snapshot();cache=(BUILD/'CMakeCache.txt').read_text();need('CMAKE_BUILD_TYPE:STRING=Release' in cache and 'CMAKE_HOME_DIRECTORY:INTERNAL='+str(ROOT) in cache,'correct Release cache');db=R.read(BUILD/'compile_commands.json');rows=[r for r in db if 'earthcall_webgpu.dir' in r['command']];need(rows and all('-O3' in shlex.split(r['command']) and '-DNDEBUG' in shlex.split(r['command']) for r in rows if r['file'].endswith('.cpp')),'all app C++ compiled Release');row=next(r for r in rows if r['file'].endswith('/entry.cpp'));cmd=shlex.split(row['command']);cmd[cmd.index('-c')+1]=str(CPP);cmd[cmd.index('-o')+1]=str(work/'engine.o');cmd+=['-std=c++17'];link=shlex.split((BUILD/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text());link[next(i for i,v in enumerate(link) if v.endswith('/src/entry.cpp.o'))]=str(work/'engine.o');link[link.index('-o')+1]=str(work/'engine')
 with (work/'build.log').open('w') as f:
  subprocess.run(cmd,cwd=BUILD,stdout=f,stderr=subprocess.STDOUT,check=True);subprocess.run(link,cwd=BUILD,stdout=f,stderr=subprocess.STDOUT,check=True)
 bundle=work/'Earthcall Transport Witness.app';mac=bundle/'Contents/MacOS';mac.mkdir(parents=True);(mac/'engine').symlink_to(work/'engine');(bundle/'Contents/Info.plist').write_bytes(plistlib.dumps(dict(CFBundleIdentifier='org.earthcall.transport-witness-6ey',CFBundleExecutable='engine',CFBundleName='Earthcall Transport Witness',CFBundlePackageType='APPL',NSHighResolutionCapable=True)));need((mac/'engine').resolve()==work/'engine','unbundled diagnostic executable remains outside bundle');objects={str(p):R.sha(p) for p in (BUILD/'CMakeFiles/earthcall_webgpu.dir').rglob('*.o')};meta=dict(before=before,after=R.snapshot(),build_type='Release',cache_sha256=R.sha(BUILD/'CMakeCache.txt'),compile_database_sha256=R.sha(BUILD/'compile_commands.json'),compile_argv=cmd,link_argv=link,object_sha256=objects,source_sha256=R.sha(CPP),main_object_sha256=R.sha(work/'engine.o'),binary_sha256=R.sha(work/'engine'),tools_sha256={p.name:R.sha(p) for p in [Path(__file__),CPP]},release_configure=R.read(BUILD/'configure-receipt.json'),release_build=R.read(BUILD/'build-receipt.json'));need(before==meta['after'],'fixed engine source');R.write(work/'build.json',meta);print('PASS real Release Engine harness',len(objects),'objects',flush=True)

def stage(directory):
 directory.mkdir();(directory/'AGENTS.md').write_text('Isolated scripted Engine fixture; no live saves or keys.\n');saves=directory/'saves';saves.mkdir();(saves/'zones/Northern Veil').mkdir(parents=True);shutil.copyfile(R.ZONE,saves/'zones/Northern Veil/zone.ecform');shutil.copytree(ROOT/'saves/materials',saves/'materials');(saves/'persons').mkdir();R.write(saves/'persons/Zach.json',dict(displayName='Zach',soulName='Zach',position=[0,48.38,75],velocity=[0,0,0],body=dict(shape='humanoid',artStyle='default',height=1.8),injected_by='Codex / GPT-6.1 Sol / isolated human-perspective test fixture',authors=['Zachary Zhang']));R.write(saves/'data-home.json',dict(earthcallDataHome=1,fixture=True));
 for name in ['src','imgui']:(directory/name).symlink_to(ROOT/name,target_is_directory=True)
 return saves

def run(work,mode):
 need(mode in ['preflight','negative','timing','session','parity','packaging','census','skiptiming','worktrace','worktrace_in','parity2b','parity2b_more'],'mode');meta=R.read(work/'build.json');need(R.snapshot()==meta['after'] and R.sha(work/'engine')==meta['binary_sha256'],'actual source/binary');need(R.sha(Path(__file__))==meta['tools_sha256'][Path(__file__).name] and R.sha(CPP)==meta['source_sha256'],'executed tools');directory=work/mode;saves=stage(directory);before=R.snapshot();frozen=tree_hash(saves);budgetfile=work.parent/('earthcall-6fe-native-budget.json' if mode in ['census','skiptiming','worktrace','worktrace_in','parity2b','parity2b_more'] else 'earthcall-6fb-native-budget.json' if mode in ['parity','packaging','parity2b'] else 'earthcall-6ey-native-budget.json');budget=R.read(budgetfile) if budgetfile.exists() else dict(seconds=0);cap=300 if mode in ['census','skiptiming','worktrace','worktrace_in','parity2b','parity2b_more'] else 600 if mode in ['parity','packaging','parity2b'] else 900;need(budget['seconds']<cap,'predeclared %ds cumulative process cap'%cap);args=[str(work/'Earthcall Transport Witness.app/Contents/MacOS/engine'),mode,str(directory/'result.json')];env={k:v for k,v in os.environ.items() if not k.startswith(('EC_','EARTHCALL_'))};env['EARTHCALL_DATA']=str(saves);log=directory/'native.log';t=time.monotonic();started=dt.datetime.now(dt.timezone.utc).isoformat()
 with log.open('w') as f:
  try:code=subprocess.run(args,cwd=directory,env=env,stdout=f,stderr=subprocess.STDOUT,timeout=min(300,cap-budget['seconds'])).returncode
  except subprocess.TimeoutExpired:code='timeout'
 elapsed=time.monotonic()-t;R.write(budgetfile,dict(seconds=budget['seconds']+elapsed));receipt=dict(mode=mode,argv=args,cwd=str(directory),started_utc=started,elapsed_seconds=elapsed,exit_code=code,build_sha256=R.sha(work/'build.json'),binary_sha256=R.sha(work/'engine'),before=before,after=R.snapshot(),initial_fixture_sha256=frozen,final_zone_sha256=R.sha(saves/'zones/Northern Veil/zone.ecform'),files_sha256={p.name:R.sha(p) for p in directory.iterdir() if p.is_file() and p.name not in ['AGENTS.md','receipt.json']});R.write(directory/'receipt.json',receipt);need(receipt['before']==receipt['after'] and receipt['final_zone_sha256']==frozen['zones/Northern Veil/zone.ecform'],'source/frozen scene preserved');text=log.read_text();need(not any(s in text.lower() for s in ['validation error','uncaptured error','device lost','[routed compiler]']),'native GPU/compiler failure')
 if mode=='negative':need(code==42 and 'scripted camera pose was not executed' in text,'intended missing-movement rejection')
 else:need(code==0 and 'PASS actual Engine '+mode in text,'actual Engine execution')
 print('PASS',mode,'seconds',round(elapsed,3),flush=True)

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('command',choices=['build','preflight','negative','timing','session','parity','packaging','census','skiptiming','worktrace','worktrace_in','parity2b','parity2b_more']);p.add_argument('--work',type=Path,required=True);a=p.parse_args();compile(a.work.resolve()) if a.command=='build' else run(a.work.resolve(),a.command)
if __name__=='__main__':main()
