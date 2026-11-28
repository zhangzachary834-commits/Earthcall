"""CPU replay of stored Mode2 ownership/refusal controls and distinct capture.
Codex/GPT-6.1 Sol/01a148a7-bd85-7550-a416-0e859a729c1a/2026-10-28.
Replays recorded results, not native publication assertions or new-ray accuracy.
"""
import argparse,hashlib,importlib.util,json,math,struct,sys,zipfile
from pathlib import Path
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(HERE))
import sonnet_surveyor_layout as L


def load(name):
    spec=importlib.util.spec_from_file_location(name,HERE/(name+'.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--record',type=Path,required=True);p.add_argument('--archive',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    need=lambda ok,msg: (_ for _ in ()).throw(ValueError(msg)) if not ok else None
    rec=json.loads(a.record.read_text());digest=lambda b:hashlib.sha256(b).hexdigest();need(digest(a.archive.read_bytes())==rec['archive_sha256'],'archive identity')
    g=load('ray_transport_full_workload_accuracy');report={'scope':'stored native evidence replay; no new native work or new-population accuracy/performance qualification'}
    with zipfile.ZipFile(a.archive) as z:
        for name,sha in rec['members_sha256'].items():need(digest(z.read(name))==sha,'member identity '+name)
        read=lambda name:json.loads(z.read(name))
        pos=read('native/positive.json')['cases'];need(len(pos)==5,'five positive cases')
        rays=read('native/original-rays.json');current=read('native/positive.json-baseline-rows.json');nominal=pos[0]['outputs'];accuracy={}
        word=lambda f:struct.unpack('<I',struct.pack('<f',f))[0]
        reference=g.read(HERE.parents[1]/'docs/audits/NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27_ROWS.json.gz')
        need(g.digest(HERE.parents[1]/'docs/audits/NORTHERN_VEIL_WIDE_REFERENCE_2026-10-27_ROWS.json.gz')==rec['reference_sha256'],'reference identity')
        for c in pos:
            n=len(rays['rays']);need(len(c['outputs'])==n and all(c[x]==0 for x in ['owners','publishers','flags','wrong','packets']),'native positive assertion report')
            intended=set(c['intended_refusal_ids']);need(len(intended)==len(c['intended_refusal_ids'])==c['refused'],'intended set coverage')
            rows=[]
            for i,row in enumerate(c['outputs']):
                need(row[0]==i and bool(row[3]&0x80000000)==(i in intended) and not row[3]&0x40000000,'actual refusal flag/order')
                truth=[word(x) for x in current[i][2:]] if i in intended else nominal[i][1]
                need(row[1]==row[2]==truth,'actual output/Current-or-bulk selection')
                rows.append([*rays['rays'][i][:2],*[g.decode(x) for x in row[1]]])
            q=g.evaluate(rows,current,reference,digest(z.read('native/original-rays.json')));need(q['both_resolutions_pass'],'old-population accuracy '+c['case']);accuracy[c['case']]=q
        controls=read('native/controls.json')['controls'];need(len(controls)==8 and controls[0]['returncode']==0,'native control count')
        for c in controls[1:]:need(c['returncode']!=0 and c['expected_error'] in z.read('native/'+c['case']+'.log').decode(),'targeted negative '+c['case'])
        meta=read('capture/metadata.json');header=meta['ray_header'];geo=L.Geometry(*header['viewport'],*header['capture_grid']);live=z.read('capture/census-live16.bin');need(len(live)==geo.slots*64,'packed census size')
        raw=bytearray(geo.census_bytes)
        for i in range(geo.slots):raw[i*256:i*256+64]=live[i*64:i*64+64]
        need(digest(raw)==header['census_sha256'],'lossless census reconstruction')
        rows,counts=L.decode_census(raw,geo);need(counts==meta['manifest']['census_report'] and len(rows)==230400,'distinct census decoder')
        reconstructed=dict(header,rays=rows);need(digest((json.dumps(reconstructed)+'\n').encode())==meta['manifest']['rays_sha256'],'distinct original ray bytes')
        lay=L.build_layout(geo,[(int(r[0]),int(r[1])) for r in rows],tuple(meta['manifest']['tile']),meta['manifest']['layout_mode']);stats=L.validate_layout(lay,geo,[(int(r[0]),int(r[1])) for r in rows]);need(stats==meta['manifest']['layout_stats'] and lay.tiles==14400,'independent tile reconstruction')
        report.update(old_population_accuracy=accuracy,native_positive_cases=5,targeted_negative_cases=7,distinct_capture_rays=len(rows),distinct_capture_tiles=lay.tiles,new_capture_reference='NOT GENERATED; no accuracy or candidate cost admission',native_assertion_scope='ownership and demand/route checks were native host assertions bound by generated source and logs; CPU replay checks their saved reports')
    a.output.write_text(json.dumps(report,indent=2)+'\n');print('PASS: five Mode2 actual-output/refusal cases, seven targeted negatives, original dual-resolution accuracy; lossless 230400-ray census and14400-tile reconstruction. No new-capture accuracy/FPS claim.')

if __name__=='__main__':main()
