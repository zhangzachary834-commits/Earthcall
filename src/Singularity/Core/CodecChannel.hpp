#pragma once

#include "ZonesOfEarth/AuthorsOfLaw/Law.hpp"
#include <string>

namespace Singularity {
namespace Core {

// CodecChannel provides semantic byte-transformations (Base64, Hex, JSON parsing).
// Extracted from FileChannel to obey the minimum-maximum principle:
// OS file I/O should not mandate parsing logic.
class CodecChannel : public Law {
public:
    CodecChannel() : Law("codec") {
        setName("Codec Channel");
        // No explicit activation needed; properties are computed on demand
    }
    ~CodecChannel() override = default;

    static std::string base64Encode(const std::string& input);
    static std::string base64Decode(const std::string& input);
    static std::string hexEncode(const std::string& input);
    static std::string hexDecode(const std::string& input);

    bool isFirstMover() const override { return true; }
    std::string getIdentifier() const override { return "codec"; }

    static void syncRegister(LawManager& laws) {
        if (!find(laws)) {
            laws.add(std::make_shared<CodecChannel>());
        }
    }

    static CodecChannel* find(LawManager& laws) {
        return dynamic_cast<CodecChannel*>(laws.find("codec"));
    }

private:
    void buildProperties() override;

    std::string propInput() const { return _input; }
    void propSetInput(const std::string& v) { _input = v; }

    std::string propBase64() const;
    void propSetBase64(const std::string& v);

    std::string propHex() const;
    void propSetHex(const std::string& v);

    bool propJsonValid() const;
    std::string propJsonCompact() const;
    std::string propJsonPretty() const;
    std::string propEcgraphToJson() const;
    std::string propJsonToMsgpack() const;
    std::string propSerializeZoneJson() const;
    std::string propSerializeZoneEcform() const;


    // JSON Traversal Modality
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
    std::string propJsonExtract() const;

    std::string _input;
    std::string _jsonArray;
    std::string _shiftItem;
    std::string _queryKey;
};

} // namespace Core
} // namespace Singularity
