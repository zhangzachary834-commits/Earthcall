"""Sonnet skeptic CPU probe over the stored 6em archive (no GPU, no native rerun).
Claude Code / Sonnet 5.5 / session 031c3581-469a-47ad-80f1-7518cf4a33b4 / 2026-10-27.
Reports: (a) cross-series drift of the same mode, (b) blocking-poll clustering,
(c) rays that are neither refused nor completed, (d) replay blind spots.
"""
import json, statistics as st, sys, zipfile, hashlib, struct, re
from pathlib import Path
arc = Path(sys.argv[1]); z = zipfile.ZipFile(arc)
rd = lambda n: json.loads(z.read(n))
print("== (a) same-mode drift between the two ordered series, and (b) blocking clustering ==")
for style, name in ((1, 'active'), (0, 'blocking')):
    for K in (1, 2, 4, 8, 16):
        d = rd(f'native/clean-w{style}-K{K}.json'); S = d['clean_timing_ordered_samples']
        out = []
        for m in (0, 2):
            med = [st.median(x['total_ms'] for x in v if x['mode'] == m) for v in S.values()]
            allv = [x['total_ms'] for v in S.values() for x in v if x['mode'] == m]
            M = st.median(allv); tight = sum(abs(x - M) <= 0.03 for x in allv)
            out.append(f"m{m}: series medians {med[0]:.3f}/{med[1]:.3f} drift {abs(med[0]-med[1])/min(med)*100:.1f}% within±0.03ms {tight}/16")
        print(name, 'K', K, ' | '.join(out))
print("== (c) population classes in positive case ==")
c = rd('native/positive.json')['cases'][0]
ref = sum(bool(f & 0x80000000) for f in c['flags']); done = sum(bool(f & 0x40000000) for f in c['flags'])
other = len(c['flags']) - ref - done
print(f"refused {ref} completed {done} neither {other} (family_counts tiles {c['family_counts']})")
print("== (d) replay blind spots ==")
src = z.read('source/ray_transport_6em_replay.py').decode()
print("hardcoded '180' in PASS text:", "180 timing" in src)
print("replay asserts stored preflight_parity_pass boolean (not recomputed from K>1 rows, which are not archived):", "d['preflight_parity_pass']" in src)
rowsets = {m: n for m, n in rd('native/clean-w1-K1.json')['preflight_actual_rows_files'].items()}
print("modes 1-5 share one rows file (single accuracy witness x5):", len({rowsets[str(m)] for m in range(1, 6)}) == 1)
for k in ('partial_negative', 'missing_publication_negative'):
    print(k, 'returncode', rd('native/index.json')[k]['returncode'])
