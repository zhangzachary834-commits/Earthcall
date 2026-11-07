"""CPU-only corrective replay of Sparkly's 6ek gate and diagnostic timing record.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-27.
No GPU execution, timing collection, production or shipping qualification.
"""
import argparse, copy, gzip, hashlib, importlib.util, json, math, statistics, struct, zipfile
from pathlib import Path


def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / (name + '.py'))
    mod = importlib.util.module_from_spec(spec); spec.loader.exec_module(mod); return mod


def fit(xs, ys):
    mx, my = statistics.mean(xs), statistics.mean(ys)
    slope = sum((x-mx)*(y-my) for x,y in zip(xs,ys))/sum((x-mx)**2 for x in xs)
    intercept = my-slope*mx
    residuals = [y-(intercept+slope*x) for x,y in zip(xs,ys)]
    return dict(intercept_ms=intercept, slope_ms_per_K=slope,
                residual_ms_by_K=dict(zip(xs,residuals)))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root',type=Path,default=Path.cwd())
    p.add_argument('--rays',type=Path,required=True)
    p.add_argument('--output',type=Path,required=True); a=p.parse_args(); root=a.root.resolve()
    gate=load(root,'ray_transport_full_workload_accuracy')
    qualification=load(root,'ray_transport_gpu_6ek_indirect_consumer')
    archive=root/'docs/audits/RAY_TRANSPORT_FULL_REFERENCE_COUNCIL_REPLY_2026-10-27.zip'
    with zipfile.ZipFile(archive) as z:
        baseline=json.loads(gzip.decompress(z.read('baseline.json.gz')))
        candidate=json.loads(gzip.decompress(z.read('bisection.json.gz')))
    reference=gate.read(root/'docs/audits/NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27_ROWS.json.gz')
    def check(rows, current=baseline, rays=a.rays):
        return qualification.verify_32k_reference(root,rows,baseline_rows=current,rays_path=rays)
    good=check(candidate); assert good['both_resolutions_pass']
    bad=[[r[0],r[1],r[3]+.0002,r[4]+.0002,r[5]+.0002,r[2]+.0003] for r in reference['rows']]
    # Historical maximum-only decision, reproduced without running any native code.
    old_max_only=all(max(abs(c[2+k]-r[3+k]) for k in range(3))<=.002239 and abs(c[5]-r[2])<=.003467 for c,r in zip(bad,reference['rows']))
    wrong_mean=check(bad); assert old_max_only and not wrong_mean['both_resolutions_pass']
    refusals=[]
    def refuses(label, rows):
        try: check(rows)
        except ValueError: refusals.append(label); return
        raise AssertionError('malformed input accepted: '+label)
    refuses('missing output',candidate[:-1]); refuses('extra output',candidate+[candidate[-1]])
    nan=copy.deepcopy(candidate); nan[1][2]=float('nan'); refuses('NaN actual output',nan)
    duplicate=copy.deepcopy(candidate); duplicate[1][:2]=duplicate[0][:2]; refuses('misaligned pixel',duplicate)
    # Native row[1] is actual; row[2] is only the expected comparison control.
    word=lambda f:struct.unpack('<I',struct.pack('<f',f))[0]
    native={'cases':[{'case':'pose-0','repeats':1,'outputs':[[0,[word(2.)]*4,[word(1.)]*4]]}]}
    assert gate.native_rows(native,{'rays':[[7,9]]})==[[7,9,2.,2.,2.,2.]]
    controls=gate.self_tests(reference,baseline,gate.digest(a.rays))
    # An isolated toy reference proves the second resolution can reject even if finest passes.
    toy={'rays_sha256':'toy','columns':reference['columns'],'rows':[[0,0,0,0,0,0,1,1,1,1]]}
    half=gate.evaluate([[0,0,0,0,0,0]],[[0,0,.5,.5,.5,.5]],toy,'toy')
    assert half['finest']['all_six_pass'] and not half['half']['all_six_pass'] and not half['both_resolutions_pass']
    rawpath=root/'docs/audits/GPU_RAY_TRANSPORT_6EK_MULTI_SCALE_SCREEN_2026-10-27.json'
    data=gate.read(rawpath); obs=data['raw_observations']; scales=data['scales']
    assert len(obs)==160 and len({(o['scale_k'],o['mode'],o['wait_style'],o['block']) for o in obs})==160
    summary={}
    for style in (0,1):
        modes={}
        for mode in (0,1,3,4):
            byk={k:[o for o in obs if o['scale_k']==k and o['mode']==mode and o['wait_style']==style] for k in scales}
            assert all(len(v)==4 and all(x['rays']==k*32784 for x in v) for k,v in byk.items())
            comp={k:statistics.median(x['completion_ms'] for x in v) for k,v in byk.items()}
            total={k:statistics.median(x['total_ms'] for x in v) for k,v in byk.items()}
            old={k:sorted(x['completion_ms'] for x in v)[2] for k,v in byk.items()}
            published=data['fits']['waitStyle_'+str(style)][str(mode)]
            assert all(abs(published['medians_per_k'][str(k)]-old[k])<1e-9 for k in scales)
            modes[mode]=dict(completion_median_ms=comp,total_median_ms=total,
                             historical_upper_middle_ms=old,descriptive_fit=fit(scales,list(comp.values())))
        pairs=[]
        for block in range(4):
            sample={o['mode']:o['completion_ms'] for o in obs if o['scale_k']==16 and o['wait_style']==style and o['block']==block}
            pairs.append(dict(block=block,terminal_vs_Current=sample[4]/sample[0],terminal_vs_bulk=sample[4]/sample[1]))
        summary[style]=dict(modes=modes,K16_terminal_vs_Current=modes[4]['completion_median_ms'][16]/modes[0]['completion_median_ms'][16],
                            K16_terminal_vs_bulk=modes[4]['completion_median_ms'][16]/modes[1]['completion_median_ms'][16],paired_K16=pairs)
    generated=load(root,'ray_transport_gpu_6ek_multi_scale').generate(root)
    assert 'if(mode==3||mode==4) wgpuCommandEncoderClearBuffer(enc,auditWritesBuf,0,auditBytes)' in generated
    assert 'if (scaleK == 1)' in generated
    assert 'atomicStore(&auditWrites[captureBase], bitcast<u32>(s0))' in generated
    output=dict(scope='CPU replay/source inspection only; synthetic diagnostic timing, all shipping gates OPEN',
                historical_bisection_gate=good,uniform_error_counterexample=dict(old_max_only_pass=old_max_only,repaired_gate=wrong_mean),
                checker_refusals=refusals,checker_controls=controls,actual_not_expected_test=True,half_resolution_control=True,
                timings=summary,generated_source_sha256=hashlib.sha256(generated.encode()).hexdigest(),
                audit_clear_bytes_K16=32784*16*1032,
                input_sha256={str(x.relative_to(root)):gate.digest(x) for x in [archive,rawpath,root/'scratch/probes/ray_transport_gpu_6ek_indirect_consumer.py',root/'scratch/probes/ray_transport_gpu_6ek_multi_scale.py',root/'scratch/probes/ray_transport_6ek_review_replay.py']})
    a.output.write_text(json.dumps(output,indent=2)+'\n')
    print('PASS: actual-output selection, all-row dual-resolution gate, deliberate mean regression, malformed input and half-resolution controls; 160 timing rows replayed. No GPU execution.')
    for style,s in summary.items():print('wait',style,'K16 terminal/Current',s['K16_terminal_vs_Current'],'terminal/bulk',s['K16_terminal_vs_bulk'])

if __name__=='__main__': main()
