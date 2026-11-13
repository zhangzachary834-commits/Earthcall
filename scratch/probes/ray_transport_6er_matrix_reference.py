"""Independent Decimal80 inspection of the recorded GPU matrix operands.
This is NOT the authored camera/integral oracle or an acceptance tolerance.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
"""
import argparse
import json
from decimal import Decimal, localcontext
from pathlib import Path

import ray_transport_6eq_screen_bridge as B


def analyze(work):
    def arrays(name):
        p=work/name
        return tuple(memoryview((p/('capture.rgba.'+stage+'.bin')).read_bytes()).cast('f')
                     for stage in ('fragment','compute'))
    normal=arrays('trace-normal');uniform=arrays('trace-trace-uniform')
    viewport=arrays('trace-trace-viewport');ndc=arrays('trace-trace-ndc');homogeneous=arrays('trace-trace-h')
    n=len(normal[0])//16
    # Trace variants must not perturb the recorded ray prefix before they may
    # explain that prefix. The world-value trace is intentionally excluded.
    stable={}
    for name in ['trace-trace-uniform','trace-trace-viewport','trace-trace-ndc','trace-trace-h','trace-trace-world']:
        p=work/name;counts=[]
        for stage in ('fragment','compute'):
            a=(work/'trace-normal'/('capture.rgba.'+stage+'.bin')).read_bytes()
            b=(p/('capture.rgba.'+stage+'.bin')).read_bytes()
            counts.append(sum(a[i*64:i*64+48]!=b[i*64:i*64+48] for i in range(n)))
        stable[name]=counts
    B.need(all(stable[name]==[0,0] for name in stable if name!='trace-trace-world'), 'instrumented prefix changed')
    B.need(all(uniform[0][i*16+k]==uniform[1][i*16+k] and
               viewport[0][i*16+k]==viewport[1][i*16+k] and
               ndc[0][i*16+k]==ndc[1][i*16+k] for i in range(n) for k in range(12,16)),
           'matrix/viewport/NDC operands differ')
    with localcontext() as ctx:
        ctx.prec=80
        D=Decimal.from_float
        matrix=[[D(float(uniform[0][j*16+12+k])) for k in range(4)] for j in range(4)]
        maxerr=[[Decimal(0)]*4 for _ in range(2)];sumerr=[[Decimal(0)]*4 for _ in range(2)]
        closer=[0,0,0];direction_counts=[0,0,0];direction_max=[Decimal(0),Decimal(0)];samples=[];selected={i*(n-1)//31 for i in range(32)}
        for i in range(n):
            a=i*16;v=[D(float(ndc[0][a+12+k])) for k in range(4)]
            h=[sum((matrix[j][k]*v[j] for j in range(4)),Decimal(0)) for k in range(4)]
            for k in range(4):
                errors=[abs(D(float(homogeneous[stage][a+12+k]))-h[k]) for stage in range(2)]
                for stage in range(2):maxerr[stage][k]=max(maxerr[stage][k],errors[stage]);sumerr[stage][k]+=errors[stage]
                closer[0 if errors[1]<errors[0] else 1 if errors[1]>errors[0] else 2]+=1
            eye=[D(float(normal[0][a+k])) for k in range(3)]
            direction=[h[k]/h[3]-eye[k] for k in range(3)];length=sum(x*x for x in direction).sqrt();direction=[x/length for x in direction]
            errors=[max(abs(D(float(normal[stage][a+4+k]))-direction[k]) for k in range(3)) for stage in range(2)]
            direction_counts[0 if errors[1]<errors[0] else 1 if errors[1]>errors[0] else 2]+=1
            for stage in range(2):direction_max[stage]=max(direction_max[stage],errors[stage])
            if i in selected:
                samples.append(dict(index=i,fragment_max_abs_error=float(errors[0]),compute_max_abs_error=float(errors[1])))
        return dict(scope=__doc__.splitlines()[0],precision=80,records=n,
           prefix_perturbations=stable,shared_matrix_viewport_ndc=True,
           homogeneous_max_abs_error=[[float(x) for x in row] for row in maxerr],
           homogeneous_mean_abs_error=[[float(x/n) for x in row] for row in sumerr],
           homogeneous_component_counts=dict(compute_closer=closer[0],compute_worse=closer[1],equal=closer[2]),
           direction_samples=samples,direction_counts=dict(compute_closer=direction_counts[0],compute_worse=direction_counts[1],equal=direction_counts[2]),
           direction_max_abs_error=[float(x) for x in direction_max],acceptance_granted=False)


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--work',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    r=analyze(a.work);B.write(a.output,r)
    print(json.dumps({k:r[k] for k in ['homogeneous_max_abs_error','homogeneous_component_counts','prefix_perturbations']}))


if __name__=='__main__':main()
