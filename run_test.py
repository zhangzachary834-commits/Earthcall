import subprocess
import time
import os

os.environ["WGPU_LOG_LEVEL"] = "error"
proc = subprocess.Popen(["./build/earthcall"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
time.sleep(1)

# Press 'L' to open load menu? No, we can't easily simulate input this way.
# Earthcall doesn't have CLI args?
