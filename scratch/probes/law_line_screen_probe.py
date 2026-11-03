"""Native real-CLI Screen witness in an isolated first-seed store.
Codex / GPT-6.1 Sol / 01a10992-828e-7e80-890c-c64b09141e18 / 2026-10-06.
Build earthcall_webgpu first. No inhabited save or identity key is modified.
"""
from pathlib import Path
import os, shlex, subprocess, tempfile, shutil, sys, json, hashlib
from datetime import datetime, timezone
art_editor = '--art-editor' in sys.argv[1:]
property_bearers = '--property-bearers' in sys.argv[1:]
if art_editor and property_bearers:
    raise SystemExit('Choose --art-editor or --property-bearers, not both.')
root = Path(__file__).resolve().parents[2]
build = root / 'build'
stage = Path(tempfile.mkdtemp(prefix='earthcall-cli-screen-'))
(stage/'AGENTS.md').write_text('Isolated direct Screen CLI fixture.\n')
for name in ('src','imgui'):
    (stage/name).symlink_to(root/name, target_is_directory=True)
flags={}
for line in (build/'CMakeFiles/earthcall_webgpu.dir/flags.make').read_text().splitlines():
    if ' = ' in line:
        name,value=line.split(' = ',1); flags[name]=shlex.split(value)
obj=stage/'screen.o'
subprocess.run(['/usr/bin/c++',*flags['CXX_DEFINES'],*flags['CXX_INCLUDES'],*flags['CXX_FLAGS'],
    '-c',str(root/'scratch/probes/law_line_screen_probe.cpp'),'-o',str(obj)],check=True)
link=shlex.split((build/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
link[next(i for i,x in enumerate(link) if x.endswith('/src/entry.cpp.o'))]=str(obj)
link[link.index('-o')+1]=str(stage/'screen')
subprocess.run(link,cwd=build,check=True)
print('Native witness directory:',stage,flush=True)
env={k:v for k,v in os.environ.items() if not k.startswith('EARTHCALL_')}
env['EARTHCALL_TERMINAL_HISTORY']=str(stage/'history.txt')
env['EARTHCALL_DATA']=str(stage/'saves')
mode=['--property-bearers'] if property_bearers else ['--art-editor'] if art_editor else []
if property_bearers:
    with (stage/'native-witness.log').open('w') as log:
        result=subprocess.run([str(stage/'screen'),str(root),*mode],cwd=stage,env=env,stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:
        print('\n'.join((stage/'native-witness.log').read_text().splitlines()[-45:]))
        raise SystemExit(result.returncode)
    evidence=json.loads((stage/'result.json').read_text())
    evidence.update(checkout=str(root), capturedAt=datetime.now(timezone.utc).isoformat(),
        head=subprocess.check_output(['git','-c','core.fsmonitor=false','rev-parse','HEAD'],cwd=root,text=True).strip(),
        branch=subprocess.check_output(['git','-c','core.fsmonitor=false','branch','--show-current'],cwd=root,text=True).strip(),
        fixtureRoot=str(stage/'saves'),
        probeSourceSha256=hashlib.sha256((root/'scratch/probes/law_line_screen_probe.cpp').read_bytes()).hexdigest())
    (stage/'result.json').write_text(json.dumps(evidence,indent=2)+'\n')
else:
    subprocess.run([str(stage/'screen'),str(root),*mode],cwd=stage,env=env,check=True)
report=root/('scratch/verification/singular-screen-properties-2026-10-17' if property_bearers else 'scratch/verification/law-line-pixel-art-editor-2026-10-07' if art_editor else 'scratch/verification/law-line-screen-regions-2026-10-07')
report.mkdir(parents=True,exist_ok=True)
names=['result.json','gradient.png','pixel.png','lens-t0.png','lens-t1.png','region-gold.png','region-cyan.png','person-region.png']
if art_editor:names += ['art-'+name+'.png' for name in ('blank','gold','undo','redo','cyan','erase','clear')]
if property_bearers:
    names=['result.json','native-witness.log','property-bearer-programs.txt']
    names += [f'property-screen-{bearer}-{phase}.png' for bearer in ('singular','object') for phase in ('bands','edited','pixel')]
for name in names:
    shutil.copy2(stage/name,report/name)
print('Retained evidence:',report)
