import sys

file_path = "src/ZonesOfEarth/AuthorsOfLaw/Law.cpp"
with open(file_path, "r") as f:
    content = f.read()

content = content.replace("tD - tC) * 1000.0 > 0.5", "tD - tC) * 1000.0 > 0.1")
content = content.replace("tE - tA) * 1000.0 > 2.0", "tE - tA) * 1000.0 > 0.5")

with open(file_path, "w") as f:
    f.write(content)
print("Lowered thresholds!")
