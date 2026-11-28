"""Evidence validation and summaries. Unknown observations never become zero cost."""
from collections import Counter, defaultdict
import csv
import hashlib
import json
import math
from pathlib import Path
import re


class InputError(ValueError):
    pass


def finite(value, label, minimum=0):
    if isinstance(value, bool):
        raise InputError(f'{label}: expected a number, not boolean')
    try:
        number = float(value)
    except (TypeError, ValueError, OverflowError) as error:
        raise InputError(f'{label}: invalid number') from error
    if not math.isfinite(number) or number < minimum:
        raise InputError(f'{label}: expected finite number >= {minimum}')
    return number


def flag(value, label):
    if value in (True, 1, '1', 'true', 'True'):
        return True
    if value in (False, 0, '0', 'false', 'False'):
        return False
    raise InputError(f'{label}: expected true/false or 1/0')


def load_json(path):
    try:
        return json.loads(Path(path).read_text(), parse_constant=lambda x: (_ for _ in ()).throw(InputError(f'nonfinite JSON: {x}')))
    except (ValueError, UnicodeError) as error:
        raise InputError(f'{path}: {error}') from error


def fingerprint(path):
    path = Path(path).resolve()
    return {'path': str(path), 'bytes': path.stat().st_size,
            'sha256': hashlib.sha256(path.read_bytes()).hexdigest()}


def stats(values):
    values = sorted(values)
    if not values:
        return {'n': 0, 'mean': None, 'p50': None, 'p95': None, 'min': None, 'max': None}
    def percentile(p):
        x = (len(values)-1)*p; left = int(x); right = math.ceil(x)
        return values[left] + (values[right]-values[left])*(x-left)
    return {'n': len(values), 'mean': math.fsum(values)/len(values),
            'p50': percentile(.5), 'p95': percentile(.95), 'min': values[0], 'max': values[-1]}


CONTEXT = ('run', 'phase', 'zone', 'view', 'width', 'height', 'backend', 'build', 'route', 'clock_policy')
TIMINGS = ('total_ms', 'input_ms', 'locomotion_ms', 'creation_ms', 'interaction_ms',
           'zone_ms', 'zone_ground_ms', 'zone_rot_ms', 'zone_auto_ms', 'zone_phys_ms',
           'laws_ms', 'language_ms', 'audio_ms', 'render3d_ms', 'wait_surface_ms',
           'wait_submit_ms', 'imgui_ms', 'volumeCpuPrepareMs')
COUNTERS = ('drawCalls', 'uniformBytesWritten', 'pipelineSwitches', 'sdfProgramCompiles',
            'volumeProgramCompiles', 'volumeLowerings', 'volumeRepairedNodes',
            'volumeParameterBytesUploaded', 'volumeProofBytesUploaded',
            'volumeZeroProofBuilds', 'volumeZeroProofCellsProven', 'volumeZeroProofCellsTotal')
GPU = (('gpuMainPassMs', 'gpuMainPassTimingValid'), ('gpuVolumePassMs', 'gpuVolumePassTimingValid'))


def frame_records(path):
    path = Path(path)
    if path.suffix.lower() == '.csv':
        with path.open(newline='') as stream:
            rows = list(csv.DictReader(stream))
    elif path.suffix.lower() == '.jsonl':
        rows = [json.loads(x) for x in path.read_text().splitlines() if x.strip()]
    else:
        rows = load_json(path)
        if isinstance(rows, dict):
            rows = rows.get('frames')
    if not isinstance(rows, list) or not all(isinstance(x, dict) for x in rows):
        raise InputError('frames must be CSV/JSONL objects or a JSON frames array')
    return rows


def summarize_frames(rows, filters=None):
    """Separate contexts/cadences and delayed GPU sample IDs; never add nested timers."""
    filters = filters or {}
    groups = {}; excluded = Counter(); warnings = []; accepted = []
    identities = set()
    for index, original in enumerate(rows):
        row = {k: v for k,v in original.items() if v is not None and v != ''}
        if any(str(row.get(k, '')) != str(v) for k,v in filters.items()):
            excluded['context_filter'] += 1; continue
        if 'occluded' in row and flag(row['occluded'], 'occluded'):
            excluded['occluded'] += 1; continue
        if str(row.get('surface_status', 'unknown')).lower() not in ('unknown', 'success', 'optimal', 'suboptimal'):
            excluded['surface_unavailable'] += 1; continue
        context = {k: str(row[k]) if k in row else 'unknown' for k in CONTEXT}
        for key in ('width', 'height'):
            if key in row:
                n = finite(row[key], key, 1)
                if not n.is_integer(): raise InputError(f'{key}: expected integer pixels')
                context[key] = int(n)
        if 'frame_id' in row:
            identity = (context['run'], str(row['frame_id']))
            if identity in identities: raise InputError(f'duplicate frame identity: {identity}')
            identities.add(identity)
        key = tuple(context.values())
        group = groups.setdefault(key, {'context': context, 'frames': 0, 'timings': defaultdict(list), 'counters': defaultdict(list)})
        group['frames'] += 1
        for metric in TIMINGS + COUNTERS:
            if metric in row:
                val = finite(row[metric], metric)
                if metric in COUNTERS and not val.is_integer(): raise InputError(f'{metric}: expected integer counter')
                group['timings' if metric in TIMINGS else 'counters'][metric].append(val)
        if all(k in row for k in ('render3d_ms', 'wait_surface_ms', 'wait_submit_ms')):
            remainder = finite(row['render3d_ms'], 'render3d_ms') - finite(row['wait_surface_ms'], 'wait_surface_ms') - finite(row['wait_submit_ms'], 'wait_submit_ms')
            if remainder < -1e-6:
                raise InputError('surface/submit waits exceed their enclosing render3d timer')
            group['timings']['recording_remainder_ms'].append(max(0., remainder))
        accepted.append((row, context, index))
    for group in groups.values():
        group['timings'] = {k: stats(v) for k,v in group['timings'].items()}
        group['counters'] = {k: dict(stats(v), total=math.fsum(v)) for k,v in group['counters'].items()}
        if 'total_ms' in group['timings']:
            p95 = group['timings']['total_ms']['p95']
            group['observed_p95_under_33_3ms'] = p95 <= 33.3
        if any(v == 'unknown' for v in group['context'].values()):
            warnings.append('Frame context is incomplete; unknown build/route/phase/dimensions must not be silently matched in A/B comparisons.')
    # Async timestamps belong to their submitted frame, not the frame reading them.
    submitted = {(c['run'], str(r['frame_id'])): c for r,c,_ in accepted if 'frame_id' in r}
    gpu = {}; seen_gpu = {}; unavailable = Counter()
    for row, context, index in accepted:
        for metric, validity in GPU:
            if metric not in row: continue
            if 'gpuMainPassTimingSupported' in row and not flag(row['gpuMainPassTimingSupported'],'gpuMainPassTimingSupported'):
                unavailable[metric] += 1; continue
            if validity not in row or not flag(row[validity], validity):
                unavailable[metric] += 1; continue
            value = finite(row[metric], metric)
            if value <= 0:
                unavailable[metric] += 1; continue
            if 'gpuTimingSampleFrame' not in row:
                warnings.append('GPU duration without source frame ID excluded; validity alone cannot align delayed samples.')
                unavailable[metric] += 1; continue
            identity = (context['run'], str(row['gpuTimingSampleFrame']), metric)
            if identity in seen_gpu:
                if seen_gpu[identity] != value: raise InputError('conflicting repeated GPU sample')
                continue
            seen_gpu[identity] = value
            owner = submitted.get(identity[:2])
            gpu.setdefault(metric, []).append({'source_frame': identity[1], 'ms': value,
                                               'context': owner, 'observed_at_record': index})
    return {'evidence': 'imported CPU wall-clock samples; GPU samples separately validated',
            'groups': list(groups.values()), 'excluded': dict(excluded), 'gpu_samples': gpu,
            'gpu_unavailable': dict(unavailable), 'warnings': sorted(set(warnings)),
            'interpretation': ['Nested CPU timers and overlapping CPU/GPU work are not additive.',
                               'Surface acquisition measures a blocking location, not the underlying cause.',
                               'recording_remainder_ms is an enclosing wall-clock remainder, not isolated command-encoding CPU usage.',
                               'A finite p95 result is not Person acceptance or sustained movement certification.']}


def summarize_demand(rows):
    if not isinstance(rows, list): raise InputError('demand results must be the existing census results array')
    result = []
    for row in rows:
        if not isinstance(row, dict): raise InputError('demand row must be an object')
        g = row.get('globals', {}); media = row.get('media', [])
        if not isinstance(g,dict) or not isinstance(media,list) or not all(isinstance(x,dict) for x in media):
            raise InputError('demand globals/media require an object and object array')
        for v in g.values():
            if not isinstance(v,(int,float)) or not finite(v, 'global demand counter').is_integer():
                raise InputError('demand counters require nonnegative numeric integers')
        checked = all(x in g for x in ('sample_visits','all_proven_jumps','transport_updates'))
        if checked and g['sample_visits'] != g['all_proven_jumps']+g['transport_updates']:
            raise InputError('demand census global accounting mismatch')
        ranked = []
        for i, member in enumerate(media):
            for val in member.values():
                if not isinstance(val,(int,float)) or not finite(val, 'medium demand counter').is_integer(): raise InputError('noninteger demand counter')
            required = ('proof_query','proven_empty','density_demand','shared_shape_demand','noise_demand','positive_density')
            if all(x in member for x in required):
                if member['proof_query'] != member['proven_empty']+member['density_demand']:
                    raise InputError('demand census proof accounting mismatch')
                if member['positive_density'] > member['noise_demand']:
                    raise InputError('positive density exceeds noise demands')
            factors=[f'factor{i}_zero' for i in range(5)]
            if 'shared_shape_demand' in member and 'noise_demand' in member and all(k in member for k in factors):
                if member['shared_shape_demand'] != member['noise_demand']+sum(member[k] for k in factors):
                    raise InputError('demand census factor accounting mismatch')
            ranked.extend({'medium':i,'operation':k,'count':v} for k,v in member.items())
        result.append({'view':row.get('view'), 'time':row.get('time'), 'camera':row.get('camera'),
                       'framebuffer':row.get('framebuffer'), 'globals':g,
                       'ranked_counts':sorted(ranked,key=lambda x:-x['count']),
                       'pixels_identical':row.get('pixels_identical'), 'max_error':row.get('max_error'),
                       'scope':row.get('scope','unspecified diagnostic scope')})
    return {'evidence':'instrumented sample counts, never milliseconds or baseline-exact execution',
            'rows':result, 'warnings':['No extrapolation from selected fragments to all pixels.',
                                     'Counters can perturb shader compilation, pixels and execution cost.',
                                     'A high invocation count does not establish the largest time cost.']}


def summarize_model(model):
    if not isinstance(model, dict): raise InputError('model must be an object')
    dims = model.get('dimensions', {}); terms = []; names = set()
    if not isinstance(dims,dict) or not isinstance(model.get('terms',[]),list):
        raise InputError('model requires dimensions object and terms array')
    for name, dim in dims.items():
        if not isinstance(dim, dict) or not dim.get('meaning') or not dim.get('source'):
            raise InputError(f'model dimension {name} requires meaning and source')
        finite(dim.get('value'), name)
    for term in model.get('terms', []):
        if not isinstance(term,dict):raise InputError('model term must be an object')
        name = str(term.get('name',''))
        if not name or name in names: raise InputError('model terms need unique names')
        names.add(name); value = finite(term.get('constant',1), name)
        factors = term.get('factors', {})
        if not isinstance(factors,dict):raise InputError('model factors must be an object')
        for dim, power in factors.items():
            if dim not in dims: raise InputError(f'unknown model dimension: {dim}')
            p = finite(power, dim)
            if not p.is_integer(): raise InputError('model exponents must be nonnegative integers')
            try: value *= finite(dims[dim]['value'],dim)**int(p)
            except OverflowError as error: raise InputError('model overflow') from error
        if not math.isfinite(value): raise InputError('model overflow')
        if not term.get('assumptions') or not term.get('unit') or not term.get('cadence'):
            raise InputError('model term requires assumptions, unit and cadence')
        factors_text = ' * '.join(f'{k}^{int(v)}' if v != 1 else k for k,v in factors.items() if v)
        terms.append(dict(term, modeled_count=value, complexity='O('+ (factors_text or '1') + ')'))
    return {'evidence':'explicit structural model; not measured demands, GPU instructions or a proof of runtime Big-O',
            'dimensions':dims,'terms':terms,'warnings':['Counts with different units/cadences must not be summed or ranked as time.']}


def summarize_scaling(frames, metric):
    """Observed pixel-count scaling with fixed labeled context; never a complexity proof."""
    grouped=defaultdict(list)
    for group in frames['groups']:
        context=group['context']
        keys=[k for k in CONTEXT if k not in ('run','width','height')]
        if any(context[k]=='unknown' for k in keys) or not isinstance(context['width'],int) or not isinstance(context['height'],int):
            raise InputError('pixel scaling requires complete fixed context including build/route/clocks')
        if metric not in group['timings']:continue
        value=group['timings'][metric]['p50']
        if value<=0:raise InputError('log scaling requires positive median duration')
        grouped[tuple(context[k] for k in keys)].append({'pixels':context['width']*context['height'],
            'width':context['width'],'height':context['height'],'p50_ms':value,'n':group['timings'][metric]['n']})
    results=[]
    for key,points in grouped.items():
        points.sort(key=lambda p:p['pixels'])
        if len({p['pixels'] for p in points})!=len(points):
            raise InputError('multiple runs/aspect ratios at one pixel count; select explicit runs before fitting')
        if len(points)<2:continue
        xs=[math.log(p['pixels']) for p in points];ys=[math.log(p['p50_ms']) for p in points]
        xm=sum(xs)/len(xs);ym=sum(ys)/len(ys)
        slope=sum((x-xm)*(y-ym) for x,y in zip(xs,ys))/sum((x-xm)**2 for x in xs)
        results.append({'fixed_context':dict(zip(keys,key)),'metric':metric,'points':points,'observed_log_log_slope':slope})
    if not results:raise InputError('pixel scaling needs at least two distinct pixel counts in the same complete context')
    return {'evidence':'empirical log-log fit of imported median wall-clock samples; not asymptotic Big-O',
            'fits':results,'warnings':['Resolution can change coverage/divergence and waits; a fitted slope does not establish GPU cause.',
                                     'Fits do not establish matched thermal/load/sampling conditions or a permissible production resolution change.']}


def summarize_wgsl(source, entries=None):
    # WGSL block comments may nest. Remove comments before recognizing declarations.
    cleaned = []; i = 0; depth = 0
    while i < len(source):
        if source.startswith('/*',i): depth += 1; cleaned.append('  '); i += 2
        elif depth and source.startswith('*/',i): depth -= 1; cleaned.append('  '); i += 2
        elif depth: cleaned.append('\n' if source[i]=='\n' else ' '); i += 1
        elif source.startswith('//',i):
            end = source.find('\n',i); end = len(source) if end < 0 else end
            cleaned.append(' '*(end-i)); i = end
        else: cleaned.append(source[i]); i += 1
    if depth: raise InputError('unterminated WGSL block comment')
    text = ''.join(cleaned); functions = {}
    for match in re.finditer(r'\bfn\s+([A-Za-z_]\w*)\s*\(',text):
        name = match[1]; start = text.find('{',match.end())
        if start < 0: raise InputError('WGSL function without body')
        depth = 1; end = start+1
        while end < len(text) and depth:
            depth += (text[end]=='{')-(text[end]=='}'); end += 1
        if depth: raise InputError('unbalanced WGSL body')
        if name in functions: raise InputError('duplicate WGSL function name')
        calls = Counter(x for x in re.findall(r'\b([A-Za-z_]\w*)\s*\(',text[start+1:end-1])
                        if x not in ('if','for','while','switch','return'))
        functions[name] = {'name':name,'line':text.count('\n',0,match.start())+1,'calls':dict(calls)}
    if not functions: raise InputError('no WGSL functions; Metal/C++ inputs are not accepted as WGSL')
    entries = entries or list(functions)
    if any(x not in functions for x in entries): raise InputError('unknown WGSL entry function')
    reachable = set(); pending = list(entries)
    while pending:
        name = pending.pop()
        if name in reachable: continue
        reachable.add(name); pending.extend(x for x in functions[name]['calls'] if x in functions)
    return {'evidence':'emitted WGSL syntax only; loops/branches and backend optimization not expanded',
            'entries':entries,'functions':list(functions.values()),'reachable_functions':sorted(reachable),
            'reachable_call_sites':dict(sum((Counter(functions[x]['calls']) for x in reachable),Counter())),
            'warnings':['Function call sites are not runtime call counts or hardware instruction counts.',
                        'Matching function names or text does not establish coordinate/clock equivalence.']}
