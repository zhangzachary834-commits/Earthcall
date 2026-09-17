import re
with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

# Replace the png data array with the black one
black_png = "const unsigned char png_data[] = { 0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x08, 0x06, 0x00, 0x00, 0x00, 0x72, 0xb6, 0x0d, 0x24, 0x00, 0x00, 0x00, 0x0b, 0x49, 0x44, 0x41, 0x54, 0x08, 0x5b, 0x63, 0x60, 0x00, 0x02, 0x00, 0x00, 0x05, 0x00, 0x01, 0x09, 0x92, 0x32, 0x98, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae, 0x42, 0x60, 0x82 };"
content = re.sub(r'const unsigned char png_data\[\] = \{[^}]*\};', black_png, content)

# Replace selectorA to have mathNode
content = re.sub(
    r'auto selectorA_json = nlohmann::json::parse\(R"\(\{.*\}\)"\);',
    r'auto selectorA_json = nlohmann::json::parse(R"({"input":"x","pieces":[{"mathNode":{"op":0,"scalarForm":{"terms":[{"c":1.0,"factors":{}}]}},"where":{"children":[{"op":1,"var":"v"},{"op":0,"scalarForm":{"terms":[{"c":0.5,"factors":{}}]}}],"op":5}}]})");',
    content
)

# Also fix selectorB just in case, though we already fixed it earlier but let's make sure
content = re.sub(
    r'auto selectorB_json = nlohmann::json::parse\(R"\(\{.*\}\)"\);',
    r'auto selectorB_json = nlohmann::json::parse(R"({"input":"x","pieces":[{"mathNode":{"op":0,"scalarForm":{"terms":[{"c":1.0,"factors":{}}]}},"where":{"children":[{"op":1,"var":"u"},{"op":0,"scalarForm":{"terms":[{"c":0.5,"factors":{}}]}}],"op":5}}]})");',
    content
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
