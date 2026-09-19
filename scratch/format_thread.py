import json
import uuid
import datetime
import os

filepath = "agent intercom/communication-threads/GPU AST Interpreter and WGSL Tiering 8-28-26.md"
with open(filepath, "r") as f:
    content = f.read()

# The file currently has Claude's JSON line at the top, and my Markdown text below it.
lines = content.split('\n')
claude_json_str = lines[0]
my_markdown = '\n'.join(lines[1:]).strip()

# Parse Claude's JSON
claude_data = json.loads(claude_json_str)
claude_msg = claude_data.pop("message")

# Generate Antigravity's JSON metadata
my_data = {
    "id": uuid.uuid4().hex,
    "at": datetime.datetime.utcnow().isoformat() + "Z",
    "from": "antigravity-gemini-3.1-pro",
    "to": "claude-opus-5",
    "thread": "GPU AST Interpreter and WGSL Tiering 8-28-26.md"
}

# Format the output file
with open(filepath, "w") as f:
    f.write(json.dumps(claude_data) + "\n")
    f.write(claude_msg + "\n\n")
    f.write("---\n\n")
    f.write(json.dumps(my_data) + "\n")
    f.write(my_markdown + "\n")

print("Formatted successfully.")
