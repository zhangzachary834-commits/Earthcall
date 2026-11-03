"""Replay native stage divergence, ordered-matrix matches and image deltas.
No new GPU run or transport/accuracy/performance qualification.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28. Zach commissioned the Screen bridge continuation.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import random
import struct
import tempfile
import zipfile

import ray_transport_6er_ray_setup as R
import ray_transport_6er_matrix_reference as M
import ray_transport_fact_codec as C

B=R.B
def sha(b):return hashlib.sha256(b).hexdigest()


def replay(record,archive,out):
    rec=B.read(record);B.need(B.digest(archive)==rec['archive_sha256'],'archive identity')
    passed=[];legacy=[];mutations=[];coverage=[]
    with zipfile.ZipFile(archive) as z,tempfile.TemporaryDirectory(prefix='rays-6er-replay-') as tmp:
        work=Path(tmp);objects=work/'objects';objects.mkdir()
        B.need(len(z.namelist())==len(set(z.namelist()))==len(rec['members_sha256']),'archive members')
        for name,h in rec['members_sha256'].items():B.need(sha(z.read(name))==h,'member identity '+name)
        for label,info in rec['builds'].items():
            meta=json.loads(z.read(info['metadata']));base='builds/'+label+'/'
            B.need(sha(z.read(base+'WebGpuRenderer.cpp'))==meta['generated_renderer_sha256'] and
                   sha(z.read(base+'screen-probe.cpp'))==meta['generated_probe_sha256'],'compiled source identity')
            B.need(R.renderer_source(B.RENDERER.read_text()).encode()==z.read(base+'WebGpuRenderer.cpp'),'renderer source derivation')
            B.need(B.digest(B.RENDERER)==meta['original_renderer_sha256'] and B.digest(B.PROBE)==meta['original_probe_sha256'],'original source drift')
            control=meta['link_argv']['control'];expected=list(control)
            old=next(v for v in control if v.endswith('/src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp.o'))
            expected[expected.index(old)]=meta['compile_argv'][1][meta['compile_argv'][1].index('-o')+1]
            expected[expected.index('-o')+1]=meta['link_argv']['bridge'][meta['link_argv']['bridge'].index('-o')+1]
            B.need(expected==meta['link_argv']['bridge'],'isolated link control')
        # Decode each content-addressed array once. No sampled/dropped words.
        for h,info in rec['facts'].items():
            packed=z.read(info['member']);B.need(sha(packed)==info['encoded_sha256'],'packed fact identity')
            raw=C.decode(packed,info['raw_bytes']);B.need(sha(raw)==h==info['raw_sha256'],'lossless fact reconstruction')
            (objects/h).write_bytes(raw)
        for case in rec['cases']:
            name=case['case'];B.need(name.replace('-','').isalnum(),'case path')
            directory=work/name;directory.mkdir();receipt=json.loads(z.read(case['receipt']))
            meta=json.loads(z.read(rec['builds'][case['build']]['metadata']))
            B.need(receipt['before']==receipt['after'] and receipt['returncode']==0,'native execution identity')
            B.need(receipt['before']['binary_sha256']==meta['binaries'][case['binary']] and
                   receipt['before']['build_receipt_sha256']==sha(z.read(rec['builds'][case['build']]['metadata'])) and
                   receipt['before']['zone_sha256']==rec['zone_sha256'],'native source/binary/fixture binding')
            image=z.read(case['image_member']);B.need(sha(image)==receipt['rgba_sha256'],'image identity')
            B.write(directory/'receipt.json',receipt);(directory/'capture.rgba').write_bytes(image)
            log=z.read('cases/'+name+'/run.log').decode()
            B.need('SOURCE mode=authored radiance=1' in log and
                   not any(x in log.lower() for x in ['validation error','uncaptured error','shader failed','panic']),'native environment/source')
            if 'facts' not in case:continue
            d=receipt['diagnostic'];B.need(d['bytes_per_stage']==d['width']*d['rows']*64<=C.MAX_BYTES,'bounded record extent')
            B.need(d['dispatches']==(0 if case['mutation']=='no-compute' else 3),'dispatch ownership evidence')
            for stage,h in case['facts'].items():os.link(objects/h,directory/('capture.rgba.'+stage+'.bin'))
            if case.get('trace_only'):continue
            try:actual=R.compare(directory);code=0;error=''
            except ValueError as e:actual=B.read(directory/'comparison.json');code=2;error='REJECT '+str(e)
            B.need(code==case['validator_returncode'] and actual==json.loads(z.read('cases/'+name+'/comparison.json')),'stored exact-gate result')
            if code==0:
                B.need(case['mutation']=='canonical-matvec','unqualified legacy setup promoted');passed.append(name)
                if name.startswith('canonical-full-'):coverage.append((d['row_start'],d['rows'],d['width'],d['height']))
            elif case['mutation']=='normal':B.need(error=='REJECT Device ray bit mismatch','legacy failure boundary');legacy.append(name)
            else:
                reasons={'pixel-corner':'Device pixel-centre mismatch','ignore-depth':'Device opaque-depth sample mismatch',
                         'no-compute':'Missing compute ray ownership','no-fragment':'Missing fragment ray ownership'}
                B.need(error=='REJECT '+reasons[case['mutation']],'targeted ownership/input rejection');mutations.append(name)
        B.need(len(passed)==10 and len(legacy)==6 and len(mutations)==4,'case qualification counts')
        B.need(sorted(coverage)==[(i,360,2560,1440) for i in [0,360,720,1080]],'complete disjoint native row coverage')
        matrix=M.analyze(work);B.need(matrix==json.loads(z.read('summaries/matrix-reference.json')),'independent Decimal reference replay')
        deltas={}
        pairs=[('current-clear','canonical-clear'),('current-front','canonical-front'),('current-spawn','canonical-spawn'),
               ('current-threshold-lo','canonical-threshold-lo'),('current-threshold-hi','canonical-threshold-hi')]
        for a,b in pairs:
            x=(work/a/'capture.rgba').read_bytes();y=(work/b/'capture.rgba').read_bytes()
            deltas[b]=dict(control=a,candidate=b,changed_bytes=sum(u!=v for u,v in zip(x,y)),max_byte_delta=max(abs(u-v) for u,v in zip(x,y)))
        B.need(deltas==json.loads(z.read('summaries/canonical-image-deltas.json')),'actual Current image changes')
        full={};x=(work/'current-full/capture.rgba').read_bytes()
        for start in [0,360,720,1080]:
            name='canonical-full-'+str(start);y=(work/name/'capture.rgba').read_bytes()
            full[name]=dict(changed_bytes=sum(u!=v for u,v in zip(x,y)),max_byte_delta=max(abs(u-v) for u,v in zip(x,y)))
        B.need(full==json.loads(z.read('summaries/full-image-deltas.json')) and all(v==dict(changed_bytes=1164,max_byte_delta=1) for v in full.values()),'full image delta scope')
        # Controls for the lossless evidence container, not shader correctness.
        rng=random.Random(6319);raw=struct.pack('<'+'I'*4112,*[rng.getrandbits(32) for _ in range(4112)])
        packed=C.encode(raw);B.need(C.decode(packed,len(raw))==raw,'independent random codec roundtrip')
        for bad,n in [(b'bad'+packed[3:],len(raw)),(packed+b'extra',len(raw)),(packed,len(raw)+64)]:
            try:C.decode(bad,n)
            except ValueError:pass
            else:raise ValueError('codec corruption admitted')
    report=dict(native_cases=33,ordered_stage_matches=passed,legacy_exact_gate_rejections=legacy,
       targeted_subgate_rejections=mutations,full_native_pixels=2560*1440,
       matrix_direction_counts=matrix['direction_counts'],matrix_direction_max_abs_error=matrix['direction_max_abs_error'],
       current_image_deltas=deltas,full_current_image_deltas=full,ordered_contract_adopted=False,
       authored_image_accuracy_qualified=False,gpu_performance_measured=False)
    B.write(out,report);print('PASS replay:33 native cases;10 ordered matches,6 legacy rejections,4 subgate controls; no shipping admission')


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--record',type=Path,required=True);p.add_argument('--archive',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args();replay(a.record,a.archive,a.output)


if __name__=='__main__':main()
