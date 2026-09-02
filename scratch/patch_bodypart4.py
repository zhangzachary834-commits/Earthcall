import re

HPP_PATH = "src/Person/Body/BodyPart/BodyPart.hpp"
with open(HPP_PATH, 'r') as f:
    hpp = f.read()

automations_methods = """
    void setAutomationRest(const glm::mat4& rest) { if (_primaryObject) _primaryObject->setAutomationRest(rest); }
    void advanceAutomations(float dt) { if (_primaryObject) _primaryObject->advanceAutomations(dt); }
    void clearAutomations() { if (_primaryObject) _primaryObject->clearAutomations(); }
    void addAutomation(const AnimationClip& clip) { if (_primaryObject) _primaryObject->addAutomation(clip); }
    AutomationState& automationState() { return _primaryObject->automationState(); }
    const AutomationState& automationState() const { return _primaryObject->automationState(); }
"""

# Insert right after sampleAutomations
insert_idx = hpp.find("glm::mat4 sampleAutomations(const glm::mat4& restTransform)")
if insert_idx != -1:
    end_of_method = hpp.find("}", insert_idx) + 1
    hpp = hpp[:end_of_method] + automations_methods + hpp[end_of_method:]

with open(HPP_PATH, 'w') as f:
    f.write(hpp)

