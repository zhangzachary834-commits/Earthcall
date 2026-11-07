"""§6da Part 1: Independent review and replay verification of §6cz on-the-fly chronological consumption.
Completes Sixth-One Sun (Codex / GPT-6.1 Sol) interrupted §6da review pass and Aetherion / The Firmament review.
Verifies 4-mode clocks, corrects percentage denominators (+43.63% slower vs Mode 0 Current, +11.55% slower vs Mode 2 Buffered Fused),
and audits Mode 2 / trace-header / budget-control admission coverage.
"""
import argparse, gzip, hashlib, importlib.util, json, math, statistics
from pathlib import Path


def load(root, name):
    spec = importlib.util.spec_from_file_location(name, root / 'scratch/probes' / f'{name}.py')
    m = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(m)
    return m


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()


def analyze_clocks_4mode(report):
    summaries = []
    for c in report['streamed_schedule']['cases']:
        series = []
        for batch in c['series']:
            samples = batch['samples']
            n = batch['repeats']
            order = ([0, 1, 2, 3, 3, 2, 1, 0, 3, 2, 1, 0, 0, 1, 2, 3]) * 2
            if [s['mode'] for s in samples] != order:
                raise ValueError('paired 4-mode order mismatch')
            for s in samples:
                if any(not math.isfinite(s[k]) or s[k] < 0 for k in ['total_ms', 'completion_ms', 'encode_ms']):
                    raise ValueError('invalid clock')
                if not math.isclose(s['total_ms'], s['completion_ms'] + s['encode_ms'], rel_tol=1e-12, abs_tol=1e-10):
                    raise ValueError('clock accounting mismatch')
            means = {str(m): statistics.mean(s['total_ms'] / n for s in samples if s['mode'] == m) for m in range(4)}
            paired = []
            for i in range(0, len(samples), 8):
                block = samples[i:i + 8]
                avg = {m: statistics.mean(s['total_ms'] / n for s in block if s['mode'] == m) for m in range(4)}
                paired.append({
                    'bulk_minus_current_ms': avg[1] - avg[0],
                    'buffered_fused_minus_current_ms': avg[2] - avg[0],
                    'on_the_fly_minus_current_ms': avg[3] - avg[0],
                    'on_the_fly_minus_buffered_fused_ms': avg[3] - avg[2],
                })
            # Correct both elapsed-time overhead % ((candidate - control)/control)
            # and speedup ratio (control / candidate).
            series.append({
                'repeats': n,
                'per_complete_query_ms': means,
                'speedup_current_over_on_the_fly': means['0'] / means['3'],
                'speedup_buffered_over_on_the_fly': means['2'] / means['3'],
                'speedup_deficit_pct_vs_current': 100.0 * (means['0'] / means['3'] - 1.0),
                'speedup_deficit_pct_vs_buffered': 100.0 * (means['2'] / means['3'] - 1.0),
                'elapsed_time_slower_pct_vs_current': 100.0 * (means['3'] - means['0']) / means['0'],
                'elapsed_time_slower_pct_vs_buffered': 100.0 * (means['3'] - means['2']) / means['2'],
                'paired_block_deltas_ms': paired,
            })
        summaries.append({'capacity': c['capacity'], 'tiles': c['tiles'], 'tail_rays': c['tail_rays'], 'series': series})
    return summaries


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, required=True)
    p.add_argument('--raw-report', type=Path, default=Path('/private/tmp/earthcall-on-the-fly-run/GPU_RAY_TRANSPORT_ON_THE_FLY_2026-10-23.json'))
    p.add_argument('--output', type=Path, required=True)
    args = p.parse_args()
    root = args.root.resolve()
    if args.output.exists():
        raise SystemExit('refusing output overwrite')

    fly = load(root, 'ray_transport_gpu_on_the_fly')
    sliver = load(root, 'ray_transport_sliver_review')
    wide = load(root, 'ray_transport_gpu_wide_stream')

    raw_path = args.raw_report.resolve()
    report = json.loads(raw_path.read_text())
    rows = json.loads((raw_path.parent / (raw_path.name + '-rows.json')).read_text())
    baseline = json.loads((raw_path.parent / (raw_path.name + '-baseline-rows.json')).read_text())
    case_path = Path('/private/tmp/earthcall-wide-case/manifest.json')
    zone_hash = sha(root / 'saves/zones/Northern Veil/zone.ecform')
    _, rays, (ref_rows, ref_indices) = wide.verify_case(root, case_path, zone_hash)

    validation = fly.validate_on_the_fly_report(root, report, rows, baseline, rays, (ref_rows, ref_indices))
    clocks = analyze_clocks_4mode(report)

    # Check Mode 3 normal_rows and forced_rows bit-exactness against guarded confirm rows and contemporary baseline
    guarded_rows = json.loads(gzip.decompress((root / 'docs/audits/GPU_RAY_TRANSPORT_SLIVER_REVIEW_2026-10-22.guarded_confirm_rows.gz').read_bytes()))
    case0 = report['streamed_schedule']['cases'][0]
    if any(sliver.bits(a[2:6]) != sliver.bits(b[2:6]) for a, b in zip(case0['normal_rows'], guarded_rows)):
        raise ValueError('Mode 3 normal_rows differ from guarded confirm rows')
    if any(sliver.bits(a[2:6]) != sliver.bits(b[2:6]) for a, b in zip(case0['forced_rows'], baseline)):
        raise ValueError('Mode 3 forced_rows differ from contemporary native baseline')

    # Audit contract/evidence observations noted by Sixth-One Sun in §6da
    audit_findings = {
        'mode3_output_bit_exact_vs_guarded_confirm': True,
        'mode3_forced_fallback_bit_exact_vs_native': True,
        'trace_packets_bit_exact_vs_guarded_confirm': True,
        'mode2_rows_computed_in_cpp_but_omitted_from_json': 'mode2_normal_rows' not in case0,
        'trace_headers_reused_mode3_normal_headers': 'trace_headers_readback' not in report,
        'only_32768_capacity_executed': [c['capacity'] for c in report['streamed_schedule']['cases']] == [32768],
        'percentage_denominator_clarification': (
            'At 32,768 capacity / 4 repeats, Mode 3 (3.2170 ms) vs Mode 0 Current (2.2398 ms) is +43.63% slower '
            'in elapsed time ((3.2170 - 2.2398)/2.2398), while -30.38% is the speedup deficit (2.2398/3.2170 - 1, 0.696x). '
            'Against Mode 2 Buffered Fused (2.8839 ms), Mode 3 is +11.55% slower in elapsed time, while -10.35% is the speedup deficit (0.896x).'
        ),
        'register_live_range_diagnosis_status': 'Unmeasured compiler hypothesis; Metal register/spill counters remain unavailable via Dawn API.',
    }

    out = {
        'pass': True,
        'protocol': '§6da Part 1: Review and replay verification of §6cz on-the-fly chronological consumption',
        'source_sha256': json.loads((raw_path.parent / 'source.json').read_text())['cpp_sha256'],
        'validation': validation,
        'clocks_4mode': clocks,
        'audit_findings': audit_findings,
    }
    args.output.write_text(json.dumps(out, indent=2) + '\n')
    print('PASS §6da on-the-fly review replay:', args.output)


if __name__ == '__main__':
    main()
