"""Native real-CLI Screen witness in an isolated first-seed store.
Codex / GPT-6.1 Sol / 01a10992-828e-7e80-890c-c64b09141e18 / 2026-10-06.
Build earthcall_webgpu first. No inhabited save or identity key is modified.
"""
from pathlib import Path
import os, shlex, subprocess, tempfile, shutil
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
subprocess.run([str(stage/'screen'),str(root)],cwd=stage,env=env,check=True)
report=root/'scratch/verification/law-line-screen-2026-10-06'
report.mkdir(parents=True,exist_ok=True)
for name in ('result.json','gradient.png','pixel.png','lens-t0.png','lens-t1.png'):
    shutil.copy2(stage/name,report/name)
print('Retained evidence:',report)
