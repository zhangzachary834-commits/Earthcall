import re

with open("src/Singularity/Core/CreationChannel.cpp", "r") as f:
    text = f.read()

def replace_func(m):
    return """    auto enabledCond = ConditionNode::compare("@creation-channel.enabled", ConditionNode::Op::Eq, PropertyValue(true));
    auto modeCond = ConditionNode::compare("@creation-channel.active3DMode", ConditionNode::Op::Eq, PropertyValue(std::string("Create")));
    law->setConditionModel(ConditionNode::logicalAnd({enabledCond, modeCond}));"""

text = re.sub(r'law->setConditionModel.*?PropertyValue\(std::string\("Create"\)\)\)\);', replace_func, text, flags=re.DOTALL)

with open("src/Singularity/Core/CreationChannel.cpp", "w") as f:
    f.write(text)
