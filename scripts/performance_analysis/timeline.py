"""Bounded Chrome trace JSON analysis; no runtime hooks or causal inference."""
from collections import Counter, defaultdict
from .analysis import InputError, finite, stats


def union_length(intervals):
    total = 0.; right = None
    for a,b in sorted(intervals):
        if right is None or a >= right:
            total += b-a; right = b
        elif b > right:
            total += b-right; right = b
    return total


def summarize_trace(data):
    events = data.get('traceEvents') if isinstance(data,dict) else data
    if not isinstance(events,list): raise InputError('trace requires a traceEvents array or event array')
    stacks = defaultdict(list); slices = []; ignored = Counter(); counters = []; names = {}
    # Chrome ts/dur are microseconds, irrespective of displayTimeUnit.
    for event in events:
        if not isinstance(event,dict): raise InputError('trace event must be an object')
        if not isinstance(event.get('args',{}),dict):raise InputError('trace event args must be an object')
        ph = event.get('ph'); track = (str(event.get('pid',0)),str(event.get('tid',0)))
        if ph == 'M':
            if event.get('name')=='thread_name': names[track] = str(event.get('args',{}).get('name',''))
            continue
        if ph not in ('X','B','E','C'):
            ignored[str(ph)] += 1; continue
        ts = finite(event.get('ts'),'trace timestamp')/1000
        if ph == 'C':
            for k,v in event.get('args',{}).items():
                counters.append({'pid':track[0],'tid':track[1],'name':str(event.get('name',''))+'.'+str(k),
                                 'ts_ms':ts,'value':finite(v,'trace counter',-float('inf'))})
            continue
        if ph == 'B':
            stacks[track].append((ts,event)); continue
        if ph == 'E':
            if not stacks[track]: raise InputError('unmatched trace end event')
            start, begin = stacks[track].pop()
            if ts < start: raise InputError('negative B/E duration')
            event = begin; duration = ts-start; ts = start
        else:
            duration = finite(event.get('dur'),'trace duration')/1000
        slices.append({'pid':track[0],'tid':track[1], 'name':str(event.get('name','unnamed')),
                       'category':str(event.get('cat','unspecified')),'start_ms':ts,
                       'duration_ms':duration,'end_ms':ts+duration})
    if any(stacks.values()): raise InputError('unclosed trace begin event')
    tracks = defaultdict(list)
    for item in slices: tracks[(item['pid'],item['tid'])].append(item)
    track_reports = []; ranking = []; warnings = []
    for key, items in sorted(tracks.items()):
        items.sort(key=lambda x:(x['start_ms'],-x['end_ms']))
        stack = []; ambiguous = False
        for item in items:
            item['children_ms'] = 0.
            while stack and stack[-1]['end_ms'] <= item['start_ms']: stack.pop()
            if stack:
                if item['end_ms'] > stack[-1]['end_ms']:
                    ambiguous = True
                else:
                    stack[-1]['children_ms'] += item['duration_ms']
            stack.append(item)
        groups = defaultdict(list)
        for item in items: groups[(item['category'],item['name'])].append(item)
        for (cat,name), group in groups.items():
            exclusive = None if ambiguous else stats([max(0.,x['duration_ms']-x['children_ms']) for x in group])
            ranking.append({'pid':key[0],'tid':key[1],'category':cat,'name':name,
                            'inclusive_ms':stats([x['duration_ms'] for x in group]),
                            'exclusive_ms':exclusive,
                            'inclusive_total_ms':sum(x['duration_ms'] for x in group),
                            'exclusive_total_ms':None if ambiguous else sum(max(0.,x['duration_ms']-x['children_ms']) for x in group)})
        if ambiguous: warnings.append(f'Track {key}: non-nested overlapping spans; exclusive times unavailable.')
        track_reports.append({'pid':key[0],'tid':key[1],'label':names.get(key,''),
                              'covered_ms':union_length([(x['start_ms'],x['end_ms']) for x in items]),
                              'span_count':len(items),'exclusive_valid':not ambiguous})
    ranking.sort(key=lambda x:-(x['exclusive_total_ms'] if x['exclusive_total_ms'] is not None else x['inclusive_total_ms']))
    if ignored: warnings.append('Unsupported/instant/flow event phases counted but not included in duration attribution; use Perfetto for full trace semantics.')
    return {'evidence':'imported trace spans; duration is not CPU utilization or proof of GPU hardware execution',
            'tracks':track_reports,'ranked_spans':ranking,'counter_samples':counters,
            'timeline':sorted(slices,key=lambda x:x['start_ms'])[:5000],
            'timeline_truncated':len(slices)>5000,'ignored_phases':dict(ignored),'warnings':warnings,
            'interpretation':['CPU/GPU and thread durations overlap; track totals must not be added into frame time.',
                              'Wait spans identify blocking locations; cause and critical path need external scheduler/queue evidence.',
                              'No CPU-to-GPU clock alignment or dependency flows are invented.',
                              'Chrome JSON timestamps/durations use microseconds; displayTimeUnit is only a viewer preference.']}
