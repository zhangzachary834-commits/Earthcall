import re

HPP_PATH = "src/Person/Body/BodyPart/BodyPart.hpp"
with open(HPP_PATH, 'r') as f:
    hpp = f.read()

hpp = hpp.replace("AutomationState&", "Automation::State&")
hpp = hpp.replace("const AutomationState&", "const Automation::State&")

# Add include
include_str = '#include "ConstructedBeing/Singular/Object/Automation/Automation.hpp"\n'
if include_str not in hpp:
    hpp = hpp.replace('#include "Relation/Formation/Formation.hpp"', '#include "Relation/Formation/Formation.hpp"\n' + include_str)

with open(HPP_PATH, 'w') as f:
    f.write(hpp)

