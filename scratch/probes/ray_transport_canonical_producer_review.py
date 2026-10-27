"""Review §6cf using the candidate's exact C++ root lambda and bounded CPU witnesses.

Codex / GPT-6.1 Sol / session 01a148a7-bd85-7550-a416-0e859a729c1a.
This diagnoses known failures; success means they reproduced, not that the producer passed.
No native GPU, production application build, save mutation, or performance run.
"""
import argparse
import ast
import hashlib
import importlib.util
import json
import math
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    args = parser.parse_args()
    root, work = args.root.resolve(), args.work.resolve()
    if root == work or root in work.parents:
        raise SystemExit('Work must be a fresh directory outside the checkout')
    work.mkdir(parents=True, exist_ok=False)
    candidate = root / 'scratch/probes/ray_transport_canonical_producer_metal.py'
    source = candidate.read_text()
    tree = ast.parse(source)
    function = next(n for n in tree.body if isinstance(n, ast.FunctionDef)
                    and n.name == 'build_canonical_probe_source')
    producer = next(ast.literal_eval(n.value) for n in function.body
                    if isinstance(n, ast.Assign) and any(
                        isinstance(t, ast.Name) and t.id == 'producer_cpp'
                        for t in n.targets))
    actual_lambda = producer[producer.index('    auto isolateRoots'):
                             producer.index('    auto tProducer0')]
    cpp = ('#include <vector>\n#include <functional>\n#include <cmath>\n'
           '#include <iostream>\n#include <iomanip>\nint main(){\n'
           + actual_lambda + r'''
    std::cout << std::setprecision(17);
    auto emit=[&](const char* label,auto f,double lo,double hi,double M) {
        std::vector<double> roots;
        isolateRoots(f,lo,hi,0.,M,roots);
        std::cout << label;
        for(auto r:roots)std::cout << " " << r;
        std::cout << "\n";
    };
    emit("three_root_cubic",[](double s){return (s-.02)*(s-.05)*(s-.08);},0.,1.,6.);
    emit("narrow_root_pair",[](double s){return (s-.0007)*(s-.0007)-1e-10;},0.,1.,2.);
}
''')
    (work / 'exact_root_lambda.cpp').write_text(cpp)
    subprocess.run(['c++', '-std=c++20', '-O2', str(work / 'exact_root_lambda.cpp'),
                    '-o', str(work / 'root_witness')], check=True, capture_output=True, text=True)
    out = subprocess.check_output([str(work / 'root_witness')], text=True)
    values = {line.split()[0]: list(map(float, line.split()[1:]))
              for line in out.splitlines()}
    assert len(values['three_root_cubic']) == 1
    assert all(abs(r - .02) > 1e-5 and abs(r - .05) > 1e-5
               for r in values['three_root_cubic'])
    assert len(values['narrow_root_pair']) == 1
    midpoint = values['narrow_root_pair'][0]
    assert abs(midpoint - .00069) > 1e-5 and abs(midpoint - .00071) > 1e-5

    test_path = root / 'scratch/probes/ray_transport_canonical_producer_test.py'
    spec = importlib.util.spec_from_file_location('candidate_6cf_test', test_path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    medium = dict(module.MEDIA_PARAMS[0], origin=[0., 0., 0.],
                  scale=[160., 100., 100.], Wx=1., Wy=10., wz=30.,
                  amps=[0., 0., 0.], M2_x=0.)
    ro = [1.000001, 0., 0.]
    rd = [-1e-7, 0., math.sqrt(1 - 1e-14)]
    pieces = module.process_ray_member(ro, rd, 0., 20., medium)
    point = [ro[k] + 15 * rd[k] for k in range(3)]
    assert pieces == []
    assert abs(point[0]) < medium['Wx'] and abs(point[1]) < medium['Wy']
    assert abs(point[2]) < medium['wz']

    old = json.loads((root / 'docs/audits/NORTHERN_VEIL_MATCHED_NATIVE_CONSUMER_2026-10-21.json').read_text())
    new = json.loads((root / 'docs/audits/NORTHERN_VEIL_CANONICAL_PRODUCER_2026-10-21.json').read_text())
    prior_work = old['series'][0]['execution_workload']
    assert prior_work['invocations_per_dispatch'] == 128 * new['rays']
    fixtures = {}
    for relative in ['saves/zones/Northern Veil/zone.ecform',
                     'docs/audits/NORTHERN_VEIL_NATIVE_CF4_PREFLIGHT_2026-10-21_RAYS.json',
                     'docs/audits/NORTHERN_VEIL_PROJECTED_REFERENCE_2026-10-21_ROWS.json']:
        fixtures[relative] = hashlib.sha256((root / relative).read_bytes()).hexdigest()
    report = {
        'scope': 'Source review and executed scalar/CPU counterexamples; no native rerun or speedup measurement.',
        'reviewed_commit': '017499162163bb55d03652a594ca24681664fe65',
        'author': 'Codex / GPT-6.1 Sol / 01a148a7-bd85-7550-a416-0e859a729c1a',
        'candidate_source_sha256': hashlib.sha256(candidate.read_bytes()).hexdigest(),
        'candidate_python_test_sha256': hashlib.sha256(test_path.read_bytes()).hexdigest(),
        'extracted_lambda_sha256': hashlib.sha256(actual_lambda.encode()).hexdigest(),
        'cpp_witness_sha256': hashlib.sha256(cpp.encode()).hexdigest(),
        'fixture_sha256': fixtures,
        'exact_cpp_counterexamples': {
            'three_root_cubic': {'formula': '(s-.02)*(s-.05)*(s-.08)',
                                  'domain': [0., 1.], 'valid_abs_second_derivative_bound': 6.,
                                  'expected_roots': [.02, .05, .08],
                                  'emitted_points': values['three_root_cubic']},
            'unresolved_narrow_pair': {'formula': '(s-.0007)^2-1e-10',
                                        'domain': [0., 1.], 'valid_abs_second_derivative_bound': 2.,
                                        'expected_roots': [.00069, .00071],
                                        'emitted_points': values['narrow_root_pair'],
                                        'emitted_point_residual': (midpoint - .0007)**2 - 1e-10}},
        'python_candidate_near_parallel': {
            'synthetic_medium': medium, 'origin': ro, 'direction': rd, 'clip': [0., 20.],
            'pieces_returned': pieces, 'strictly_positive_support_point_at_s15': point,
            'note': 'Candidate process_ray_member executed directly; C++ has the same threshold branch.'},
        'timing_basis': {'new_invocations_per_dispatch': new['rays'],
                         'quoted_prior_workload': prior_work,
                         'old_to_new_invocation_ratio': 128,
                         'new_saved_stage_times': new['connected_timing'],
                         'end_to_end_speedup_established': False},
        'attributed_native_artifact': {
            'accuracy_gates_all_pass': new['accuracy_gates_all_pass'],
            'regressions_vs_native': new['regressions_vs_native'],
            'native_rerun_this_review': False},
        'known_findings_reproduced': True,
    }
    (work / 'review.json').write_text(json.dumps(report, indent=2) + '\n')
    print(out, end='')
    print('Reproduced root loss, unresolved midpoint, near-parallel omission and timing-basis mismatch.')
    print(work / 'review.json')


if __name__ == '__main__':
    main()
