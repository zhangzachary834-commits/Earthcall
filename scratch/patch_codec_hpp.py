import re

with open("src/Singularity/Core/CodecChannel.hpp", "r") as f:
    text = f.read()

props = """    bool propJsonValid() const;
    std::string propJsonCompact() const;
    std::string propJsonPretty() const;
    std::string propEcgraphToJson() const;
    std::string propJsonToMsgpack() const;
"""
text = text.replace('    bool propJsonValid() const;\n    std::string propJsonCompact() const;\n    std::string propJsonPretty() const;', props)

with open("src/Singularity/Core/CodecChannel.hpp", "w") as f:
    f.write(text)
