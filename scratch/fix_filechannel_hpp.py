import re

with open("src/Singularity/Storage/FileChannel.hpp", "r") as f:
    text = f.read()

# 1. Remove static encoding methods
text = re.sub(r'\s*static std::string base64Encode\(const std::string& input\);\n\s*static std::string base64Decode\(const std::string& input\);\n\s*static std::string hexEncode\(const std::string& input\);\n\s*static std::string hexDecode\(const std::string& input\);', '', text)

# 2. Remove propEncoding
text = re.sub(r'\s*std::string propEncoding\(\) const \{ return _encoding; \}\n\s*void propSetEncoding\(const std::string& v\) \{ _encoding = v; \}', '', text)

# 3. Remove contentBase64 / contentHex
text = re.sub(r'\s*std::string propContentBase64\(\) const;\n\s*void propSetContentBase64\(const std::string& v\);\n\s*std::string propContentHex\(\) const;\n\s*void propSetContentHex\(const std::string& v\);', '', text)

# 4. Remove json properties
text = re.sub(r'\s*bool propJsonValid\(\) const;\n\s*std::string propJsonCompact\(\) const;\n\s*std::string propJsonPretty\(\) const;', '', text)

# 5. Remove _encoding field
text = re.sub(r'\s*std::string _encoding = "auto";\s*// "auto", "text", "base64", "hex"', '', text)

with open("src/Singularity/Storage/FileChannel.hpp", "w") as f:
    f.write(text)

