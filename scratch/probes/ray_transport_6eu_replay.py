"""Replay retained actual-renderer §6eu evidence; never implies a fresh GPU run.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-28.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import re
import statistics
import struct
import zipfile


def need(ok, message):
    if not ok:
        raise ValueError(message)


def stats(values):
    v=sorted(values)
    return dict(mean=statistics.mean(v),p99=v[math.ceil(.99*len(v))-1],max=max(v))


def replay(path):
    with zipfile.ZipFile(path) as z:
        j=lambda n:json.loads(z.read(n))
        manifest=j('manifest.json')
        need(set(z.namelist())==set(manifest['sha256'])|{'manifest.json'},'archive inventory')
        for name,expected in manifest['sha256'].items():
            digest=hashlib.sha256()
            with z.open(name) as f:
                while data:=f.read(1048576):digest.update(data)
            need(digest.hexdigest()==expected,'archive bytes: '+name)
        build=j('final/build.json')
        need(build['before']==build['after'],'build source identity')
        cases={};images={}
        for name in manifest['cases']:
            prefix='final/'+name+'/'
            receipt=j(prefix+'receipt.json');log=z.read(prefix+'native.log').decode()
            need(receipt['before']==receipt['after']==build['after'],'source binding '+name)
            need(receipt['binary_sha256']==build['binary_sha256'],'binary binding '+name)
            need(receipt['exit_code']==0,'native process '+name)
            need(not any(s in log.lower() for s in ['validation error','uncaptured error','device lost','[routed compiler]']),'GPU/compiler '+name)
            need('MOVEMENT default=12' in log and 'ROUTE_DEFAULT 1' in log,'actual defaults '+name)
            for file,sha in receipt['files_sha256'].items():need(manifest['sha256'][prefix+file]==sha,'receipt bytes '+name)
            w,h=receipt['width'],receipt['height'];image=z.read(prefix+'image.rgba');images[name]=image
            need(len(image)==w*h*4,'Screen coverage '+name)
            dispatch=[int(v) for v in re.findall(r'REVIEW routedSets=(\d+)',log)]
            routed=receipt['mode']!='off' and receipt['mask']=='0123' and not receipt.get('no_source',False)
            need(len(dispatch)==5 and all(v==int(routed) for v in dispatch),'dispatch eligibility '+name)
            row=dict(width=w,height=h,dispatches=dispatch)
            if routed:
                pub=z.read(prefix+'image.rgba.publication.bin');need(len(pub)==w*h*16,'publication coverage '+name)
                counts=dict(accepted=0,refused=0,nonempty=0,nonfinite=0)
                for v in struct.iter_unpack('<4f',pub):
                    counts['nonfinite']+=sum(not math.isfinite(x) for x in v)
                    if v[3]<0:counts['refused']+=1
                    else:
                        counts['accepted']+=1
                        counts['nonempty']+=int(v[3]>1e-5 or max(abs(x) for x in v[:3])>1e-6)
                m=re.search(r'PUBLICATION accepted=(\d+) refused=(\d+) nonempty=(\d+) nonfinite=(\d+)',log)
                need(m and list(map(int,m.groups()))==list(counts.values()),'GPU readback census '+name)
                need(counts['nonfinite']==0,'finite GPU publication '+name)
                row['publication']=counts
            cases[name]=row
        comparisons={}
        for a,b in [('forced','off'),('full-forced','full-off'),('unsupported-on','unsupported-off'),('front-forced','front-off'),('outside-forced','outside-off'),('nosource-on','nosource-off')]:
            need(images[a]==images[b],'exact Current fallback '+a)
            comparisons[a+' == '+b]=dict(bytes=len(images[a]),different_bytes=0)
        full=z.read('final/full-on/image.rgba.publication.bin');fallback=[]
        for i,v in enumerate(struct.iter_unpack('<4f',full)):
            if v[3]<0:
                need(images['full-on'][i*4:i*4+4]==images['full-off'][i*4:i*4+4],'natural refusal Screen '+str(i));fallback.append(i)
        front=z.read('final/front-on/image.rgba.publication.bin')
        for y in range(190):
            for x in range(167):
                i=y*334+x
                need(struct.unpack_from('<4f',front,i*16)==(0,0,0,0),'opaque-front publication')
                need(images['front-on'][i*4:i*4+4]==images['front-off'][i*4:i*4+4],'opaque-front Screen')
        need('RECIPE PASS checks=33 media=4' in z.read('final/recipe.log').decode(),'authored recipe controls')
        rays=j('reference/inputs.json');refs=j('reference/reference.json');need(len(rays['rays'])==len(refs['rows'])==80,'explicit common-target pilot')
        need(rays['zone_sha256']==build['after']['zone_sha256'],'reference fixture identity')
        need(j('final/on/receipt.json')['camera']=='0,50,75,40,62,110' and rays['time']==7.25 and rays['origin']==[0.0,50.0,75.0],'explicit pilot camera/time')
        need(z.read('baseline/baseline-off.rgba')==z.read('baseline/baseline-on.rgba')==images['off'],'unrepaired inert lane and retained Current image')
        for name in ['off','on']:
            log=z.read('baseline/baseline-'+name+'.log').decode();need(re.findall(r'REVIEW routedSets=(\d+)',log)==['0']*5,'unrepaired actual dispatch census')
        pub=z.read('final/on/image.rgba.publication.bin');pilot={}
        for label,offset in [('2^16',0),('2^15',4)]:
            errors={k:[] for k in ['historical_Current_float_T','historical_Current_float_RGB','live_routed_T','live_routed_RGB','fresh_Current_RGBA8_RGB','fresh_routed_RGBA8_RGB']}
            for ray,ref in zip(rays['rays'],refs['rows']):
                i=int(ray[1])*334+int(ray[0]);v=struct.unpack_from('<4f',pub,i*16);truth=ref[offset:offset+4]
                errors['historical_Current_float_T'].append(abs(ray[7]-truth[0]));errors['historical_Current_float_RGB'].append(max(abs(a-b) for a,b in zip(ray[8:11],truth[1:])))
                errors['live_routed_T'].append(abs(1-v[3]-truth[0]));errors['live_routed_RGB'].append(max(abs(a-b) for a,b in zip(v[:3],truth[1:])))
                for name,key in [('off','fresh_Current_RGBA8_RGB'),('on','fresh_routed_RGBA8_RGB')]:errors[key].append(max(abs(images[name][i*4+k]/255-truth[k+1]) for k in range(3)))
            pilot[label]={k:stats(v) for k,v in errors.items()}
            for kind in ['T','RGB']:
                baseline=pilot[label]['historical_Current_float_'+kind];candidate=pilot[label]['live_routed_'+kind]
                need(all(candidate[k]<=baseline[k] for k in ['mean','p99','max']),'pilot float metrics '+kind)
            need(all(pilot[label]['fresh_routed_RGBA8_RGB'][k]<=pilot[label]['fresh_Current_RGBA8_RGB'][k] for k in ['mean','p99','max']),'fresh Screen pilot metrics')
        cost=j('final/native-cost-receipts.json');need(len(cost)==16,'four connected ABBA blocks');samples=[]
        for i,r in enumerate(cost):
            need(r['mode']==['off','on','on','off'][i%4] and r['exit_code']==0,'cost order/process')
            need(r['source_before']==r['source_after']==build['after'] and r['binary_sha256']==build['binary_sha256'],'cost source/binary identity')
            log=z.read('final/'+r['log']).decode();need(hashlib.sha256(log.encode()).hexdigest()==r['log_sha256'],'cost log identity')
            need(not any(t in log.lower() for t in ['validation error','uncaptured error','device lost','[routed compiler]']),'cost GPU/compiler')
            m=re.search(r'COST frames=12 total_ms=([.0-9]+)',log);need(m,'actual complete cost');samples.append(float(m[1]))
        blocks=[]
        for i in range(0,16,4):
            off=statistics.mean([samples[i],samples[i+3]]);on=statistics.mean(samples[i+1:i+3]);blocks.append(dict(off_ms=off,on_ms=on,ratio=on/off))
        result=dict(schema='6eu-retained-live-renderer-review-1',cases=cases,exact_comparisons=comparisons,natural_refused_pixels=fallback,opaque_front_zero_pixels=167*190,recipe_checks=33,common_ideal_camera_80_pixel_pilot=pilot,connected_debug_frame_cost=dict(blocks=blocks,median_ratio=statistics.median(b['ratio'] for b in blocks),frames_per_sample=12,samples=16),scope='Retained Apple M5 offscreen evidence. Historical float Current comparison is explicitly historical; fresh Screen comparison is native RGBA8. No full-population reference closure, moving-scene FPS, display pacing, resize/edit/device recreation or Person acceptance claim.')
        return result


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('archive',type=Path);p.add_argument('--output',type=Path);a=p.parse_args();result=replay(a.archive)
    if a.output:a.output.write_text(json.dumps(result,indent=2,allow_nan=False)+'\n')
    print('PASS retained native replay:',len(result['cases']),'captures;',len(result['exact_comparisons']),'exact comparisons; 33 authored recipe controls; 80-pixel common-target pilot; 16 complete-frame clocks.')

if __name__=='__main__':main()
