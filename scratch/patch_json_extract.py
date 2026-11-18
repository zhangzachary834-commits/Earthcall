with open("src/Singularity/Core/CodecChannel.hpp", "r") as f:
    text = f.read()

text = text.replace("bool propArrayEmpty() const;", "bool propArrayEmpty() const;\n    std::string propJsonExtract() const;")
with open("src/Singularity/Core/CodecChannel.hpp", "w") as f:
    f.write(text)


with open("src/Singularity/Core/CodecChannel.cpp", "r") as f:
    text = f.read()

impl = """bool CodecChannel::propArrayEmpty() const {
    if (_jsonArray.empty()) return true;
    try {
        auto j = nlohmann::json::parse(_jsonArray);
        return j.empty();
    } catch (...) {
        return true;
    }
}

std::string CodecChannel::propJsonExtract() const {
    if (_input.empty() || _queryKey.empty()) return "";
    try {
        auto j = nlohmann::json::parse(_input);
        if (j.contains(_queryKey)) {
            if (j[_queryKey].is_string()) return j[_queryKey].get<std::string>();
            return j[_queryKey].dump();
        }
    } catch (...) {}
    return "";
}"""
text = text.replace("""bool CodecChannel::propArrayEmpty() const {
    if (_jsonArray.empty()) return true;
    try {
        auto j = nlohmann::json::parse(_jsonArray);
        return j.empty();
    } catch (...) {
        return true;
    }
}""", impl)

text = text.replace('        "codec.arrayEmpty", this, &CodecChannel::propArrayEmpty, nullptr));', '        "codec.arrayEmpty", this, &CodecChannel::propArrayEmpty, nullptr));\n    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(\n        "codec.jsonExtract", this, &CodecChannel::propJsonExtract, nullptr));')

with open("src/Singularity/Core/CodecChannel.cpp", "w") as f:
    f.write(text)

