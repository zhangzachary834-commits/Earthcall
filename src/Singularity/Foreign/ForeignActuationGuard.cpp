#include "Singularity/Foreign/ForeignActuationGuard.hpp"

#include "Identity/KeyPair.hpp"
#include "ConstructedBeing/Singular/Property/PropertyPath.hpp"
#include "Singularity/TransferPolicy.hpp"

namespace Singularity {
namespace Foreign {

nlohmann::json ForeignChallenge::toJson() const {
    return nlohmann::json{
        {"protocol", "earthcall-first-mover-session-v1"},
        {"challengeId", challengeId},
        {"nonce", nonce},
        {"connection", connection},
        {"expiresAtMs", expiresAtMs},
    };
}

nlohmann::json ForeignAuthResult::toJson() const {
    nlohmann::json j{{"status", ok ? "authenticated" : "refused"},
                     {"reasonCode", reasonCode},
                     {"reason", reason}};
    if (mover.isValid()) j["moverId"] = mover.toString();
    return j;
}

ForeignChallenge ForeignSessionAuthenticator::issueChallenge(const std::string& connection,
                                                             int64_t nowMs) {
    for (auto it = _pending.begin(); it != _pending.end();) {
        if (it->second.connection == connection || it->second.expiresAtMs <= nowMs) {
            it = _pending.erase(it);
        } else {
            ++it;
        }
    }
    // mintOpaque is the CSPRNG (OpenSSL RAND_bytes); it throws rather than
    // hand out a guessable value.
    ForeignChallenge c;
    c.challengeId = Identity::SingularId::mintOpaque().toString();
    c.nonce = Identity::SingularId::mintOpaque().toString();
    c.connection = connection;
    c.expiresAtMs = nowMs + kChallengeLifetimeMs;
    _pending[c.challengeId] = c;
    return c;
}

ForeignAuthResult ForeignSessionAuthenticator::authenticate(const std::string& connection,
                                                            const std::string& challengeId,
                                                            const std::string& moverId,
                                                            const std::string& signatureHex,
                                                            const Identity::FirstMoverRegister& reg,
                                                            int64_t nowMs) {
    ForeignAuthResult r;
    auto refuse = [&](const char* code, std::string why) {
        r.ok = false;
        r.reasonCode = code;
        r.reason = std::move(why);
        return r;
    };

    auto it = _pending.find(challengeId);
    if (it == _pending.end()) return refuse("unknown-challenge", "no such challenge (already used, withdrawn, or never issued)");
    const ForeignChallenge challenge = it->second;
    _pending.erase(it);   // single use, even on failure

    if (challenge.connection != connection) {
        return refuse("challenge-connection-mismatch", "challenge was issued to a different connection");
    }
    if (challenge.expiresAtMs <= nowMs) return refuse("challenge-expired", "challenge expired; request a new one");

    r.mover = Identity::SingularId::parse(moverId);
    if (!r.mover.canAuthenticate()) {
        return refuse("mover-id-invalid", "mover id is not a cryptographic SingularId");
    }

    // Standing before signature: a well-signed answer from a mover no Person
    // stands behind is still a refusal, and it should say so.
    const Identity::Standing standing = reg.standing(r.mover);
    if (standing != Identity::Standing::Recognized) {
        return refuse(Identity::standingCode(standing), reg.explainStanding(r.mover));
    }

    const auto signature = Identity::hexDecode(signatureHex);
    const auto transcript = Identity::foreignSessionTranscript(
        challenge.challengeId, challenge.nonce, challenge.connection, r.mover);
    const Identity::PublicKey pub = Identity::PublicKey::fromId(r.mover);
    if (signature.empty() || !pub.isValid() || !pub.verify(transcript, signature)) {
        return refuse("signature-invalid", "signature does not prove possession of this mover's key");
    }

    _bound[connection] = r.mover;
    r.ok = true;
    r.reasonCode = "authenticated";
    r.reason = "connection bound to First Mover";
    return r;
}

const Identity::SingularId* ForeignSessionAuthenticator::moverFor(const std::string& connection) const {
    auto it = _bound.find(connection);
    return it == _bound.end() ? nullptr : &it->second;
}

void ForeignSessionAuthenticator::drop(const std::string& connection) {
    _bound.erase(connection);
    for (auto it = _pending.begin(); it != _pending.end();) {
        it = (it->second.connection == connection) ? _pending.erase(it) : std::next(it);
    }
}

nlohmann::json ForeignActuationDecision::toJson() const {
    nlohmann::json j{{"status", allowed ? "authorized" : "refused"},
                     {"reasonCode", reasonCode},
                     {"reason", reason},
                     {"modality", modality}};
    if (!moverId.empty()) j["moverId"] = moverId;
    if (!resource.empty()) j["resource"] = resource;
    if (!property.empty()) j["property"] = property;
    return j;
}

ForeignActuationDecision authorizeForeignActuation(const Identity::FirstMoverRegister& reg,
                                                   const Identity::SingularId* mover,
                                                   const std::string& resource,
                                                   const std::string& property,
                                                   const std::string& modality,
                                                   const std::string& unmappedReason) {
    ForeignActuationDecision d;
    d.resource = resource;
    d.property = property;
    d.modality = modality;

    if (!mover || !mover->canAuthenticate()) {
        d.reasonCode = "no-first-mover-session";
        d.reason = "this connection has not authenticated as a First Mover; reads stay open, "
                   "but changing the world requires a Person-granted mover "
                   "(first_mover_challenge -> first_mover_authenticate)";
        return d;
    }
    d.moverId = mover->toString();

    // Re-checked on every act: a Person may have revoked the grant since the
    // connection authenticated.
    const Identity::Standing standing = reg.standing(*mover);
    if (standing != Identity::Standing::Recognized) {
        d.reasonCode = Identity::standingCode(standing);
        d.reason = reg.explainStanding(*mover);
        return d;
    }

    if (resource.empty()) {
        d.reasonCode = "unmapped-resource";
        d.reason = unmappedReason.empty()
            ? "Earthcall has no durable resource coordinate for this act yet, so it is refused rather than granted '**'"
            : unmappedReason;
        return d;
    }

    if (!reg.mayWrite(*mover, resource)) {
        d.reasonCode = "outside-scope";
        d.reason = reg.explain(*mover, resource);
        return d;
    }

    if (!property.empty() &&
        !TransferPolicy::instance().canTransfer(PropertyPath::parse(property))) {
        d.reasonCode = "transfer-policy-closed";
        d.reason = "TransferPolicy has this property's gate closed; a Law must open it";
        return d;
    }

    d.allowed = true;
    d.reasonCode = "authorized";
    d.reason = "First Mover stands and the act lies within its Person-granted scope";
    return d;
}

std::vector<Singular*> foreignLawAuthors(Identity::FirstMoverRegister& reg,
                                         const Identity::SingularId& mover) {
    if (reg.standing(mover) != Identity::Standing::Recognized) return {};
    Identity::FirstMover* m = reg.findMutable(mover);
    return m ? std::vector<Singular*>{m} : std::vector<Singular*>{};
}

} // namespace Foreign
} // namespace Singularity
