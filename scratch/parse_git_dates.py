import subprocess
from pathlib import Path

intercom_dir = Path("/Users/zacharyzhang/Documents/GitHub/Earthcall/agent intercom/communication-threads/ontomath-light-and-image")
files = list(intercom_dir.glob("*.md"))
data = []
for f in files:
    # get commit date
    cmd = ["git", "log", "-1", "--format=%ct", "--", str(f)]
    try:
        timestamp_str = subprocess.check_output(cmd).decode().strip()
        timestamp = int(timestamp_str) if timestamp_str else 0
    except:
        timestamp = 0
    data.append((timestamp, f.name))

data.sort()
for ts, name in data:
    print(f"{ts} - {name}")
