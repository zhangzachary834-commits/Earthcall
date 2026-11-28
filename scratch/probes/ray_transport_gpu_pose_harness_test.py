"""Focused negative controls for external camera/reference qualification.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
No GPU, live-save writes or fixture modification.
"""
import argparse,copy,hashlib,json,sys,tempfile
from pathlib import Path

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--output',type=Path,required=True)
    a=p.parse_args();root=a.root.resolve();sys.path.insert(0,str(root/'scratch/probes'))
    import ray_transport_gpu_pose_qualification as q
    if a.output.exists():raise ValueError('refusing overwrite')
    work=Path(tempfile.mkdtemp(prefix='earthcall-gpu-pose-negative-',dir='/private/tmp'));records=[]
    zh=q.sha(root/'saves/zones/Northern Veil/zone.ecform')
    rays={'viewport':[2560,1440],'time':7.25,'origin':[0,50,75],'rays':[[i,20,0,0,1,0,10,1,0,0,0]+[0]*12 for i in range(5)]}
    refs={'rows':[[i,20,1,0,0,0,1,0,0,0] for i in range(5)]}
    def files(r,f):
        rp=work/'rays.json';fp=work/'reference.json';rp.write_text(json.dumps(r));fp.write_text(json.dumps(f))
        m={'zone_sha256':zh,'rays':str(rp),'reference':str(fp),'rays_sha256':q.sha(rp),'reference_sha256':q.sha(fp)}
        return m
    def verify(m):
        mp=work/'manifest.json';mp.write_text(json.dumps(m));return q.verify_case(root,mp,zh)
    verify(files(rays,refs))
    def refuse(name,m):
        try:verify(m)
        except (ValueError,KeyError,TypeError):records.append({'case':name,'refused':True});return
        raise AssertionError(name+' incorrectly accepted')
    m=files(rays,refs);Path(m['rays']).write_text(Path(m['rays']).read_text()+' ');refuse('edited native rays',m)
    m=files(rays,refs);m['zone_sha256']='wrong';refuse('wrong authored source',m)
    m=files(rays,refs);m['reference_sha256']='wrong';refuse('wrong reference bytes',m)
    for name,change in [('nonfinite clock',lambda r:r.update(time=float('nan'))),
        ('wrong viewport',lambda r:r.update(viewport=[640,360])),
        ('duplicate pixels',lambda r:r['rays'][1].__setitem__(0,0)),
        ('reversed clip',lambda r:r['rays'][0].__setitem__(5,11)),
        ('short native row',lambda r:r['rays'][0].pop())]:
        r=copy.deepcopy(rays);change(r);refuse(name,files(r,refs))
    f=copy.deepcopy(refs);f['rows'].reverse();refuse('misordered reference pixels',files(rays,f))
    r=copy.deepcopy(rays);f=copy.deepcopy(refs);r['rays']=r['rays'][:4];f['rows']=f['rows'][:4];refuse('insufficient reference rays',files(r,f))
    m=files(rays,refs);m['rays']='rays.json';refuse('relative input path',m)
    # The finest grid favors candidate; the independently retained half grid
    # favors current. A fine-only result must not become broader acceptance.
    r=copy.deepcopy(rays);f=[];raw=[]
    for row in r['rays']:
        row[7:11]=[.3]*4
        f.append([*row[:2]]+[.4]*4+[.3]*4)
        raw.append([*row[:2]]+[.4]*4+[.4]*4+[.4]*4+[0])
    err=abs(.4-.3);saved={lane:{ch:{k:(err if lane=='native_baseline' else 0.0) for k in ['mean','p99','max']}
        for ch in ['RGB_pooled_channels','T']} for lane in ['native_baseline','unfused_A_star_on_device']}
    report={'accuracy_vs_independent_reference':saved}
    result=q.independent_check(report,raw,r,f)
    assert result['finest']['all_six_pass'] and not result['half']['all_six_pass'] and not result['both_resolutions_pass']
    records.append({'case':'half-grid disagreement never promotes fine-only success','refused':True})
    broken=copy.deepcopy(raw);broken[0][2]=float('nan')
    try:q.independent_check(report,broken,r,f)
    except ValueError:records.append({'case':'nonfinite native output','refused':True})
    else:raise AssertionError('nonfinite native output accepted')
    a.output.write_text(json.dumps({'scope':__doc__,'pass':True,'checks':records},indent=2)+'\n')
    print('PASS',len(records),'external-case/failure controls')

if __name__=='__main__':main()
