"""Independent raw replay for §6cq review; no GPU and no trusted saved summaries.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Input paths may name raw JSON or gzip archives; output refuses overwrite.
"""
import argparse, copy, gzip, hashlib, importlib.util, json, math, statistics, struct
from pathlib import Path


def load_json(path):
    p=Path(path)
    return json.loads(gzip.decompress(p.read_bytes()) if p.suffix=='.gz' else p.read_bytes())


def reference_metrics(raw,rays,refs):
    # Independent decoder/statistics; do not call the producer driver's comparator.
    if len(raw)!=len(refs) or len(raw)!=len(rays['rays']):raise ValueError('reference coverage')
    def stats(values):
        ordered=sorted(values)
        if not ordered or not all(math.isfinite(x) for x in ordered):raise ValueError('reference finiteness')
        return {'mean':sum(ordered)/len(ordered),'p99':ordered[len(ordered)*99//100],'max':ordered[-1]}
    result={}
    for name,start in [('finest',2),('half',6)]:
        metrics={}
        for lane in ['current','candidate']:
            rgb=[];trans=[]
            for actual,captured,truth in zip(raw,rays['rays'],refs):
                if actual[:2]!=captured[:2] or actual[:2]!=truth[:2]:raise ValueError('reference pixel order')
                color=captured[8:11] if lane=='current' else actual[2:5]
                t=captured[7] if lane=='current' else actual[5]
                rgb.extend(abs(color[k]-truth[start+1+k]) for k in range(3));trans.append(abs(t-truth[start]))
            metrics[lane]={'RGB_pooled_channels':stats(rgb),'T':stats(trans)}
        gates={channel+'.'+stat:metrics['candidate'][channel][stat]<=metrics['current'][channel][stat]
               for channel in ['RGB_pooled_channels','T'] for stat in ['mean','p99','max']}
        result[name]={'metrics':metrics,'gates':gates,'all_six_pass':all(gates.values())}
    result['reference_convergence']={'T_max_delta':max(abs(r[2]-r[6]) for r in refs),
        'RGB_max_delta':max(abs(r[3+k]-r[7+k]) for r in refs for k in range(3))}
    result['both_resolutions_pass']=result['finest']['all_six_pass'] and result['half']['all_six_pass']
    return result


def bits(v): return struct.pack('<'+str(len(v))+'f',*v)


def union(census):
    result=[]
    for row in census:
        members=[]
        for m in range(4):
            intervals=sorted([p[:2] for p in row[3] if p[2]==m]+[p[:2] for p in row[4] if p[2]&(1<<m)])
            merged=[]
            for a,b in intervals:
                if merged and a<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],b)
                else:merged.append([a,b])
            members.append(merged)
        result.append(members)
    return result


def clocks(report):
    summaries=[]
    for c in report['streamed_schedule']['cases']:
        series=[]
        for batch in c['series']:
            samples=batch['samples'];n=batch['repeats']
            if not isinstance(n,int) or n<=0:raise ValueError('invalid repeats')
            order=([0,1,2,2,1,0,2,1,0,0,1,2])*2
            if [s['mode'] for s in samples]!=order:raise ValueError('paired mode order')
            for s in samples:
                if any(not math.isfinite(s[k]) or s[k]<0 for k in ['total_ms','completion_ms','encode_ms']):raise ValueError('invalid clock')
                if not math.isclose(s['total_ms'],s['completion_ms']+s['encode_ms'],rel_tol=1e-12,abs_tol=1e-10):raise ValueError('clock accounting')
            means={str(m):statistics.mean(s['total_ms']/n for s in samples if s['mode']==m) for m in range(3)}
            paired=[]
            for i in range(0,len(samples),6):
                block=samples[i:i+6]
                avg={m:statistics.mean(s['total_ms']/n for s in block if s['mode']==m) for m in range(3)}
                paired.append({'bulk_minus_current_ms':avg[1]-avg[0],'stream_minus_current_ms':avg[2]-avg[0]})
            series.append({'repeats':n,'per_complete_query_ms':means,'bulk_mean_improvement_percent':100*(1-means['1']/means['0']),
                'stream_mean_improvement_percent':100*(1-means['2']/means['0']),'paired_block_deltas':paired,
                'scope':'mode1 is global bulk; capacity/tiles describe only mode2 bounded stream; single-series observations, not qualified speedup'})
        summaries.append({'capacity':c['capacity'],'tiles':c['tiles'],'series':series})
    return summaries


def check(root,report,raw,baseline,rays,refs,indices):
    n=len(rays['rays'])
    if not(len(raw)==len(baseline)==len(report['schedule_census_inputs'])==n):raise ValueError('query coverage')
    for i,(a,b,c) in enumerate(zip(raw,baseline,report['schedule_census_inputs'])):
        pixel=rays['rays'][i][:2]
        if any(v[:2]!=pixel for v in [a,b,c]) or len(a)<6 or len(b)!=6:raise ValueError('pixel alignment')
        if not all(math.isfinite(x) for x in a+b):raise ValueError('nonfinite output')
        captured=[rays['rays'][i][8],rays['rays'][i][9],rays['rays'][i][10],rays['rays'][i][7]]
        if max(abs(x-y) for x,y in zip(b[2:],captured))>=1e-5:raise ValueError('current baseline fidelity exceeded original1e-5 guard')
        if c[2]&0x80000000 and bits(a[2:6])!=bits(b[2:6]):raise ValueError('normal fallback mismatch')
        if not c[2]&0x80000000:
            if c[2]!=len(c[4]):raise ValueError('overlap header count')
            for lo,hi,member in c[3]:
                if not all(math.isfinite(x) for x in [lo,hi]) or not hi>lo or member not in range(4):raise ValueError('single interval')
            for lo,hi,mask in c[4]:
                if not all(math.isfinite(x) for x in [lo,hi]) or not hi>lo or not isinstance(mask,int) or not 1<mask<=15 or mask.bit_count()<2:raise ValueError('overlap interval')
            ordered=sorted([x[:2] for x in c[3]+c[4]])
            if any(b[0]<a[1] for a,b in zip(ordered,ordered[1:])):raise ValueError('chronological coverage')
    selected=dict(rays,rays=[rays['rays'][i] for i in indices])
    accuracy=reference_metrics([raw[i] for i in indices],selected,refs)
    if not accuracy['both_resolutions_pass']:raise ValueError('reference accuracy')
    contemporary=dict(selected,rays=[list(row) for row in selected['rays']])
    for row,i in zip(contemporary['rays'],indices):row[7:11]=[baseline[i][5],*baseline[i][2:5]]
    contemporary_accuracy=reference_metrics([raw[i] for i in indices],contemporary,refs)
    if not contemporary_accuracy['both_resolutions_pass']:raise ValueError('contemporary reference accuracy')
    for c in report['streamed_schedule']['cases']:
        cap=c['capacity'];tiles=(n+cap-1)//cap
        if (c['tiles'],c['tail_rays'])!=(tiles,n-(tiles-1)*cap):raise ValueError('chunk coverage')
        if len(c['normal_rows'])!=n or len(c['forced_rows'])!=n or c['forced_fallback_rays']!=n:raise ValueError('fallback/query coverage')
        for i,(a,b) in enumerate(zip(c['normal_rows'],c['forced_rows'])):
            if a[:2]!=raw[i][:2] or b[:2]!=baseline[i][:2]:raise ValueError('stream pixel order')
            if bits(a[2:])!=bits(raw[i][2:6]):raise ValueError('normal stream/bulk mismatch')
            if bits(b[2:])!=bits(baseline[i][2:]):raise ValueError('forced native mismatch')
    if 'guarded_sliver_policy' in report:
        controls=report['guarded_sliver_policy']['policy_native_controls'];expected=[1,1,0,0,0,0,0,0,0]
        if len(controls)!=len(expected) or any(c['case']!=i or c['expected']!=v or c['observed']!=v or c['pass'] is not True for i,(c,v) in enumerate(zip(controls,expected))):raise ValueError('native merge policy control')
    if not report['qualification']['independent_AST_agreement_pass']:raise ValueError('AST gate')
    for key in ['max_delta_RGB','max_delta_T']:
        if not math.isfinite(report['cpu_vs_gpu_arithmetic_deviation'][key]) or report['cpu_vs_gpu_arithmetic_deviation'][key]>=1e-5:raise ValueError('AST threshold')
    # AST transport scalars are native-run assertions, not independently recomputed
    # here: raw AST node values are not exported. Distinguish this from reference replay.
    return {'pass':True,'queries':n,'reference_queries':len(refs),'all_query_native_AST_assertions_recorded':True,
        'raw_AST_nodes_independently_recomputed_offline':False,'accuracy':accuracy,'contemporary_accuracy':contemporary_accuracy,'clocks':clocks(report),
        'normal_fallbacks':sum(bool(c[2]&0x80000000) for c in report['schedule_census_inputs']),
        'singles':sum(len(c[3]) for c in report['schedule_census_inputs']),
        'overlaps':sum(len(c[4]) for c in report['schedule_census_inputs'])}


def affine_endpoints(before,after,rays,bindings):
    # CPU-f32 formula locates the relevant captured GPU endpoint (within2ULP);
    # preservation is judged on that original endpoint's EXACT bits, not by
    # pretending CPU and Metal divisions are bit-identical.
    f=lambda x:struct.unpack('<f',struct.pack('<f',x))[0]
    word=lambda x:struct.unpack('<I',struct.pack('<f',x))[0]
    matched=missing=0;examples=[]
    for i,(a,b) in enumerate(zip(before,after)):
        for member,spec in enumerate(bindings):
            def ends(row):return set(t for p in row[3] if p[2]==member for t in p[:2])|set(t for p in row[4] if p[2]&(1<<member) for t in p[:2])
            aa,bb=ends(a),ends(b);o=[f(f(x)-y) for x,y in zip(rays['origin'],spec['origin'])];d=list(map(f,rays['rays'][i][2:5]));knots=[]
            if d[0]:knots.append(('x0',f(-o[0]/d[0])))
            if d[1]:knots.extend([('y0',f(-o[1]/d[1])),('yknee',f(f(spec['limits'][2]-o[1])/d[1]))])
            for name,t in knots:
                close=[x for x in aa if t>0 and x>0 and abs(word(x)-word(t))<=2]
                if not close:continue
                endpoint=min(close,key=lambda x:abs(word(x)-word(t)));matched+=1
                if endpoint not in bb:
                    missing+=1
                    if len(examples)<8:examples.append({'ray':i,'member':member,'kind':name,'CPU_f32_knot':t,'old_GPU_endpoint':endpoint,
                        'new_crossing_intervals':[p for p in b[3] if p[2]==member and p[0]<endpoint<p[1]]+[p for p in b[4] if p[2]&(1<<member) and p[0]<endpoint<p[1]]})
    return {'matched':matched,'missing':missing,'examples':examples,
        'scope':'Nearest original GPU endpoint within2ULP of CPU-f32 affine formula; exact-bit endpoint retention, not an exact CPU/GPU division claim.'}


def controls(root,report,raw,baseline,rays,refs,indices):
    rejected=[]
    def bad(name,alter):
        r=copy.deepcopy(report);a=copy.deepcopy(raw);b=copy.deepcopy(baseline);alter(r,a,b)
        try:check(root,r,a,b,rays,refs,indices)
        except (ValueError,KeyError,IndexError,TypeError):rejected.append(name)
        else:raise ValueError('corruption accepted: '+name)
    bad('wrong mode order',lambda r,a,b:r['streamed_schedule']['cases'][0]['series'][0]['samples'][0].update(mode=9))
    bad('missing forced output',lambda r,a,b:r['streamed_schedule']['cases'][0]['forced_rows'].pop())
    bad('forced native mismatch',lambda r,a,b:r['streamed_schedule']['cases'][0]['forced_rows'][0].__setitem__(2,99.0))
    bad('nonfinite elapsed',lambda r,a,b:r['streamed_schedule']['cases'][0]['series'][0]['samples'][0].update(total_ms=float('nan')))
    bad('missing query',lambda r,a,b:a.pop())
    bad('captured baseline mismatch',lambda r,a,b:b[0].__setitem__(2,99.0))
    if 'guarded_sliver_policy' in report:bad('native policy control changed',lambda r,a,b:r['guarded_sliver_policy']['policy_native_controls'][2].update(observed=1))
    return rejected


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True)
    p.add_argument('--record',type=Path)
    p.add_argument('--report',type=Path);p.add_argument('--rows',type=Path);p.add_argument('--baseline',type=Path)
    p.add_argument('--rays',type=Path);p.add_argument('--reference',type=Path);p.add_argument('--output',type=Path,required=True)
    p.add_argument('--before-census',type=Path);p.add_argument('--bindings-report',type=Path);p.add_argument('--compare-rows',type=Path)
    p.add_argument('--corruption-controls',action='store_true');a=p.parse_args()
    if a.output.exists():raise SystemExit('refusing output overwrite')
    if a.record:
        record=load_json(a.record);files={}
        for entry in record['archives']:
            path=(a.root/entry['path']).resolve()
            if a.root.resolve() not in path.parents:raise ValueError('archive outside root')
            data=path.read_bytes()
            if hashlib.sha256(data).hexdigest()!=entry['sha256']:raise ValueError('archive hash')
            raw=gzip.decompress(data)
            if hashlib.sha256(raw).hexdigest()!=entry['raw_sha256'] or len(raw)!=entry['raw_bytes']:raise ValueError('raw archive identity')
            files[entry['key']]=path
        results={}
        for name,recipe in record['replays'].items():
            report,rows,baseline,rays,ref=[load_json(files[recipe[k]]) for k in ['report','rows','baseline','rays','reference']]
            result=check(a.root,report,rows,baseline,rays,ref['rows'],ref['ray_indices'])
            result['corruption_refusals']=controls(a.root,report,rows,baseline,rays,ref['rows'],ref['ray_indices'])
            if 'before_census' in recipe:
                before=load_json(files[recipe['before_census']])['schedule_census_inputs']
                binding=load_json(files[recipe['bindings_report']]) if 'bindings_report' in recipe else report
                result['affine_endpoint_retention']=affine_endpoints(before,report['schedule_census_inputs'],rays,binding['guarded_sliver_policy']['curtain_bindings'])
                result['ray_member_unions_changed']=sum(x!=y for a,b in zip(union(before),union(report['schedule_census_inputs'])) for x,y in zip(a,b))
                if name.startswith('guarded') and (result['affine_endpoint_retention']['missing'] or result['ray_member_unions_changed']):raise ValueError('archived guarded structure')
            if 'compare_rows' in recipe:
                compare=load_json(files[recipe['compare_rows']])
                if len(compare)!=len(rows) or any(x[:2]!=y[:2] or bits(x[2:6])!=bits(y[2:6]) for x,y in zip(compare,rows)):raise ValueError('archived strict old output')
                result['strict_old_output_differences']=0
            results[name]=result
        a.output.write_text(json.dumps({'pass':True,'verified_archives':len(files),'replays':results},indent=2)+'\n')
        print('PASS self-contained archive replay',len(files),'archives',a.output);return
    if not all([a.report,a.rows,a.baseline,a.rays,a.reference]):raise ValueError('report/rows/baseline/rays/reference required')
    report,rows,baseline,rays,ref=map(load_json,[a.report,a.rows,a.baseline,a.rays,a.reference])
    result=check(a.root,report,rows,baseline,rays,ref['rows'],ref['ray_indices'])
    if a.before_census:
        before=load_json(a.before_census)['schedule_census_inputs'];after=report['schedule_census_inputs']
        if len(before)!=len(after) or any(x[:2]!=y[:2] for x,y in zip(before,after)):raise ValueError('structural comparison coverage')
        bindings=load_json(a.bindings_report)['guarded_sliver_policy']['curtain_bindings'] if a.bindings_report else report['guarded_sliver_policy']['curtain_bindings']
        result['affine_endpoint_retention']=affine_endpoints(before,after,rays,bindings)
        result['ray_member_unions_changed']=sum(a!=b for x,y in zip(union(before),union(after)) for a,b in zip(x,y))
        if report.get('guarded_sliver_policy') and (result['affine_endpoint_retention']['missing'] or result['ray_member_unions_changed']):raise ValueError('guarded source structure lost')
    if a.compare_rows:
        compare=load_json(a.compare_rows)
        if len(compare)!=len(rows) or any(x[:2]!=y[:2] for x,y in zip(compare,rows)):raise ValueError('strict comparison coverage')
        result['strict_old_output_differences']=sum(bits(x[2:6])!=bits(y[2:6]) for x,y in zip(compare,rows))
        if result['strict_old_output_differences']:raise ValueError('strict old-output equality failed')
    if a.corruption_controls:result['corruption_refusals']=controls(a.root,report,rows,baseline,rays,ref['rows'],ref['ray_indices'])
    result['files']=[{'path':str(path),'sha256':hashlib.sha256(path.read_bytes()).hexdigest()} for path in [a.report,a.rows,a.baseline,a.rays,a.reference,a.before_census,a.bindings_report,a.compare_rows] if path]
    a.output.write_text(json.dumps(result,indent=2)+'\n');print('PASS raw reference/stream/fallback/clock replay',a.output)


if __name__=='__main__':main()
