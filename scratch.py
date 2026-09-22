import os
import glob
from collections import defaultdict

docs_dir = "docs"
md_files = glob.glob(f"{docs_dir}/**/*.md", recursive=True)

links = defaultdict(list)
for md_file in md_files:
    with open(md_file, "r") as f:
        content = f.read()

    for other_file in md_files:
        if other_file != md_file:
            basename = os.path.basename(other_file)
            if basename in content:
                links[md_file].append(other_file)

for f, targets in links.items():
    print(f"{f}: {len(targets)} links")
