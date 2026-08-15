import shutil
import os

src = "/Users/zacharyzhang/Documents/GitHub/Earthcall/src/Integration/py/robotics"
dst = "/Users/zacharyzhang/Documents/GitHub/Earthcall/src/Singularity/Physical/py"

if not os.path.exists(dst):
    os.makedirs(dst)

shutil.move(src, os.path.join(dst, "robotics"))
print("Moved robotics")
