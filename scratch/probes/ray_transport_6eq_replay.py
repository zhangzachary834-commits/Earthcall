"""Replay stored native Screen-publication pixels and intended rejections.
No new GPU execution, routed numerical admission, or performance measurement.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28. Sonnet Bridge/Firmament proposal; commissioned by Zach.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tempfile
import zipfile

import ray_transport_6eq_screen_bridge as B


def sha(b):return hashlib.sha256(b).hexdigest()


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--record',type=Path,required=True)
    p.add_argument('--archive',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    rec=B.read(a.record);B.need(B.digest(a.archive)==rec['archive_sha256'],'archive identity')
    positives=[];negatives=[];shader_restorations=0
    lane=re.findall(r'R"WGSL\((.*?)\)WGSL"',B.HOOKS,re.S)[0]
    binding='\n@group(0) @binding(3) var<storage, read> routedPublication: array<vec4<f32>>;'
    with zipfile.ZipFile(a.archive) as z,tempfile.TemporaryDirectory(prefix='screen-6eq-replay-') as tmp:
        B.need(len(z.namelist())==len(set(z.namelist()))==len(rec['members_sha256']),'archive members')
        for n,h in rec['members_sha256'].items():B.need(sha(z.read(n))==h,'member identity '+n)
        build=json.loads(z.read('build/build.json'));work=Path(tmp)
        B.need(build['binaries']['bridge']!=build['binaries']['control'],'independent control binary')
        control_link=build['link_argv']['control'];bridge_link=build['link_argv']['bridge']
        renderer_object=next(v for v in control_link if v.endswith('/src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp.o'))
        expected_link=list(control_link)
        expected_link[expected_link.index(renderer_object)]=build['compile_argv'][1][build['compile_argv'][1].index('-o')+1]
        expected_link[expected_link.index('-o')+1]=bridge_link[bridge_link.index('-o')+1]
        B.need(expected_link==bridge_link,'control/bridge link differs beyond isolated renderer')
        B.need(B.digest(B.RENDERER)==build['original_renderer_sha256'] and
               B.digest(B.PROBE)==build['original_probe_sha256'],'source tree drift')
        B.need(B.renderer_source(B.RENDERER.read_text()).encode()==z.read('build/WebGpuRenderer.cpp') and
               B.probe_source(B.PROBE.read_text()).encode()==z.read('build/screen-probe.cpp'),'compiled source regeneration')
        # Renderer compilation retains the original CMake flags, not the O3
        # profile of the common diagnostic main.
        renderer_flags=build['compile_argv'][1]
        B.need(not any(v.startswith(('-O','-ffast','-ffp')) for v in renderer_flags),'renderer profile changed')
        cases={c['case']:c for c in rec['cases']}
        off=cases['off-odd']['shaders']
        for case in rec['cases']:
            name=case['case'];directory=work/name;directory.mkdir()
            pixels=z.read(case['image_member']);receipt=json.loads(z.read('cases/'+name+'/receipt.json'))
            (directory/'capture.rgba').write_bytes(pixels);B.write(directory/'receipt.json',receipt)
            B.need(receipt['rgba_sha256']==sha(pixels) and receipt['before']==receipt['after'], 'native receipt identity')
            B.need(receipt['before']['build_receipt_sha256']==sha(z.read('build/build.json')) and
                   receipt['before']['zone_sha256']==rec['zone_sha256'] and
                   receipt['before']['binary_sha256']==build['binaries'][case['binary']], 'native execution binding')
            log=z.read('cases/'+name+'/run.log').decode()
            B.need(receipt['returncode']==0 and 'SOURCE mode=authored radiance=1' in log and
                   not any(v in log.lower() for v in ('validation error','uncaptured error','shader failed','panic')), 'native environment/source refusal')
            diag=receipt['diagnostic'];enabled=case['mode'] not in ('off','budget-refuse')
            B.need(diag['enabled']==enabled and diag['mode']==case['mode'],'lane admission')
            B.need(diag['buffer_bytes']==(case['width']*case['height']*16 if enabled else 0) and
                   diag['initial_uploads']==int(enabled) and
                   (diag['buffer_bindings']>=1 if enabled else diag['buffer_bindings']==0), 'buffer binding/lifetime')
            if case['binary']=='control':B.need(diag['shader_calls']==0,'unmodified renderer control')
            else:B.need(diag['shader_calls']>=1,'actual renderer pipeline hook')
            for n,h in case['shaders'].items():
                text=z.read('cases/'+name+'/shaders/'+n).decode();B.need(sha(text.encode())==h,'encountered WGSL identity')
                if enabled and not case.get('negative'):
                    B.need(text.count(lane)==text.count(binding)==1,'publication WGSL sites')
                    restored=text.replace(lane,'').replace(binding,'')
                    B.need(sha(restored.encode()) in set(off.values()),'Current math changed beyond publication lane')
                    shader_restorations+=1
            if case['mode']=='budget-refuse':B.need(case['shaders']==off,'budget refusal must reuse lane-off shader')
        for case in rec['cases']:
            if case.get('expect') is None:continue
            name=case['case'];directory=work/name
            control=work/case['control']/'capture.rgba' if case.get('control') else None
            if case.get('negative'):
                try:B.verify_image(directory,case['expect'],control)
                except ValueError as e:
                    B.need(case['expected_error'] in str(e),'wrong native negative boundary')
                    stored=z.read('cases/'+name+'/rejection.log').decode()
                    B.need(case['expected_error'] in stored and case['validator_returncode']==2,'stored targeted rejection')
                    negatives.append(name)
                else:raise ValueError('native negative control admitted '+name)
                # Distinguish intended semantics from an unrelated failed draw.
                if name=='mut-order':B.need((directory/'capture.rgba').read_bytes()==bytes([32,64,128,255])*(case['width']*case['height']),'wrong-order discard signature')
                if name=='mut-bypass':B.need((directory/'capture.rgba').read_bytes()==(work/'control-odd/capture.rgba').read_bytes(),'omitted read must show Current')
            else:
                result=B.verify_image(directory,case['expect'],control)
                B.need(result==json.loads(z.read('cases/'+name+'/verification.json')),'stored pixel validator result')
                positives.append(name)
    B.need(len(positives)==11 and len(negatives)==2 and len(rec['cases'])==16,'bounded witness count')
    report=dict(native_cases_replayed=len(rec['cases']),positive_pixel_checks=positives,
         targeted_native_rejections=negatives,shader_text_restorations=shader_restorations,
         full_native_refusal_rgba_bytes=2560*1440*4,routed_transport_connected=False,
         opaque_geometry_depth_tested=False,gpu_performance_measured=False)
    B.write(a.output,report);print('PASS:16 stored native cases,11 pixel checks,2 targeted rejections; Screen protocol only')


if __name__=='__main__':main()
