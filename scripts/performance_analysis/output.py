"""Self-contained offline report; all input strings are escaped, never evaluated."""
import html
import json
from pathlib import Path


def render_html(report):
    esc = lambda x: html.escape(str(x),quote=True)
    def table(headers,rows):
        return '<div class="scroll"><table><thead><tr>'+''.join('<th>'+esc(x)+'</th>' for x in headers)+'</tr></thead><tbody>'+''.join('<tr>'+''.join('<td>'+esc(x)+'</td>' for x in row)+'</tr>' for row in rows)+'</tbody></table></div>'
    sections = ['<h1>Earthcall · performance evidence</h1><p>Separate offline analysis · '+esc(report['created_at'])+'</p>',
                '<p class="notice">Measured spans, instrumented counts and structural estimates are separate evidence. This report does not certify a rendering optimization.</p>']
    if report.get('context'):
        label=report['context'].get('label','Run description supplied with inputs') if isinstance(report['context'],dict) else 'Run description supplied with inputs'
        sections.append('<details><summary>'+esc(label)+'</summary><pre>'+esc(json.dumps(report['context'],indent=2))+'</pre></details>')
    warnings = report.get('gaps',[])[:]
    frames = report.get('frames')
    if frames:
        warnings.extend(frames['warnings'])
        sections.append('<h2>Frame samples</h2><p>Separate contexts; milliseconds. These nested wall-clock timers are not additive. Surface waits do not establish their cause.</p>')
        for group in frames['groups']:
            c = group['context']
            sections.append('<h3>'+esc(' · '.join(str(c[k]) for k in ('run','phase','zone','view','width','height','backend')))+f" · {group['frames']} frames</h3>")
            rows = [(k,v['n'],round(v['p50'],4),round(v['p95'],4),round(v['mean'],4)) for k,v in sorted(group['timings'].items(),key=lambda kv:-kv[1]['p95'])]
            sections.append(table(['CPU wall-clock metric','n','p50 ms','p95 ms','mean ms'],rows))
            if group['counters']:
                sections.append(table(['Counter','samples','total','mean'],[(k,v['n'],v['total'],round(v['mean'],3)) for k,v in group['counters'].items()]))
        sections.append('<p>Excluded records: '+esc(json.dumps(frames['excluded']))+'</p>')
        sections.append('<h3>Delayed GPU samples</h3>'+table(['Metric','source frame','ms','source context'],[(k,x['source_frame'],x['ms'],json.dumps(x['context'])) for k,xs in frames['gpu_samples'].items() for x in xs]))
        sections.append('<p>Unavailable samples: '+esc(json.dumps(frames['gpu_unavailable']))+'</p>')
    trace = report.get('trace')
    if trace:
        warnings.extend(trace['warnings'])
        sections.append('<h2>Trace durations</h2><p>Exclusive ranking is per track. Cross-thread and CPU/GPU durations cannot be added. Trace spans may include waiting.</p>')
        sections.append(table(['Process/thread','Category','Span','n','inclusive total ms','exclusive total ms'],[(x['pid']+'/'+x['tid'],x['category'],x['name'],x['inclusive_ms']['n'],round(x['inclusive_total_ms'],4),None if x['exclusive_total_ms'] is None else round(x['exclusive_total_ms'],4)) for x in trace['ranked_spans']]))
        items = trace['timeline']
        if items:
            first=min(x['start_ms'] for x in items);last=max(x['end_ms'] for x in items);span=max(last-first,.001)
            tracks=sorted(set((x['pid'],x['tid']) for x in items));height=40*len(tracks)+40
            track_index={key:i for i,key in enumerate(tracks)}
            svg=[f'<svg role="img" aria-label="Trace timeline" viewBox="0 0 1000 {height}" width="100%">']
            for idx,key in enumerate(tracks):
                svg.append(f'<text x="5" y="{30+idx*40}">{esc("/".join(key))}</text>')
            for item in sorted(items,key=lambda x:-x['duration_ms']):
                y=16+track_index[(item['pid'],item['tid'])]*40
                x=120+(item['start_ms']-first)/span*870;w=max(.5,item['duration_ms']/span*870)
                svg.append(f'<rect x="{x:.3f}" y="{y}" width="{w:.3f}" height="22"><title>{esc(item["name"])} · {item["duration_ms"]:.4f} ms</title></rect>')
            svg.append('</svg>'); sections.append(''.join(svg))
            sections.append('<p>Timeline tooltips show durations. Nested spans share a track; tables carry attribution. '+('Timeline limited to first 5,000 spans; summaries cover all.' if trace['timeline_truncated'] else '')+'</p>')
    demand = report.get('demand')
    if demand:
        warnings.extend(demand['warnings']);sections.append('<h2>Instrumented demand census</h2><p>Selected-fragment counts only. No conversion to milliseconds or full-frame totals.</p>')
        for row in demand['rows']:
            sections.append('<h3>'+esc(f"{row['view']} · t={row['time']} · {row['framebuffer']}")+'</h3><p>Pixels identical: '+esc(row['pixels_identical'])+' · maximum channel difference: '+esc(row['max_error'])+'</p>')
            sections.append(table(['Medium','Observed operation','count'],[(x['medium'],x['operation'],x['count']) for x in row['ranked_counts']]))
    shader=report.get('shader')
    if shader:
        warnings.extend(shader['warnings']);sections.append('<h2>Emitted WGSL structure</h2><p>Syntax only: call sites do not establish runtime calls, surviving instructions or costs.</p>')
        sections.append(table(['Reachable call','syntactic sites'],sorted(shader['reachable_call_sites'].items(),key=lambda x:-x[1])))
        sections.append(table(['Function','source line','calls'],[(x['name'],x['line'],json.dumps(x['calls'])) for x in shader['functions']]))
    model=report.get('model')
    if model:
        warnings.extend(model['warnings']);sections.append('<h2>Explicit complexity model</h2><p>Assumptions supplied by the model. Counts are not measured runtime work or time predictions.</p>')
        sections.append(table(['Term','Cadence','Complexity','modeled count','unit','assumptions'],[(x['name'],x['cadence'],x['complexity'],x['modeled_count'],x['unit'],json.dumps(x['assumptions'])) for x in model['terms']]))
    artifact=report.get('artifact')
    if artifact:
        sections.append('<h2>Historical native fidelity artifact</h2><p>'+esc(artifact['scope'])+'</p><p>Compilation records: '+esc(artifact['module_records'])+' · no conversion to compilation or GPU milliseconds.</p>')
        sections.append(table(['View','Clock','Lane','Changed bytes','Maximum channel difference'],[(x.get('camera'),x.get('time'),x.get('lane'),x.get('changed_bytes'),x.get('maximum_channel_difference')) for x in artifact['images']]))
    if report.get('comparison'):
        c=report['comparison'];warnings.extend(c['warnings']);sections.append('<h2>Exploratory A/B comparison</h2><p>Matched context; unpaired samples. No statistical significance or retention verdict is inferred.</p>')
        sections.append(table(['Context','Metric','Baseline p50','Candidate p50','p50 change %','p95 change %'],[(json.dumps(x['context']),x['metric'],x['baseline']['p50'],x['candidate']['p50'],x['p50_change_percent'],x['p95_change_percent']) for x in c['rows']]))
    if report.get('scaling'):
        scaling=report['scaling'];warnings.extend(scaling['warnings'])
        sections.append('<h2>Observed pixel scaling</h2><p>'+esc(scaling['evidence'])+'</p>')
        sections.append(table(['Fixed context','Metric','Observed slope','Points'],[(json.dumps(x['fixed_context']),x['metric'],round(x['observed_log_log_slope'],5),json.dumps(x['points'])) for x in scaling['fits']]))
    sections.append('<h2>Limits and next measurements</h2><ul>'+''.join('<li>'+esc(x)+'</li>' for x in sorted(set(warnings)))+'</ul>')
    sections.append('<h2>Input provenance</h2>'+table(['Input path','Bytes','SHA256'],[(x['path'],x['bytes'],x['sha256']) for x in report['inputs']]))
    return '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Earthcall performance evidence</title>
<style>body{font:16px/1.5 system-ui;background:#0d1523;color:#e5edf9;max-width:1200px;margin:32px auto;padding:0 20px}h1,h2,h3{color:#9cd6ec}h2{margin-top:42px}p,li{max-width:100ch}.notice{border-left:4px solid #e5bc68;padding:12px;background:#182538}.scroll{overflow:auto}table{border-collapse:collapse;width:100%;font-size:14px;margin:12px 0}th,td{border-bottom:1px solid #30415b;text-align:left;padding:8px;vertical-align:top;overflow-wrap:anywhere}th{background:#182538}pre{white-space:pre-wrap}input{padding:10px;background:#182538;color:white;border:1px solid #617894;width:min(440px,90%)}rect{fill:#4c91b9;stroke:#c4eaff;stroke-width:.4}svg text{fill:#e5edf9;font-size:14px}</style>
<label>Filter table rows <input id="filter" type="search" placeholder="e.g. wait_surface, noise, compiler"></label>'''+''.join(sections)+'''<script>document.getElementById('filter').addEventListener('input',e=>{let q=e.target.value.toLowerCase();document.querySelectorAll('tbody tr').forEach(r=>r.hidden=!r.textContent.toLowerCase().includes(q));});</script></html>'''


def write_report(directory, report):
    directory=Path(directory)
    directory.mkdir(parents=True,exist_ok=False)
    (directory/'report.json').write_text(json.dumps(report,indent=2,allow_nan=False)+'\n')
    (directory/'report.html').write_text(render_html(report))
    lines=['Earthcall performance evidence',report['created_at'],'']
    if report.get('frames'):
        for g in report['frames']['groups']:
            lines.append(json.dumps(g['context']))
            for k,v in sorted(g['timings'].items(),key=lambda kv:-kv[1]['p95']):
                lines.append(f"  {k}: p50 {v['p50']:.3f} ms; p95 {v['p95']:.3f} ms; n={v['n']}")
    for key in ('trace','demand','shader','model','artifact','scaling'):
        if report.get(key): lines.append(key+': '+report[key]['evidence'])
    lines.extend(['','Gaps:']+['- '+x for x in report.get('gaps',[])])
    (directory/'summary.txt').write_text('\n'.join(lines)+'\n')
