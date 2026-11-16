with open("src/Singularity/Core/CodecChannel.hpp", "r") as f:
    text = f.read()

props = """    // JSON Traversal Modality
    std::string propJsonArray() const { return _jsonArray; }
    void propSetJsonArray(const std::string& v) { _jsonArray = v; }

    std::string propShiftItem() const { return _shiftItem; }
    void propSetShiftItem(const std::string& v) { _shiftItem = v; }

    bool propArrayShift() const { return false; }
    void propSetArrayShift(const bool& v);

    std::string propQueryKey() const { return _queryKey; }
    void propSetQueryKey(const std::string& v) { _queryKey = v; }

    std::string propQueryValue() const;
    bool propArrayEmpty() const;

    std::string _input;
    std::string _jsonArray;
    std::string _shiftItem;
    std::string _queryKey;
"""
text = text.replace("    std::string _input;\n", props)

with open("src/Singularity/Core/CodecChannel.hpp", "w") as f:
    f.write(text)
