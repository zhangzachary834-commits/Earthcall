"""Capture Metal GPU traces and pipeline facts for True Terminal S1 §6dt on Apple M5.
Isolated diagnostic child run only; strictly separated from clean timing series.
Antigravity / Gemini / 50cf28df-09e9-4c2b-a4d5-1f796abbc73a / 2026-10-26.
"""

import argparse, ast, hashlib, importlib.util, json, os, re, shlex, subprocess, sys
from pathlib import Path

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--build-root', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--case', type=Path, default=Path('/private/tmp/earthcall-wide-case/manifest.json'))
    a = p.parse_args()

    root = a.root.resolve()
    build_root = a.build_root.resolve()
    b = build_root / 'build'
    work = a.work.resolve()
    base = root / 'scratch/probes'

    if work.exists() or root == work or root in work.parents:
        raise ValueError('fresh external work required')
    work.mkdir(parents=True)

    spec = importlib.util.spec_from_file_location('terminal_s1', root / 'scratch/probes/ray_transport_gpu_6dt_true_terminal_s1.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)

    v = {x.targets[0].id: ast.literal_eval(x.value) for x in ast.parse((root / 'scratch/probes/ray_transport_metal_diagnostic.py').read_text()).body if isinstance(x, ast.Assign) and isinstance(x.value, ast.Constant) and isinstance(x.value.value, str)}
    cpp = m.make_terminal_s1_cpp_source(v['CPP_SOURCE'], root, wide=True)

    needle_main = 'int main(int argc, char** argv) {'
    assert cpp.count(needle_main) == 1
    cpp = cpp.replace(needle_main,
                      'extern "C" void rtInstallMetalObserver();\n' +
                      'extern "C" void rtBeginCapture(const char*);\n' +
                      'extern "C" void rtEndCapture();\n' +
                      'extern "C" void rtMetalSetContext(const char*,const char*,size_t);\n' +
                      needle_main + '\n    rtInstallMetalObserver();')

    hooks = [
        ('WGPUComputePipeline cp = wgpuDeviceCreateComputePipeline(gpu.device, &cpd);', 'rtMetalSetContext("rayMain",fullCode.data(),fullCode.size());\n        '),
        ('auto pipeline = wgpuDeviceCreateComputePipeline(gpu.device, &pd);', 'rtMetalSetContext(entry,text.data(),text.size());\n        '),
        ('auto pipeline=wgpuDeviceCreateComputePipeline(gpu.device,&pd);', 'rtMetalSetContext(entry,text.data(),text.size());\n        ')
    ]
    for needle, pre in hooks:
        assert cpp.count(needle) == 1, (needle, cpp.count(needle))
        cpp = cpp.replace(needle, pre + needle)

    hook_warmup = '        for(int k=0;k<2;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);execute(3,1,false);}'
    assert cpp.count(hook_warmup) == 1
    captures = (
        '        rtBeginCapture("' + str(work / 'current.gputrace') + '"); execute(0, 1, false); rtEndCapture();\n' +
        '        rtBeginCapture("' + str(work / 'true_terminal_s1.gputrace') + '"); execute(3, 1, false); rtEndCapture();\n'
    )
    cpp = cpp.replace(hook_warmup, captures + hook_warmup)

    (work / 'probe.cpp').write_text(cpp)
    observer = base / 'ray_transport_metal_pipeline_observer.mm'
    subprocess.run(['/usr/bin/clang++', '-std=c++20', '-O2', '-fobjc-arc', '-c', str(observer), '-o', str(work / 'observer.o')], check=True)

    db = json.loads((b / 'compile_commands.json').read_text())
    entry = next(x for x in db if x['file'].endswith('/entry.cpp') and 'earthcall_webgpu.dir' in x['command'])
    cmd = shlex.split(entry['command'])
    cmd[cmd.index('-c') + 1] = str(work / 'probe.cpp')
    cmd[cmd.index('-o') + 1] = str(work / 'probe.o')
    cmd += ['-std=c++20', '-O3']
    subprocess.run(cmd, cwd=b, check=True)

    cmd = shlex.split((b / 'CMakeFiles/earthcall_webgpu.dir/link.txt').read_text())
    cmd[next(i for i, x in enumerate(cmd) if x.endswith('/src/entry.cpp.o'))] = str(work / 'probe.o')
    cmd[cmd.index('-o') + 1] = str(work / 'probe')
    cmd += [str(work / 'observer.o'), '-framework', 'Metal', '-framework', 'Foundation']
    subprocess.run(cmd, cwd=b, check=True)

    zone = root / 'saves/zones/Northern Veil/zone.ecform'
    zone_before = hashlib.sha256(zone.read_bytes()).hexdigest()
    wide = importlib.util.spec_from_file_location('wide', root / 'scratch/probes/ray_transport_gpu_wide_stream.py')
    wm = importlib.util.module_from_spec(wide)
    wide.loader.exec_module(wm)

    meta, rays, _ = wm.verify_case(root, a.case.resolve(), zone_before)
    report = work / 'result.json'
    env = dict(os.environ,
               EC_RAY_PIPELINE_FACTS=str(work / 'pipeline-facts.jsonl'),
               METAL_CAPTURE_ENABLED='1',
               EC_POSE_RAYS=meta['rays'],
               EC_POSE_REFERENCE=meta['reference'],
               EC_POSE_CLOCK=str(rays['time']),
               EC_MATCH_REPEAT='1',
               EC_GPU_BUDGET='1024',
               EC_GPU_SUB_CAP='64',
               EC_GPU_OVER_CAP='16',
               EC_GPU_MARGIN='.001')
    env.pop('EC_GPU_GEOMETRY_CASE', None)

    with (work / 'native.log').open('x') as f:
        subprocess.run([str(work / 'probe'), str(report)], cwd=root, env=env, stdout=f, stderr=subprocess.STDOUT, check=True)

    print('SUCCESS: captured traces and pipeline facts in', work)

if __name__ == '__main__':
    main()
