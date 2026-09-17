with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

# Change the law to expect greenList
content = content.replace(
    'glm::vec3 red(1.0f, 0.0f, 0.0f);',
    'glm::vec3 red(1.0f, 0.0f, 0.0f);\n    glm::vec3 green(0.0f, 1.0f, 0.0f);'
)
content = content.replace(
    'auto expectedRedList = std::make_shared<PropertyList>();\n    for (int i = 0; i < 2; i++) expectedRedList->elements.push_back(PropertyValue(red));',
    'auto expectedGreenList = std::make_shared<PropertyList>();\n    for (int i = 0; i < 2; i++) expectedGreenList->elements.push_back(PropertyValue(green));'
)
content = content.replace(
    'ConditionNode::compare("regionB", ConditionNode::Op::Eq, PropertyValue(expectedRedList))',
    'ConditionNode::compare("regionB", ConditionNode::Op::Eq, PropertyValue(expectedGreenList))'
)

# Fix selectorB so it doesn't return std::nullopt (needs mathNode!)
content = content.replace(
    r'{"input":"x","pieces":[{"where":{"children":[{"op":1,"var":"u"},{"op":0,"scalarForm":{"terms":[{"c":0.5,"factors":{}}]}}],"op":5}},{"guard":{"children":[],"kind":3}}]}',
    r'{"input":"x","pieces":[{"mathNode":{"op":0,"scalarForm":{"terms":[{"c":1.0,"factors":{}}]}},"where":{"children":[{"op":1,"var":"u"},{"op":0,"scalarForm":{"terms":[{"c":0.5,"factors":{}}]}}],"op":5}}]}'
)

# Change writing regionA to write greenList
content = content.replace(
    'auto redList = std::make_shared<PropertyList>();\n    for (int i = 0; i < 2; i++) redList->elements.push_back(PropertyValue(red));',
    'auto greenList = std::make_shared<PropertyList>();\n    for (int i = 0; i < 2; i++) greenList->elements.push_back(PropertyValue(green));'
)
content = content.replace(
    'PropertyPath::parse("regionA").setValue(*obj, PropertyValue(redList))',
    'PropertyPath::parse("regionA").setValue(*obj, PropertyValue(greenList))'
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
