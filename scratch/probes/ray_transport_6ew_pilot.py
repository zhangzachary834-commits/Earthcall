"""Freeze a bounded multi-view numerical pilot before reference computation.
Current floats are observed through one identical shader with runtime record
selection; unmodified full Screen images are independent observer controls.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
"""
import argparse
from decimal import Decimal,localcontext
import json
import math
from pathlib import Path
import struct
import msgpack
import ray_transport_6ew_native as N
import ray_transport_6es_camera_reference as C
R=N.R


def f32(x):return struct.unpack('<f',struct.pack('<f',x))[0]


def native_arrays(work,scene):
 on=work/(scene+'-on-observed');plain=work/(scene+'-on-plain');off=work/(scene+'-off-plain');records=[work/(scene+'-off-observed-m'+str(i)) for i in range(3)]
 image=(off/'image.rgba').read_bytes();candidate_image=(on/'image.rgba').read_bytes();R.need(candidate_image==(plain/'image.rgba').read_bytes(),'candidate observer Screen '+scene)
 metadata=R.read(on/'image.rgba.observer.json');R.need(metadata['full_publication_sha256']==R.read(plain/'image.rgba.observer.json')['full_publication_sha256'],'entire candidate publication observer '+scene);R.need(metadata['copied_rows']==1800,'host setup copy coverage '+scene)
 for directory in records:R.need((directory/'image.rgba').read_bytes()==image,'Current observer Screen '+scene+' '+directory.name)
 raw=[(directory/'image.rgba.mrt.bin').read_bytes() for directory in records];rays=(on/'image.rgba.rays.bin').read_bytes();pub=(on/'image.rgba.publication.bin').read_bytes();count=640*360
 R.need(all(len(x)==count*16 for x in [*raw,pub]) and len(rays)==count*48,'complete stride-four native records')
 return on,records,image,candidate_image,raw,rays,pub


def rows(work,scene):
 on,dirs,image,cimage,raw,rays,pub=native_arrays(work,scene);valid=[];eye=list(map(float,N.SCENES[scene][0].split(',')[:3]));current=[];candidate=[]
 for i in range(640*360):
  ro=struct.unpack_from('<4f',raw[0],i*16);rd=struct.unpack_from('<4f',raw[1],i*16);val=struct.unpack_from('<4f',raw[2],i*16);ray=struct.unpack_from('<8f',rays,i*48);flags=struct.unpack_from('<I',rays,i*48+44)[0];v=struct.unpack_from('<4f',pub,i*16)
  if not(rd[3]>ro[3] and flags==0 and v[3]>=0):continue
  R.need(list(ro[:3])==list(ray[:3])==eye,'native observer eye binding '+scene)
  R.need(all(math.isfinite(x) for x in [*ro,*rd,*val,*ray,*v]) and 0<=val[0]<=1 and 0<=v[3]<=1,'finite valid native row')
  x=(i%640)*4;y=(i//640)*4;pixel=y*2560+x
  # Reconstruct the observable premultiplied-black Screen at sampled pixels.
  color=bytes(max(0,min(255,round(255*z))) for z in val[1:])+bytes([255]);R.need(color==image[pixel*4:pixel*4+4] or (scene=='front' and x<1280),'Current float-to-Screen reconstruction '+scene)
  out=bytes(max(0,min(255,round(255*z))) for z in v[:3])+bytes([255]);R.need(out==cimage[pixel*4:pixel*4+4] or (scene=='front' and x<1280),'candidate float-to-Screen reconstruction '+scene)
  # Publication comparison uses 1-alpha for both paths; preserve raw Current T too.
  cur=[x,y,*rd[:3],ro[3],rd[3],1-f32(1-val[0]),*val[1:]];can=[x,y,*ray[4:7],ray[3],ray[7],1-v[3],*v[:3]]
  valid.append(i);current.append(cur);candidate.append(can)
 R.need(len(valid)>=80,'paired native coverage '+scene)
 return dict(valid=valid,current=current,candidate=candidate,current_dirs=dirs,candidate_dir=on)


def prepare(work,out):
 R.need(not out.exists(),'fresh frozen selection');out.mkdir();jobs=[];captures={};geometry=json.loads(msgpack.unpackb(R.ZONE.read_bytes(),raw=False)['MigrationRoot']);fields=geometry['spatialFields'];lo=[min(f['origin'][k]-abs(f['scale'][k]) for f in fields) for k in range(3)];hi=[max(f['origin'][k]+abs(f['scale'][k]) for f in fields) for k in range(3)]
 for scene in N.SCENES:
  data=rows(work,scene);valid=data['valid'];n=len(valid);base={k*(n-1)//63 for k in range(64)}
  rank=sorted(range(n),key=lambda k:(-abs(data['candidate'][k][7]-data['current'][k][7]),valid[k]))
  for k in rank:
   if len(base)>=80:break
   base.add(k)
  positions=sorted(base);R.need(len(positions)==80,'fixed pilot cardinality');indices=[valid[k] for k in positions];camera=N.SCENES[scene][0];eye=list(map(float,camera.split(',')[:3]));look=list(map(float,camera.split(',')[3:]));caps={}
  for kind in ['current','candidate']:
   values=[data[kind][k] for k in positions];r=[[ *row[:7],*(row[7:11]*4)] for row in values]
   cap=dict(schema='6ew-actual-native-own-input-1',scene=scene,lane=kind,time=7.25,origin=eye,camera=camera,viewport=[2560,1440],sample_stride=4,rays=r,selection=dict(indices=indices,policy='64 uniform paired valid pixels plus largest native T differences until80; fixed before references'),zone_sha256=R.sha(R.ZONE),diagnostic_repeated_output_columns=True,native_receipts_sha256=[R.sha(d/'receipt.json') for d in data['current_dirs']] if kind=='current' else [R.sha(data['candidate_dir']/'receipt.json')])
   path=out/(scene+'-'+kind+'-rays.json');R.write(path,cap);caps[kind]=cap;jobs.append(dict(name=scene+'-'+kind,input=path.name,input_sha256=R.sha(path),count=80))
  if scene!='front':
   cap=dict(caps['current']);r=[]
   with localcontext() as context:
    context.prec=80;D=Decimal;origin=list(map(D,eye));axes=C.basis(origin,list(map(D,look)))
    for row in caps['current']['rays']:
     x,y=row[:2];direction=C.ideal_direction(x,y,2560,1440,axes);entries=[];exits=[]
     for k in range(3):
      if direction[k]==0:R.need(D(lo[k])<=origin[k]<=D(hi[k]),'parallel outside ideal ray');continue
      a=(D(lo[k])-origin[k])/direction[k];b=(D(hi[k])-origin[k])/direction[k];entries.append(min(a,b));exits.append(max(a,b))
     a=max(D(0),max(entries));b=min(exits);R.need(b>a,'empty ideal selected ray requires explicit treatment');r.append([x,y,*map(float,direction),float(a),float(b),*row[7:]])
   cap.update(schema='6ew-explicit-ideal-camera-input-1',lane='ideal',rays=r,input_convention='Decimal80 direct60degree look-at camera/exact fixture union slab, rounded once to binary64 for existing OntoMath reference; no opaque scene',bounds=dict(min=lo,max=hi));path=out/(scene+'-ideal-rays.json');R.write(path,cap);jobs.append(dict(name=scene+'-ideal',input=path.name,input_sha256=R.sha(path),count=80))
  captures[scene]=dict(paired_valid_sampled_pixels=len(valid),selected_indices=indices,camera=camera,opaque=N.SCENES[scene][1])
 R.write(out/'selection.json',dict(schema='6ew-frozen-multiview-pilot-1',jobs=jobs,captures=captures,zone_sha256=R.sha(R.ZONE),log2N=16,threads=8,reference_seconds_cap=600,maximum_reference_rows_including_continuity=960,full_population_qualified=False));print('FROZEN',len(jobs),'reference jobs',len(jobs)*80,'pilot rows',flush=True)


def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--work',type=Path,required=True);p.add_argument('--out',type=Path,required=True);a=p.parse_args();prepare(a.work.resolve(),a.out.resolve())
if __name__=='__main__':main()
