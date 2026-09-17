import re

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

jsonA = '{"input": "v", "pieces": [{"where": {"op": 5, "children": [{"op": 1, "var": "v"}, {"op": 0, "scalarForm": {"terms": [{"c": 0.5, "factors": {}}]}}]}, "call": null, "fold": null}, {"guard": {"kind": 3, "children": []}}]}'
jsonB = '{"input": "v", "pieces": [{"where": {"op": 5, "children": [{"op": 0, "scalarForm": {"terms": [{"c": 0.5, "factors": {}}]}}, {"op": 1, "var": "v"}]}, "call": null, "fold": null}, {"guard": {"kind": 3, "children": []}}]}'

content = re.sub(
    r'auto selectorA_json = nlohmann::json::parse\(R"\(\{.*?\}\)"\);',
    f'auto selectorA_json = nlohmann::json::parse(R"({jsonA})");',
    content
)

content = re.sub(
    r'auto selectorB_json = nlohmann::json::parse\(R"\(\{.*?\}\)"\);',
    f'auto selectorB_json = nlohmann::json::parse(R"({jsonB})");',
    content
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
