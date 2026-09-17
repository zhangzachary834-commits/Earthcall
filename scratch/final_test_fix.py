import re

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    'R"({"input": "v", "pieces": [{"where": {"op": 5, "children": [{"op": 0, "scalarForm": {"terms": [{"c": 0.5, "factors": {}}]}}, {"op": 1, "var": "v"}]}, "mathNode": {"op": 0, "scalarForm": {"terms": [{"c": 1.0, "factors": {}}]}}}, {"guard": {"kind": 3, "children": []}}]})"',
    'R"({"input": "u", "pieces": [{"where": {"op": 5, "children": [{"op": 1, "var": "u"}, {"op": 0, "scalarForm": {"terms": [{"c": 0.5, "factors": {}}]}}]}, "mathNode": {"op": 0, "scalarForm": {"terms": [{"c": 1.0, "factors": {}}]}}}, {"guard": {"kind": 3, "children": []}}]})"'
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
