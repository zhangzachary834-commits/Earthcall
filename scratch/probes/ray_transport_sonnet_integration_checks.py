"""CPU-only integration controls for the five Sonnet merge. No GPU work.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-27.
"""
import argparse,copy,hashlib,importlib.util,json,sys,tempfile,zipfile,statistics
from pathlib import Path
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
import sonnet_surveyor_layout as L
import sonnet_surveyor_capture as C
import sonnet_surveyor_test as T
import sonnet_oracle_capture_gate as G
import ray_transport_6em_replay as R


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    checks=[]
    def ok(label,fn):fn();checks.append(label)
    def refuses(label,fn,reason):
        try:fn()
        except ValueError as e:
            if reason not in str(e):raise AssertionError(label+': wrong refusal '+str(e))
            checks.append(label);return
        raise AssertionError(label+': admitted')
    with tempfile.TemporaryDirectory(prefix='sonnet-integrated-') as tmp:
        w=Path(tmp);geo=L.Geometry(40,20,4,2);raw,_=T.synth(geo,lambda x,y:True)
        (w/'census.bin').write_bytes(raw);(w/'zone.json').write_text('{}')
        m=C.build_case(w/'census.bin',w/'case',geo,'synthetic',[0,50,75],7.25,w/'zone.json',(4,4),'compact',0)
        C.verify_case(HERE.parents[1],w/'case/manifest.json')
        cap,sha=G.surveyor_capture(w/'case/manifest.json',{'m0':7.25,'m1':7.25},m['zone_sha256'])
        def equal_rows():
            original=json.loads(Path(m['rays']).read_text())
            assert cap['rays']==original['rays'] and sha==C.sha(m['rays'])
            assert any(abs(sum(v*v for v in row[2:5])-1)>1e-4 for row in cap['rays'])
        ok('Surveyor -> Oracle preserves non-unit directions, original rows and byte identity',equal_rows)
        def no_default_stub():
            assert m['harness_env']['EC_POSE_REFERENCE']==m['reference']
            assert not Path(m['reference']).exists()
            assert 'STUB' in m['diagnostic_only_harness_env']['EC_POSE_REFERENCE']
        ok('missing real reference fails closed by default; stub is explicitly diagnostic-only',no_default_stub)
        refuses('different independent medium clocks remain unsupported',lambda:G.surveyor_capture(w/'case/manifest.json',{'m0':7.25,'m1':7.5},m['zone_sha256']),'medium-specific clocks differ')
        rawref={'columns':['T','Lr','Lg','Lb','T_half','Lr_half','Lg_half','Lb_half'],'rows':[[.8,.1,.2,.3,.8,.1,.2,.3] for _ in cap['rays']]}
        refsha=hashlib.sha256(json.dumps(rawref).encode()).hexdigest()
        receipt=dict(rays_sha256=sha,reference_sha256=refsha,zone_sha256=cap['zone_sha256'],time=cap['time'],origin=cap['origin'],medium_clocks=cap['medium_clocks'],generator_sha256=G.sha256_file(HERE/'veil_projected_reference.cpp'))
        convert=lambda identity=receipt,rsha=refsha:G.reference_to_checker_form(rawref,cap,sha,reference_identity=identity,reference_sha256=rsha)
        wrong_generator=dict(receipt,generator_sha256='f'*64)
        refuses('stale reference generator source',lambda:convert(wrong_generator),'generator identity mismatch')
        ref=convert();candidate=[[r[0],r[1],.1,.2,.3,.8] for r in cap['rays']]
        def good():assert G.evaluate_capture(cap,candidate,candidate,ref,rays_sha256=sha,zone_sha256=cap['zone_sha256'])['both_resolutions_pass']
        ok('receipt-bound original capture passes all-row dual-resolution gate',good)
        refuses('raw reference cannot be relabelled without a generation receipt',lambda:convert(None),'receipt required')
        for key in ['rays_sha256','reference_sha256','zone_sha256','time','origin','medium_clocks']:
            wrong=copy.deepcopy(receipt);wrong[key]='wrong'
            refuses('stale reference receipt '+key,lambda wrong=wrong:convert(wrong),'identity mismatch: '+key)
        # Re-sign the capture after a fractional pixel edit; independent validation must still reject.
        rays_path=Path(m['rays']);d=json.loads(rays_path.read_text());d['rays'][0][0]+=.25;rays_path.write_text(json.dumps(d));m['rays_sha256']=C.sha(rays_path);(w/'case/manifest.json').write_text(json.dumps(m))
        refuses('re-signed fractional capture pixel',lambda:C.verify_case(HERE.parents[1],w/'case/manifest.json'),'row 0')
    huge=dict(L.OBSERVED_M5_LIMITS,maxStorageBufferBindingSize=2**60,maxBufferSize=2**60)
    def dispatch():
        assert any('per-ray dispatch' in p for p in L.budget(4194241,1,huge)['problems'])
        assert any('indirect-producer' in p for p in L.budget(1,262141,huge)['problems'])
    ok('per-ray and indirect-producer dispatch ceilings checked independently',dispatch)
    rows=[[i,0.,0.,0.,1.] for i in range(4)];R.validate_actual_rows(rows,4)
    bad=copy.deepcopy(rows);bad[2][0]=1
    refuses('preflight row identity cannot be reassigned by enumeration',lambda:R.validate_actual_rows(bad,4),'index/coverage')
    refuses('preflight extra row',lambda:R.validate_actual_rows(rows+rows[:1],4),'index/coverage')
    root=HERE.parents[1];arc=root/'docs/audits/GPU_RAY_TRANSPORT_6EM_CORRECTIVE_TAKEOVER_2026-10-27.zip';paired={}
    with zipfile.ZipFile(arc) as z:
        s=z.read('native/clean.cpp').decode();start=s.index('else if(mode==2)');end=s.index('else if(mode==3)',start);mode2=s[start:end]
        def actual_mode():
            assert all(x in mode2 for x in ['cpReset','cpClassify','cpArgs','cpS1Unfused','cpS2','cpFallback','cpUnfused','fallbackCP'])
            assert 'for(const auto& tile:tiles)' not in mode2
        ok('archived actual Mode2 is routed-unfused, not obsolete streamed branch',actual_mode)
        for style in [0,1]:
            d=json.loads(z.read(f'native/clean-w{style}-K16.json'));values=[]
            for series,samples in d['clean_timing_ordered_samples'].items():
                assert len(samples)==48
                for b in range(4):
                    block=samples[b*12:(b+1)*12];med=lambda mode:statistics.median(x['total_ms'] for x in block if x['mode']==mode)
                    values.append({'series':series,'block':b,'routed_vs_Current':med(2)/med(0)})
            ratios=[x['routed_vs_Current'] for x in values]
            paired[style]={'blocks':values,'median_ratio':statistics.median(ratios),'range':[min(ratios),max(ratios)],'scope':'descriptive paired ABBA block ratios, no confidence interval or new timing'}
    a.output.write_text(json.dumps({'checks':checks,'count':len(checks),'paired_K16':paired,'scope':'CPU source/model/evidence integration; no new native/GPU execution'},indent=2)+'\n')
    print('SONNET INTEGRATION PASS:',len(checks),'controls; paired K16:',{k:[round(x,4) for x in v['range']] for k,v in paired.items()})

if __name__=='__main__':main()
