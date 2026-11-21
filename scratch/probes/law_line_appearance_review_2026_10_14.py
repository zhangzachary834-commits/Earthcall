#!/usr/bin/env python3
"""Link the review counterexamples against the current focused-test objects.

Build line_editor_test first. No CMake registration, production edit or live save
is needed. Codex / GPT-6.1 Sol / 01a10992-828e-7e80-890c-c64b09141e18 / 2026-10-14 PDT.
"""
from pathlib import Path
import shlex
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[2]
build = repo / 'build'
source = Path(__file__).with_suffix('.cpp')
lines = (build / 'CMakeFiles/line_editor_test.dir/flags.make').read_text().splitlines()
flags = {line.split(' = ', 1)[0]: shlex.split(line.split(' = ', 1)[1]) for line in lines if ' = ' in line}
link = shlex.split((build / 'CMakeFiles/line_editor_test.dir/link.txt').read_text())
with tempfile.TemporaryDirectory(prefix='earthcall-appearance-review-build-') as temp:
    obj = str(Path(temp) / 'review.o')
    binary = str(Path(temp) / 'review')
    subprocess.run([link[0], *flags['CXX_DEFINES'], *flags['CXX_INCLUDES'], *flags['CXX_FLAGS'],
                    '-c', str(source), '-o', obj], cwd=build, check=True)
    link = [obj if part.endswith('/line_editor_test.cpp.o') else part for part in link]
    link[link.index('-o') + 1] = binary
    subprocess.run(link, cwd=build, check=True)
    subprocess.run([binary], cwd=repo, check=True)
