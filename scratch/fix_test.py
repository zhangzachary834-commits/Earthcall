with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    code = f.read()

import re

# Fix ConditionNode block
cond_replacement = """
    auto cond = std::make_shared<ConditionNode>(
        ConditionNode::compare("regionB", Op::Eq, PropertyValue(expectedRedList))
    );
"""
code = re.sub(r'auto cond = std::make_shared<ConditionNode>\(\);.*?law->setCondition\(cond\);',
              'auto cond = std::make_shared<ConditionNode>(\\n        ConditionNode::compare("regionB", Op::Eq, PropertyValue(expectedRedList))\\n    );\\n    law->setCondition(cond);', code, flags=re.DOTALL)

# Fix ActionNode block
act_replacement = """
    auto act = std::make_shared<ActionNode>(
        ActionNode::set("witness_passed", PropertyValue(true))
    );
"""
code = re.sub(r'auto act = std::make_shared<ActionNode>\(\);.*?law->setAction\(act\);',
              'auto act = std::make_shared<ActionNode>(\\n        ActionNode::set("witness_passed", PropertyValue(true))\\n    );\\n    law->setAction(act);', code, flags=re.DOTALL)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(code)
