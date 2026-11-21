#include "Identity/Claim.hpp"
#include "Identity/KeyPair.hpp"
#include "Identity/SingularId.hpp"
#include "json.hpp"
#include <cassert>
#include <iostream>

using namespace Identity;

int main() {
    std::cout << "Testing Claim...\n";
    {
        auto key = PrivateKey::generate();
        assert(key.isValid());

        SingularId subject = SingularId::mintOpaque();
        SingularId object = SingularId::mintOpaque();

        Claim c = Claim::issue(key, subject, "owns", object, 12345);

        assert(c.isWellFormed());
        assert(c.verify());
        assert(c.issuer() == key.id());
        assert(c.subject() == subject);
        assert(c.predicate() == "owns");
        assert(c.object() == object);
        assert(c.issuedAt() == 12345);
    }

    {
        auto key = PrivateKey::generate();
        SingularId subject = SingularId::mintOpaque();
        SingularId object = SingularId::mintOpaque();

        Claim original = Claim::issue(key, subject, "authored", object, 9999);
        nlohmann::json j = original.toJson();

        Claim loaded = Claim::fromJson(j);
        assert(loaded.isWellFormed());
        assert(loaded.verify());

        assert(loaded.issuer() == original.issuer());
        assert(loaded.subject() == original.subject());
        assert(loaded.predicate() == original.predicate());
        assert(loaded.object() == original.object());
        assert(loaded.issuedAt() == original.issuedAt());
    }

    {
        auto key = PrivateKey::generate();

        SingularId subject = SingularId::mintOpaque();
        SingularId object = SingularId::mintOpaque();

        Claim c = Claim::issue(key, subject, "owns", object, 12345);
        assert(c.verify());

        // Tamper with the JSON
        nlohmann::json j = c.toJson();
        j["predicate"] = "stole";

        Claim tampered = Claim::fromJson(j);
        // The claim is well-formed structurally, but the signature doesn't cover "stole".
        assert(tampered.isWellFormed());
        assert(!tampered.verify());
    }

    std::cout << "All Claim tests passed.\n";
    return 0;
}
