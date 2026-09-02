import re

HPP_PATH = "src/Person/Body/BodyPart/BodyPart.hpp"
with open(HPP_PATH, 'r') as f:
    hpp = f.read()

hpp = hpp.replace("AnimationClip&", "Automation::Clip&")

with open(HPP_PATH, 'w') as f:
    f.write(hpp)

