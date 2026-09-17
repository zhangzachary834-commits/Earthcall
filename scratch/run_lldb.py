import subprocess
p = subprocess.Popen(["lldb", "build/synthesis_studio_app_test"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
out, err = p.communicate(input="run\nbt\nquit\n")
print(out)
