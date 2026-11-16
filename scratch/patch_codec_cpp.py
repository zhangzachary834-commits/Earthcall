with open("src/Singularity/Core/CodecChannel.cpp", "r") as f:
    text = f.read()

impls = """void CodecChannel::propSetArrayShift(const bool& v) {
    if (!v || _jsonArray.empty()) return;
    try {
        auto j = nlohmann::json::parse(_jsonArray);
        if (j.is_array() && !j.empty()) {
            _shiftItem = j[0].dump();
            j.erase(0);
            _jsonArray = j.dump();
        } else {
            _shiftItem = "{}";
            _jsonArray = "[]";
        }
    } catch (...) {
        _shiftItem = "{}";
        _jsonArray = "[]";
    }
}

std::string CodecChannel::propQueryValue() const {
    if (_shiftItem.empty() || _queryKey.empty()) return "";
    try {
        auto j = nlohmann::json::parse(_shiftItem);
        if (j.contains(_queryKey)) {
            if (j[_queryKey].is_string()) return j[_queryKey].get<std::string>();
            return j[_queryKey].dump();
        }
    } catch (...) {}
    return "";
}

bool CodecChannel::propArrayEmpty() const {
    if (_jsonArray.empty()) return true;
    try {
        auto j = nlohmann::json::parse(_jsonArray);
        return j.empty();
    } catch (...) {
        return true;
    }
}

void CodecChannel::buildProperties() {"""

text = text.replace("void CodecChannel::buildProperties() {", impls)

regs = """    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.jsonArray", this, &CodecChannel::propJsonArray, &CodecChannel::propSetJsonArray));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.shiftItem", this, &CodecChannel::propShiftItem, &CodecChannel::propSetShiftItem));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, bool>>(
        "codec.arrayShift", this, &CodecChannel::propArrayShift, &CodecChannel::propSetArrayShift));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.queryKey", this, &CodecChannel::propQueryKey, &CodecChannel::propSetQueryKey));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, std::string>>(
        "codec.queryValue", this, &CodecChannel::propQueryValue, nullptr));
    registerProperty(std::make_unique<ComputedProperty<CodecChannel, bool>>(
        "codec.arrayEmpty", this, &CodecChannel::propArrayEmpty, nullptr));
}"""

text = text.replace("}\n\n} // namespace Core", regs + "\n\n} // namespace Core")

with open("src/Singularity/Core/CodecChannel.cpp", "w") as f:
    f.write(text)
