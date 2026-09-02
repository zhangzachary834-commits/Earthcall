import re

HPP_PATH = "src/Person/Body/BodyPart/BodyPart.hpp"
with open(HPP_PATH, 'r') as f:
    hpp = f.read()

# find duplicate
first_idx = hpp.find("Object* getPrimaryObject() { return _primaryObject.get(); }")
if first_idx != -1:
    second_idx = hpp.find("Object* getPrimaryObject() { return _primaryObject.get(); }", first_idx + 1)
    if second_idx != -1:
        # replace the second occurrence and the const version
        sub = hpp[second_idx:]
        sub = sub.replace("Object* getPrimaryObject() { return _primaryObject.get(); }\n    const Object* getPrimaryObject() const { return _primaryObject.get(); }", "", 1)
        hpp = hpp[:second_idx] + sub

with open(HPP_PATH, 'w') as f:
    f.write(hpp)

