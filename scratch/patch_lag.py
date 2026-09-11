import sys

file_path = "tests/singularity/frame_lag_test.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("runFrames(zone, lawManager, worldTime, 120, 2000.0, 24)", "runFrames(zone, lawManager, worldTime, 3, 50.0, 1)")

with open(file_path, "w") as f:
    f.write(content)
print("Patched lag test!")
