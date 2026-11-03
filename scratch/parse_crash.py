import json

with open("/Users/zacharyzhang/Library/Logs/DiagnosticReports/chess_castling_test-2026-09-18-181314.ips") as f:
    lines = f.readlines()

for line in lines:
    if line.startswith('{'):
        try:
            d = json.loads(line)
            if 'threads' in d:
                for t in d['threads']:
                    if t.get('triggered'):
                        for frame in t['frames']:
                            print(f"{frame.get('imageOffset', 0):x} in {frame.get('symbol', 'unknown')}")
        except:
            pass
