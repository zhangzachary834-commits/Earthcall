with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "r") as f:
    lines = f.readlines()

new_lines = []
skip = False
for i, line in enumerate(lines):
    if i >= 131 and i <= 136:
        continue # delete the mistakenly added describe() strings
    new_lines.append(line)

with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "w") as f:
    f.writelines(new_lines)
