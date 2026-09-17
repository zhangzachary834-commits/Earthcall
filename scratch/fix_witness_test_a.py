with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "r") as f:
    content = f.read()

content = content.replace(
    r'{"input":"x","pieces":[{"where":{"children":[{"op":1,"var":"v"},{"op":0,"scalarForm":{"terms":[{"c":0.5,"factors":{}}]}}],"op":5}}]}',
    r'{"input":"x","pieces":[{"mathNode":{"op":0,"scalarForm":{"terms":[{"c":1.0,"factors":{}}]}},"where":{"children":[{"op":1,"var":"v"},{"op":0,"scalarForm":{"terms":[{"c":0.5,"factors":{}}]}}],"op":5}}]}'
)

with open("tests/singularity/ontomath_two_direction_witness_test.cpp", "w") as f:
    f.write(content)
