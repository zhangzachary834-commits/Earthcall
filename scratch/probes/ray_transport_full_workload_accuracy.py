"""Full-workload ray transport accuracy gate, commissioned by Zach after Opus's reference.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-27.
All rays and both reference resolutions; no sampling, tolerance widening or GPU work.
Numerical projected-model reference agreement is not a universal certificate or FPS.
"""
import argparse,gzip,hashlib,json,math,statistics,struct
from pathlib import Path
def need(ok,msg):
    if not ok:raise ValueError(msg)
def read(path):
    p=Path(path);b=p.read_bytes();return json.loads(gzip.decompress(b) if p.suffix=='.gz' else b)
def digest(path):return hashlib.sha256(Path(path).read_bytes()).hexdigest()
def decode(w):
    need(type(w)==int and 0<=w<=0xffffffff,'invalid f32 word')
    return struct.unpack('<f',struct.pack('<I',w))[0]
def native_rows(report,rays,mode=None):
    cases=[c for c in report['cases'] if c['case']=='pose-0' and c['repeats']==1 and (mode is None or c.get('mode')==mode)]
    need(bool(cases),'normal full-row case missing');c=cases[0];need(len(c['outputs'])==len(rays['rays']),'native output coverage')
    out=[]
    for index,(i,actual,expected) in enumerate(c['outputs']):
        need(i==index and len(actual)==4,'native ray order/width');out.append([*rays['rays'][i][:2],*[decode(w) for w in actual]])
    return out
def evaluate(candidate,baseline,reference,rays_sha256):
    need(reference['rays_sha256']==rays_sha256,'reference/ray identity mismatch')
    need(reference['columns']==['px','py','T','Lr','Lg','Lb','T_half','Lr_half','Lg_half','Lb_half'],'reference column schema')
    rows=reference['rows'];n=len(rows);need(n>0 and len(candidate)==len(baseline)==n,'full coverage required')
    need(len({tuple(r[:2]) for r in rows})==n,'duplicate reference pixels')
    for a,b,r in zip(candidate,baseline,rows):
        need(len(a)==len(b)==6 and len(r)==10,'row width');need(a[:2]==b[:2]==r[:2],'pixel order differs')
        need(all(isinstance(x,(int,float)) and math.isfinite(x) for x in a+b+r),'nonfinite/non-numeric value')
    def stats(x):
        v=sorted(x);return {'mean':statistics.mean(v),'p99':v[len(v)*99//100],'max':v[-1]}
    result={}
    for name,start in [('finest',2),('half',6)]:
        all_metrics={};errors={}
        for label,values in [('current',baseline),('candidate',candidate)]:
            rgb=[];trans=[];per_ray=[]
            for a,r in zip(values,rows):
                er=[abs(a[2+k]-r[start+1+k]) for k in range(3)];et=abs(a[5]-r[start]);rgb.extend(er);trans.append(et);per_ray.append([max(er),et])
            all_metrics[label]={'RGB_pooled_channels':stats(rgb),'T':stats(trans)};errors[label]=per_ray
        gates={channel+'.'+stat:all_metrics['candidate'][channel][stat]<=all_metrics['current'][channel][stat] for channel in ['RGB_pooled_channels','T'] for stat in ['mean','p99','max']}
        bar=all_metrics['current'];beyond=[i for i,(rgb,t) in enumerate(errors['candidate']) if rgb>bar['RGB_pooled_channels']['max'] or t>bar['T']['max']]
        result[name]={'metrics':all_metrics,'gates':gates,'all_six_pass':all(gates.values()),'rays_beyond_current_max':len(beyond),'beyond_current_max_indices':beyond}
    result.update(rays=n,gate_scope='ALL reference rows; finite projected-model numerical reference, not a mathematical/native execution certificate',both_resolutions_pass=result['finest']['all_six_pass'] and result['half']['all_six_pass'],reference_convergence={'RGB_max_delta':max(abs(r[3+k]-r[7+k]) for r in rows for k in range(3)),'T_max_delta':max(abs(r[2]-r[6]) for r in rows)})
    return result
def self_tests(reference,baseline,rays_sha):
    import copy
    refused=[]
    def bad(name,fn):
        try:fn()
        except ValueError:refused.append(name);return
        raise ValueError('corruption admitted: '+name)
    bad('missing ray',lambda:evaluate(baseline[:-1],baseline,reference,rays_sha))
    a=copy.deepcopy(baseline);a[1][:2]=a[0][:2];bad('duplicate/misaligned pixel',lambda:evaluate(a,baseline,reference,rays_sha))
    a=copy.deepcopy(baseline);a[0][2]=float('nan');bad('NaN output',lambda:evaluate(a,baseline,reference,rays_sha))
    bad('wrong ray identity',lambda:evaluate(baseline,baseline,reference,'wrong'))
    a=copy.deepcopy(baseline);a[len(a)//2][2]+=1;d=evaluate(a,baseline,reference,rays_sha);need(not d['both_resolutions_pass'] and d['finest']['rays_beyond_current_max']>0,'localized error escaped max gate')
    return {'schema_refusals':refused,'localized_error_rejected':True,'identical_Current_pass':evaluate(baseline,baseline,reference,rays_sha)['both_resolutions_pass']}
def main():
    p=argparse.ArgumentParser(description=__doc__)
    for name in ['reference','rays','baseline','output']:p.add_argument('--'+name,type=Path,required=True)
    group=p.add_mutually_exclusive_group(required=True);group.add_argument('--native-report',type=Path);group.add_argument('--rows',type=Path);p.add_argument('--mode',type=int);p.add_argument('--self-test',action='store_true');a=p.parse_args()
    rays=read(a.rays);reference=read(a.reference);baseline=read(a.baseline);candidate=native_rows(read(a.native_report),rays,a.mode) if a.native_report else read(a.rows);d=evaluate(candidate,baseline,reference,digest(a.rays))
    if a.self_test:d['checker_tests']=self_tests(reference,baseline,digest(a.rays))
    d['input_sha256']={str(path):digest(path) for path in [a.reference,a.rays,a.baseline,a.native_report or a.rows]};a.output.write_text(json.dumps(d,indent=2)+'\n');print('FULL-WORKLOAD',len(candidate),'PASS' if d['both_resolutions_pass'] else 'FAIL','finest beyond Current max',d['finest']['rays_beyond_current_max']);raise SystemExit(0 if d['both_resolutions_pass'] else 2)
if __name__=='__main__':main()
