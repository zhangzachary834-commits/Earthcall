#include "../../src/Identity/SingularId.hpp"
#include <algorithm>
#include <array>
#include <cstdlib>
#include <iostream>
#include <set>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

using namespace Identity;

namespace {

[[noreturn]] void fail(const char* expression, const char* file, int line) {
    std::cerr << "CHECK failed: " << expression << " at " << file << ':' << line << '\n';
    std::exit(EXIT_FAILURE);
}

#define CHECK(expr) do { if (!(expr)) fail(#expr, __FILE__, __LINE__); } while (false)

std::string nonCanonicalAlias(std::string canonical) {
    static const std::string alphabet = "abcdefghijklmnopqrstuvwxyz234567";
    CHECK(!canonical.empty());
    const std::size_t pos = alphabet.find(canonical.back());
    CHECK(pos != std::string::npos);
    CHECK(pos + 1 < alphabet.size());
    canonical.back() = alphabet[pos + 1];
    return canonical;
}

} // namespace

void testHex() {
    std::vector<uint8_t> empty;
    CHECK(hexEncode(empty) == "");
    CHECK(hexDecode("") == empty);

    const std::vector<uint8_t> data = {0x00, 0x1A, 0x2B, 0x3C, 0xFF};
    CHECK(hexEncode(data) == "001a2b3cff");
    CHECK(hexDecode("001a2b3cff") == data);

    CHECK(hexDecode("001a2b3cf").empty()); // odd length
    CHECK(hexDecode("001X2b3cff").empty()); // invalid char
}

void testBase32() {
    std::vector<uint8_t> out;
    CHECK(base32Encode({}) == "");
    CHECK(base32Decode("", out) && out.empty());

    // RFC 4648 test vectors, lower-case and without padding.
    const std::vector<std::pair<std::string, std::string>> vectors = {
        {"f", "my"},
        {"fo", "mzxq"},
        {"foo", "mzxw6"},
        {"foob", "mzxw6yq"},
        {"fooba", "mzxw6ytb"},
        {"foobar", "mzxw6ytboi"},
    };

    for (const auto& [plain, encoded] : vectors) {
        const std::vector<uint8_t> bytes(plain.begin(), plain.end());
        CHECK(base32Encode(bytes) == encoded);
        out.clear();
        CHECK(base32Decode(encoded, out));
        CHECK(out == bytes);
    }

    CHECK(!base32Decode("my1", out)); // '1' is outside the RFC 4648 alphabet.
    CHECK(!base32Decode("MY", out));  // Text form is deliberately canonical lower-case.
    CHECK(!base32Decode("a", out));   // Impossible unpadded Base32 quantum.

    // "my" canonically encodes the byte 'f'. If unused tail bits are ignored,
    // "mz" decodes to the same complete byte and creates an alias spelling.
    CHECK(!base32Decode("mz", out));
}

void testMintOpaque() {
    const SingularId id = SingularId::mintOpaque();
    CHECK(id.isValid());
    CHECK(id.kind() == SingularId::Kind::Opaque);
    CHECK(!id.canAuthenticate());
    CHECK(id.bytes().size() == 16); // 128 bits

    const std::string canonical = id.toString();
    CHECK(canonical.rfind("ec1:", 0) == 0);
    CHECK(SingularId::parse(canonical) == id);

    // The same underlying bytes must not be accepted through a non-canonical
    // final Base32 symbol with non-zero unused tail bits.
    CHECK(!SingularId::parse(nonCanonicalAlias(canonical)).isValid());
}

void testFromPublicKey() {
    std::array<uint8_t, 32> pubkey{};
    for (std::size_t i = 0; i < pubkey.size(); ++i) {
        pubkey[i] = static_cast<uint8_t>(i);
    }

    const SingularId id = SingularId::fromPublicKey(pubkey);
    CHECK(id.isValid());
    CHECK(id.kind() == SingularId::Kind::Key);
    CHECK(id.canAuthenticate());
    CHECK(id.bytes().size() == 32);
    CHECK(std::equal(pubkey.begin(), pubkey.end(), id.bytes().begin()));

    const std::string canonical = id.toString();
    CHECK(canonical.rfind("did:earthcall:", 0) == 0);
    CHECK(SingularId::parse(canonical) == id);
    CHECK(!SingularId::parse(nonCanonicalAlias(canonical)).isValid());
}

void testParseInvalid() {
    CHECK(!SingularId::parse("").isValid());
    CHECK(!SingularId::parse("bad:ec1:1234").isValid());
    CHECK(!SingularId::parse("ec1:").isValid());
    CHECK(!SingularId::parse("did:earthcall:").isValid());
    CHECK(!SingularId::parse("ec1:1234").isValid());

    const std::vector<uint8_t> shortData(15, 0);
    const std::vector<uint8_t> longData(17, 0);
    CHECK(!SingularId::parse("ec1:" + base32Encode(shortData)).isValid());
    CHECK(!SingularId::parse("ec1:" + base32Encode(longData)).isValid());

    const std::vector<uint8_t> shortKey(31, 0);
    const std::vector<uint8_t> longKey(33, 0);
    CHECK(!SingularId::parse("did:earthcall:" + base32Encode(shortKey)).isValid());
    CHECK(!SingularId::parse("did:earthcall:" + base32Encode(longKey)).isValid());
}

void testRelationalAndHash() {
    // Deterministic opaque ids avoid making ordering/hash coverage depend on a
    // probabilistic non-collision assertion from the CSPRNG.
    const SingularId id1 = SingularId::parse(
        "ec1:" + base32Encode(std::vector<uint8_t>(16, 1)));
    const SingularId id2 = SingularId::parse(
        "ec1:" + base32Encode(std::vector<uint8_t>(16, 2)));

    std::array<uint8_t, 32> pk1{};
    std::array<uint8_t, 32> pk2{};
    pk1.fill(1);
    pk2.fill(2);
    const SingularId key1 = SingularId::fromPublicKey(pk1);
    const SingularId key2 = SingularId::fromPublicKey(pk2);

    CHECK(id1.isValid() && id2.isValid());
    CHECK(id1 == id1);
    CHECK(id1 != id2);

    const std::set<SingularId> ordered = {id1, id2, key1, key2};
    CHECK(ordered.size() == 4);

    const std::unordered_set<SingularId> hashed = {id1, id2, key1, key2};
    CHECK(hashed.size() == 4);
    CHECK(hashed.count(id1) == 1);

    const SingularId invalid;
    CHECK(!invalid.isValid());
    CHECK(invalid == SingularId());
}

void testAbbreviated() {
    const SingularId opaque = SingularId::parse(
        "ec1:" + base32Encode(std::vector<uint8_t>(16, 3)));
    CHECK(opaque.abbreviated().rfind("ec1:", 0) == 0);

    std::array<uint8_t, 32> pk{};
    pk.fill(1);
    const SingularId key = SingularId::fromPublicKey(pk);
    CHECK(key.abbreviated().rfind("did:", 0) == 0);
    CHECK(SingularId().abbreviated() == "<invalid>");
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
    return EXIT_SUCCESS;
}
