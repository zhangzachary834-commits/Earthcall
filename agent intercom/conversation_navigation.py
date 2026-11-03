#!/usr/bin/env python3
"""Repository document navigation, integrated with the existing Intercom CLI.

Workshop tool commissioned by Zach: keep replies connected to their conversation.
Codex / GPT-6.1 Sol / 01a122ec-b377-7391-ad6f-86d11b501d1b / 2026-10-09.
Source documents remain canonical; explicit connections are an append-only journal.
SQLite FTS5 is an in-memory search accelerator, never a second document store.
"""
from __future__ import annotations

import argparse
from collections import defaultdict, deque
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import re
import sqlite3
import tempfile
import unicodedata
from urllib.parse import unquote, urlsplit
import uuid

ROOT = Path(__file__).resolve().parents[1]
JOURNAL = 'agent intercom/conversation_links.jsonl'
LINEAGE = {'responds-to', 'continues', 'reflects-on', 'summarizes'}
NAV = re.compile(rb'<!-- NAV_BLOCK_START -->.*?<!-- NAV_BLOCK_END -->', re.S)
SUFFIXES = {'.md', '.txt'}


def without_nav(raw):
    return NAV.sub(b'', raw)


def corpus(root):
    paths = []
    for name in ('docs', 'agent intercom'):
        base = root / name
        if base.exists():
            for folder, dirs, files in os.walk(base, followlinks=False):
                dirs[:] = sorted(d for d in dirs if not d.startswith('.') and d != '__pycache__')
                paths.extend(Path(folder) / f for f in sorted(files)
                             if not f.startswith('.') and Path(f).suffix.lower() in SUFFIXES)
    paths.extend(root / f for f in ('README.md', 'AGENTS.md') if (root / f).exists())
    return sorted(p for p in paths if not p.is_symlink() and p.resolve().is_relative_to(root))


def slug(text):
    text = re.sub(r'<[^>]*>', '', text).replace('`', '')
    return ''.join(c for c in unicodedata.normalize('NFC', text.lower())
                   if c.isalnum() or c in '_- ').replace(' ', '-')


def anchors(text):
    result, counts = {}, defaultdict(int)
    fenced = None
    for n, line in enumerate(text.splitlines(), 1):
        fence = re.match(r"^\s*(`{3,}|~{3,})", line)
        if fence:
            if fenced is None: fenced = fence[1][0]
            elif fence[1][0] == fenced: fenced = None
            continue
        if fenced: continue
        match = re.match(r'^#{1,6}\s+(.+?)(?:\s+#+)?\s*$', line)
        if match:
            key = slug(match[1]); occurrence = counts[key]; counts[key] += 1
            result[key + (f'-{occurrence}' if occurrence else '')] = n
        for match in re.finditer(r'<(?:a|[a-z]+)\b[^>]*\bid=["\']([^"\']+)["\']', line):
            result[match[1]] = n
        try:
            message = json.loads(line)
            if isinstance(message, dict) and 'id' in message:
                result[str(message['id'])] = n
        except ValueError:
            pass
    return result


def destinations(text):
    """Recognize inline/ref Markdown and exact backtick file citations.

    Balanced parentheses permit filenames/URLs containing parentheses. Images,
    fenced code and old generated navigation are not conversation references.
    References never imply a response relationship; only the journal does that.
    """
    lines = text.splitlines(); definitions = {}; fenced = None
    for n, line in enumerate(lines, 1):
        fence = re.match(r'^\s*(`{3,}|~{3,})', line)
        if fence:
            if fenced is None: fenced = fence[1][0]
            elif fence[1][0] == fenced: fenced = None
            continue
        if fenced: continue
        definition = re.match(r'^\s*\[([^]]+)\]:\s*(?:<([^>]+)>|(\S+))', line)
        if definition:
            definitions[definition[1].casefold()] = definition[2] or definition[3]
            yield n, definition[2] or definition[3]
        for match in re.finditer(r'(?<!!)\[[^]\n]+\]\(', line):
            start = match.end(); depth = 1; end = start
            while end < len(line) and depth:
                if line[end] == '\\': end += 2; continue
                if line[end] == '(': depth += 1
                elif line[end] == ')': depth -= 1
                end += 1
            if depth: continue
            value = line[start:end-1].strip()
            if value.startswith('<') and '>' in value: value = value[1:value.index('>')]
            else: value = re.sub(r'\s+["\'].*["\']\s*$', '', value)
            yield n, value
        for match in re.finditer(r'`([^`\n]+\.(?:md|txt)(?:#[^`\n]+)?)`', line):
            if not match[1].startswith(("nav ", "python", "git ")) and '"' not in match[1]:
                yield n, match[1]
    for n, line in enumerate(lines, 1):
        for match in re.finditer(r'(?<!!)\[([^]]+)\]\[([^]]*)\]', line):
            key = (match[2] or match[1]).casefold()
            if key in definitions: yield n, definitions[key]


class Archive:
    def __init__(self, root):
        self.root = root.resolve(); self.docs = {}; self.edges = []; self.issues = []
        self.database = sqlite3.connect(':memory:')
        try:
            self.database.execute('CREATE VIRTUAL TABLE search USING fts5(path UNINDEXED, title, body, tokenize="unicode61")')
            self.fts = True
        except sqlite3.OperationalError:
            self.fts = False
        for path in corpus(self.root):
            key = path.relative_to(self.root).as_posix()
            text = without_nav(path.read_bytes()).decode('utf-8', errors='replace')
            title = next((m[1].strip() for line in text.splitlines()
                          if (m := re.match(r'^#\s+(.+)', line))), path.stem)
            kind = 'document'
            if 'communication-threads/' in key: kind = 'chat'
            if 'Monastery/' in key or '/Reflections' in key or "Earthcall's Crystal/" in key: kind = 'reflection'
            if path.name in ('README.md', '00_THREAD_INDEX.md'): kind = 'index'
            self.docs[key] = dict(path=key, title=title, kind=kind, text=text,
                                  anchors=anchors(text), lines=len(text.splitlines()))
            if self.fts:
                self.database.execute('INSERT INTO search VALUES (?, ?, ?)', (key, title, text))
        self.by_name = defaultdict(list)
        for key in self.docs: self.by_name[Path(key).name.casefold()].append(key)
        for key, doc in self.docs.items():
            seen = set()
            for line, dest in destinations(doc['text']):
                target, problem = self.reference(key, dest)
                if target and target != key and target not in seen:
                    seen.add(target)
                    self.edges.append(dict(source=key, target=target, relation='references',
                                           origin='document', line=line))
                if problem: self.issues.append(dict(path=key, line=line, target=dest, problem=problem))
        try:
            self.records = read_journal(self.root / JOURNAL)
        except ValueError:
            self.close()
            raise
        active = active_records(self.records)
        for edge in active.values():
            for end in ('source', 'target'):
                try: self.select(edge[end], exact=True)
                except ValueError as error: self.issues.append(dict(path=JOURNAL, line=0, target=edge[end], problem=str(error)))
            self.edges.append(dict(edge, origin='journal'))

    def close(self):
        self.database.close()

    def reference(self, source, value):
        value = value.strip().replace('\\(', '(').replace('\\)', ')')
        if re.match(r'^[a-zA-Z][\w+.-]*://', value) or value.startswith(('mailto:', 'data:', 'codex:', 'app:')):
            return None, None
        parsed = urlsplit(value); path = unquote(parsed.path); fragment = unquote(parsed.fragment)
        if path and Path(path).suffix.lower() not in SUFFIXES: return None, None
        if not path and not fragment: return None, None
        candidate = self.root / source if not path else self.root / Path(source).parent / path
        choices = [candidate]
        if path:
            choices.append(self.root / path)
            if source.startswith('agent intercom/'):
                choices.append(self.root / 'agent intercom' / path)
            if path.startswith(('ontology/', 'law/', 'events/', 'mathematics/', 'ourverse/', 'migration/', 'Design/', 'Integration/')):
                choices.append(self.root / 'docs/architecture' / path)
        # An exact bare filename may denote a unique existing document, as old
        # Intercom prose often does; duplicates must never be silently selected.
        for p in choices:
            p = p.resolve()
            if not p.is_relative_to(self.root): continue
            key = p.relative_to(self.root).as_posix()
            if key in self.docs:
                target = key + (f'#{fragment}' if fragment else '')
                if fragment and fragment not in self.docs[key]['anchors']:
                    return target, 'missing anchor'
                return target, None
            if p.is_file(): return None, None  # valid source/artifact outside the document corpus
        if path and '/' not in path and len(self.by_name[path.casefold()]) == 1:
            key = self.by_name[path.casefold()][0]
            if fragment and fragment not in self.docs[key]['anchors']: return key + '#' + fragment, 'missing anchor'
            return key + (f'#{fragment}' if fragment else ''), None
        if '<' in path or '*' in path: return None, None  # documented template
        return None, 'ambiguous filename' if len(self.by_name[Path(path).name.casefold()]) > 1 else 'missing document'

    def select(self, value, exact=False):
        path, _, fragment = value.partition('#'); path = unquote(path); fragment = unquote(fragment)
        p = Path(path)
        choices = [p.resolve(), (self.root / path).resolve(), (self.root / 'agent intercom' / path).resolve()]
        keys = []
        for p in choices:
            if p.is_relative_to(self.root):
                key = p.relative_to(self.root).as_posix()
                if key in self.docs and key not in keys: keys.append(key)
        if not keys and not exact: keys = self.by_name[Path(path).name.casefold()]
        if len(keys) != 1:
            raise ValueError(f'document must resolve uniquely: {value}' + ('\n  ' + '\n  '.join(keys) if keys else ''))
        key = keys[0]
        if fragment and fragment not in self.docs[key]['anchors']:
            raise ValueError(f'missing heading/message anchor: {key}#{fragment}')
        return key + (f'#{fragment}' if fragment else '')

    def find(self, query, limit):
        tokens = re.findall(r'\w+', query, re.UNICODE)
        if not tokens: return list(self.docs.values())[:limit]
        if self.fts:
            match = ' AND '.join('"' + t.replace('"', '""') + '"' for t in tokens)
            keys = [r[0] for r in self.database.execute(
                'SELECT path FROM search WHERE search MATCH ? ORDER BY bm25(search, 0, 4, 1), path LIMIT ?', (match, limit))]
        else:
            keys = [k for k, d in self.docs.items()
                    if all(t.casefold() in (k+' '+d['title']+' '+d['text']).casefold() for t in tokens)][:limit]
        return [self.docs[k] for k in keys]


def read_journal(path):
    if not path.exists(): return []
    raw = path.read_bytes()
    if raw and not raw.endswith(b'\n'): raise ValueError(f'{path}: incomplete journal tail; repair before appending')
    records = []; ids = set()
    for n, line in enumerate(raw.splitlines(), 1):
        try:
            entry = json.loads(line)
            if entry.get('version') != 1 or entry.get('op') not in ('connect', 'disconnect'):
                raise ValueError('unsupported record')
            required = ('id', 'at', 'by', 'source', 'target', 'relation')
            if not all(isinstance(entry.get(k), str) and entry[k].strip() for k in required):
                raise ValueError('missing record fields')
            if entry['id'] in ids: raise ValueError('duplicate record id')
            ids.add(entry['id']); records.append(entry)
        except (ValueError, AttributeError) as error:
            raise ValueError(f'{path}:{n}: invalid connection journal: {error}') from error
    return records


def edge_key(edge):
    return edge['source'], edge['target'], edge['relation']


def active_records(records):
    active = {}
    for record in records:
        key = edge_key(record)
        if record['op'] == 'connect': active[key] = record
        else: active.pop(key, None)
    return active


def connect(archive, args):
    from conversation_history_injection import mutation_lock
    source = archive.select(args.source); target = archive.select(args.target)
    if source == target: raise ValueError('a document cannot connect to itself at the same anchor')
    relation = args.relation.strip(); author = args.by.strip()
    if not relation or not author: raise ValueError('relationship and session attribution are required')
    entry = dict(version=1, id=uuid.uuid4().hex, at=datetime.now(timezone.utc).isoformat(),
                 by=author, op='disconnect' if args.action == 'unlink' else 'connect',
                 source=source, target=target, relation=relation, note=args.note or '')
    path = archive.root / JOURNAL
    if not path.resolve().is_relative_to(archive.root): raise ValueError('connection journal escapes repository root')
    path.parent.mkdir(parents=True, exist_ok=True)
    with mutation_lock(path):
        records = read_journal(path); active = active_records(records); key = edge_key(entry)
        if (entry['op'] == 'connect' and key in active) or (entry['op'] == 'disconnect' and key not in active):
            return dict(changed=False, source=source, target=target, relation=relation)
        with path.open('ab') as handle:
            handle.write((json.dumps(entry, ensure_ascii=False, separators=(',', ':'))+'\n').encode())
            handle.flush(); os.fsync(handle.fileno())
    return dict(changed=True, **entry)


def connected(archive, target):
    key = target.split('#')[0]
    return sorted([e for e in archive.edges if key in (e['source'].split('#')[0], e['target'].split('#')[0])],
                  key=lambda e:(e['origin'] != 'journal', e['relation'], e['source'], e['target']))


def trace(archive, target, limit, include_references=False):
    adjacent = defaultdict(list)
    for edge in archive.edges:
        if not include_references and edge['relation'] not in LINEAGE: continue
        a, b = edge['source'].split('#')[0], edge['target'].split('#')[0]
        adjacent[a].append((b, edge)); adjacent[b].append((a, edge))
    start = target.split('#')[0]; queue = deque([(start, 0)]); seen = {start}; nodes = []; edges = []
    while queue:
        key, depth = queue.popleft(); nodes.append(dict(path=key, depth=depth))
        for other, edge in sorted(adjacent[key], key=lambda p:(p[0], p[1]['relation'])):
            if edge not in edges: edges.append(edge)
            if other not in seen and len(seen) < limit: seen.add(other); queue.append((other, depth+1))
    return dict(nodes=nodes, edges=[e for e in edges if e['source'].split('#')[0] in seen and e['target'].split('#')[0] in seen],
                truncated=any(other not in seen for key in seen for other, _ in adjacent[key]))


def tidy(archive, args):
    """Remove only identifiable machine blocks, with original-byte backups.

    Common mutation locks coordinate with Intercom sends; direct writers must
    obey the same protocol. A byte recheck refuses observed concurrent edits.
    Nothing outside marker spans is normalized, moved or reconstructed.
    """
    from conversation_history_injection import mutation_lock
    plans = []
    for key in archive.docs:
        if not key.startswith('agent intercom/'): continue
        path = archive.root / key; raw = path.read_bytes(); matches = list(NAV.finditer(raw))
        if not matches: continue
        for match in matches:
            block = match.group()
            if b'Thread Navigation:' not in block or b'[View Full Thread Index]' not in block:
                raise ValueError(f'{key}: unfamiliar generated block; refusing cleanup')
        plans.append((path, raw, without_nav(raw)))
    report = dict(files=len(plans), bytes_removed=sum(len(a)-len(b) for _, a, b in plans), applied=False)
    if not args.apply or not plans: return report
    if not args.by.strip(): raise ValueError('--by session attribution is required for cleanup')
    backup = archive.root / 'scratch/backups/intercom-navigation' / datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%S.%fZ')
    backup.mkdir(parents=True, exist_ok=False); records = []
    for path, original, updated in plans:
        with mutation_lock(path):
            if path.read_bytes() != original: raise ValueError(f'{path}: changed during cleanup; retry from current bytes')
            key = path.relative_to(archive.root).as_posix(); saved = backup / key; saved.parent.mkdir(parents=True, exist_ok=True)
            saved.write_bytes(original)
            fd, name = tempfile.mkstemp(prefix='.'+path.name+'.nav-', dir=path.parent)
            try:
                with os.fdopen(fd, 'wb') as handle:
                    handle.write(updated); handle.flush(); os.fsync(handle.fileno())
                os.chmod(name, path.stat().st_mode)
                if path.read_bytes() != original: raise ValueError(f'{key}: concurrent edit; original left intact')
                assert without_nav(saved.read_bytes()) == Path(name).read_bytes()
                os.replace(name, path)
            finally:
                if os.path.exists(name): os.unlink(name)
        records.append(dict(path=key, before=hashlib.sha256(original).hexdigest(), after=hashlib.sha256(updated).hexdigest()))
        # Persist progress after each file so a interrupted migration remains auditable.
        (backup/'manifest.json').write_text(json.dumps(dict(by=args.by, files=records), indent=2)+'\n')
    return dict(report, applied=True, backup=str(backup.relative_to(archive.root)))


BROWSER = '''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Earthcall · Connected Conversations</title><style>
:root{color-scheme:dark;font:15px system-ui;background:#10161c;color:#e8edf0}*{box-sizing:border-box}body{margin:0}header{padding:22px 26px;border-bottom:1px solid #33414c}h1{margin:0 0 8px;font-size:26px}p{color:#adbdc7}input,select,button{font:inherit;background:#1e2b34;color:inherit;border:1px solid #536571;border-radius:6px;padding:9px}input{width:100%;margin:10px 0}button{cursor:pointer;text-align:left}button:hover{border-color:#6ad4ba}main{display:grid;grid-template-columns:360px 1fr;min-height:80vh}aside{padding:16px;border-right:1px solid #33414c}#results{max-height:65vh;overflow:auto}#results button{display:block;width:100%;margin:7px 0}small{display:block;color:#9bafb9;font-size:12px;overflow-wrap:anywhere}article{padding:24px;min-width:0}#relations{display:grid;grid-template-columns:1fr 1fr;gap:20px}#relations button{display:block;width:100%;margin:6px 0}pre{max-height:65vh;overflow:auto;white-space:pre-wrap;overflow-wrap:anywhere;font:14px/1.6 ui-monospace,monospace;background:#151f27;padding:20px;border-radius:8px}a{color:#79d8c1}#status{font-size:13px}.path{overflow-wrap:anywhere}@media(max-width:850px){main{grid-template-columns:1fr}#relations{grid-template-columns:1fr}aside{border-right:0}article{padding:16px}}
</style><header><h1>Earthcall · Connected Conversations</h1><p>Follow the reply, recover its parent, and keep the human thread in view.</p><div id="status"></div></header>
<main><aside><label for="q">Find a chat, reflection, or document</label><input id="q" placeholder="Try SourceRho, Two Houses, or Law Line"><label for="kind">Document group</label> <select id="kind"><option value="">All documents</option><option>chat</option><option>reflection</option><option>document</option><option>index</option></select><div id="count"></div><div id="results"></div></aside>
<article><h2 id="title">Choose a document</h2><p class="path" id="path"></p><button id="back">← Previous document</button> <label><input type="checkbox" id="refs" style="width:auto"> Include ordinary references</label><div id="relations"><section><h3>Parents / outgoing connections</h3><div id="out"></div></section><section><h3>Replies / incoming connections</h3><div id="in"></div></section></div><p id="provenance"></p><pre id="body">Search or choose a document to read its content and connected responses.</pre></article></main>
<script id="archive" type="application/json">__DATA__</script><script>
'use strict';const data=JSON.parse(document.getElementById('archive').textContent),docs=new Map(data.docs.map(d=>[d.path,d])),el=id=>document.getElementById(id);let selected=null,history=[];
el('status').textContent=`${data.docs.length} documents · ${data.edges.filter(e=>e.origin==='journal').length} explicit connections · generated ${data.at}. Source text stays in its original file.`;
function button(text,sub,run){const b=document.createElement('button');b.textContent=text;if(sub){const s=document.createElement('small');s.textContent=sub;b.append(s)}b.onclick=run;return b}
function filter(){const terms=el('q').value.toLowerCase().split(/\\s+/).filter(Boolean),kind=el('kind').value;const list=data.docs.filter(d=>(!kind||d.kind===kind)&&terms.every(t=>(d.title+' '+d.path+' '+d.text).toLowerCase().includes(t)));const rank=d=>terms.filter(t=>d.title.toLowerCase().includes(t)).length*10+terms.filter(t=>d.path.toLowerCase().includes(t)).length;list.sort((a,b)=>rank(b)-rank(a)||a.path.localeCompare(b.path));el('count').textContent=`${list.length} matches · showing up to 60`;el('results').replaceChildren(...list.slice(0,60).map(d=>button(d.title,d.path,()=>openDoc(d.path))));}
function openDoc(target,remember=true){const [key,anchor]=target.split('#'),doc=docs.get(key);if(!doc)return;if(remember&&selected)history.push(selected);selected=target;el('title').textContent=doc.title;el('path').textContent=target;el('body').textContent=doc.text;el('provenance').textContent=anchor?`Selected section/message #${anchor}, source line ${doc.anchors[anchor]||'unknown'}.`:'Document text; connections below retain their registering session and source.';renderLinks();if(anchor&&doc.anchors[anchor]){const line=doc.anchors[anchor];el('body').textContent=doc.text.split('\\n').slice(Math.max(0,line-2)).join('\\n');}location.hash=encodeURIComponent(target);if(window.innerWidth<850)el('title').scrollIntoView({block:'start'});}
function renderLinks(){if(!selected)return;const key=selected.split('#')[0],refs=el('refs').checked;for(const [id,side,other]of[['out','source','target'],['in','target','source']]){const list=data.edges.filter(e=>e[side].split('#')[0]===key&&(refs||e.relation!=='references'));el(id).replaceChildren(...list.map(e=>button(`${e.relation} ${id==='in'?'←':'→'} ${docs.get(e[other].split('#')[0])?.title||e[other]}`,`${e[other]} · ${e.origin==='journal'?e.by+' · '+e.at:'reference at line '+e.line}`,()=>openDoc(e[other]))));if(!list.length)el(id).textContent=refs?'No connections in this direction.':'No explicit conversation connections yet; include references or register a response with the nav link command.';}}
window.addEventListener('hashchange',()=>{try{const target=decodeURIComponent(location.hash.slice(1));if(target&&target!==selected)openDoc(target,false)}catch{}});
el('q').oninput=filter;el('kind').onchange=filter;el('refs').onchange=renderLinks;el('back').onclick=()=>{const p=history.pop();if(p)openDoc(p,false)};filter();if(location.hash){try{openDoc(decodeURIComponent(location.hash.slice(1)),false)}catch{}}
</script></html>'''


def browse(archive, args):
    output = (archive.root / args.output).resolve()
    if output.suffix.lower() != '.html': raise ValueError('browser export requires an .html destination')
    if output == archive.root / JOURNAL or any(output == archive.root/k for k in archive.docs):
        raise ValueError('cannot overwrite a source document')
    payload = dict(docs=list(archive.docs.values()), edges=archive.edges, at=datetime.now(timezone.utc).isoformat())
    encoded = json.dumps(payload, ensure_ascii=False).replace('<', '\\u003c').replace('>', '\\u003e').replace('&', '\\u0026')
    output.parent.mkdir(parents=True, exist_ok=True)
    fd, temp = tempfile.mkstemp(dir=output.parent, prefix='.conversation-map-')
    try:
        with os.fdopen(fd, 'w', encoding='utf-8') as handle: handle.write(BROWSER.replace('__DATA__', encoded))
        os.replace(temp, output)
    finally:
        if os.path.exists(temp): os.unlink(temp)
    return dict(output=str(output), documents=len(archive.docs), connections=len(archive.edges))


def positive(value):
    number = int(value)
    if number < 1: raise argparse.ArgumentTypeError('use a positive line number')
    return number


def bounded(value):
    number = int(value)
    if not 1 <= number <= 200: raise argparse.ArgumentTypeError('use a bound between 1 and 200')
    return number


def add_parser(subcommands):
    nav = subcommands.add_parser('nav', help='search, trace, connect and browse chats/reflections/documents')
    nav.add_argument('--root', type=Path, default=ROOT, help='repository root (defaults to this checkout)')
    nav.add_argument('--json', action='store_true', help='machine-readable output')
    actions = nav.add_subparsers(dest='action', required=True)
    find = actions.add_parser('find', help='bounded full-text search'); find.add_argument('query'); find.add_argument('--limit', type=bounded, default=20)
    show = actions.add_parser('show', help='parents/replies/references and bounded source excerpt'); show.add_argument('document'); show.add_argument('--start', type=positive, default=1); show.add_argument('--lines', type=bounded, default=35); show.add_argument('--limit', type=bounded, default=20)
    walk = actions.add_parser('trace', help='follow the conversation across documents'); walk.add_argument('document'); walk.add_argument('--limit', type=bounded, default=40); walk.add_argument('--references', action='store_true')
    for action in ('link', 'unlink'):
        p = actions.add_parser(action, help='append an attributed connection'+(' withdrawal' if action=='unlink' else ''))
        p.add_argument('source', help='response document, optionally #heading or #message-id'); p.add_argument('target', help='parent document, optionally #heading or #message-id'); p.add_argument('--relation', default='responds-to'); p.add_argument('--by', required=True, help='harness/model/session'); p.add_argument('--note', default='')
    doctor = actions.add_parser('doctor', help='bounded report of missing links and unconnected response documents'); doctor.add_argument('--limit', type=bounded, default=30)
    export = actions.add_parser('browse', help='export a self-contained searchable document browser'); export.add_argument('--output', default='scratch/intercom-navigation.html')
    cleanup = actions.add_parser('tidy', help='preview/remove legacy generated nav blocks with preserved originals'); cleanup.add_argument('--apply', action='store_true'); cleanup.add_argument('--by', default='')
    nav.set_defaults(func=run)


def run(args):
    archive = Archive(args.root)
    try:
        return run_archive(archive, args)
    finally:
        archive.close()


def run_archive(archive, args):
    if args.action in ('link', 'unlink'): result = connect(archive, args)
    elif args.action == 'find': result = [dict(path=d['path'], title=d['title'], kind=d['kind'], lines=d['lines']) for d in archive.find(args.query, args.limit)]
    elif args.action == 'show':
        selected = archive.select(args.document); key, _, anchor = selected.partition('#'); doc = archive.docs[key]
        start = doc['anchors'].get(anchor, args.start); edges = connected(archive, selected)
        result = dict(document=selected, title=doc['title'], kind=doc['kind'], start=start,
                      excerpt=[dict(line=n, text=line) for n,line in enumerate(doc['text'].splitlines(),1) if start <= n < start+args.lines],
                      connections=edges[:args.limit], total_connections=len(edges))
    elif args.action == 'trace': result = trace(archive, archive.select(args.document), args.limit, args.references)
    elif args.action == 'browse': result = browse(archive, args)
    elif args.action == 'tidy': result = tidy(archive, args)
    else:
        explicit_sources={e['source'].split('#')[0] for e in archive.edges if e['origin']=='journal' and e['relation'] in LINEAGE}
        candidates=[k for k,d in archive.docs.items() if d['kind'] in ('chat','reflection') and k not in explicit_sources
                    and re.search(r'\b(reply|response|responding|in reply)\b',d['title']+' '+d['text'][:1800],re.I)]
        result=dict(documents=len(archive.docs), references=sum(e['origin']=='document' for e in archive.edges),
                    explicit_connections=sum(e['origin']=='journal' for e in archive.edges),
                    issues=archive.issues[:args.limit], total_issues=len(archive.issues),
                    unconnected_response_candidates=candidates[:args.limit], candidate_count=len(candidates))
    if args.json: print(json.dumps(result,ensure_ascii=False,indent=2))
    elif args.action == 'find':
        for d in result: print(f"[{d['kind']}] {d['title']}\n  {d['path']}")
    elif args.action == 'show':
        print(f"{result['title']}\n{result['document']}\n")
        for edge in result['connections']: print(f"  {edge['source']} --{edge['relation']}--> {edge['target']} [{edge['origin']}]")
        print(f"\n{result['total_connections']} connections; showing {len(result['connections'])}. Source excerpt:")
        for line in result['excerpt']: print(f"{line['line']:4d} {line['text']}")
    elif args.action == 'trace':
        for node in result['nodes']: print('  '*node['depth']+node['path'])
        for edge in result['edges']: print(f"  {edge['source']} --{edge['relation']}--> {edge['target']}")
        if result['truncated']: print('More connected documents exist; raise --limit (maximum 200).')
    else: print(json.dumps(result,ensure_ascii=False,indent=2))
    return 1 if args.action == 'doctor' and result['total_issues'] else 0
