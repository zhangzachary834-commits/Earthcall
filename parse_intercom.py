import os
import re
import datetime
from pathlib import Path

intercom_dir = Path("/Users/zacharyzhang/Documents/GitHub/Earthcall/agent intercom/communication-threads/ontomath-light-and-image")
files = list(intercom_dir.glob("*.md"))

data = []
for f in files:
    content = f.read_text()
    title = ""
    date_str = ""
    pr = ""
    lines = content.split('\n')
    for line in lines[:20]:
        if line.startswith("# ") and not title:
            title = line[2:].strip()
        elif line.startswith("Date: "):
            date_str = line.split("Date: ")[1].strip()
        elif line.startswith("PR: "):
            pr = line.split("PR: ")[1].strip()
            # Extract PR number
            match = re.search(r'#(\d+)', pr)
            if match:
                pr = match.group(1)
                
    # Also get modified time as fallback
    mtime = f.stat().st_mtime
    
    data.append({
        "path": f,
        "name": f.name,
        "title": title,
        "date_str": date_str,
        "pr": pr,
        "mtime": mtime
    })

data.sort(key=lambda x: x['mtime'])
for d in data:
    print(f"PR: {d['pr']:<5} | File: {d['name']}")
