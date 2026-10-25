"""§6cw: independently replay fused outputs; refuse unsupported fusion admission.
Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a / 2026-10-22.
Standard library only. Historical output equality is not packet/capacity admission.
"""
import argparse, gzip, hashlib, importlib.util, json, re
from pathlib import Path


def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root/'scratch/probes'/(name+'.py'))
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module); return module


def capacities(shader):
    return tuple(int(re.search(r'var local_'+name+r': array<\w+, (\d+)>', shader)[1])
                 for name in ['subpanels', 'overlaps'])


def require_contract(shader):
    # §6cv was assigned unchanged 64/16 capacities. Smaller private arrays are
    # safe from OOB because guarded, but introduce additional fallback reasons.
    if capacities(shader) != (64, 16):
        raise ValueError('fusion admission refused: private capacities differ from guarded 64/16 contract; '
                         'historical 32/8 result remains a rejected diagnostic')


def require_packet_evidence(report):
    # schedule_census_inputs belongs to cpProducer, NOT cpFusedNormal. Never
    # silently accept it as evidence about invocation-local fused packets.
    if 'fused_packet_census_inputs' not in report:
        raise ValueError('fusion admission refused: actual fused packet trace is absent')


def admission(root, report, rows, rays, reference):
    require_packet_evidence(report)
    portable = load(root, 'ray_transport_sliver_review')
    historical = json.loads(gzip.decompress((root/'docs/audits/GPU_RAY_TRANSPORT_SLIVER_REVIEW_2026-10-22.guarded_confirm_report.gz').read_bytes()))
    trace = report['fused_packet_census_inputs']; original = historical['schedule_census_inputs']
    packets_equal = load(root, 'ray_transport_root_review').packets_equal
    if len(trace) != len(original) or any(not packets_equal(a, b, portable) for a, b in zip(trace, original)):
        raise ValueError('fusion admission refused: fused packet endpoints/counts/masks differ')
    # This also checks actual mode2 normal/forced output equality, not only the
    # legacy device_fused_unfused_parity_pass field.
    return load(root, 'ray_transport_gpu_wide_stream').independent_check(report, rows, rays, reference)


def replay(root, get, record):
    portable = load(root, 'ray_transport_sliver_review')
    r, rows, baseline, rays, ref = [get(k) for k in ['report','rows','baseline','rays','reference']]
    check = portable.check(root, r, rows, baseline, rays, ref['rows'], ref['ray_indices'])
    source = get('compiled_source')
    if hashlib.sha256(source).hexdigest() != get('source_identity')['cpp_sha256']:
        raise ValueError('compiled source identity')
    source = source.decode()
    shader = source.split('static const char* kFusedNormalShader = R"WGSL(', 1)[1].split(')WGSL";', 1)[0]
    if capacities(shader) != (32, 8): raise ValueError('historical capacity identity')
    # Frozen guarded endpoints here qualify only the UNFUSED control.
    old_rows = json.loads(gzip.decompress((root/record['guarded_rows']).read_bytes()))
    old_report = json.loads(gzip.decompress((root/record['guarded_report']).read_bytes()))
    if any(portable.bits(a[2:6]) != portable.bits(b[2:6]) for a,b in zip(rows, old_rows)):
        raise ValueError('guarded output parity')
    if r['schedule_census_inputs'] != old_report['schedule_census_inputs']:
        raise ValueError('unfused control packet parity')
    actual = []
    for c in r['streamed_schedule']['cases']:
        selected = dict(rays, rays=[rays['rays'][i] for i in ref['ray_indices']])
        metrics = portable.reference_metrics([c['normal_rows'][i] for i in ref['ray_indices']], selected, ref['rows'])
        if not metrics['both_resolutions_pass']: raise ValueError('actual fused reference gate')
        actual.append({'capacity':c['capacity'], 'queries':len(c['normal_rows']),
                       'forced_queries':len(c['forced_rows']), 'reference':metrics})
    refused = portable.controls(root,r,rows,baseline,rays,ref['rows'],ref['ray_indices'])
    for label,call in [('changed private capacities refused',lambda:require_contract(shader)),
                       ('missing actual fused packets refused',lambda:require_packet_evidence(r))]:
        try: call()
        except ValueError: refused.append(label)
        else: raise ValueError('invalid admission accepted: '+label)
    require_contract(shader.replace('array<Subpanel, 32>', 'array<Subpanel, 64>').replace('array<Overlap, 8>', 'array<Overlap, 16>'))
    census = r['schedule_census_inputs']; count = len(census)
    singles = sum(len(x[3]) for x in census); overlaps = sum(len(x[4]) for x in census)
    return {'pass':True, 'historical_fusion_admitted':False, 'actual_fused_outputs':actual,
            'unfused_control':check, 'historical_private_capacities':[32,8], 'assigned_capacities':[64,16],
            'unfused_max_single_packets':max(len(x[3]) for x in census),
            'unfused_max_overlap_packets':max(len(x[4]) for x in census),
            'storage_accounting':{'single_packets':singles, 'overlap_packets':overlaps,
                'logical_populated_packet_bytes':(singles+overlaps)*16,
                'one_write_plus_one_read_logical_bytes':(singles+overlaps)*32,
                'padded_schedule_packet_bytes':count*(64+16)*16,
                'retained_ray_header_bytes':count*48,
                'schedule_and_header_capacity_bytes':count*((64+16)*16+48),
                'device_memory_traffic_measured':False,
                'global_control_buffers_still_allocated':True},
            'corruption_and_admission_refusals':refused,
            'limits':'Actual mode2 output/fallback parity and sparse-reference gates replay. Counts/packets/AST fields '
                     'come from unfused control. No actual fused normal-header/packet readback or generic capacity '
                     'failure witness. No measured spill/occupancy cause. Historical timings remain rejected; no new native run.'}


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--root',type=Path,required=True)
    p.add_argument('--record',type=Path,required=True);p.add_argument('--output',type=Path,required=True);a=p.parse_args()
    if a.output.exists(): raise SystemExit('refusing output overwrite')
    record=json.loads(a.record.read_text());bundle=load(a.root,'ray_transport_root_review').Bundle(a.root/record['bundle'],record['bundle_sha256'])
    for name in bundle.index: bundle.read(name)
    for name in ['guarded_report','guarded_rows']:
        if hashlib.sha256((a.root/record[name]).read_bytes()).hexdigest()!=record[name+'_sha256']:
            raise ValueError('guarded source identity')
    result=replay(a.root,bundle.read,record)
    a.output.write_text(json.dumps(result,indent=2)+'\n')
    print('PASS fused evidence replay; historical candidate refused;',len(result['corruption_and_admission_refusals']),'refusals')


if __name__=='__main__':main()
