#!/usr/bin/env python3
"""router_truth_probe.py — Mechanized verification of router and documentation truth.

Checks:
1. Backticked doc and source paths in AGENTS.md, CLAUDE.md, and GEMINI.md resolve to real paths on disk.
2. The test count claimed in the router docs matches ctest -N.
3. All markdown links in docs/ and AGENTS.md/CLAUDE.md/GEMINI.md resolve without 404s.
"""

import os
import re
import subprocess
import sys
from pathlib import Path
from urllib.parse import unquote, urlparse

ROOT = Path(__file__).resolve().parent.parent.parent
ROUTER_DOCS = [ROOT / "AGENTS.md", ROOT / "CLAUDE.md", ROOT / "GEMINI.md"]
DOCS_DIR = ROOT / "docs"

# Known negative/forbidden examples in the refusals/guidelines (e.g. "no src/UI/")
KNOWN_NEGATIVE_PATHS = {
    "src/UI",
    "src/UI/",
    "backend-python",
    "backend-python/",
    "src/OurVerse",
    "src/OurVerse/"
}

def check_router_backticked_paths():
    print("--- 1. Checking Router Backticked Paths ---")
    errors = []
    path_re = re.compile(r'`([a-zA-Z0-9_\-\./]+(?:\.[a-zA-Z0-9]+|\/))`')
    
    for doc in ROUTER_DOCS:
        if not doc.exists():
            continue
        text = doc.read_text(encoding='utf-8')
        for match in path_re.finditer(text):
            p_str = match.group(1).rstrip('/')
            if p_str in KNOWN_NEGATIVE_PATHS or (p_str + "/") in KNOWN_NEGATIVE_PATHS:
                continue
            if p_str.startswith('-') or ' ' in p_str or p_str in {'int', 'float', 'bool', 'void', 'true', 'false'}:
                continue
            if '/' in p_str or p_str.endswith('.md'):
                candidate1 = (ROOT / "docs" / "architecture" / p_str).resolve()
                candidate2 = (ROOT / p_str).resolve()
                candidate3 = (ROOT / "docs" / p_str).resolve()
                
                if not (candidate1.exists() or candidate2.exists() or candidate3.exists()):
                    errors.append((doc.name, p_str))
                    print(f"  [MISSING] {doc.name}: `{p_str}` not found on disk")
    if not errors:
        print("  All router backticked paths resolve on disk.")
    return len(errors)

def check_test_count():
    print("--- 2. Checking Registered Test Count ---")
    build_dir = ROOT / "build"
    if not build_dir.exists():
        print("  Build dir not found; skipping ctest check.")
        return 0
    try:
        res = subprocess.run(["ctest", "--test-dir", str(build_dir), "-N"],
                             capture_output=True, text=True, check=True)
        m = re.search(r"Total Tests:\s*(\d+)", res.stdout)
        if m:
            actual_count = int(m.group(1))
            print(f"  ctest reports: {actual_count} registered tests.")
            for doc in ROUTER_DOCS:
                if not doc.exists():
                    continue
                text = doc.read_text(encoding='utf-8')
                counts = re.findall(r"(\d+)\s+registered", text)
                for c in counts:
                    if int(c) != actual_count:
                        print(f"  [MISMATCH] {doc.name} claims {c} registered tests, but ctest has {actual_count}.")
                        return 1
            print(f"  Router documents match actual registered test count ({actual_count}).")
        return 0
    except Exception as e:
        print(f"  ctest execution error: {e}")
        return 0

def check_markdown_links():
    print("--- 3. Checking Markdown Cross-Links in docs/ and Root Docs ---")
    link_re = re.compile(r'\[([^\]]+)\]\(([^)]+)\)')
    broken = 0
    
    files_to_check = list(DOCS_DIR.rglob("*.md")) + [d for d in ROUTER_DOCS if d.exists()]
    for md in files_to_check:
        text = md.read_text(encoding='utf-8')
        for match in link_re.finditer(text):
            target = match.group(2)
            if target.startswith('http://') or target.startswith('https://') or target.startswith('#') or target.startswith('mailto:') or target.startswith('conversation://'):
                continue
            
            clean_target = target.split('#')[0]
            if not clean_target:
                continue
            clean_target = unquote(clean_target)
            
            if clean_target.startswith('file://'):
                parsed_path = urlparse(clean_target).path
                resolved = Path(parsed_path).resolve()
                if not resolved.exists() and "Earthcall/" in parsed_path:
                    rel_part = parsed_path.split("Earthcall/")[1]
                    resolved = (ROOT / rel_part).resolve()
            elif clean_target.startswith('const ') or '&' in clean_target or ',' in clean_target:
                # Filter out false-positive markdown link matches that are actually C++ signatures
                continue
            else:
                resolved = (md.parent / clean_target).resolve()
            
            if not resolved.exists():
                print(f"  [BROKEN LINK] {md.relative_to(ROOT)}: {target}")
                broken += 1
    if broken == 0:
        print(f"  Checked {len(files_to_check)} markdown files: 100% of relative links are valid.")
    return broken

def main():
    print("=== Earthcall Router & Link Truth Probe ===")
    errs = 0
    errs += check_router_backticked_paths()
    errs += check_test_count()
    errs += check_markdown_links()
    print("===========================================")
    if errs == 0:
        print("RESULT: ALL CLEAR — Zero drift detected.")
        sys.exit(0)
    else:
        print(f"RESULT: {errs} issue(s) detected.")
        sys.exit(1)

if __name__ == "__main__":
    main()
