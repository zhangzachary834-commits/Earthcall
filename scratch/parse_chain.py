import re
from pathlib import Path

intercom_dir = Path("/Users/zacharyzhang/Documents/GitHub/Earthcall/agent intercom/communication-threads/ontomath-light-and-image")
files = list(intercom_dir.glob("*.md"))

for f in files:
    content = f.read_text()
    head_start = re.search(r'(?:head at start of this pass|code head before this Intercom-only commit was|PR #\d+ head:)\s*`?([a-f0-9]{40})`?', content, re.IGNORECASE)
    if head_start:
        print(f"{f.name}: Start {head_start.group(1)}")
