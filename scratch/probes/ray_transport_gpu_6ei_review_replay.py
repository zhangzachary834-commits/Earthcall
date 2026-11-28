"""§6ei saved-evidence replay of frozen-shader clock/parity calibration.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-27.
Host queue-completion clocks are not hardware GPU timestamps or native FPS.
"""
import argparse,copy,hashlib,importlib.util,json,math,statistics,zipfile
from pathlib import Path
def sha(b):return hashlib.sha256(b).hexdigest()
def need(ok,msg):
    if not ok:raise ValueError(msg)
def load(root,name):
    s=importlib.util.spec_from_file_location(name,root/'scratch/probes'/f'{name}.py');m=importlib.util.module_from_spec(s);s.loader.exec_module(m);return m
def stats(v):return {'mean':statistics.mean(v),'median':statistics.median(v),'min':min(v),'max':max(v)}
def analyze(data,expected):
    need(len(data['candidate_bits'])==len(expected['outputs'])==32784,'output coverage')
    need(data['candidate_bits']==expected['outputs'],'clean output bits differ from validated candidate')
    need(data['candidate_headers']==[[len(p[2]),p[1]] for p in expected['packets']],'clean headers differ')
    pre=data['candidate_vs_validated'];need(data['candidate_admitted'] and pre['wrong_outputs']==pre['wrong_headers']==pre['wrong_nonterminal_packets']==0,'native preflight did not admit')
    need(pre['terminal_endpoints_observed'] is False,'false terminal endpoint evidence')
    need(data['wait_strategy_output_component_differences']==0,'host observer changed output')
    need(data['active_modes']==[0,1,2,3,5],'lane mapping changed')
    order=[]
    for rep in [0,1,2,4,8,16]:
        for block in range(4):
            modes=data['active_modes'][::(-1 if block%2 else 1)]
            for mode in modes:
                if rep==0 and mode!=0:continue
                for position,style in enumerate([1,0,0,1] if block%2 else [0,1,1,0]):order.append((rep,mode,style,block,position))
    need([(s['repeats'],s['mode'],s['wait_style'],s['block'],s['position']) for s in data['samples']]==order,'calibration sample coverage/order')
    for s in data['samples']:
        need(all(math.isfinite(s[k]) and s[k]>=0 for k in ['encode_ms','completion_ms','total_ms']),'invalid clock')
        need(math.isclose(s['total_ms'],s['encode_ms']+s['completion_ms'],rel_tol=1e-12,abs_tol=1e-9),'clock accounting')
        need(s['poll_calls']>=1,'completion was not observed')
    summary={}
    for style in [0,1]:
        for rep in [0,1,2,4,8,16]:
            for mode in data['active_modes']:
                rows=[s for s in data['samples'] if (s['wait_style'],s['repeats'],s['mode'])==(style,rep,mode)]
                if not rows:continue
                summary[f'{style}/{rep}/{mode}']={k:stats([r[k] for r in rows]) for k in ['encode_ms','completion_ms','total_ms','poll_calls']}
    return {'samples':len(order),'independent_native_invocations':1,'summary':summary,'scope':'HOST observer calibration; active polling consumes CPU and may change power/scheduling; no hardware-duration/FPS claim'}
def legacy_check(data):
    result={}
    for run,r in data['runs'].items():
        all_samples=[]
        for block in ['series_1','series_2']:
            for rep in ['1','2','4']:
                samples=r['clean_timing_ordered_samples'][block][rep];need(len(samples)==40,'legacy ordered samples')
                expected=[]
                for b in range(4):expected.extend([5,3,2,1,0,0,1,2,3,5] if b%2 else [0,1,2,3,5,5,3,2,1,0])
                need([s['mode'] for s in samples]==expected,'legacy order differs')
                all_samples.extend(samples)
                for mode in [0,1,2,3,5]:
                    for channel in ['encode_ms','completion_ms','total_ms']:
                        v=[s[channel] for s in samples if s['mode']==mode];saved=r['clean_timing_series'][block][rep][str(mode)][channel]
                        need(sorted(v)==saved['samples'],'legacy raw samples differ')
                        for key,k in [('mean_ms','mean'),('median_ms','median'),('min_ms','min'),('max_ms','max')]:need(math.isclose(saved[key],stats(v)[k],rel_tol=1e-12,abs_tol=1e-12),'legacy statistic differs')
        residual=[abs(s['completion_ms']-round(s['completion_ms']/1.2615)*1.2615) for s in all_samples]
        result[run]={'observations':len(all_samples),'residual_from_1_2615_ms_grid':stats(residual),'within25us':sum(x<=.025 for x in residual),'scope':'empirical clustering, not proof of scheduler cadence or true GPU time'}
    return result
def negative_checks(data,expected):
    rejected=[]
    def bad(label,change):
        d=copy.deepcopy(data);change(d)
        try:analyze(d,expected)
        except (ValueError,KeyError,IndexError):rejected.append(label);return
        raise ValueError('checker admitted '+label)
    bad('missing sample',lambda d:d['samples'].pop());bad('incorrect total clock',lambda d:d['samples'][0].__setitem__('total_ms',d['samples'][0]['total_ms']+1))
    bad('false candidate parity',lambda d:d['candidate_bits'][0].__setitem__(0,d['candidate_bits'][0][0]^1));bad('invalid lane map',lambda d:d.__setitem__('active_modes',[0,1,2,3]))
    return rejected
def replay(root,index,archive,eg_archive,ee_archive,fixture_archive):
    meta=json.loads(index.read_text());need(sha(archive.read_bytes())==meta['archive_sha256'],'archive drift')
    need(sha(eg_archive.read_bytes())==meta['eg_archive_sha256'] and sha(ee_archive.read_bytes())==meta['ee_archive_sha256'],'proof donor drift');need(sha(fixture_archive.read_bytes())==meta['fixture_archive_sha256'],'reference donor drift')
    for name,h in meta['source_sha256'].items():need(sha((root/name).read_bytes())==h,'source drift: '+name)
    with zipfile.ZipFile(archive) as z,zipfile.ZipFile(eg_archive) as eg,zipfile.ZipFile(ee_archive) as ee,zipfile.ZipFile(fixture_archive) as fz:
        def read(n):
            b=z.read(n);need(sha(b)==meta['artifacts'][n]['sha256'],'artifact drift: '+n);return b
        need(set(z.namelist())==set(meta['artifacts']),'archive membership')
        for n in meta['artifacts']:read(n)
        def chunk(n):
            dz=ee if n.startswith('donor:') else eg;n=n.removeprefix('donor:');b=dz.read(n);need(n=='chunks/'+sha(b),'proof chunk drift');return b
        raw=b''.join(chunk(n) for n in meta['expected_provenance']['parts']);need(sha(raw)==meta['expected_provenance']['sha256'],'validated native report differs');validated=json.loads(raw);del raw
        c=next(c for c in validated['cases'] if c['mode']==5);expected={'outputs':[v for i,v,e in c['outputs']],'packets':c['actual_packets']};expected_bytes=(json.dumps(expected,separators=(',',':'))+'\n').encode();need(expected_bytes==read('fresh/expected.json'),'expected data not derived from validated6eg');del validated
        factory=load(root,'ray_transport_gpu_6ei_clock_review');need(factory.generate(root).encode()==read('fresh/probe.cpp'),'fresh C++ not reconstructed')
        old=load(root,'ray_transport_gpu_6eh_clean_timing');cpp=old.clean_cpp(root).encode()
        for run in ['run_1','run_2']:need(cpp==read('legacy/'+run+'.cpp'),'old compiled source differs')
        fresh=json.loads(read('fresh/review.json'));summary=analyze(fresh,expected);published=json.loads(read('legacy/published.json'));legacy=legacy_check(published);tests=negative_checks(fresh,expected)
        for run in ['run_1','run_2']:
            projected=json.loads(read('legacy/'+run+'-timings.json'))
            need(projected['clean_timing_series']==published['runs'][run]['clean_timing_series'] and projected['clean_timing_ordered_samples']==published['runs'][run]['clean_timing_ordered_samples'],'published timing differs from native projection')
        rays=json.loads(fz.read('fixture/rays.json'));refs=json.loads(fz.read('fixture/reference.json'));baseline=json.loads(read('fresh/baseline.json'));metric=load(root,'ray_transport_sliver_review');decode=lambda w:__import__('struct').unpack('<f',__import__('struct').pack('<I',w))[0]
        selected=dict(rays,rays=[list(rays['rays'][i]) for i in refs['ray_indices']]);actual=[[*rays['rays'][i][:2],*[decode(w) for w in fresh['candidate_bits'][i]]] for i in refs['ray_indices']];captured=metric.reference_metrics(actual,selected,refs['rows'])
        for row,i in zip(selected['rays'],refs['ray_indices']):row[7:11]=[baseline[i][5],*baseline[i][2:5]]
        current=metric.reference_metrics(actual,selected,refs['rows']);accuracy={'captured':captured,'contemporary':current};need(accuracy==json.loads(read('fresh/accuracy.json')) and captured['both_resolutions_pass'] and current['both_resolutions_pass'],'fresh reference accuracy')
        return {'pass':True,'fresh':summary,'legacy':legacy,'fresh_accuracy':accuracy,'checker_negatives':tests,'clean_candidate_normal_fixture_parity':True,'complete_terminal_endpoint_parity_observed':False,'native_s2_or_new_radius_executed':False,'shipping_gates_open':[1,2,3]}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for n in ['root','record','archive','eg-archive','ee-archive','fixture','output']:p.add_argument('--'+n,type=Path,required=True)
    a=p.parse_args();r=replay(a.root,a.record,a.archive,a.eg_archive,a.ee_archive,a.fixture);a.output.write_text(json.dumps(r,indent=2)+'\n');print('PASS:480 historical timing observations,416 frozen-shader clock observations, actual validated normal-fixture parity,4 checker refusals; no hardware-GPU-time/FPS claim')
if __name__=='__main__':main()
