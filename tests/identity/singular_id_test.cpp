#include "../../src/Identity/SingularId.hpp"
#include <cassert>
#include <iostream>
#include <unordered_set>
#include <algorithm>
#include <set>

using namespace Identity;

void testHex() {
    std::vector<uint8_t> empty;
    assert(hexEncode(empty) == "");
    assert(hexDecode("") == empty);

    std::vector<uint8_t> data = {0x00, 0x1A, 0x2B, 0x3C, 0xFF};
    std::string enc = hexEncode(data);
    assert(enc == "001a2b3cff");

    std::vector<uint8_t> dec = hexDecode(enc);
    assert(dec == data);

    // Invalid lengths or characters should return empty
    assert(hexDecode("001a2b3cf").empty()); // odd length
    assert(hexDecode("001X2b3cff").empty()); // invalid char
}

void testBase32() {
    std::vector<uint8_t> empty;
    assert(base32Encode(empty) == "");
    std::vector<uint8_t> out;
    assert(base32Decode("", out));
    // out might have partial results on fail, which is okay based on base32Decode implementation

    // RFC 4648 test vectors
    // "f" -> "my======" -> no padding -> "my"
    std::vector<uint8_t> f = {'f'};
    assert(base32Encode(f) == "my");
    out.clear();
    assert(base32Decode("my", out) && out == f);

    // "fo" -> "mzxq====" -> "mzxq"
    std::vector<uint8_t> fo = {'f', 'o'};
    assert(base32Encode(fo) == "mzxq");
    out.clear();
    assert(base32Decode("mzxq", out) && out == fo);

    // "foo" -> "mzxw6===" -> "mzxw6"
    std::vector<uint8_t> foo = {'f', 'o', 'o'};
    assert(base32Encode(foo) == "mzxw6");
    out.clear();
    assert(base32Decode("mzxw6", out) && out == foo);

    // "foob" -> "mzxw6yq=" -> "mzxw6yq"
    std::vector<uint8_t> foob = {'f', 'o', 'o', 'b'};
    assert(base32Encode(foob) == "mzxw6yq");
    out.clear();
    assert(base32Decode("mzxw6yq", out) && out == foob);

    // "fooba" -> "mzxw6ytb" -> "mzxw6ytb"
    std::vector<uint8_t> fooba = {'f', 'o', 'o', 'b', 'a'};
    assert(base32Encode(fooba) == "mzxw6ytb");
    out.clear();
    assert(base32Decode("mzxw6ytb", out) && out == fooba);

    // "foobar" -> "mzxw6ytboi======" -> "mzxw6ytboi"
    std::vector<uint8_t> foobar = {'f', 'o', 'o', 'b', 'a', 'r'};
    assert(base32Encode(foobar) == "mzxw6ytboi");
    out.clear();
    assert(base32Decode("mzxw6ytboi", out) && out == foobar);

    // Invalid base32
    out.clear();
    assert(!base32Decode("my1", out)); // '1' is invalid in base32
    // out might have partial results on fail, which is okay based on base32Decode implementation
}

void testMintOpaque() {
    SingularId id1 = SingularId::mintOpaque();
    assert(id1.isValid());
    assert(id1.kind() == SingularId::Kind::Opaque);
    assert(!id1.canAuthenticate());
    assert(id1.bytes().size() == 16); // 128 bits = 16 bytes

    SingularId id2 = SingularId::mintOpaque();
    assert(id1 != id2); // highly likely unique

    std::string s = id1.toString();
    assert(s.substr(0, 4) == "ec1:");
    SingularId parsed = SingularId::parse(s);
    assert(parsed == id1);
}

void testFromPublicKey() {
    std::array<uint8_t, 32> pubkey;
    for (int i = 0; i < 32; ++i) pubkey[i] = i;

    SingularId id = SingularId::fromPublicKey(pubkey);
    assert(id.isValid());
    assert(id.kind() == SingularId::Kind::Key);
    assert(id.canAuthenticate());
    assert(id.bytes().size() == 32);

    for (int i = 0; i < 32; ++i) {
        assert(id.bytes()[i] == i);
    }

    std::string s = id.toString();
    assert(s.substr(0, 14) == "did:earthcall:");
    SingularId parsed = SingularId::parse(s);
    assert(parsed == id);
}

void testParseInvalid() {
    // Empty
    assert(!SingularId::parse("").isValid());
    // Bad prefix
    assert(!SingularId::parse("bad:ec1:1234").isValid());
    // Missing base32
    assert(!SingularId::parse("ec1:").isValid());
    assert(!SingularId::parse("did:earthcall:").isValid());
    // Invalid base32 chars
    assert(!SingularId::parse("ec1:1234").isValid());
    // Valid base32 but wrong length for opaque (16 bytes expected)
    std::vector<uint8_t> shortData(15, 0);
    assert(!SingularId::parse("ec1:" + base32Encode(shortData)).isValid());
    std::vector<uint8_t> longData(17, 0);
    assert(!SingularId::parse("ec1:" + base32Encode(longData)).isValid());

    // Valid base32 but wrong length for key (32 bytes expected)
    std::vector<uint8_t> shortKey(31, 0);
    assert(!SingularId::parse("did:earthcall:" + base32Encode(shortKey)).isValid());
    std::vector<uint8_t> longKey(33, 0);
    assert(!SingularId::parse("did:earthcall:" + base32Encode(longKey)).isValid());
}

void testRelationalAndHash() {
    SingularId id1 = SingularId::mintOpaque();
    SingularId id2 = SingularId::mintOpaque();

    std::array<uint8_t, 32> pk; pk.fill(1);
    SingularId key1 = SingularId::fromPublicKey(pk);

    std::array<uint8_t, 32> pk2; pk2.fill(2);
    SingularId key2 = SingularId::fromPublicKey(pk2);

    assert(id1 == id1);
    assert(id1 != id2);

    // Test < operator via set
    std::set<SingularId> sset = {id1, id2, key1, key2};
    assert(sset.size() == 4);

    // Test std::hash via unordered_set
    std::unordered_set<SingularId> uset = {id1, id2, key1, key2};
    assert(uset.size() == 4);
    assert(uset.count(id1) == 1);

    SingularId invalid;
    assert(!invalid.isValid());
    assert(invalid == SingularId());
}

void testAbbreviated() {
    SingularId opq = SingularId::mintOpaque();
    std::string abbrO = opq.abbreviated();
    assert(abbrO.find("ec1:") == 0);

    std::array<uint8_t, 32> pk; pk.fill(1);
    SingularId key = SingularId::fromPublicKey(pk);
    std::string abbrK = key.abbreviated();
    assert(abbrK.find("did:") == 0); // "did:" prefix is usually kept
}

int main() {
    testHex();
    testBase32();
    testMintOpaque();
    testFromPublicKey();
    testParseInvalid();
    testRelationalAndHash();
    testAbbreviated();

    std::cout << "Identity::SingularId tests passed!\n";
    return 0;
}
