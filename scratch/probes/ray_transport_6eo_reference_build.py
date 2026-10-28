"""Build an isolated optimized CPU-reference evaluator from unchanged source.
No build-tree grafts or writes: one original ScalarForm.cpp object is rebuilt in
fresh /tmp storage and substituted only in this standalone reference's link.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28. CPU validation economics only, never a production renderer change.
"""
import argparse
import json
from pathlib import Path
import shlex
import shutil
import subprocess

from ray_transport_6eo_reference_pilot import ROOT, GENERATOR, digest, git, need, read, write


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--base-build', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--link-position', choices=['original', 'first'], default='first')
    a = p.parse_args()
    base, work = a.base_build.resolve(), a.work.resolve()
    need(not work.exists() and ROOT not in work.parents, 'fresh external output required')
    metadata = read(base / 'source.json')
    build_root = Path(metadata['build_root'])
    build = build_root / 'build'
    need(metadata['cpp_sha256'] == digest(GENERATOR), 'base generator mismatch')
    need(metadata['engine_src_tree'] == git(ROOT, 'rev-parse', 'HEAD:src') ==
         git(build_root, 'rev-parse', 'HEAD:src'), 'engine tree mismatch')
    for root in (ROOT, build_root):
        need(not git(root, 'status', '--porcelain', '--', 'src'), 'dirty engine source')
    db = read(build / 'compile_commands.json')
    entry = next(r for r in db if r['file'].endswith('/ScalarForm.cpp') and
                 'earthcall_webgpu.dir' in r.get('command', ''))
    source = Path(entry['file'])
    relative_source = source.relative_to(build_root)
    need(digest(source) == digest(ROOT / relative_source), 'evaluator source mismatch')
    source_sha = digest(source)
    work.mkdir()
    obj = work / 'ScalarForm.cpp.o'
    command = shlex.split(entry['command'])
    old_object = (build / command[command.index('-o')+1]).resolve()
    original_object_sha = digest(old_object)
    command[command.index('-o')+1] = str(obj)
    # Keep the original compiler/default FP contract. No reassociation, relaxed
    # math or implementation edits. Later continuity/whole-pilot parity gates
    # must detect any optimization-induced numerical change before use.
    command += ['-O3']
    need(not any(x.startswith(('-ffast-math', '-Ofast', '-funsafe-math')) for x in command),
         'relaxed arithmetic forbidden')
    subprocess.run(command, cwd=build, check=True)
    shutil.copy2(base / 'probe.o', work / 'probe.o')
    link = shlex.split((build / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    link[next(i for i,x in enumerate(link) if x.endswith('/src/entry.cpp.o'))] = str(work / 'probe.o')
    link[next(i for i,x in enumerate(link) if x.endswith('/src/Singularity/OntoMath/ScalarForm.cpp.o'))] = str(obj)
    # Mach-O coalesces inline/template definitions across objects. Put the
    # optimized evaluator first so earlier Debug objects cannot select their
    # unoptimized copies of its shared helpers (including GLM noise). This is
    # link preference only; unchanged definitions still require exact pilot parity.
    if a.link_position == 'first':
        link.remove(str(obj))
        link.insert(1, str(obj))
    link[link.index('-o')+1] = str(work / 'probe')
    subprocess.run(link, cwd=build, check=True)
    need(digest(source) == source_sha and digest(old_object) == original_object_sha,
         'base source/object changed during isolated build')
    metadata['base_reference_binary_sha256'] = digest(base / 'probe')
    metadata['probe_object_sha256'] = digest(work / 'probe.o')
    metadata['optimized_engine_objects'] = [dict(source=str(source), source_sha256=source_sha,
         object=str(obj), object_sha256=digest(obj), original_object_sha256=original_object_sha,
         compile_argv=command, directory=str(build), optimization='-O3 without fast-math')]
    metadata['link_argv'] = link
    metadata['link_directory'] = str(build)
    metadata['optimized_weak_definitions_preferred'] = a.link_position == 'first'
    write(work / 'source.json', metadata)
    print(work / 'probe')


if __name__ == '__main__':
    main()
