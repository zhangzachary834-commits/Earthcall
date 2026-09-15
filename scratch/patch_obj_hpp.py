with open("src/ConstructedBeing/Singular/Object/Object.hpp", "r") as f:
    content = f.read()

import re
content = re.sub(
    r"(bool writeAuthoredPropertyProjection[^;]+;)",
    r"\1\n    void onDynamicPropertyChanged(Earthcall::StringId id) override;",
    content
)

with open("src/ConstructedBeing/Singular/Object/Object.hpp", "w") as f:
    f.write(content)
