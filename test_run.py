import subprocess
import time

p = subprocess.Popen(["./build/earthcall"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
time.sleep(2)
p.terminate()
out, err = p.communicate()
with open("test_run.log", "w") as f:
    f.write("STDOUT:\n")
    f.write(out)
    f.write("\nSTDERR:\n")
    f.write(err)
