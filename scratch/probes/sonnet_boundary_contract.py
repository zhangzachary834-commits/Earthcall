"""Boundary Guardian (Quest 4) CPU controls for the routed-unfused bridge.
Claude Code / Claude Sonnet 5.5 (claude-sonnet-5-5) / session 93555d50-5298-4a0f-8b26-c03a29808834 / 2026-10-27.
Commissioned by Sixth-One Sun (Codex / GPT-6.1 Sol) after docs/audits/GPU_RAY_TRANSPORT_6EM_CORRECTIVE_TAKEOVER_2026-10-27.md.

What this file IS: three CPU-only layers; no GPU, no engine build, no save access.
  A. SOURCE witnesses: text assertions on the real engine sources (VolumeRayTransport.*, WebGpuRenderer.cpp)
     for the retention keys of each retained result, each paired with a text MUTATION that must fail the witness.
  B. DEPENDENCY model: an exact-arithmetic (Fraction) stand-in for a one-medium-per-id transport with retained
     forms, an independent from-scratch lattice ORACLE (no cache, no plan), event sequences covering density-
     expression revision, medium origin, independent clocks, camera, opaque depth, and density-zero members, and
     retention MUTANTS that each must be caught by the oracle comparison.
  C. PUBLICATION model: a ray population with poisoned packet/output storage. The intended refusal set is derived
     from per-ray INPUTS (demand vs capacity, S2 class), never from the candidate's flags. Controls: missing
     producer/consumer/publication, partial packet, refused-ray packet write, double publication, over/under
     refusal, unrelated-ray perturbation, refused!=Current, and the naive flag-derived checker (which must be shown
     to MISS over-refusal).
What it is NOT: it does not execute engine C++ or WGSL. The model is a contract stand-in; engine-level execution
witnesses are prepared (not run) by sonnet_boundary_native_plan.py. The model's control mutants are the evidence
that each check can fail; passing the unmutated model says the declared contract is self-consistent, nothing more.
"""
import argparse, json, random, re, sys
from fractions import Fraction as F
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
results = []


def record(layer, name, ok, detail=''):
    results.append({'layer': layer, 'name': name, 'ok': bool(ok), 'detail': detail})
    print(('PASS ' if ok else 'FAIL ') + layer + ' ' + name + (' :: ' + detail if detail else ''))
    return ok


# ----------------------------------------------------------------------------------------------------------
# A. Source witnesses with text-mutation controls
# ----------------------------------------------------------------------------------------------------------
def func_body(text, signature):
    i = text.index(signature)
    j = text.index('{', i)
    depth = 0
    for k in range(j, len(text)):
        depth += text[k] == '{'
        depth -= text[k] == '}'
        if depth == 0:
            return text[i:k + 1]
    raise ValueError('unbalanced ' + signature)


def source_witnesses(texts):
    """texts: dict path->str. Returns list of (name, bool, detail). Pure function of the texts so it can be mutated."""
    cpp, hpp, rend = texts['cpp'], texts['hpp'], texts['rend']
    out = []
    forms = func_body(cpp, 'const VolumeRayChannelForms& VolumeRayFormCache::formsFor')
    lower = func_body(cpp, 'VolumeRayChannelForms lowerVolumeRayChannels')
    chan = func_body(cpp, 'VolumeRayChannelForm lowerChannel')
    out.append(('forms-skip-requires-expr-and-revision-and-revisioned',
                'if (!fresh && !unrevisioned && channel.expr == expr && channel.revision == revision) continue;' in forms, 'R1 key'))
    out.append(('forms-revision0-never-retained', 'const bool unrevisioned = present && revision == 0;' in forms, 'R1 rev0'))
    out.append(('forms-empty-producer-never-retained', re.search(r'if \(medium\.producerId\.empty\(\)\)\s*\{\s*_scratch', forms) is not None, 'R1 id'))
    for g in ('forms', 'lower', 'chan'):
        body = {'forms': forms, 'lower': lower, 'chan': chan}[g]
        leak = [w for w in ('origin', 'temporalCoordinate', 'halfExtent', 'camera', 'eyePos', 'depth') if w in body]
        out.append(('R1-lowering-' + g + '-reads-no-ray-tier-binding', not leak, 'leaks=' + ','.join(leak)))
    slots = re.findall(r'&VolumeDensityBinding::(\w+Revision)', cpp)
    out.append(('R1-six-channels-each-carry-a-revision', sorted(set(slots)) == sorted(
        ['densityRevision', 'extinctionRevision', 'scatteringRevision', 'volumeChromaRevision', 'phaseRevision', 'emissionRevision']), ','.join(sorted(set(slots)))))
    out.append(('R1-header-states-origin-clock-not-dependencies', 'medium origin/half-extent, camera, ray, clock: NOT dependencies' in hpp, 'header contract'))
    out.append(('segment-member-carries-origin-and-own-clock', 'glm::dvec3 origin{0.0};' in hpp and 'double temporalCoordinate = 0.0;' in hpp, 'ray-tier numeric binding'))
    out.append(('planner-density-zero-does-not-participate', re.search(r'if \(density <= 0\.0\) continue;', cpp) is not None and '++plan.participatingMembers;' in cpp, 'D<=0 skipped'))
    out.append(('planner-ExactEmpty-needs-all-D<=0-and-executed-slack', 'if (!executedEmpty)' in cpp and 'std::all_of(densities.begin(), densities.end()' in cpp, 'ExactEmpty proof'))
    out.append(('planner-fallback-is-GeneralLattice', 'return lattice(' in cpp and 'GeneralLattice,      // fallback: the existing exact sample loop must run' in hpp, 'R3 Current fallback'))
    cert = func_body(rend, 'bool WebGpuRenderer::volumeSupportCertificatesActive')
    out.append(('R2-cert-unrevisioned-refuses', '|| medium.densityRevision == 0) return false;' in cert, 'rev0 -> no cert'))
    for k in ('memo.densityRevision != medium.densityRevision', 'memo.halfExtent != halfExtent', 'memo.clockBucket != clockBucket'):
        out.append(('R2-cert-invalidated-by-' + k.split('.')[1].split(' ')[0], k in cert, k))
    leak = [w for w in ('medium.origin', '_eyePos', 'temporalCoordinate,') if re.search(re.escape(w), cert.split('const double clockBucket')[0] + cert.split('memo.estimate = {};')[1] if 'memo.estimate = {};' in cert else cert)]
    out.append(('R2-cert-rebuild-body-reads-no-origin-or-eye', 'medium.origin' not in cert.split('memo.estimate = {};')[1].split('return')[0] and '_eyePos' not in cert, 'origin/eye excluded from build'))
    return out


def mutate(texts, which):
    t = dict(texts)
    if which == 'drop-revision-from-key':
        t['cpp'] = t['cpp'].replace('channel.expr == expr && channel.revision == revision', 'channel.expr == expr')
    elif which == 'retain-rev0':
        t['cpp'] = t['cpp'].replace('!fresh && !unrevisioned && ', '!fresh && ')
    elif which == 'cache-origin-in-lowering':
        t['cpp'] = t['cpp'].replace('form.present = expr && !expr->pieces.empty();', 'form.present = expr && !expr->pieces.empty(); (void)medium_origin;', 1)
    elif which == 'cert-ignores-halfextent':
        t['rend'] = t['rend'].replace('memo.halfExtent != halfExtent ||', '')
    elif which == 'cert-ignores-clock-bucket':
        t['rend'] = t['rend'].replace('memo.clockBucket != clockBucket ||', '')
    elif which == 'cert-allows-rev0':
        t['rend'] = t['rend'].replace('|| medium.densityRevision == 0) return false;', ') return false;')
    elif which == 'zero-density-participates':
        t['cpp'] = t['cpp'].replace('if (density <= 0.0) continue;', '')
    elif which == 'forms-read-origin':
        t['cpp'] = t['cpp'].replace('forms.producerId = medium.producerId;', 'forms.producerId = medium.producerId; auto o = medium.origin; (void)o;', 1)
    else:
        raise ValueError(which)
    assert t != texts, 'mutation did not apply: ' + which
    return t


SOURCE_MUTATIONS = ['drop-revision-from-key', 'retain-rev0', 'cache-origin-in-lowering', 'forms-read-origin',
                    'cert-ignores-halfextent', 'cert-ignores-clock-bucket', 'cert-allows-rev0', 'zero-density-participates']


def layer_a():
    texts = {'cpp': (ROOT / 'src/Singularity/Screen/VolumeRayTransport.cpp').read_text(),
             'hpp': (ROOT / 'src/Singularity/Screen/VolumeRayTransport.hpp').read_text(),
             'rend': (ROOT / 'src/Singularity/Screen/WebGPU/WebGpuRenderer.cpp').read_text()}
    base = source_witnesses(texts)
    for n, ok, d in base:
        record('A.source', n, ok, d)
    base_names = {n for n, ok, _ in base if ok}
    for m in SOURCE_MUTATIONS:
        mut = source_witnesses(mutate(texts, m))
        caught = sorted(n for n, ok, _ in mut if not ok and n in base_names)
        record('A.control', 'mutation ' + m + ' is caught by >=1 source witness', bool(caught), ','.join(caught))


# ----------------------------------------------------------------------------------------------------------
# B. Dependency model + oracle + retention mutants
# ----------------------------------------------------------------------------------------------------------
class Expr:
    """Authored density D(p,t) = a + bx*px + by*py + bz*pz + c*t. Identity = object; revision lives on the medium."""
    def __init__(self, a, b, c):
        self.a, self.b, self.c = F(a), tuple(F(x) for x in b), F(c)

    def at(self, p, t):
        return self.a + sum(bi * pi for bi, pi in zip(self.b, p)) + self.c * t


class Medium:
    def __init__(self, mid, expr, rev, origin, half, clock):
        self.id, self.expr, self.rev, self.origin, self.half, self.clock = mid, expr, rev, tuple(map(F, origin)), tuple(map(F, half)), F(clock)


def clone(world):
    return [Medium(m.id, m.expr, m.rev, m.origin, m.half, m.clock) for m in world]


def inside(m, w):
    return all(abs(w[i] - m.origin[i]) <= m.half[i] for i in range(3))


def lattice_oracle(world, cam, d, depth, step, t_in=F(1)):
    """From-scratch, cache-free, plan-free sample loop: the Current path. Samples at s=i*step < min(depth,...)."""
    T, L, consumed = t_in, F(0), 0
    n = int(depth / step)
    for i in range(1, n + 1):
        s = i * step
        w = tuple(cam[k] + d[k] * s for k in range(3))
        sig, src = F(0), F(0)
        for m in world:
            if not inside(m, w):
                continue
            D = m.expr.at(tuple(w[k] - m.origin[k] for k in range(3)), m.clock)
            if D <= 0:
                continue                     # density-zero participation boundary: D<=0 contributes nothing
            sig += D / 2
            src += D
        q = 1 - sig * step
        Lstep = T * (1 - q) * (src / sig) if sig > 0 else F(0)
        L += Lstep
        T *= q
        consumed += 1
    return T, L, consumed


class Candidate:
    """Retained-form pipeline: lowered forms cached by (id, rev, expr identity); ray tier binds origin/clock/camera/depth.
    Per ray: if every in-box member's D is exactly constant over the segment -> closed form; else Current lattice."""
    def __init__(self, mutant=None):
        self.cache, self.lowerings, self.mutant = {}, 0, mutant
        self.stale_origin, self.stale_clock, self.span_cache = {}, {}, {}

    def forms(self, m):
        key = m.id
        if self.mutant == 'retain-rev0' and m.rev == 0:
            ent = self.cache.get(key)
            if ent: return ent[2]
        ent = self.cache.get(key)
        if ent and ent[0] == (m.rev if self.mutant != 'ignore-revision' else 0) and ent[1] is m.expr and not (m.rev == 0 and self.mutant != 'retain-rev0'):
            return ent[2]
        self.lowerings += 1
        low = (m.expr.a, m.expr.b, m.expr.c)
        self.cache[key] = (m.rev if self.mutant != 'ignore-revision' else 0, m.expr, low)
        return low

    def ray(self, world, cam, d, depth, step):
        # NOTE: segment membership is the caller's (V5 box events) in the engine. Here every sample is box-tested, so
        # the closed form is only attempted when the ray stays strictly inside or strictly outside every box for the
        # whole span; otherwise Current lattice runs (documented fallback).
        n = int(depth / step)
        if self.mutant == 'cache-span' and (cam, d) in self.span_cache:
            n = self.span_cache[(cam, d)]
        self.span_cache[(cam, d)] = n
        if n == 0:
            return F(1), F(0), 0, 'current'
        ends = [tuple(cam[k] + d[k] * s for k in range(3)) for s in (step, n * step)]
        parts = []
        for m in world:
            ins = [inside(m, e) for e in ends]
            if not any(ins) and not all(ins):
                pass
            if all(ins):
                parts.append(m)
            elif any(ins):
                return None  # boundary crossing -> Current lattice
            elif self._segment_hits_box(m, cam, d, step, n):
                return None
        return parts, n

    @staticmethod
    def _segment_hits_box(m, cam, d, step, n):
        lo, hi = F(0), F(1)
        p0 = tuple(cam[k] + d[k] * step for k in range(3))
        p1 = tuple(cam[k] + d[k] * n * step for k in range(3))
        for k in range(3):
            dk = p1[k] - p0[k]
            a, b = m.origin[k] - m.half[k], m.origin[k] + m.half[k]
            if dk == 0:
                if not (a <= p0[k] <= b): return False
            else:
                t0, t1 = (a - p0[k]) / dk, (b - p0[k]) / dk
                if t0 > t1: t0, t1 = t1, t0
                lo, hi = max(lo, t0), min(hi, t1)
                if lo > hi: return False
        return True

    def evaluate(self, world, cam, d, depth, step):
        pre = self.ray(world, cam, d, depth, step)
        if isinstance(pre, tuple) and len(pre) == 4:
            return pre[:3] + ('current',)
        if pre is None:
            return lattice_oracle(world, cam, d, depth, step) + ('current',)
        parts, n = pre
        sig, src = F(0), F(0)
        for m in world:
            a, b, c = self.forms(m)
        for m in parts:
            a, b, c = self.forms(m)
            origin = m.origin if self.mutant != 'stale-origin' else self.stale_origin.setdefault(m.id, m.origin)
            clock = m.clock if self.mutant != 'stale-clock' else self.stale_clock.setdefault(m.id, m.clock)
            # D constant on the ray iff b . d == 0 (exact rational test); else Current lattice for this ray.
            if sum(b[k] * d[k] for k in range(3)) != 0:
                return lattice_oracle(world, cam, d, depth, step) + ('current',)
            p = tuple(cam[k] - origin[k] for k in range(3))
            D = a + sum(b[k] * p[k] for k in range(3)) + c * clock
            if D <= 0 and self.mutant != 'zero-participates':
                continue
            sig += D / 2
            src += D
        if sig == 0 and self.mutant != 'zero-participates':
            return F(1), F(0), n, 'closed'
        if sig == 0:
            return F(1), F(0), n, 'closed'
        q = 1 - sig * step
        Tn = q ** n
        Ln = (1 - Tn) * (src / sig)
        return Tn, Ln, n, 'closed'


def random_world(rng):
    ms = []
    for i in range(rng.randint(1, 3)):
        e = Expr(rng.choice([-2, -1, 0, 1, 2, 3]), [rng.choice([0, 0, 1]) * 0, rng.choice([0, 0, 1]) * 0, rng.choice([0, 0, 0, 1, -1])], rng.choice([0, 0, 1, -1]))
        ms.append(Medium('m%d' % i, e, rng.randint(1, 5), (rng.randint(-2, 2), rng.randint(-2, 2), rng.randint(-2, 2)), (rng.randint(3, 12),) * 3, rng.randint(0, 3)))
    return ms


EVENTS = ['revise-density', 'revise-density-in-place', 'edit-density-with-bump-forgotten', 'move-origin', 'advance-one-clock', 'move-camera', 'change-opaque-depth', 'resize-extent', 'rev0-edit']


def apply_event(world, ev, rng, cam, depth, bump_ok=True):
    m = rng.choice(world)
    if ev == 'revise-density':
        m.expr = Expr(rng.choice([-1, 0, 1, 2, 4]), (0, 0, rng.choice([0, 1])), rng.choice([0, 1])); m.rev += 1
    elif ev == 'revise-density-in-place':   # same expression object, authored content changed, revision bumped (FieldNode-style)
        m.expr.a, m.expr.c = F(rng.choice([-1, 0, 1, 2, 4])), F(rng.choice([0, 1])); m.rev += 1
    elif ev == 'edit-density-with-bump-forgotten':   # same object, content changed, revision NOT bumped
        m.expr.a, m.expr.c = F(rng.choice([-1, 0, 1, 2, 4])), F(rng.choice([0, 1]))
    elif ev == 'move-origin':
        m.origin = tuple(o + rng.choice([-1, 1]) for o in m.origin)
    elif ev == 'advance-one-clock':
        m.clock += rng.choice([1, 2])
    elif ev == 'move-camera':
        cam = tuple(c + rng.choice([-1, 0, 1]) for c in cam)
    elif ev == 'change-opaque-depth':
        depth = max(F(2), depth + rng.choice([-3, 3]))
    elif ev == 'resize-extent':
        m.half = tuple(h + rng.choice([-1, 1]) for h in m.half)
    elif ev == 'rev0-edit':
        m.expr = Expr(rng.choice([-1, 0, 1, 2, 4]), (0, 0, 0), rng.choice([0, 1])); m.rev = 0
    return cam, depth


def run_sequences(mutant, seeds, events):
    """Return (mismatches, closed_count, current_count, zero_boundary_hits). Oracle recomputed from scratch each step."""
    mismatches = closed = current = zero_boundary = 0
    for seed in seeds:
        rng = random.Random(seed)
        world = random_world(rng)
        cam, depth, step = (F(0), F(0), F(0)), F(rng.choice([6, 9, 12])), F(1, 2)
        cand = Candidate(mutant)
        for step_i in range(24):
            ev = rng.choice(events)
            if ev == 'edit-density-with-bump-forgotten':
                continue                                   # reported separately: contract hazard, not a regression
            cam, depth = apply_event(world, ev, rng, cam, depth)
            for d in [(F(1), F(0), F(0)), (F(0), F(0), F(1)), (F(0), F(1), F(0)), (F(1), F(1), F(0))]:
                got = cand.evaluate(world, cam, d, depth, step)
                want = lattice_oracle(world, cam, d, depth, step)
                if got[:3] != want:
                    mismatches += 1
                closed += got[3] == 'closed'; current += got[3] == 'current'
                zero_boundary += any(m.expr.at(tuple(cam[k] - m.origin[k] for k in range(3)), m.clock) == 0 for m in world)
    return mismatches, closed, current, zero_boundary


def layer_b():
    seeds = range(1, 81)
    ok_events = [e for e in EVENTS if e != 'edit-density-with-bump-forgotten']
    mm, closed, current, zb = run_sequences(None, seeds, ok_events)
    record('B.model', 'retained candidate == cache-free lattice oracle (80 seeds x 24 events x 4 rays, exact Fractions)', mm == 0, 'mismatches=%d closed=%d current=%d' % (mm, closed, current))
    record('B.model', 'both closed-form and Current-fallback paths exercised (not a vacuous run)', closed > 100 and current > 100, 'closed=%d current=%d' % (closed, current))
    record('B.model', 'density-zero boundary reached (D==0 exactly at the camera for some media)', zb > 0, 'hits=%d' % zb)
    for ev in ok_events:
        mm2, _, _, _ = run_sequences(None, range(1, 41), [ev])
        record('B.model', 'single-event stream ' + ev + ' still equals oracle', mm2 == 0, 'mismatches=%d' % mm2)
    # Mutant controls: each retention error must be caught by the independent oracle.
    needs = {'ignore-revision': ['revise-density-in-place'], 'retain-rev0': ['rev0-edit'], 'stale-origin': ['move-origin'],
             'stale-clock': ['advance-one-clock'], 'cache-span': ['change-opaque-depth'], 'zero-participates': ['revise-density', 'advance-one-clock', 'move-origin']}
    for mut, evs in needs.items():
        n, _, _, _ = run_sequences(mut, range(1, 81), evs + ['move-camera'])
        record('B.control', 'mutant ' + mut + ' is caught by oracle on events ' + '+'.join(evs), n > 0, 'mismatches=%d' % n)
    # Known contract hazard (reported, not claimed green): edit with the revision NOT bumped is stale-by-design.
    hazard = 0
    for seed in range(1, 61):
        rng = random.Random(seed); world = random_world(rng); cam, depth, step = (F(0),) * 3, F(8), F(1, 2)
        cand = Candidate(None); d = (F(0), F(0), F(1))
        cand.evaluate(world, cam, d, depth, step)
        m = world[0]; m.expr.a += 3   # in place, same identity, revision NOT bumped
        if cand.evaluate(world, cam, d, depth, step)[:3] != lattice_oracle(world, cam, d, depth, step):
            hazard += 1
    record('B.hazard', 'forgotten revision bump yields a STALE retained form (contract relies on bumps; engine verifier must be witnessed natively)', hazard > 0,
           'stale_in=%d/60 -- this is a documented limitation, not a pass of the engine' % hazard)

    # Per-dependency NECESSITY: each retained-result dependency actually changes the oracle somewhere; each declared
    # non-dependency of the lowered forms leaves the cached forms untouched (zero relowerings).
    rng = random.Random(7)
    w = [Medium('a', Expr(1, (0, 0, 0), 1), 3, (0, 0, 4), (3, 3, 6), 0)]
    base = lattice_oracle(w, (F(0),) * 3, (F(0), F(0), F(1)), F(8), F(1, 2))
    var = {}
    w2 = clone(w); w2[0].expr = Expr(2, (0, 0, 0), 1); var['density-expression'] = lattice_oracle(w2, (F(0),) * 3, (F(0), F(0), F(1)), F(8), F(1, 2))
    w2 = clone(w); w2[0].origin = (F(0), F(0), F(40)); var['medium-origin'] = lattice_oracle(w2, (F(0),) * 3, (F(0), F(0), F(1)), F(8), F(1, 2))
    w2 = clone(w); w2[0].clock = F(5); var['medium-clock'] = lattice_oracle(w2, (F(0),) * 3, (F(0), F(0), F(1)), F(8), F(1, 2))
    var['camera'] = lattice_oracle(w, (F(0), F(0), F(-20)), (F(0), F(0), F(1)), F(8), F(1, 2))
    var['opaque-depth'] = lattice_oracle(w, (F(0),) * 3, (F(0), F(0), F(1)), F(5), F(1, 2))
    w2 = clone(w); w2[0].half = (F(3), F(3), F(1)); var['extent'] = lattice_oracle(w2, (F(0),) * 3, (F(0), F(0), F(1)), F(8), F(1, 2))
    for k, v in var.items():
        record('B.necessity', k + ' changes the from-scratch result (retention must key or rebind it)', v != base, '')
    cand = Candidate(); cand.evaluate(w, (F(0),) * 3, (F(0), F(0), F(1)), F(8), F(1, 2)); n0 = cand.lowerings
    for mover in ('origin', 'clock', 'camera', 'depth', 'extent'):
        w2 = clone(w)
        cam, depth = (F(0),) * 3, F(8)
        if mover == 'origin': w2[0].origin = (F(0), F(0), F(5))
        if mover == 'clock': w2[0].clock = F(9)
        if mover == 'camera': cam = (F(0), F(0), F(-1))
        if mover == 'depth': depth = F(6)
        if mover == 'extent': w2[0].half = (F(3), F(3), F(7))
        cand.evaluate(w2, cam, (F(0), F(0), F(1)), depth, F(1, 2))
    record('B.necessity', 'origin/clock/camera/depth/extent moves cause ZERO relowering (only authored expression does)', cand.lowerings == n0, 'lowerings=%d' % cand.lowerings)
    cand.evaluate([Medium('a', Expr(2, (0, 0, 0), 1), 4, (0, 0, 4), (3, 3, 6), 0)], (F(0),) * 3, (F(0), F(0), F(1)), F(8), F(1, 2))
    record('B.necessity', 'a revision bump relowers exactly that medium', cand.lowerings == n0 + 1, 'lowerings=%d' % cand.lowerings)

    # Independent clocks: two media with different clocks must not share one.
    wc = [Medium('p', Expr(0, (0, 0, 0), 1), 1, (0, 0, 4), (4, 4, 6), 2), Medium('q', Expr(0, (0, 0, 0), 1), 1, (0, 0, 4), (4, 4, 6), 5)]
    both = lattice_oracle(wc, (F(0),) * 3, (F(0), F(0), F(1)), F(8), F(1, 2))
    wc[1].clock = wc[0].clock
    shared = lattice_oracle(wc, (F(0),) * 3, (F(0), F(0), F(1)), F(8), F(1, 2))
    record('B.necessity', 'independent medium clocks differ from a shared clock (shared-clock retention would be wrong)', both != shared, '')


# ----------------------------------------------------------------------------------------------------------
# C. Publication / refusal model with poisoned storage
# ----------------------------------------------------------------------------------------------------------
POISON = object()
CAP_SUB, CAP_OVER = 4, 3


class Ray:
    def __init__(self, i, demand, overlaps, cls):
        self.i, self.demand, self.overlaps, self.cls = i, demand, overlaps, cls   # cls in {'S1','S2'}; S2 = late member-1 refusal


def current_value(r): return ('cur', r.i)          # distinct, exact per-ray values; no tolerance anywhere
def bulk_value(r): return ('bulk', r.i)


def intended_refusals(rays):
    """INPUT-derived: S2 class (late) or capacity overflow. Never reads candidate flags."""
    return {r.i for r in rays if r.cls == 'S2' or r.demand > CAP_SUB or r.overlaps > CAP_OVER}


class Bridge:
    def __init__(self, rays, fault=None, over=(), under=()):
        n = len(rays)
        self.rays, self.fault = rays, fault
        self.flags = [False] * n
        self.sub_count = [0] * n
        self.sub = [[POISON] * CAP_SUB for _ in range(n)]       # packet words, poisoned
        self.out = [POISON] * n                                  # output, poisoned
        self.publishers = [0] * n
        self.producer_entries = [0] * n
        self.over, self.under = set(over), set(under)

    def run(self):
        for r in self.rays:
            if self.fault == 'missing-producer' and r.i == 3: continue
            self.producer_entries[r.i] += 1
            refuse = r.cls == 'S2' or r.demand > CAP_SUB or r.overlaps > CAP_OVER or r.i in self.over
            if r.i in self.under: refuse = False
            if self.fault == 'partial-packet' and refuse:
                self.sub_count[r.i] = 1; self.sub[r.i][0] = ('partial', r.i)
            if self.fault == 'refused-writes-packet' and refuse:
                self.sub[r.i][1] = ('stray', r.i)
            if refuse:
                self.flags[r.i] = True
                continue
            for k in range(min(r.demand, CAP_SUB)):
                self.sub[r.i][k] = ('sub', r.i, k)
            self.sub_count[r.i] = min(r.demand, CAP_SUB)
        for r in self.rays:                                      # consumer + final fallback publisher
            if self.fault == 'missing-consumer' and not self.flags[r.i] and r.i == 5: continue
            if not self.flags[r.i]:
                if self.fault == 'missing-producer' and r.i == 3: continue
                self.publish(r.i, bulk_value(r) if self.fault != 'unrelated-perturbed' or r.i != 7 else ('bad', r.i))
                if self.fault == 'double-publish' and r.i == 2: self.publish(r.i, bulk_value(r))
        for r in self.rays:
            if self.flags[r.i]:
                if self.fault == 'missing-fallback' and r.i == self.first_refused(): continue
                self.publish(r.i, current_value(r) if self.fault != 'refused-not-current' else bulk_value(r))
        return self

    def first_refused(self):
        return min(i for i, f in enumerate(self.flags) if f)

    def publish(self, i, v):
        self.publishers[i] += 1
        self.out[i] = v


def checker(b, rays, naive=False):
    """Return list of violation strings. naive=True derives the expected set from the candidate flags (the mistake)."""
    errs = []
    intended = {i for i, f in enumerate(b.flags) if f} if naive else intended_refusals(rays)
    for r in rays:
        i = r.i
        if b.flags[i] != (i in intended): errs.append('refusal-set:%d' % i)
        if b.producer_entries[i] != 1: errs.append('producer-ownership:%d' % i)
        if b.publishers[i] != 1: errs.append('final-publishers:%d=%d' % (i, b.publishers[i]))
        if b.out[i] is POISON: errs.append('unpublished:%d' % i); continue
        want = current_value(r) if i in intended else bulk_value(r)
        if b.out[i] != want: errs.append('value:%d' % i)
        if i in intended:
            if b.sub_count[i] != 0: errs.append('partial-packet:%d' % i)
            if any(w is not POISON for w in b.sub[i]): errs.append('refused-wrote-storage:%d' % i)
        else:
            if any(b.sub[i][k] is POISON for k in range(b.sub_count[i])): errs.append('hole-in-packet:%d' % i)
    return errs


def make_rays(seed):
    rng = random.Random(seed)
    rays = []
    for i in range(16):
        cls = 'S2' if i % 5 == 4 else 'S1'
        rays.append(Ray(i, rng.randint(1, 3), rng.randint(0, 2), cls))
    rays[1].demand = CAP_SUB + 1            # capacity overflow (subpanel)
    rays[6].overlaps = CAP_OVER + 1         # capacity overflow (overlap)
    rays[2].demand, rays[2].overlaps = CAP_SUB, CAP_OVER   # exactly AT capacity: must NOT refuse
    return rays


def layer_c():
    rays = make_rays(1)
    inten = intended_refusals(rays)
    record('C.model', 'intended set is non-trivial and includes S2 + both capacity kinds, excludes at-capacity ray', {1, 4, 6, 9, 14}.issubset(inten) and 2 not in inten, str(sorted(inten)))
    b = Bridge(rays).run()
    record('C.model', 'faithful bridge passes every check with poisoned packets/output', checker(b, rays) == [], str(checker(b, rays)[:4]))
    record('C.model', 'refused rays equal Current, others equal bulk, packet storage still fully poisoned for refused rays',
           all(b.out[i] == current_value(rays[i]) for i in inten) and all(all(w is POISON for w in b.sub[i]) for i in inten), '')
    faults = {'missing-producer': 'producer-ownership', 'missing-consumer': 'unpublished', 'missing-fallback': 'unpublished',
              'partial-packet': 'partial-packet', 'refused-writes-packet': 'refused-wrote-storage', 'double-publish': 'final-publishers',
              'unrelated-perturbed': 'value', 'refused-not-current': 'value'}
    for f, expect in faults.items():
        bb = Bridge(rays, fault=f).run()
        errs = checker(bb, rays)
        hit = any(e.startswith(expect) for e in errs)
        record('C.control', 'fault ' + f + ' is caught as ' + expect, hit, str(errs[:3]))
    over = Bridge(rays, over={0, 3}).run()
    e_ind, e_naive = checker(over, rays), checker(over, rays, naive=True)
    record('C.control', 'over-refusal (rays 0,3 extra, each correctly equals Current) is caught by the INPUT-derived checker', any(e.startswith('refusal-set') for e in e_ind), str(e_ind[:3]))
    value_errs = [e for e in e_naive if e.startswith('value')]
    record('C.control', 'the flag-derived NAIVE checker MISSES that same over-refusal (demonstrates the self-comparison trap)', e_naive == [] , str(e_naive[:3]))
    under = Bridge(rays, under={1}).run()
    e_under = checker(under, rays)
    record('C.control', 'under-refusal of a capacity-overflow ray is caught (truncated packet would otherwise publish)', any(e.startswith('refusal-set:1') for e in e_under), str(e_under[:3]))
    # Population sweep: random demands, faithful bridge must always pass; every single-fault must always fail.
    bad_pass = bad_fail = 0
    for s in range(200):
        rs = make_rays(s)
        if checker(Bridge(rs).run(), rs): bad_pass += 1
        for f in faults:
            if not checker(Bridge(rs, fault=f).run(), rs): bad_fail += 1
    record('C.model', '200 populations: faithful passes, all 8 faults fail every time', bad_pass == 0 and bad_fail == 0, 'faithful_failures=%d fault_escapes=%d' % (bad_pass, bad_fail))
    unrelated_ok = all(Bridge(rays, over={0}).run().out[i] == bulk_value(rays[i]) for i in (5, 8, 11))
    record('C.model', 'refusing one extra ray leaves unrelated rays at bulk values (isolation)', unrelated_ok, '')


# ----------------------------------------------------------------------------------------------------------
def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--json', type=Path, help='write results here')
    a = ap.parse_args()
    layer_a(); layer_b(); layer_c()
    fails = [r for r in results if not r['ok']]
    print('\nTOTAL %d checks, %d failed' % (len(results), len(fails)))
    for r in fails: print('  FAILED:', r['layer'], r['name'], r['detail'])
    if a.json: a.json.write_text(json.dumps({'results': results, 'failed': len(fails)}, indent=1))
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
