"""Replay section6es native float outputs, own-input references and common target.
Stored native/CPU observations only; no shipping admission or new GPU execution.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28. Implements Zach's requirement that changed appearance be judged by math.
"""
import argparse
import copy
import hashlib
import json
import math
from pathlib import Path
import struct
import tempfile
import zipfile

import ray_transport_6es_optical_setup as S
import ray_transport_6es_camera_reference as M
import ray_transport_fact_codec as C
B=S.B


def stats(v):
    v=sorted(v);B.need(v and all(math.isfinite(x) for x in v),'finite metrics')
    return dict(mean=sum(v)/len(v),p99=v[len(v)*99//100],max=v[-1])


def errors(rows,reference):
    B.need(len(rows)==len(reference)==80,'paired pilot coverage')
    return {label:dict(T=stats([abs(a[7]-b[off]) for a,b in zip(rows,reference)]),
                      RGB=stats([abs(a[7+k]-b[off+k]) for a,b in zip(rows,reference) for k in range(1,4)]))
            for label,off in [('finest',0),('half',4)]}


def gates(normal,ordered):
    return {res:{channel:{stat:ordered[res][channel][stat]<=normal[res][channel][stat]
            for stat in ['mean','p99','max']} for channel in ['T','RGB']} for res in ['finest','half']}


def capture_binding(actual,expected):
    B.need(actual==expected,'native capture input binding')


def reference_check(cap,ref,receipt,selection_sha,input_sha,ref_sha,source_meta_sha):
    B.need(receipt['status']=='PILOT_CONVERGED' and receipt['exit_code']==0,'reference process/convergence status')
    B.need(receipt['before']==receipt['after'] and receipt['before']['zone_sha256']==B.digest(B.ZONE),'reference execution source/fixture')
    B.need(receipt['before']['generator_sha256']==S.E.derivation_identity()['derived_generator_sha256'],'reference derivation')
    B.need(receipt['before']['build_receipt_sha256']==source_meta_sha,'reference compiled source receipt')
    B.need(receipt['selection_sha256']==selection_sha and receipt['input_sha256']==receipt['input_sha256_after']==input_sha and receipt['reference_sha256']==ref_sha,'reference input/output binding')
    B.need(ref['columns']==['T','Lr','Lg','Lb','T_half','Lr_half','Lg_half','Lb_half'] and len(ref['rows'])==len(cap['rays'])==80,'reference coverage/order')
    B.need(all(len(row)==8 and all(math.isfinite(v) for v in row) for row in ref['rows']),'reference finite rows')
    t=max(abs(v[0]-v[4]) for v in ref['rows']);rgb=max(abs(v[k]-v[k+4]) for v in ref['rows'] for k in range(1,4))
    B.need(t==receipt['max_convergence_T'] and rgb==receipt['max_convergence_RGB'] and max(t,rgb)<=1e-6,'reference convergence')
    B.need(receipt['argv'][2:5]==['16','80','8'] and 0<receipt['elapsed_seconds']<300,'bounded reference command')


def summarize(work,pilot,ideal,prior):
    observer={};image_reconstruction={}
    for scene,oldcase in [('clear','inside-clear'),('front','inside-front')]:
        control=(work/(scene+'-control')/'capture.rgba').read_bytes()
        for kind,previous in [('normal',oldcase),('ordered','canonical-'+scene)]:
            tag=scene+'-'+kind;directory=work/tag;receipt,raw,u,f=S.arrays(directory)
            image=(directory/'capture.rgba').read_bytes();old=(prior/previous/'capture.rgba').read_bytes();oldraw=(prior/previous/'capture.rgba.fragment.bin').read_bytes()
            observer[tag]=dict(changed_vs_current=sum(x!=y for x,y in zip(image,control)),changed_vs_prior=sum(x!=y for x,y in zip(image,old)),
                prefix_changes=sum(raw[i:i+48]!=oldraw[i:i+48] for i in range(0,len(raw),64)),valid=len(S.optical_check(directory)))
            B.need(observer[tag]['changed_vs_prior']==observer[tag]['prefix_changes']==0,'optical instrumentation observer effect')
            B.need(observer[tag]['changed_vs_current']==(0 if kind=='normal' else 232 if scene=='clear' else 15),'original Current image comparison')
            changed=0;pixels=0
            for i in range(len(raw)//64):
                if scene=='front' and i%334<167:continue # only independently known black background
                a=i*16;B.need(u[a+8]==1 and u[a+9]==3,'publication reconstruction coverage')
                T=f[a+12];L=list(f[a+13:a+16]);B.need(0<=T<=1,'captured T')
                alpha=struct.unpack('<f',struct.pack('<f',1-T))[0]
                expected=bytes([0,0,0,255]) if alpha<=1e-5 and max(abs(x) for x in L)<=1e-6 else bytes([max(0,min(255,round(255*x))) for x in L]+[255])
                changed+=sum(x!=y for x,y in zip(expected,image[4*i:4*i+4]));pixels+=1
            B.need(changed==0,'float output does not reproduce observable black-background pixels')
            image_reconstruction[tag]=dict(pixels=pixels,changed_bytes=changed)
    own={};common={}
    for scene in ['clear','front']:
        for kind in ['normal','ordered']:
            tag=scene+'-'+kind;cap=B.read(pilot/(tag+'-rays.json'));ref=B.read(pilot/(tag+'-reference.json'))
            own[tag]=errors(cap['rays'],ref['rows'])
    reference=B.read(ideal/'clear-ideal-reference.json')['rows']
    for kind in ['normal','ordered']:common[kind]=errors(B.read(pilot/('clear-'+kind+'-rays.json'))['rays'],reference)
    caps=[B.read(f) for f in list(pilot.glob('*-rays.json'))+list(ideal.glob('*-rays.json'))]
    return dict(observer=observer,black_background_reconstruction=image_reconstruction,
        own_input_errors=own,own_input_gates={scene:gates(own[scene+'-normal'],own[scene+'-ordered']) for scene in ['clear','front']},
        common_ideal_errors=common,common_ideal_pilot_gates=gates(common['normal'],common['ordered']),
        reference_rows=400,distinct_reference_inputs=len({tuple([*c['origin'],c['time'],*r[:7]]) for c in caps for r in c['rays']}),
        distinct_pixel_positions=len({tuple(r[:2]) for c in caps for r in c['rays']}),
        full_population_qualified=False,production_contract_adopted=False,gpu_performance_measured=False)


def replay(record,archive,parent_record,parent_archive,out):
    rec=B.read(record);B.need(B.digest(archive)==rec['archive_sha256'],'6es archive identity')
    B.need(B.digest(parent_record)==rec['parent_record_sha256'] and B.digest(parent_archive)==rec['parent_archive_sha256'],'6er parent identity')
    parent=B.read(parent_record);B.need(B.digest(parent_archive)==parent['archive_sha256'],'6er archive bound to record')
    with tempfile.TemporaryDirectory(prefix='6es-replay-') as tmp,zipfile.ZipFile(archive) as z,zipfile.ZipFile(parent_archive) as pz:
        root=Path(tmp);work=root/'native';pilot=root/'pilot';ideal=root/'ideal';prior=root/'prior'
        for d in [work,pilot,ideal,prior]:d.mkdir()
        B.need(len(z.namelist())==len(set(z.namelist()))==len(rec['members_sha256']),'6es archive members')
        for name,h in rec['members_sha256'].items():B.need(hashlib.sha256(z.read(name)).hexdigest()==h,'member identity '+name)
        meta=json.loads(z.read('build/build.json'))
        B.need(S.R.renderer_source(B.RENDERER.read_text()).encode()==z.read('build/WebGpuRenderer.cpp') and
               S.probe_source(B.PROBE.read_text()).encode()==z.read('build/screen-probe.cpp'),'native source derivation')
        B.need(hashlib.sha256(z.read('build/WebGpuRenderer.cpp')).hexdigest()==meta['generated_renderer_sha256'] and hashlib.sha256(z.read('build/screen-probe.cpp')).hexdigest()==meta['generated_probe_sha256'],'native compiled source')
        B.need('glm::vec4(0, 0, 0, 1)' in z.read('build/screen-probe.cpp').decode(),'actual black background')
        expected=list(meta['link_argv']['control']);old=next(v for v in expected if v.endswith('/src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp.o'))
        expected[expected.index(old)]=meta['compile_argv'][1][meta['compile_argv'][1].index('-o')+1];expected[expected.index('-o')+1]=meta['link_argv']['bridge'][meta['link_argv']['bridge'].index('-o')+1]
        B.need(expected==meta['link_argv']['bridge'],'unmodified renderer link control')
        for case in rec['cases']:
            directory=work/case['name'];directory.mkdir();receipt=json.loads(z.read(case['receipt']))
            B.need(receipt['returncode']==0 and receipt['before']==receipt['after'] and receipt['before']['engine_src_tree']==rec['engine_src_tree'] and receipt['before']['zone_sha256']==B.digest(B.ZONE),'native source execution')
            B.need(receipt['before']['build_receipt_sha256']==hashlib.sha256(z.read('build/build.json')).hexdigest() and receipt['before']['binary_sha256']==meta['binaries'][case['binary']],'native binary build binding')
            log=z.read('native/'+case['name']+'/run.log').decode();B.need('SOURCE mode=authored radiance=1' in log and 'clock=7.250000000' in log and not any(v in log.lower() for v in ['validation error','uncaptured error','panic']),'native authored clock/environment')
            B.write(directory/'receipt.json',receipt);image=z.read(case['image']);B.need(hashlib.sha256(image).hexdigest()==receipt['rgba_sha256'] and len(image)==334*190*4,'native image coverage');(directory/'capture.rgba').write_bytes(image)
            for stage,info in case.get('facts',{}).items():
                raw=C.decode(z.read(info['member']),info['raw_bytes']);B.need(hashlib.sha256(raw).hexdigest()==info['raw_sha256'],'lossless optical facts');(directory/('capture.rgba.'+stage+'.bin')).write_bytes(raw)
            if case['name']=='mut-no-optics':
                try:S.optical_check(directory)
                except ValueError as e:B.need(str(e)=='invalid optical value','missing optical writer failure boundary')
                else:raise ValueError('missing native optical write admitted')
        for case in parent['cases']:
            if case['case'] not in ['inside-clear','inside-front','canonical-clear','canonical-front']:continue
            directory=prior/case['case'];directory.mkdir();(directory/'capture.rgba').write_bytes(pz.read(case['image_member']))
            info=parent['facts'][case['facts']['fragment']];raw=C.decode(pz.read(info['member']),info['raw_bytes']);B.need(hashlib.sha256(raw).hexdigest()==info['raw_sha256'],'prior observer facts');(directory/'capture.rgba.fragment.bin').write_bytes(raw)
        for location,directory in [('pilot',pilot),('ideal',ideal)]:
            for name in z.namelist():
                if name.startswith(location+'/') and name.count('/')==1:(directory/Path(name).name).write_bytes(z.read(name))
        generated=root/'regenerated';S.prepare(work,generated)
        for name in ['clear-normal','clear-ordered','front-normal','front-ordered']:capture_binding(B.read(pilot/(name+'-rays.json')),B.read(generated/(name+'-rays.json')))
        generated_ideal=root/'regenerated-ideal';S.prepare_ideal(pilot,generated_ideal);B.need(B.read(generated_ideal/'clear-ideal-rays.json')==B.read(ideal/'clear-ideal-rays.json'),'independent ideal-camera input binding')
        sm=json.loads(z.read('reference-build/source.json'));source_meta_sha=hashlib.sha256(z.read('reference-build/source.json')).hexdigest()
        derived=S.E.derive_source(S.E.GENERATOR.read_text()).encode();B.need(derived==z.read('reference-build/veil_reference_worker_bindings.cpp') and hashlib.sha256(derived).hexdigest()==sm['cpp_sha256'],'closed CPU derivation identity')
        for directory,names in [(pilot,['clear-normal','clear-ordered','front-normal','front-ordered']),(ideal,['clear-ideal'])]:
            for name in names:
                cap=B.read(directory/(name+'-rays.json'));ref=B.read(directory/(name+'-reference.json'));receipt=B.read(directory/(name+'-reference-receipt.json'))
                reference_check(cap,ref,receipt,B.digest(directory/'selection.json'),B.digest(directory/(name+'-rays.json')),B.digest(directory/(name+'-reference.json')),source_meta_sha)
                B.need(B.digest(directory/(name+'-reference.log'))==receipt['log_sha256'],'reference native CPU log')
        # Independently regenerate actual GPU input bindings; merely re-signing
        # a foreign input must not make it the input that was captured.
        original=B.read(pilot/'clear-ordered-rays.json');expected_cap=B.read(generated/'clear-ordered-rays.json');controls=[]
        for label,edit in [('clock',lambda d:d.update(time=8.25)),('direction',lambda d:d['rays'][0].__setitem__(2,d['rays'][0][2]+1e-7)),('native-output',lambda d:d['rays'][0].__setitem__(7,d['rays'][0][7]+1e-7)),('truncated',lambda d:d['rays'].pop())]:
            bad=copy.deepcopy(original);edit(bad)
            try:capture_binding(bad,expected_cap)
            except ValueError:controls.append(label)
            else:raise ValueError('capture input mutation admitted: '+label)
        cap=B.read(pilot/'clear-normal-rays.json');ref=B.read(pilot/'clear-normal-reference.json');receipt=B.read(pilot/'clear-normal-reference-receipt.json')
        for label,key,value in [('foreign-input-receipt','input_sha256','0'*64),('timeout-receipt','exit_code','timeout'),('false-convergence','max_convergence_T',0)]:
            bad=copy.deepcopy(receipt);bad[key]=value
            try:reference_check(cap,ref,bad,B.digest(pilot/'selection.json'),B.digest(pilot/'clear-normal-rays.json'),B.digest(pilot/'clear-normal-reference.json'),source_meta_sha)
            except ValueError:controls.append(label)
            else:raise ValueError('reference mutation admitted: '+label)
        report=summarize(work,pilot,ideal,prior);B.need(report==json.loads(z.read('summaries/numerical.json')),'native/reference metrics replay')
        camera80=M.analyze(parent_record,parent_archive,80);camera110=M.analyze(parent_record,parent_archive,110)
        B.need(camera80==json.loads(z.read('summaries/camera80.json')) and camera110==json.loads(z.read('summaries/camera110.json')),'independent camera statistics')
        B.need(camera80['errors']==camera110['errors'] and camera80['ordered_vs_fragment_counts']==camera110['ordered_vs_fragment_counts'],'camera precision stability')
        report=dict(report,cpu_binding_controls_rejected=controls,native_missing_writer_rejected=True,camera=camera80,native_cases=7)
    B.write(out,report);print('PASS 6es replay:7 native cases,5 references,400 rows; common-target pilot passes; no shipping admission')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['record','archive','parent-record','parent-archive','output']:p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();replay(a.record,a.archive,a.parent_record,a.parent_archive,a.output)


if __name__=='__main__':main()
