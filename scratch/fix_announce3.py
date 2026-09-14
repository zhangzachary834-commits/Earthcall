with open('src/ConstructedBeing/Singular/Property/PropertyPath.cpp', 'r') as f:
    content = f.read()

import re

old_code = """
            // (If it was a nested dict write, slot.owner might be the top-level singular)
            std::string announceName = segments.back();
            return announce(PathResult::Ok, nullptr, slot.owner, announceName);
        }
    }
"""

new_code = """
            // (If it was a nested dict write, slot.owner might be the top-level singular)
            return announce(PathResult::Ok, nullptr, slot.owner, slot.dynamicKey);
        }
    }
"""

if old_code in content:
    content = content.replace(old_code, new_code)
    with open('src/ConstructedBeing/Singular/Property/PropertyPath.cpp', 'w') as f:
        f.write(content)
    print("Replaced!")
