import re

with open("src/Singularity/Storage/FileChannel.cpp", "r") as f:
    text = f.read()

# 1. Remove base64/hex encode/decode static methods
text = re.sub(r'std::string FileChannel::base64Encode.*?^}\n', '', text, flags=re.MULTILINE|re.DOTALL)
text = re.sub(r'std::string FileChannel::base64Decode.*?^}\n', '', text, flags=re.MULTILINE|re.DOTALL)
text = re.sub(r'std::string FileChannel::hexEncode.*?^}\n', '', text, flags=re.MULTILINE|re.DOTALL)
text = re.sub(r'std::string FileChannel::hexDecode.*?^}\n', '', text, flags=re.MULTILINE|re.DOTALL)

# 2. Remove kBase64Chars
text = re.sub(r'static const char kBase64Chars\[\].*?;\n', '', text, flags=re.MULTILINE|re.DOTALL)

# 3. Remove JSON property getters
text = re.sub(r'bool FileChannel::propJsonValid.*?^}\n', '', text, flags=re.MULTILINE|re.DOTALL)
text = re.sub(r'std::string FileChannel::propJsonCompact.*?^}\n', '', text, flags=re.MULTILINE|re.DOTALL)
text = re.sub(r'std::string FileChannel::propJsonPretty.*?^}\n', '', text, flags=re.MULTILINE|re.DOTALL)

# 4. Remove contentBase64 / contentHex properties
text = re.sub(r'std::string FileChannel::propContentBase64.*?^}\n', '', text, flags=re.MULTILINE|re.DOTALL)
text = re.sub(r'void FileChannel::propSetContentBase64.*?^}\n', '', text, flags=re.MULTILINE|re.DOTALL)
text = re.sub(r'std::string FileChannel::propContentHex.*?^}\n', '', text, flags=re.MULTILINE|re.DOTALL)
text = re.sub(r'void FileChannel::propSetContentHex.*?^}\n', '', text, flags=re.MULTILINE|re.DOTALL)

# 5. Remove properties registration
text = re.sub(r'\s*registerProperty\(std::make_unique<ComputedProperty<FileChannel, std::string>>\(\n\s*"file\.encoding".*?\)\);', '', text, flags=re.MULTILINE|re.DOTALL)
text = re.sub(r'\s*registerProperty\(std::make_unique<ComputedProperty<FileChannel, std::string>>\(\n\s*"file\.contentBase64".*?\)\);', '', text, flags=re.MULTILINE|re.DOTALL)
text = re.sub(r'\s*registerProperty\(std::make_unique<ComputedProperty<FileChannel, std::string>>\(\n\s*"file\.contentHex".*?\)\);', '', text, flags=re.MULTILINE|re.DOTALL)
text = re.sub(r'\s*registerProperty\(std::make_unique<ComputedProperty<FileChannel, bool>>\(\n\s*"file\.jsonValid".*?\)\);', '', text, flags=re.MULTILINE|re.DOTALL)
text = re.sub(r'\s*registerProperty\(std::make_unique<ComputedProperty<FileChannel, std::string>>\(\n\s*"file\.jsonCompact".*?\)\);', '', text, flags=re.MULTILINE|re.DOTALL)
text = re.sub(r'\s*registerProperty\(std::make_unique<ComputedProperty<FileChannel, std::string>>\(\n\s*"file\.jsonPretty".*?\)\);', '', text, flags=re.MULTILINE|re.DOTALL)

# 6. Remove detectMimeType JSON sniffing
text = re.sub(r'\s*// JSON syntax check.*?}\n\s*}\n\s*}', '', text, flags=re.MULTILINE|re.DOTALL)

# 7. Remove `if (_encoding == ...` blocks and just replace them with what's in `else` or leave default
# Pattern A: reading
#         if (_encoding == "base64") {
#             _content = base64Encode(memData);
#         } else if (_encoding == "hex") {
#             _content = hexEncode(memData);
#         } else {
#             _content = std::move(memData);
#         }
text = re.sub(r'\s*if \(_encoding == "base64"\) \{\s*_content = base64Encode\(memData\);\s*\} else if \(_encoding == "hex"\) \{\s*_content = hexEncode\(memData\);\s*\} else \{\s*_content = std::move\(memData\);\s*\}', r'\n        _content = std::move(memData);', text)

text = re.sub(r'\s*if \(_encoding == "base64"\) \{\s*_content = base64Encode\(raw\);\s*\} else if \(_encoding == "hex"\) \{\s*_content = hexEncode\(raw\);\s*\} else \{\s*_content = std::move\(raw\);\s*\}', r'\n        _content = std::move(raw);', text)

# Pattern B: writing
#         std::string payload = _content;
#         if (_encoding == "base64") {
#             payload = base64Decode(_content);
#         } else if (_encoding == "hex") {
#             payload = hexDecode(_content);
#         }
text = re.sub(r'\s*std::string payload = _content;\s*if \(_encoding == "base64"\) \{\s*payload = base64Decode\(_content\);\s*\} else if \(_encoding == "hex"\) \{\s*payload = hexDecode\(_content\);\s*\}', r'\n        std::string payload = _content;', text)

with open("src/Singularity/Storage/FileChannel.cpp", "w") as f:
    f.write(text)

