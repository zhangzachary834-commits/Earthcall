"""Capture additional native camera rays and build independent AST references.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Frozen authored save only; sparse fragment instrumentation, not frame timing.
"""
import argparse,hashlib,json,math,os,struct,subprocess
from pathlib import Path

CASES={'inside':('0,50,75,40,62,110',7.25),'reverse':('0,60,160,0,55,75',7.25),
       'side':('90,55,90,-30,55,90',7.25),'spawn':('0,2,0,0,60,90',7.25),
       'inside-zero-clock':('0,50,75,40,62,110',0.0)}
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--work',type=Path,required=True)
    p.add_argument('--capture-bin',type=Path,required=True);p.add_argument('--reference-bin',type=Path)
    p.add_argument('--samples',type=int,default=192);p.add_argument('--log2n',type=int,default=16);p.add_argument('--threads',type=int,default=8)
    p.add_argument('--cases',default=','.join(CASES));p.add_argument('--reference-only',action='store_true')
    a=p.parse_args();root=a.root.resolve();work=a.work.resolve()
    if root==work or root in work.parents or not 5<=a.samples<=2304 or not 12<=a.log2n<=20 or not 1<=a.threads<=16:raise SystemExit('invalid bounded scratch request')
    selected=a.cases.split(',')
    if not all(x in CASES for x in selected) or len(set(selected))!=len(selected):raise SystemExit('invalid cases')
    zone=root/'saves/zones/Northern Veil/zone.ecform';zone_hash=sha(zone)
    if not a.reference_only:work.mkdir(parents=True,exist_ok=False)
    for name in selected:
        camera,clock=CASES[name];directory=work/name
        if not a.reference_only:
            directory.mkdir();capture=directory/'capture.bin'
            env=dict(os.environ,EC_CAM=camera,EC_VOLUME_SOURCE_MODE='authored',EC_CENSUS=str(capture),EC_CF4_BASELINE='1')
            for key in ['EC_FULL_LATTICE','EC_REFERENCE_SAMPLES','EC_VOLUME_OUTPUT_ERROR_BUDGET']:env.pop(key,None)
            cmd=[str(a.capture_bin.resolve()),str(zone),'0123','2560','1440',str(clock),str(directory/'capture.rgba'),'1']
            with (directory/'capture.log').open('w') as log:subprocess.run(cmd,cwd=root,env=env,stdout=log,stderr=subprocess.STDOUT,check=True)
            b=capture.read_bytes()
            if len(b)!=64*36*64*4:raise ValueError('unexpected capture size')
            u=struct.unpack('<'+'I'*(len(b)//4),b);f=struct.unpack('<'+'f'*(len(b)//4),b)
            rows=[];origins=[]
            for i in range(2304):
                at=i*64
                if u[at]!=1 or u[at+3]!=1:continue
                row=[u[at+1],u[at+2],*f[at+8:at+11],f[at+7],f[at+11],*f[at+12:at+16]]
                if not all(math.isfinite(x) for x in row) or row[5]>row[6]:raise ValueError('nonfinite/reversed native ray')
                rows.append(row+[0.0]*12);origins.append(list(f[at+4:at+7]))
            if len(rows)<a.samples or not all(x==origins[0] for x in origins):raise ValueError(('insufficient or mixed-origin capture',name,len(rows)))
            # Uniform rank selection among actual rasterized volume rays; no
            # density, error or candidate output enters this selection.
            keep=[min(len(rows)-1,int((k+.5)*len(rows)/a.samples)) for k in range(a.samples)]
            rays={'scope':'Actual native f32 camera rays/current transport from instrumented sparse volume fragments; not an uninstrumented full-frame oracle',
                  'view':name,'viewport':[2560,1440],'time':clock,'origin':origins[0],
                  'camera':camera,'capture_sha256':sha(capture),'native_volume_rays':len(rows),
                  'selection':'Uniform rank among captured volume rays, before candidate/reference evaluation',
                  'column_groups':['pixel x,y','direction x,y,z','clip start,end','native current T,Lr,Lg,Lb','12 unused legacy placeholders'],
                  'rays':[rows[i] for i in keep]}
            (directory/'rays.json').write_text(json.dumps(rays,indent=2)+'\n')
            print('CAPTURE',name,'native',len(rows),'selected',len(keep),flush=True)
        if a.reference_bin:
            rays=json.loads((directory/'rays.json').read_text());raw=directory/'reference-raw.json'
            if raw.exists():raise ValueError('refusing reference overwrite')
            cmd=[str(a.reference_bin.resolve()),str(directory/'rays.json'),str(a.log2n),str(a.samples),str(a.threads),str(raw)]
            with (directory/'reference.log').open('w') as log:subprocess.run(cmd,cwd=root,stdout=log,stderr=subprocess.STDOUT,check=True)
            ref=json.loads(raw.read_text());rr=ref['rows']
            if len(rr)!=len(rays['rays']) or any(len(x)!=8 or not all(math.isfinite(v) for v in x) for x in rr):raise ValueError('invalid reference rows')
            result={'scope':'Independent original MathNode evaluation, cumulative Simpson transport; finite nested convergence witness, not exact oracle',
                    'columns':['pixel x','pixel y',*ref['columns']],
                    'finest_intervals':1<<a.log2n,'half_intervals':1<<(a.log2n-1),
                    'rows':[[*rays['rays'][i][:2],*row] for i,row in enumerate(rr)]}
            (directory/'reference.json').write_text(json.dumps(result,indent=2)+'\n')
            manifest={'scope':'Pinned camera/clock numerical experiment only; no source generality or native omission admission',
                'case':name,'zone_sha256':zone_hash,'rays':str(directory/'rays.json'),'rays_sha256':sha(directory/'rays.json'),
                'reference':str(directory/'reference.json'),'reference_sha256':sha(directory/'reference.json'),
                'capture_binary_sha256':sha(a.capture_bin.resolve()),'reference_binary_sha256':sha(a.reference_bin.resolve())}
            (directory/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
            print('REFERENCE',name,'complete',flush=True)
        if sha(zone)!=zone_hash:raise ValueError('authored fixture changed')

if __name__=='__main__':main()
