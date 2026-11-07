"""Patch-install the Zone compiled through Law Line; preserve all existing work.
Codex / GPT-6.1 Sol / 01a10992-828e-7e80-890c-c64b09141e18 / 2026-10-20.
New dependency roots first, authoritative Zone last. Existing roots are never
rewritten; a repeat is a no-op. Inhabited updates require the storage office.
"""
from pathlib import Path
import argparse,hashlib,json,os,tempfile
from earthcall_data_home import data_root

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--data-root',type=Path,default=None)
    parser.add_argument('--bundle',type=Path,default=Path(__file__).resolve().parents[1]/'examples/law_line_shape_garden_bundle')
    parser.add_argument('--dry-run',action='store_true')
    args=parser.parse_args(); root=args.data_root or data_root();bundle=args.bundle
    manifest=json.loads((bundle/'manifest.json').read_text())
    zone_path=Path('zones/ShapeGarden/zone.json')
    paths=[Path('laws')/identifier/'law.json' for identifier in manifest['lawIds']]+[zone_path]
    assert len(set(paths))==len(paths)
    pending=[]
    for rel in paths:
        source=bundle/rel;source_bytes=source.read_bytes();new=json.loads(source_bytes)
        assert hashlib.sha256(source_bytes).hexdigest()==manifest['sha256'][str(rel)],f'Bundle checksum mismatch: {rel}'
        target=root/rel
        if target.exists():
            old=json.loads(target.read_text())
            if rel==zone_path:
                # Do not replace the Person's generated objects, sidebar state,
                # ownership, Relations or geometry with an installation snapshot.
                if old.get('identifier')!='ShapeGarden' or not str(old.get('injected_by','')).startswith('Codex / GPT-6.1 Sol / 01a10992'):
                    raise SystemExit('Refusing to replace an independently authored ShapeGarden Zone.')
                if not set(manifest['lawIds']).issubset(set(old.get('lawRefs',[]))):
                    raise SystemExit('Existing Zone needs a targeted authored update; use the storage office.')
            elif old!=new:
                raise SystemExit(f'Existing Law differs; preserving it: {target}')
            continue
        # A missing JSON must not hide an existing semantic/binary root.
        if any(target.parent.glob('*.ecform')) or any(target.parent.glob('*.ecmatter')):
            raise SystemExit(f'Existing semantic root needs the storage office: {target.parent}')
        pending.append((target,source_bytes))
    if args.dry_run:
        print(json.dumps({'root':str(root),'newFiles':[str(p.relative_to(root)) for p,_ in pending],'author':manifest['author']},indent=2));return
    # This operation adds new roots only: no existing bytes are changed or
    # deleted, so there is no old file to back up. Each file is staged beside
    # its destination and checked before its final atomic rename.
    for target,content in pending:
        target.parent.mkdir(parents=True,exist_ok=True)
        fd,name=tempfile.mkstemp(prefix='.shape-garden-',suffix='.json',dir=target.parent)
        try:
            with os.fdopen(fd,'wb') as out:
                out.write(content);out.flush();os.fsync(out.fileno())
            staged=Path(name)
            assert staged.read_bytes()==content
            json.loads(staged.read_text())
            if target.exists():raise RuntimeError(f'Concurrent root appeared; refusing overwrite: {target}')
            os.replace(staged,target)
        finally:
            if Path(name).exists():Path(name).unlink()
    print(json.dumps({'root':str(root),'added':len(pending),'zone':'ShapeGarden','lawCount':len(manifest['lawIds']),'author':manifest['author']},indent=2))
if __name__=='__main__': main()
