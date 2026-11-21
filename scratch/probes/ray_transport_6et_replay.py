"""Replay six native distinct-ray executions and their complete resident cost.
No GPU re-execution, whole-population reference or FPS admission.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28. Keeps Zach's accuracy and measured economics boundaries distinct.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import struct
import tempfile
import zipfile

import ray_transport_gpu_6et_distinct_cost as H
import ray_transport_6et_analysis as A
import sonnet_surveyor_layout as L
B=H


def replay(record,archive,out):
    rec=B.read(record);B.need(B.digest(archive)==rec['archive_sha256'],'evidence archive identity')
    for name,h in rec['parent_files_sha256'].items():B.need(B.digest(B.ROOT/name)==h,'parent evidence/checker identity '+name)
    with zipfile.ZipFile(archive) as z,tempfile.TemporaryDirectory(prefix='6et-replay-') as tmp:
        work=Path(tmp);B.need(len(z.namelist())==len(set(z.namelist()))==len(rec['members_sha256']),'archive coverage')
        for name,h in rec['members_sha256'].items():B.need(hashlib.sha256(z.read(name)).hexdigest()==h,'member identity '+name)
        B.need(H.generate().encode()==z.read('probe.cpp'),'closed host-only source derivation')
        for name in ['probe.cpp','layout.json','input.json']:(work/name).write_bytes(z.read(name))
        # Source of truth: reconstruct the capture from every actual native
        # census slot, not the candidate's own input JSON or route counters.
        parent=B.ROOT/'docs/audits/GPU_RAY_TRANSPORT_6EN_MODE2_OWNERSHIP_2026-10-28'
        parent_record=B.read(parent.with_suffix('.json'))
        with zipfile.ZipFile(parent.with_suffix('.zip')) as pz:
            metadata=pz.read('capture/metadata.json');B.need(hashlib.sha256(metadata).hexdigest()==parent_record['members_sha256']['capture/metadata.json'],'parent metadata')
            meta=json.loads(metadata);header=meta['ray_header'];geo=L.Geometry(*header['viewport'],*header['capture_grid']);live=pz.read('capture/census-live16.bin')
            B.need(hashlib.sha256(live).hexdigest()==parent_record['members_sha256']['capture/census-live16.bin'] and len(live)==geo.slots*64,'packed native slots')
            raw=bytearray(geo.census_bytes)
            for i in range(geo.slots):raw[i*256:i*256+64]=live[i*64:i*64+64]
            B.need(hashlib.sha256(raw).hexdigest()==header['census_sha256'],'native raw census reconstruction')
            rows,counts=L.decode_census(raw,geo);reconstructed=dict(header,rays=rows)
            native_input=(json.dumps(reconstructed)+'\n').encode()
            B.need(hashlib.sha256(native_input).hexdigest()==B.read(work/'input.json')['rays_sha256'],'every captured native input')
            (work/'rays.json').write_bytes(native_input)
        layout=B.read(work/'layout.json');expected=L.build_layout(geo,[tuple(r[:2]) for r in rows],tuple(layout['tile']),layout['mode'])
        for key,value in L.layout_json(expected).items():B.need(layout[key]==value,'independent layout '+key)
        inputs=B.read(work/'input.json');B.need(B.digest(work/'rays.json')==inputs['rays_sha256'] and B.digest(work/'layout.json')==inputs['layout_sha256'] and inputs['zone_sha256']==B.digest(B.ZONE),'capture input binding')
        build=json.loads(z.read('build/source.json'));B.need(build['cpp_sha256']==hashlib.sha256(z.read('probe.cpp')).hexdigest() and build['engine_src_tree']==inputs['engine_src_tree'],'native build source binding')
        for run in rec['executions']:
            label=run['label'];receipt=json.loads(z.read('runs/'+label+'-receipt.json'));log=z.read('runs/'+label+'.log').decode()
            B.need(receipt['before']==receipt['after'] and receipt['before']['cpp_sha256']==build['cpp_sha256'] and receipt['before']['engine_src_tree']==inputs['engine_src_tree'] and receipt['before']['zone_sha256']==inputs['zone_sha256'],'executed source/fixture identity')
            B.need(receipt['before']['binary_sha256']==rec['native_binary_sha256'] and receipt['before']['build_receipt_sha256']==hashlib.sha256(z.read('build/source.json')).hexdigest(),'executed binary receipt')
            B.need(receipt['before']['rays_sha256']==inputs['rays_sha256'] and receipt['before']['layout_sha256']==inputs['layout_sha256'],'executed exact input bytes')
            B.need(hashlib.sha256(log.encode()).hexdigest()==receipt['log_sha256'] and 0<receipt['elapsed_seconds']<180 and not any(v in log.lower() for v in ['validation error','uncaptured error','device lost']),'native execution bounds/environment')
            if run['negative']:
                B.need(receipt['exit_code'] not in [0,'timeout'] and '6et nonfinite/unpublished mode=2 ray=0' in log,'native missing publication rejection')
            else:B.need(receipt['exit_code']==0,'native positive exit')
            expected_style=0 if label.startswith('blocking-') else 1
            B.need(receipt['wait_style']==expected_style and receipt['phase']==('timing' if label in ['active-a','active-b','blocking-a','blocking-b'] else 'preflight'),'native timing phase/style identity')
            for name,h in receipt['outputs_sha256'].items():
                data=z.read(run['outputs'][name]);B.need(hashlib.sha256(data).hexdigest()==h,'native output binding '+name);(work/name).write_bytes(data)
        negative=rec['missing_producer'];nreceipt=json.loads(z.read('negative-producer/receipt.json'));nmeta=json.loads(z.read('negative-producer/source.json'));nlog=z.read('negative-producer/run.log').decode()
        nsource=z.read('negative-producer/probe.cpp');B.need(nsource==H.missing_s1_source().encode(),'isolated missing-producer derivation')
        B.need(nreceipt['before']==nreceipt['after'] and nreceipt['before']['cpp_sha256']==hashlib.sha256(nsource).hexdigest()==nmeta['cpp_sha256'],'negative producer source identity')
        B.need(nreceipt['before']['binary_sha256']==negative['binary_sha256'] and nreceipt['before']['build_receipt_sha256']==hashlib.sha256(z.read('negative-producer/source.json')).hexdigest(),'negative producer binary identity')
        B.need(nreceipt['before']['rays_sha256']==inputs['rays_sha256'] and nreceipt['before']['layout_sha256']==inputs['layout_sha256'] and nreceipt['before']['engine_src_tree']==inputs['engine_src_tree'] and nreceipt['before']['zone_sha256']==inputs['zone_sha256'],'negative producer fixture identity')
        B.need(nreceipt['exit_code']!=0 and 0<nreceipt['elapsed_seconds']<180 and '6et full distinct parity failure: 95648' in nlog and not any(v in nlog.lower() for v in ['validation error','uncaptured error','device lost']),'native missing-producer rejection boundary')
        B.need(hashlib.sha256(nlog.encode()).hexdigest()==nreceipt['log_sha256'],'negative producer log identity')
        for name,h in nreceipt['outputs_sha256'].items():B.need(hashlib.sha256(z.read(negative['outputs'][name])).hexdigest()==h,'negative producer output identity')
        qualification=A.qualify(work,'preflight');B.need(qualification==json.loads(z.read('summaries/preflight-pilot.json')) and qualification['pilot_both_candidate_modes_pass'],'bound1094pilot/full parity replay')
        labels=['active-a','blocking-a','blocking-b','active-b']
        for label in labels:
            B.need(A.qualify(work,label)==qualification,'repeated fresh preflight numerical identity')
            data=B.read(work/(label+'.json'));B.need(data['wait_style']==(0 if label.startswith('blocking-') else 1),'reported timing style');B.need(data['preflight_full_schedule_poison'] and not data['obsolete_stream_allocations'] and data['bytes']==dict(rays=11059200,subpanels=235929600,overlaps=58982400,outputs=3686400),'schedule/resource scope')
        timing=A.timings(work,labels);B.need(timing==json.loads(z.read('summaries/timing-summary.json')),'paired total/encoding/completion statistics')
        # This mutation lies outside the reference pilot. Full-population
        # own-method parity must still catch it independently of pilot metrics.
        _,indices,reference,cohort_sha,_=A.pilot_rows(work/'rays.json');not_pilot=next(i for i in range(230400) if i not in set(indices));path=work/'preflight.json.mode2.bin';original=path.read_bytes();bad=bytearray(original);bad[not_pilot*16]^=1;path.write_bytes(bad)
        try:A.qualify(work,'preflight')
        except ValueError as e:B.need(str(e)=='full population own-method parity','outside-pilot mutation boundary')
        else:raise ValueError('unreferenced-ray corruption admitted')
        path.write_bytes(original[:-16])
        try:A.qualify(work,'preflight')
        except ValueError as e:B.need(str(e)=='complete actual-output extent','truncated output boundary')
        else:raise ValueError('truncated outputs admitted')
        path.write_bytes(original)
        timer=work/'active-a.json';saved=timer.read_bytes();data=json.loads(saved);data['observations'][0]['mode']=2;B.write(timer,data)
        try:A.timings(work,['active-a'])
        except ValueError as e:B.need(str(e)=='paired order','pair-order mutation boundary')
        else:raise ValueError('wrong pair order admitted')
        data=json.loads(saved);data['observations'][0]['total_ms']-=1;B.write(timer,data)
        try:A.timings(work,['active-a'])
        except ValueError as e:B.need(str(e)=='complete timing scope arithmetic','partial timing scope boundary')
        else:raise ValueError('partial timing scope admitted')
        timer.write_bytes(saved)
        _,current=A.values(work/'preflight.json.mode0.bin');baseline=[[*rows[i][:2],*current[i]] for i in indices]
        gate_tests=A.G.self_tests(reference,baseline,cohort_sha)
    result=dict(qualification=qualification,timing=timing,native_executions=7,native_missing_consumer_rejected=True,native_missing_S1_rejected=True,
        controls=['unreferenced ray corruption','truncated actual outputs','wrong paired order','partial timing scope'],shared_checker_controls=gate_tests,
        full_reference_qualified=False,production_screen_connected=False,frame_fps_claim=False,shipping_gates_open=3)
    B.write(out,result);print('PASS6et replay:230400exact inputs/outputs,1094reference pilot,192complete timings, native missing-consumer + CPU controls; no shipping/FPS admission')


def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['record','archive','output']:p.add_argument('--'+name,type=Path,required=True)
    a=p.parse_args();replay(a.record,a.archive,a.output)


if __name__=='__main__':main()
