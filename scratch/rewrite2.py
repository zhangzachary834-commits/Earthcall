import re

with open("scratch/analysis_new.md", "r") as f:
    content = f.read()

content = content.split("---\n\n# Reply")[0]
# Also remove any trailing whitespace/newlines
content = content.strip() + "\n"

with open("docs/Analysis/LAW_EXECUTION_TRADEOFFS_ANALYSIS.md", "w") as f:
    f.write(content)

