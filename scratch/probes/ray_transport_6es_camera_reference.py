"""Decimal camera directions from perspective/look-at, independently of inversion.
Only the frozen diagnostic camera/viewport; not a generic rendering contract.
Codex / GPT-6.1 Sol / Sixth-One Sun / 01a148a7-bd85-7550-a416-0e859a729c1a /
2026-10-28. Zach requires authored faithfulness rather than pleasing differences.
"""
import argparse
from decimal import Decimal,localcontext
import hashlib
import json
from pathlib import Path
import struct
import zipfile

import ray_transport_6er_ray_setup as R
import ray_transport_fact_codec as C
B=R.B


def unit(v):
    length=sum(x*x for x in v).sqrt()
    B.need(length>0,'degenerate camera vector')
    return [x/length for x in v]


def cross(a,b):return [a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]]


def basis(eye,look):
    forward=unit([b-a for a,b in zip(eye,look)])
    right=unit(cross(forward,[Decimal(0),Decimal(1),Decimal(0)]))
    return forward,right,cross(right,forward)


def ideal_direction(x,y,w,h,axes):
    # tan(60 degrees/2)=1/sqrt(3). Exact integer pixel centres, aspect
    # and authored integer eye/look values avoid rounded matrix/NDC reuse.
    D=Decimal;f,r,u=axes
    nx=(D(2*x+1)/D(w)-1)*D(w)/D(h)/D(3).sqrt()
    ny=(1-D(2*y+1)/D(h))/D(3).sqrt()
    return unit([f[k]+nx*r[k]+ny*u[k] for k in range(3)])


def metrics(values):
    values=sorted(values);B.need(values and all(v.is_finite() for v in values),'finite error coverage')
    return dict(count=len(values),mean=float(sum(values)/len(values)),p99=float(values[len(values)*99//100]),max=float(values[-1]))


def analyze(record,archive,precision=80):
    rec=B.read(record);B.need(B.digest(archive)==rec['archive_sha256'],'6er evidence archive identity')
    cases={v['case']:v for v in rec['cases']}
    with zipfile.ZipFile(archive) as z:
        def get(name,stage='fragment'):
            key=cases[name]['facts'][stage];info=rec['facts'][key]
            packed=z.read(info['member']);B.need(hashlib.sha256(packed).hexdigest()==info['encoded_sha256'],'encoded facts')
            raw=C.decode(packed,info['raw_bytes']);B.need(hashlib.sha256(raw).hexdigest()==key,'native facts')
            return raw,memoryview(raw).cast('f')
        normal,n=get('trace-normal');ordered,o=get('canonical-clear');compute,c=get('trace-normal','compute')
        _,uniform=get('trace-trace-uniform');_,ndc=get('trace-trace-ndc');canonical_h,ch=get('canonical-h')
        receipt=json.loads(z.read(cases['canonical-clear']['receipt']))
        B.need(receipt['camera']=='0,50,75,40,62,110' and receipt['width']==334 and receipt['height']==190,'frozen camera identity')
        B.need(len(normal)==len(ordered)==len(compute)==len(canonical_h)==334*190*64,'camera population extent')
        B.need(all(ordered[i*64:i*64+48]==canonical_h[i*64:i*64+48] for i in range(334*190)),'ordered H trace prefix perturbation')
    with localcontext() as ctx:
        ctx.prec=precision;D=Decimal;F=Decimal.from_float
        axes=basis(list(map(D,[0,50,75])),list(map(D,[40,62,110])))
        # Independent analytic sanity controls: centre ray is forward; opposite
        # corners mirror their transverse components in a standard camera.
        standard=basis([D(0)]*3,[D(0),D(0),D(-1)])
        B.need(ideal_direction(1,1,3,3,standard)==[D(0),D(0),D(-1)],'analytic centre control')
        a=ideal_direction(0,0,2,2,standard);b=ideal_direction(1,1,2,2,standard)
        B.need(a[0]==-b[0] and a[1]==-b[1] and a[2]==b[2] and a[1]>0,'analytic orientation/symmetry control')
        matrix=[[F(float(uniform[j*16+12+k])) for k in range(4)] for j in range(4)]
        errors={kind:{name:[] for name in ['fragment','ordered','compute']} for kind in ['ideal_camera','rounded_operand_unprojection']}
        counts={kind:dict(ordered_closer=0,ordered_worse=0,equal=0) for kind in errors}
        worst={kind:[] for kind in errors}
        for i in range(334*190):
            at=i*16;v=[F(float(ndc[at+12+k])) for k in range(4)]
            h=[sum((matrix[j][k]*v[j] for j in range(4)),D(0)) for k in range(4)]
            operand_ray=unit([h[k]/h[3]-F(float(n[at+k])) for k in range(3)])
            ideal=ideal_direction(i%334,i//334,334,190,axes)
            for kind,reference in [('ideal_camera',ideal),('rounded_operand_unprojection',operand_ray)]:
                e=[]
                for name,data in [('fragment',n),('ordered',o),('compute',c)]:
                    value=max(abs(F(float(data[at+4+k]))-reference[k]) for k in range(3))
                    errors[kind][name].append(value);e.append(value)
                counts[kind]['ordered_closer' if e[1]<e[0] else 'ordered_worse' if e[1]>e[0] else 'equal']+=1
        report=dict(schema='6es-camera-reference-1',precision=precision,records=334*190,
            definition='Ideal 60 degree vertical perspective, exact viewport aspect/pixel centres, exact integer eye/look; independent direct look-at basis. Conditional second oracle uses uploaded f32 matrix and f32 NDC.',
            parent_record_sha256=B.digest(record),parent_archive_sha256=B.digest(archive),
            ordered_H_trace_prefix_perturbed=False,controls=['centre ray','corner symmetry and y orientation'],
            errors={kind:{name:metrics(v) for name,v in rows.items()} for kind,rows in errors.items()},
            ordered_vs_fragment_counts=counts,transport_qualified=False,portable_contract_adopted=False)
        for kind in errors:
            report.setdefault('worst',{})[kind]={name:[i%334,i//334] for name,values in errors[kind].items() for i in [max(range(len(values)),key=lambda j:values[j])]}
        return report


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--record',type=Path,required=True);p.add_argument('--archive',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--precision',type=int,choices=[80,110],default=80);a=p.parse_args()
    result=analyze(a.record,a.archive,a.precision);B.write(a.output,result)
    print(json.dumps(dict(errors=result['errors'],counts=result['ordered_vs_fragment_counts'])))


if __name__=='__main__':main()
