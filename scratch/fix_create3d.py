import re

with open("src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/Create3DConsole.cpp", "r") as f:
    text = f.read()

# Replace the "Shape Dimensions" switch block and the "Polyhedron Configuration" header block.
# We will just remove the contents carefully.
text = re.sub(r'ImGui::TextColored\(ImVec4\(0\.9f, 0\.85f, 0\.55f, 1\.0f\), "Shape Dimensions"\);.*?ImGui::ColorEdit3', 'ImGui::ColorEdit3', text, flags=re.DOTALL)
text = re.sub(r'if \(state\.polyhedron\.shapeKind == ObjectTypes::ShapeKind::Polyhedron\) \{.*?\}\s*\}\s*break;', 'break;', text, flags=re.DOTALL)

with open("src/Singularity/FirstMoverOntology/FirstMoverWindowTools/CreatorConsole/Create3DConsole.cpp", "w") as f:
    f.write(text)
