from pathlib import Path
import json,shlex,subprocess,os,shutil,hashlib
ROOT=Path('/Users/zacharyzhang/Documents/GitHub/Earthcall')
OUT=Path('/private/tmp/earthcall-metal-01a148a7-captures')
OUT.mkdir(exist_ok=False)
fixture=Path('/private/tmp/earthcall-metal-01a148a7-capture.cpp')
manifest={'source_head':'ee905784589ed42626c61c0c2e6fa67c39657ec3','parent':'5bcb63837','saves':{}}
for lane,source,build in [('after',ROOT,ROOT/'build'),('before',Path('/private/tmp/earthcall-metal-01a148a7-parent'),Path('/private/tmp/earthcall-metal-01a148a7-parent/build'))]:
    work=OUT/lane;work.mkdir();(work/'AGENTS.md').write_text('Isolated native verification fixture.\n')
    for name in ['src','imgui','resources','assets']:
        p=source/name
        if p.exists():(work/name).symlink_to(p,target_is_directory=True)
    for zone in ['Prism Cathedral','Radiance Gallery','Sky-Island Sanctuary']:
        d=ROOT/'saves/zones'/zone
        p=next((d/n for n in ['zone.ecform','zone.json'] if (d/n).exists()))
        target=work/'saves/zones'/zone/p.name;target.parent.mkdir(parents=True)
        shutil.copy2(p,target)
        manifest['saves'][str(p)]=hashlib.sha256(p.read_bytes()).hexdigest()
    # Complete existing reference closure, copied without author/identity edits.
    for zone in ['Prism Cathedral','Radiance Gallery','Sky-Island Sanctuary']:
        original=ROOT/'saves/zones'/zone
        path=next((original/n for n in ['zone.ecform','zone.json'] if (original/n).exists()))
        import msgpack
        raw=path.read_bytes()
        doc=json.loads(raw) if raw[:1]==b'{' else json.loads(msgpack.unpackb(raw)['MigrationRoot'])
        for ref in doc.get('lawRefs',[]):
            shutil.copytree(ROOT/'saves/laws'/ref,work/'saves/laws'/ref,dirs_exist_ok=True)
        for ref in doc.get('materialRefs',[]):
            name=ref.removeprefix('material.')
            shutil.copytree(ROOT/'saves/materials'/name,work/'saves/materials'/name,dirs_exist_ok=True)
    (work/'saves/persons').mkdir(exist_ok=True)
    shutil.copy2(ROOT/'saves/persons/Zach.ecform',work/'saves/persons/Zach.ecform')
    db=json.loads((build/'compile_commands.json').read_text())
    item=next(c for c in db if c['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in c['command'])
    args=shlex.split(item['command']);args[args.index('-c')+1]=str(fixture);args[args.index('-o')+1]=str(work/'capture.o')
    subprocess.run(args,cwd=build,check=True)
    link=shlex.split((build/'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    link[next(i for i,a in enumerate(link) if a.endswith('/src/entry.cpp.o'))]=str(work/'capture.o')
    link[link.index('-o')+1]=str(work/'capture')
    subprocess.run(link,cwd=build,check=True)
    (work/'compile.json').write_text(json.dumps(args,indent=2))
    (work/'link.json').write_text(json.dumps(link,indent=2))
    print('READY',lane,work/'capture',flush=True)
(OUT/'manifest.json').write_text(json.dumps(manifest,indent=2))
