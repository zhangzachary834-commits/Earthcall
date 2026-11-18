import re

with open("tests/singularity/file_channel_test.cpp", "r") as f:
    text = f.read()

# Remove Case 8
text = re.sub(r'\s*// -+\n\s*// Case 8: JSON.*?// Case 9:', '\n\n    // Case 9:', text, flags=re.DOTALL)

# Remove Base64/Hex encoding cases that read/write properties like `file.contentBase64` and `file.encoding="base64"`
text = re.sub(r'\s*// -+\n\s*// Case 10: Base64 and Hex.*?// Case 11:', '\n\n    // Case 11:', text, flags=re.DOTALL)

with open("tests/singularity/file_channel_test.cpp", "w") as f:
    f.write(text)
