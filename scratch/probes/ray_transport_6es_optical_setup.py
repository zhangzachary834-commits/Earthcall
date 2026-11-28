"""Section6es: native Current float transport on separately bound ray inputs.
External renderer copy only; no routed consumer, shipping or speed admission.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28. Zach commissioned numerical qualification after section6er.
"""
import argparse
import datetime as dt
import json
import math
import os
from pathlib import Path
import struct
import subprocess
import time

import ray_transport_6er_ray_setup as R
import ray_transport_6ep_reference_bindings as E
B=R.B

# The invariant original transport still executes. This additional diagnostic
# write occurs before output discard, and records actual T rather than 1-alpha.
# Its slots intentionally replace the normal record's bounds/pixel tail.
INSERT=r'''    const std::string opticalAnchor="    let alpha = 1.0 - transmittance;";
    const auto op=s.find(opticalAnchor);
    if(op==std::string::npos||s.find(opticalAnchor,op+1)!=std::string::npos)std::abort();
    if(rayMutation!="no-optics")s.insert(op,R"WGSL(    {
        let ecX=u32(in.position.x);let ecY=u32(in.position.y);
        if(ecY>=RAY_START && ecY<RAY_START+RAY_ROWS){
            let ecAt=((ecY-RAY_START)*u32(u.viewport.x)+ecX)*4u;
            fragmentFacts[ecAt+3u]=bitcast<vec4<u32>>(vec4<f32>(transmittance,integratedRadiance));
        }
    }
)WGSL");
'''


def probe_source(s):
    t=R.probe_source(s)
    a='    if(const char* path=std::getenv("EC_SCREEN_SHADER_DIR")){'
    return B.once(t,a,INSERT+a)


def arrays(directory):
    receipt=B.read(directory/'receipt.json');d=receipt['diagnostic']
    raw=(directory/'capture.rgba.fragment.bin').read_bytes()
    B.need(len(raw)==d['width']*d['rows']*64,'complete optical facts')
    return receipt,raw,memoryview(raw).cast('I'),memoryview(raw).cast('f')


def optical_check(directory):
    receipt,raw,u,f=arrays(directory);valid=[]
    for i in range(len(raw)//64):
        a=i*16
        if u[a+8]!=1 or u[a+9]!=3:continue
        B.need(all(math.isfinite(f[a+k]) for k in [0,1,2,3,4,5,6,7,12,13,14,15]),'nonfinite optical facts')
        B.need(0<=f[a+12]<=1 and min(f[a+13:a+16])>=0,'invalid optical value')
        valid.append(i)
    B.need(len(valid)>0,'no valid optical coverage')
    return valid


def prepare(work,out):
    B.need(not out.exists(),'fresh selected optical work required');out.mkdir()
    jobs=[]
    for scene in ['clear','front']:
        normal=work/(scene+'-normal');ordered=work/(scene+'-ordered')
        na=arrays(normal);ca=arrays(ordered)
        # Fix uniform coverage plus largest input/observable changes BEFORE any
        # reference is evaluated. Same pixel positions for both arithmetic paths.
        eligible=sorted(set(optical_check(normal))&set(optical_check(ordered)))
        B.need(len(eligible)>=64,'insufficient paired coverage')
        indices=set(eligible[k*(len(eligible)-1)//63] for k in range(64))
        delta=lambda i:max(abs(na[3][16*i+4+k]-ca[3][16*i+4+k]) for k in range(3))
        indices.update(sorted(eligible,key=lambda i:(-delta(i),i))[:8])
        normal_image=(normal/'capture.rgba').read_bytes();candidate_image=(ordered/'capture.rgba').read_bytes()
        changed=[i for i in eligible if normal_image[4*i:4*i+4]!=candidate_image[4*i:4*i+4]]
        if len(changed)>1:
            indices.update(changed[k*(len(changed)-1)//min(7,len(changed)-1)] for k in range(min(8,len(changed))))
        else:indices.update(changed)
        indices=sorted(indices)
        for kind,directory,data in [('normal',normal,na),('ordered',ordered,ca)]:
            receipt,raw,u,f=data;d=receipt['diagnostic'];rows=[]
            for i in indices:
                a=i*16;x=i%d['width'];y=i//d['width']+d['row_start']
                # Canonical generator's four output columns are populated with
                # the SAME Current value solely to satisfy its diagnostic ABI;
                # no four different native methods were executed here.
                val=list(f[a+12:a+16]);rows.append([x,y,*f[a+4:a+7],f[a+3],f[a+7],*(val*4)])
            name=scene+'-'+kind
            cap=dict(schema='6es-own-input-optical-pilot-1',time=7.25,origin=list(f[:3]),rays=rows,
                selection=dict(indices=indices,policy='64 uniform valid +8 largest direction changes +up to8 changed RGBA pixels; fixed before references'),
                native_receipt_sha256=B.digest(directory/'receipt.json'),facts_sha256=B.digest(directory/'capture.rgba.fragment.bin'),
                zone_sha256=B.digest(B.ZONE),native_case=name,diagnostic_repeated_output_columns=True)
            path=out/(name+'-rays.json');B.write(path,cap)
            jobs.append(dict(name=name,count=len(rows),input=str(path),input_sha256=B.digest(path),native_directory=str(directory)))
    B.write(out/'selection.json',dict(schema='6es-optical-selection-1',jobs=jobs,zone_sha256=B.digest(B.ZONE),log2N=16,threads=8,timeout_seconds=300))
    print(json.dumps([{k:j[k] for k in ['name','count']} for j in jobs]))



def prepare_ideal(pilot,output):
    # Additional, explicitly distinct camera-target experiment. Original own-ray
    # references and their selection receipt are never relabelled or overwritten.
    from decimal import Decimal,localcontext
    import msgpack
    import ray_transport_6es_camera_reference as camera
    B.need(not output.exists(),'fresh ideal reference directory');output.mkdir()
    base=B.read(pilot/'clear-normal-rays.json');fixture=json.loads(msgpack.unpackb(B.ZONE.read_bytes(),raw=False)['MigrationRoot'])
    fields=fixture['spatialFields'];B.need(len(fields)==4,'fixed four-medium fixture')
    # These fixture coordinates are exactly representable in f32; refuse a
    # changed fixture needing a separate rounding/lowering contract.
    for f in fields:
        B.need(all(float(x)==struct.unpack('<f',struct.pack('<f',x))[0] for x in f['origin']+f['scale']),'inexact fixture bounds')
    lo=[min(f['origin'][k]-abs(f['scale'][k]) for f in fields) for k in range(3)]
    hi=[max(f['origin'][k]+abs(f['scale'][k]) for f in fields) for k in range(3)]
    B.need(lo==[-165,8,29] and hi==[165,136,184],'fixed union bounds')
    with localcontext() as ctx:
        ctx.prec=80;D=Decimal;eye=list(map(D,[0,50,75]));axes=camera.basis(eye,list(map(D,[40,62,110])));rows=[]
        for native in base['rays']:
            x,y=native[:2];direction=camera.ideal_direction(x,y,334,190,axes)
            entries=[];exits=[]
            for k in range(3):
                B.need(direction[k]!=0,'parallel ideal diagnostic ray')
                a=(D(lo[k])-eye[k])/direction[k];b=(D(hi[k])-eye[k])/direction[k]
                entries.append(min(a,b));exits.append(max(a,b))
            t0=max(D(0),max(entries));t1=min(exits);B.need(t1>t0,'empty ideal diagnostic ray')
            rows.append([x,y,*map(float,direction),float(t0),float(t1),*native[7:]])
    # Inherited native_case identifies the parent captured values, not an
    # ideal-camera GPU execution (none was performed).
    cap=dict(base,schema='6es-explicit-ideal-camera-pilot-1',rays=rows,
        input_convention='Direct Decimal80 ideal60degree look-at camera and exact fixture union slab clip, rounded once to binary64 for existing CPU generator; no opaque scene',
        parent_own_input_sha256=B.digest(pilot/'clear-normal-rays.json'),bounds=dict(min=lo,max=hi),
        diagnostic_repeated_output_columns=True)
    path=output/'clear-ideal-rays.json';B.write(path,cap)
    B.write(output/'selection.json',dict(schema='6es-ideal-optical-selection-1',jobs=[dict(name='clear-ideal',count=len(rows),input=str(path),input_sha256=B.digest(path))],zone_sha256=B.digest(B.ZONE),log2N=16,threads=8,timeout_seconds=300))


def run(selection,binary,name):
    sel=B.read(selection);job=next(j for j in sel['jobs'] if j['name']==name);path=Path(job['input'])
    B.need(B.digest(path)==job['input_sha256'] and B.digest(B.ZONE)==sel['zone_sha256'],'optical reference input identity')
    before=E.fingerprint(binary);out=selection.parent/(name+'-reference.json');log=out.with_suffix('.log');receipt_path=out.with_name(name+'-reference-receipt.json')
    B.need(not out.exists() and not log.exists() and not receipt_path.exists(),'reference overwrite refused')
    argv=[str(binary),str(path),'16',str(job['count']),'8',str(out)]
    env={k:v for k,v in os.environ.items() if not k.startswith('EC_')};t=time.monotonic();started=dt.datetime.now(dt.timezone.utc).isoformat()
    with log.open('w') as stream:
        try:code=subprocess.run(argv,cwd=B.ROOT,env=env,stdout=stream,stderr=subprocess.STDOUT,timeout=300).returncode
        except subprocess.TimeoutExpired:code='timeout'
    after=E.fingerprint(binary)
    rec=dict(schema='6es-own-input-reference-execution-1',argv=argv,started_utc=started,elapsed_seconds=time.monotonic()-t,exit_code=code,
        before=before,after=after,selection_sha256=B.digest(selection),input_sha256=B.digest(path),input_sha256_after=B.digest(path),
        reference_sha256=B.digest(out) if out.exists() else None,log_sha256=B.digest(log),status='REFUSED')
    if code==0 and before==after and rec['input_sha256']==job['input_sha256']:
        values=B.read(out);B.need(len(values['rows'])==job['count'] and values['columns']==['T','Lr','Lg','Lb','T_half','Lr_half','Lg_half','Lb_half'],'reference coverage')
        B.need(all(len(v)==8 and all(math.isfinite(x) for x in v) for v in values['rows']),'reference finite values')
        rec['max_convergence_T']=max(abs(v[0]-v[4]) for v in values['rows'])
        rec['max_convergence_RGB']=max(abs(v[k]-v[k+4]) for v in values['rows'] for k in range(1,4))
        if max(rec['max_convergence_T'],rec['max_convergence_RGB'])<=1e-6:rec['status']='PILOT_CONVERGED'
    B.write(receipt_path,rec);B.need(rec['status']=='PILOT_CONVERGED','own-input pilot reference refused');print(json.dumps(rec))


def main():
    p=argparse.ArgumentParser(description=__doc__);sub=p.add_subparsers(dest='command',required=True)
    q=sub.add_parser('build');q.add_argument('--build-root',type=Path,required=True);q.add_argument('--work',type=Path,required=True)
    q=sub.add_parser('capture');q.add_argument('--work',type=Path,required=True);q.add_argument('--case',required=True);q.add_argument('--binary',type=Path,required=True);q.add_argument('--scene',choices=['none','front'],default='none');q.add_argument('--mutation',choices=['normal','canonical-matvec','control','no-optics'],required=True)
    q=sub.add_parser('prepare');q.add_argument('--work',type=Path,required=True);q.add_argument('--out',type=Path,required=True)
    q=sub.add_parser('reference');q.add_argument('--selection',type=Path,required=True);q.add_argument('--binary',type=Path,required=True);q.add_argument('--name',required=True)
    a=p.parse_args()
    if a.command=='build':B.renderer_source=R.renderer_source;B.probe_source=probe_source;B.build(a.build_root.resolve(),a.work.resolve())
    elif a.command=='capture':
        rec=B.invoke(a.work.resolve(),a.case,a.binary.resolve(),f'{a.scene}:{a.mutation}:0:190',334,190,'0,50,75,40,62,110');B.write(a.work/a.case/'receipt.json',rec)
        if a.mutation!='control':
            try:print('valid optical records',len(optical_check(a.work/a.case)))
            except ValueError as e:print('REJECT',e);raise SystemExit(2)
    elif a.command=='prepare':prepare(a.work,a.out)
    else:run(a.selection,a.binary,a.name)


if __name__=='__main__':main()
