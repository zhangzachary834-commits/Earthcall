#!/usr/bin/env python3
import runpy
import subprocess

# Temporary branch-only bridge: the large topology patch already landed. Run
# the bounded regression-test patch, then commit it from the authenticated CI
# checkout. This helper is deleted as soon as the commit lands.
runpy.run_path("scripts/sol_one_shot_shape_tests.py", run_name="__main__")

subprocess.run(["git", "diff", "--check"], check=True)
subprocess.run([
    "git", "add", "tests/constructed-being/shape_hydration_integrity_test.cpp"
], check=True)

staged = subprocess.run(["git", "diff", "--cached", "--quiet"])
if staged.returncode != 0:
    subprocess.run(["git", "config", "user.name", "GPT-5.6 Sol"], check=True)
    subprocess.run([
        "git", "config", "user.email",
        "41898282+github-actions[bot]@users.noreply.github.com"
    ], check=True)
    subprocess.run([
        "git", "commit", "-m", "Add adversarial matter authority regression coverage"
    ], check=True)
    subprocess.run([
        "git", "push", "origin", "HEAD:sol/shape-hydration-integrity-20260916"
    ], check=True)
else:
    print("adversarial matter tests already landed")
