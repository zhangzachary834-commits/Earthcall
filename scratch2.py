import os
import glob
from collections import defaultdict
import re

docs_dir = "docs"
md_files = glob.glob(f"{docs_dir}/**/*.md", recursive=True)

for md_file in md_files:
    if "interrelations" in md_file:
        with open(md_file, "r") as f:
            content = f.read()

        # find [text](link.md)
        links = re.findall(r'\[.*?\]\((.*?\.md)\)', content)
        print(f"--- {md_file} ---")
        for link in links:
            print(f"  {link}")
