"""§6cy: census instrumentation corrections and independent diagnostic admission.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-23.
Quarantined evidence only. Work counts do not measure latency or register spills.
"""
import argparse, copy, gzip, hashlib, importlib.util, json, math, statistics
from pathlib import Path

FIELDS = ('boxHits coarseBinsVisited coarseBinsBowRejected subdivisionSplits maxStackTop maxDepthReached '
          'evalPhiCoarseKnots evalPhiSubdivision evalPhiBisection evalPhiPieceMid evalPhiTotal signChangeBrackets '
          'bisectionItersTotal uncertainBandsRetained candidateIntervalsChecked piecesEmitted mergedKnots '
          'singleSubpanels overlapIntervals singleEvalEnvelopeCalls singleCnoise3Calls overlapEvalEnvelopeCalls '
          'overlapCnoise3Calls expAttenSteps').split()
CURRENT = 'marchLoopSteps zeroProofSkippedSteps currentDensityEvals currentCnoise3Calls'.split()


def load(root, name):
    spec=importlib.util.spec_from_file_location(name,root/'scratch/probes'/(name+'.py'))
    m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m);return m


def correct_source(s):
    def once(a,b):
        nonlocal s
        if s.count(a)!=1:raise ValueError(('census source hook identity',s.count(a),a[:70]))
        s=s.replace(a,b)
    once('if (min(va, vb) > bow || max(va, vb) < -bow) { g_coarseBinsBowRejected++; continue; }',
         'if (min(va, vb) > bow || max(va, vb) < -bow) { if (depth == 0u) { g_coarseBinsBowRejected++; } continue; }')
    # These hooks occur only in the instrumented shader.
    once('g_bisectionItersTotal++;\n                        g_evalPhiBisection++;', 'g_evalPhiBisection++;')
    census_start=s.index('static const char* kFusedCensusShader')
    end=s.index(')WGSL";',census_start)
    shader=s[census_start:end]
    shader=shader.replace('top--; let cell = stack[top];',
        'g_maxStackTop = max(g_maxStackTop, top); top--; let cell = stack[top];')
    shader=shader.replace('let depth = getDepth(depthBits, top);',
        'let depth = getDepth(depthBits, top); g_maxDepthReached = max(g_maxDepthReached, depth);')
    shader=shader.replace('for (var iter = 0u; iter < 32u; iter++) {',
        'for (var iter = 0u; iter < 32u; iter++) { g_bisectionItersTotal++;')
    s=s[:census_start]+shader+s[end:]
    # The counter counts skipped accepted-lattice indices, not overshoot beyond
    # the end. Renderer step arithmetic itself stays unchanged.
    once('zeroProofSkippedSteps += u32(max(0, nextStep - step));',
         'zeroProofSkippedSteps += u32(max(0, min(nextStep, segmentSamples - 1) - step));')
    once('if (p != std::string::npos) currentCensusShader.insert(p + hook.size(), "\\n    g_currentDensityEvals++;\\n");',
         'if (p == std::string::npos) throw std::runtime_error("density census hook missing"); currentCensusShader.insert(p + hook.size(), "\\n    g_currentDensityEvals++;\\n");')
    once('forcedRows = correctness(true);\n        unsigned forcedCount = count;', '''forcedRows = correctness(true);
        auto forcedHeaderBytes = readBackBuffer(rBuf, rayBytes);
        const auto* forcedHeaders = reinterpret_cast<const GpuRayHeader*>(forcedHeaderBytes.data());
        unsigned forcedCount = 0;
        for(size_t i=0;i<count;++i) if(forcedHeaders[i].overlapCount & 0x80000000u) ++forcedCount;''')
    once('for(int k=0;k<2;++k){execute(0,1,false);execute(1,1,false);execute(2,1,false);}',
         '// §6cy diagnostics only: no repeated stream timing campaign.')
    once('for(int repeats:{1,2,4}){', 'for(int repeats:std::vector<int>{}){')
    once('{"normal_rows",normalRows},{"forced_rows",forcedRows}',
         '{"normal_rows",normalRows},{"normal_headers",normalHeaderRows},{"forced_rows",forcedRows}')
    once('finalHeaders.assign(normalHeaders, normalHeaders + count);', '''finalHeaders.assign(normalHeaders, normalHeaders + count);
        J normalHeaderRows=J::array();
        for(size_t i=0;i<count;++i)normalHeaderRows.push_back({R[i][0],R[i][1],normalHeaders[i].subpanelCount,normalHeaders[i].overlapCount});''')
    once('    // --- Diagnostic Packet Trace, Synthetic Controls, and Work Census ---', '''
    auto resultRows=[&](const std::vector<GpuResult>& data){J a=J::array();for(size_t i=0;i<count;++i)a.push_back({R[i][0],R[i][1],data[i].rgb_T[0],data[i].rgb_T[1],data[i].rgb_T[2],data[i].rgb_T[3]});return a;};
    // --- Diagnostic Packet Trace, Synthetic Controls, and Work Census ---''')
    # Remove duplicate allocations leaking their first handles.
    once('bgptEntries[6].binding = 6; diagSubpanelsBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, diagSubSingleBytes, nullptr);',
         'bgptEntries[6].binding = 6;')
    once('bgptEntries[7].binding = 7; diagOverlapsBuf = makeBuffer(WGPUBufferUsage_Storage | WGPUBufferUsage_CopySrc, diagOverBytes, nullptr);',
         'bgptEntries[7].binding = 7;')
    once('auto traceResults = readResults();', 'auto traceResults = readResults(); report["trace_output_rows"]=resultRows(traceResults);')
    once('auto subBytes = readBackBuffer(diagSubpanelsBuf, diagSubSingleBytes);', '''auto traceHeaderBytes=readBackBuffer(rBuf,rayBytes);
    const auto* traceHeaders=reinterpret_cast<const GpuRayHeader*>(traceHeaderBytes.data());
    for(size_t i=0;i<count;++i)if(traceHeaders[i].subpanelCount!=finalHeaders[i].subpanelCount || traceHeaders[i].overlapCount!=finalHeaders[i].overlapCount)throw std::runtime_error("trace/normal header mismatch");
    auto subBytes = readBackBuffer(diagSubpanelsBuf, diagSubSingleBytes);''')
    once('const auto& hdr = finalHeaders[i];','const auto& hdr = traceHeaders[i];')
    once('budgetControlResults.push_back({{"budget", b}, {"fallbacks", fallbacks}, {"expected", expected}, {"pass", fallbacks == expected}});', '''
        J flags=J::array();for(uint32_t i=0;i<16u;++i)flags.push_back(hdrs[i].overlapCount);
        if(fallbacks!=expected)throw std::runtime_error("synthetic exact budget control failed");
        budgetControlResults.push_back({{"budget", b}, {"fallbacks", fallbacks}, {"expected", expected}, {"flags",flags}, {"pass", true}});''')
    # A stale previous image cannot make either capacity witness green.
    needle='        WGPUCommandEncoder enc = wgpuDeviceCreateCommandEncoder(gpu.device, nullptr);\n        auto pass = wgpuCommandEncoderBeginComputePass(enc, nullptr);\n        wgpuComputePassEncoderSetPipeline(pass, cpFusedNormal);'
    if s.count(needle)!=3:raise ValueError('budget/capacity dispatch hook count')
    s=s.replace(needle,'''        std::vector<GpuResult> poison(executionRays);for(auto& p:poison)for(float& v:p.rgb_T)v=NAN;
        wgpuQueueWriteBuffer(gpu.queue,outBuf,0,poison.data(),outBytes);
'''+needle)
    for cap,bit in [('sub',16),('over',32)]:
        count='subFallbacks' if cap=='sub' else 'overFallbacks'
        anchor='        capacityControlResults.push_back({\n            {"control", "'+('maxSubPerRay=4' if cap=='sub' else 'maxOverPerRay=1')+'"},'
        code='''        J flags=J::array();
        for(size_t i=0;i<count;++i){
            const bool refused=(hdrs[i].overlapCount&0x80000000u)!=0;
            if(refused && (hdrs[i].overlapCount&0x7fffffffu)!=REASONu)throw std::runtime_error("wrong capacity fallback reason");
            const auto& expected=refused?resCurrent[i]:resUnfused[i];
            for(int k=0;k<4;++k)if(!std::isfinite(res[i].rgb_T[k]) || std::bit_cast<uint32_t>(res[i].rgb_T[k])!=std::bit_cast<uint32_t>(expected.rgb_T[k]))throw std::runtime_error("capacity mixed output mismatch");
            flags.push_back(hdrs[i].overlapCount);
        }
        if(!fallbackParityPass || FALLBACKS==0)throw std::runtime_error("capacity witness failed or vacuous");
'''.replace('REASON',str(bit)).replace('FALLBACKS',count)
        once(anchor,code+anchor)
        once('{"fallbacks", '+count+'},\n            {"fallback_bit_exact_pass", fallbackParityPass}',
             '{"fallbacks", '+count+'},{"flags",flags},{"output_rows",resultRows(res)},\n            {"fallback_bit_exact_pass", fallbackParityPass}')
    once('auto censusResults = readResults();','auto censusResults = readResults(); report["fused_census_output_rows"]=resultRows(censusResults);')
    once('auto curResults = readResults();','auto curResults = readResults(); report["current_census_output_rows"]=resultRows(curResults);')
    fused_export='    J fusedRaw=J::array();for(size_t i=0;i<count;++i){const auto& r=fRecs[i];fusedRaw.push_back({R[i][0],R[i][1],'+','.join('r.'+x for x in FIELDS)+'});}report["work_census_fused_rows"]=fusedRaw;\n'
    once('    // Aggregate Fused Census Records',fused_export+'    // Aggregate Fused Census Records')
    once('    std::vector<uint32_t> vecMarchSteps',
         '    J currentRaw=J::array();for(size_t i=0;i<count;++i){const auto& r=cRecs[i];currentRaw.push_back({R[i][0],R[i][1],'+','.join('r.'+x for x in CURRENT)+'});}report["work_census_current_rows"]=currentRaw;\n    std::vector<uint32_t> vecMarchSteps')
    once('    std::cout << report.dump(2) << std::endl;', '''
    report["census_review"]={{"version","6cy"},{"stream_timing_series_disabled",true},{"counter_scope","per ray; no per-member records"},{"hardware_cost_inferred",false}};
    report["fused_normal_path"]["thread_private_subpanels_capacity"]=64;
    report["fused_normal_path"]["thread_private_overlaps_capacity"]=16;
    report["fused_normal_path"]["global_schedule_storage_eliminated"]=false;
    std::cout << report.dump(2) << std::endl;''')
    return s


def stats(values):
    a=sorted(values);n=len(a)
    return dict(total=sum(a),mean=sum(a)/n,min=a[0],max=a[-1],p50=a[n//2],p90=a[int(n*.9)],p99=a[int(n*.99)])


def validate_report(root, report, rows, baseline, rays, reference):
    p=load(root,'ray_transport_sliver_review');load(root,'ray_transport_gpu_fused_review').admission(root,report,rows,rays,reference)
    count=len(rows)
    reference_rows,indices=reference
    portable=p.check(root,report,rows,baseline,rays,reference_rows,indices)
    for c in report['synthetic_budget_controls']:
        expected=16 if c['budget']==23 else 0
        if c['budget'] not in [23,24,25] or c['fallbacks']!=expected or c['expected']!=expected or not c['pass']:
            raise ValueError('budget control failure')
        if 'flags' in c and (len(c['flags'])!=16 or any(x!=(0x80000001 if expected else 0) for x in c['flags'])):
            raise ValueError('budget flags')
    if [c['budget'] for c in report['synthetic_budget_controls']]!=[23,24,25]:raise ValueError('budget coverage')
    caps=report['synthetic_capacity_controls']
    if [x['control'] for x in caps]!=['maxSubPerRay=4','maxOverPerRay=1']:raise ValueError('capacity coverage')
    for c,reason in zip(caps,[16,32]):
        if c['fallbacks']<=0 or not c['fallback_bit_exact_pass']:raise ValueError('capacity failure/vacuity')
        if 'flags' in c:
            if len(c['flags'])!=count or len(c['output_rows'])!=count:raise ValueError('capacity raw coverage')
            if sum(bool(x&0x80000000) for x in c['flags'])!=c['fallbacks']:raise ValueError('capacity fallback count')
            for flag,a,b,current in zip(c['flags'],c['output_rows'],rows,baseline):
                if flag&0x80000000 and flag&0x7fffffff!=reason:raise ValueError('capacity reason')
                if a[:2]!=b[:2] or p.bits(a[2:6])!=p.bits((current if flag&0x80000000 else b)[2:6]):raise ValueError('capacity actual outputs')
    result={'queries':count,'portable_output_reference_check':portable,'raw_census_available':'work_census_fused_rows' in report,'fused_packets_equal':True,
            'instrumentation_cost_is_not_latency':True}
    if not result['raw_census_available']:
        result['limits']='Historical controls have aggregate flags/parity only; no raw per-ray counters. Width does not establish root provenance.'
        return result
    for field,expected in [('trace_output_rows',rows),('fused_census_output_rows',rows),('current_census_output_rows',baseline)]:
        actual=report[field]
        if len(actual)!=count or any(a[:2]!=b[:2] or p.bits(a[2:6])!=p.bits(b[2:6]) for a,b in zip(actual,expected)):raise ValueError('instrumented output parity')
    for kind,fields in [('fused',FIELDS),('current',CURRENT)]:
        raw=report['work_census_'+kind+'_rows'];saved=report['work_census_'+kind]
        if len(raw)!=count:raise ValueError('raw census coverage')
        for i,row in enumerate(raw):
            if row[:2]!=rows[i][:2] or len(row)!=len(fields)+2 or any(not isinstance(x,int) or x<0 for x in row[2:]):raise ValueError('raw counter schema')
        for k,name in enumerate(fields):
            computed=stats([x[2+k] for x in raw])
            if any(not math.isclose(v,saved[name][s],rel_tol=1e-12,abs_tol=1e-12) for s,v in computed.items()):raise ValueError('counter statistic drift: '+name)
    for raw,packet in zip(report['work_census_fused_rows'],report['fused_packet_census_inputs']):
        d=dict(zip(FIELDS,raw[2:]));singles=len(packet[3]);overs=len(packet[4])
        if d['coarseBinsBowRejected']>d['coarseBinsVisited'] or d['coarseBinsVisited']!=24*d['boxHits']:raise ValueError('coarse counter semantics')
        if d['maxStackTop']<bool(d['boxHits']) or d['maxStackTop']>20 or d['maxDepthReached']>14:raise ValueError('stack semantics')
        if d['evalPhiTotal']!=sum(d[k] for k in ['evalPhiCoarseKnots','evalPhiSubdivision','evalPhiBisection','evalPhiPieceMid']):raise ValueError('Phi accounting')
        if d['bisectionItersTotal']<d['evalPhiBisection']:raise ValueError('bisection work accounting')
        if (d['singleSubpanels'],d['overlapIntervals'],d['singleEvalEnvelopeCalls'],d['expAttenSteps'])!=(singles,overs,2*singles,singles+4*overs):raise ValueError('consumer packet accounting')
        if d['overlapEvalEnvelopeCalls']!=sum(4*int(x[2]).bit_count() for x in packet[4]):raise ValueError('overlap evaluation accounting')
        if d['singleCnoise3Calls']>d['singleEvalEnvelopeCalls'] or d['overlapCnoise3Calls']>d['overlapEvalEnvelopeCalls']:raise ValueError('noise accounting')
    for c in report['streamed_schedule']['cases']:
        if len(c['normal_headers'])!=count:raise ValueError('normal header coverage')
        for h,packet in zip(c['normal_headers'],report['fused_packet_census_inputs']):
            if h!=[packet[0],packet[1],len(packet[3]),len(packet[4])]:raise ValueError('actual normal header counts')
    result['fused_counters']=report['work_census_fused'];result['current_counters']=report['work_census_current']
    return result


def controls(root,report,rows,baseline,rays,reference):
    rejected=[]
    def bad(label,change):
        r=copy.deepcopy(report);change(r)
        try:validate_report(root,r,rows,baseline,rays,reference)
        except (ValueError,KeyError,IndexError,TypeError):rejected.append(label)
        else:raise ValueError('corruption accepted: '+label)
    bad('failed budget boolean',lambda r:r['synthetic_budget_controls'][1].update(**{'pass':False}))
    bad('failed capacity parity',lambda r:r['synthetic_capacity_controls'][0].update(fallback_bit_exact_pass=False))
    bad('vacuous capacity control',lambda r:r['synthetic_capacity_controls'][0].update(fallbacks=0))
    if 'work_census_fused_rows' in report:
        bad('counter row omitted',lambda r:r['work_census_fused_rows'].pop())
        bad('counter statistics corrupted',lambda r:r['work_census_fused']['coarseBinsBowRejected'].update(total=9999999))
        bad('unwritten capacity output',lambda r:r['synthetic_capacity_controls'][0]['output_rows'][0].__setitem__(2,99.0))
        bad('normal header mismatch',lambda r:r['streamed_schedule']['cases'][0]['normal_headers'][0].__setitem__(2,99))
        bad('instrumented output changed',lambda r:r['fused_census_output_rows'][0].__setitem__(2,99.0))
    return rejected


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True);p.add_argument('--record',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if a.output.exists():raise SystemExit('refusing output overwrite')
    record=json.loads(a.record.read_text());bundle=load(a.root,'ray_transport_root_review').Bundle(a.root/record['bundle'],record['bundle_sha256'])
    for k in bundle.index:bundle.read(k)
    for item in record['guarded_archives']:
        if hashlib.sha256((a.root/item['path']).read_bytes()).hexdigest()!=item['sha256']:raise ValueError('guarded archive hash')
    rays=bundle.read('rays');ref=bundle.read('reference');refs=(ref['rows'],ref['ray_indices']);results={}
    for stage in ['historical','corrected']:
        report=bundle.read(stage+'_report');rows=bundle.read(stage+'_rows');base=bundle.read(stage+'_baseline')
        cpp=bundle.read(stage+'_source');identity=bundle.read(stage+'_identity')
        if hashlib.sha256(cpp).hexdigest()!=identity['cpp_sha256']:raise ValueError('compiled source hash')
        shader=cpp.decode().split('static const char* kFusedNormalShader = R"WGSL(',1)[1].split(')WGSL";',1)[0]
        load(a.root,'ray_transport_gpu_fused_review').require_contract(shader)
        result=validate_report(a.root,report,rows,base,rays,refs)
        result['corruption_refusals']=controls(a.root,report,rows,base,rays,refs)
        if stage=='corrected':
            if any(c['series'] for c in report['streamed_schedule']['cases']):raise ValueError('unexpected corrected stream timing campaign')
            hist={}
            for kind,fields in [('fused',FIELDS),('current',CURRENT)]:
                raw=report['work_census_'+kind+'_rows'];hist[kind]={}
                for j,name in enumerate(fields):
                    h={}
                    for row in raw:h[str(row[j+2])]=h.get(str(row[j+2]),0)+1
                    hist[kind][name]=h
            result['per_ray_histograms']=hist
            result['affine']=load(a.root,'ray_transport_sliver_review').affine_endpoints(report['schedule_census_inputs'],report['fused_packet_census_inputs'],rays,report['guarded_sliver_policy']['curtain_bindings'])
            widths=[b-a for row in report['fused_packet_census_inputs'] for a,b,m in row[3]]
            result['width_counts_only_not_root_provenance']=[sum(h<=.02 for h in widths),sum(.02<h<=.05 for h in widths),sum(.05<h<=.2 for h in widths),sum(h>.2 for h in widths)]
        results[stage]=result
    phelper=load(a.root,'ray_transport_sliver_review')
    before=bundle.read('historical_rows');after=bundle.read('corrected_rows')
    if len(before)!=len(after) or any(x[:2]!=y[:2] or phelper.bits(x[2:6])!=phelper.bits(y[2:6]) for x,y in zip(before,after)):raise ValueError('correction changed output')
    failed=copy.deepcopy(bundle.read('historical_report'));failed['synthetic_capacity_controls'][0]['fallback_bit_exact_pass']=False
    if not load(a.root,'ray_transport_gpu_fused_review').admission(a.root,failed,before,rays,refs)['both_resolutions_pass']:raise ValueError('old ignored-control witness')
    results['old_admission_accepts_failed_control']=True;results['pass']=True
    results['limits']='Historical counters have aggregate-only evidence; corrected counters are per-ray, not per-member. Recorded native AST assertions, not independent offline AST reevaluation.192 dense holdout rays at two resolutions. No new stream timing or FPS gain; no measured spill/occupancy cause.'
    a.output.write_text(json.dumps(results,indent=2)+'\n');print('PASS census review replay; historical3/corrected8 control refusals; raw counters and output/reference parity')


if __name__=='__main__':main()
