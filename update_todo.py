import re

with open("docs/Agenda/Tasks/To-do list.md", "r") as f:
    content = f.read()

done_item = "- ✅ **Implement Perlin noise floor** — **done (2026-08-27)**: Authored `perlin-ground-plane` as a Field (`ShapeKind 10`) evaluated via `geom::SdfNode` mapping an OntoMath `Op::Noise` AST. Fixed serialization to emit a proper `SdfNode` structure so the WebGPU raymarcher evaluates the SDF correctly instead of defaulting to an invisible 1e9 empty space. Added green `faceColors` so it renders visibly, shifted its Y origin to prevent clipping the default player spawn, and added a fixture law (`law-toggle-ground`) to toggle the `baseline: ground` (the huge rectangular prism) by pressing the `G` key.\n"

# Insert before "## R&D:"
if "## R&D:" in content:
    content = content.replace("## R&D:", done_item + "\n## R&D:")
else:
    content += "\n" + done_item

with open("docs/Agenda/Tasks/To-do list.md", "w") as f:
    f.write(content)
