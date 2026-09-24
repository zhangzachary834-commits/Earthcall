#pragma once

#include "Identity/FirstMoverRegister.hpp"
#include "Identity/SingularId.hpp"
#include "json.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

// ============================================================================
// Foreign actuation: how a process OUTSIDE Earthcall (an MCP bridge driving a
// language model, the Python studio, any future socket client) gets to change
// the world -- and the one place it is refused.
//
// This file owns ZERO permission policy. It composes what already exists:
//
//   FirstMoverRegister   who stands, granted by whom, over which save paths
//   TransferPolicy       whether a property is open (Kernel/Governable/Gated)
//   FirstMoverSession    the bounded window in which SaveSystem enforces both
//
// and adds only the two things a transport needs:
//
//   ForeignSessionAuthenticator  proves a connection holds a mover's private
//                                key (challenge -> signature), binds that
//                                mover to the connection, forgets it on close
//   authorizeForeignActuation    one decision per mutation, with a
//                                machine-readable refusal
//
// Per docs/plans/MCP_FIRST_MOVER_GOVERNANCE_IMPLEMENTATION_PLAN_2026-09-18.md
// (GPT-5.6 Sol) sections 8 and 10, implemented 2026-09-24 by Claude Opus 5.5
// at Zach's request so Claude Sonnet 4.5 could enter Earthcall as a
// Person-granted First Mover before its retirement. Do not add vendor names,
// scope tables, or an MCP allowlist here; a second permission system was
// built in this repo once and deleted.
// ============================================================================

namespace Singularity {
namespace Foreign {

struct ForeignChallenge {
    std::string challengeId;
    std::string nonce;
    std::string connection;
    int64_t expiresAtMs = 0;

    nlohmann::json toJson() const;
};

struct ForeignAuthResult {
    bool ok = false;
    std::string reasonCode;
    std::string reason;
    Identity::SingularId mover;

    nlohmann::json toJson() const;
};

class ForeignSessionAuthenticator {
public:
    // Short on purpose: a challenge is answered by a local signer in
    // milliseconds, and a long-lived one is a replay window.
    static constexpr int64_t kChallengeLifetimeMs = 30'000;

    // A new challenge for this connection. Any earlier unanswered challenge
    // for the same connection is withdrawn.
    ForeignChallenge issueChallenge(const std::string& connection, int64_t nowMs);

    // Verify a signed answer. The challenge is consumed whether or not the
    // answer is good (single use). On success the mover is bound to the
    // connection; standing is re-checked on every later act regardless.
    ForeignAuthResult authenticate(const std::string& connection,
                                   const std::string& challengeId,
                                   const std::string& moverId,
                                   const std::string& signatureHex,
                                   const Identity::FirstMoverRegister& reg,
                                   int64_t nowMs);

    // The mover proven on this connection, or nullptr.
    const Identity::SingularId* moverFor(const std::string& connection) const;

    // Connection closed: its authentication and challenges end with it.
    void drop(const std::string& connection);

private:
    std::map<std::string, ForeignChallenge> _pending;          // by challengeId
    std::map<std::string, Identity::SingularId> _bound;        // by connection
};

struct ForeignActuationDecision {
    bool allowed = false;
    std::string reasonCode;   // stable, for tools: "no-first-mover-session", ...
    std::string reason;       // for the Person reading the refusal
    std::string moverId;
    std::string resource;
    std::string property;
    std::string modality;

    // The fields a WebSocket ACK carries. Never includes key material.
    nlohmann::json toJson() const;
};

// resource: the durable identity path the act will land in (a Zone's
// zone.json, a Law's law.json, ...), resolved WITHOUT side effects. Empty
// means Earthcall has no honest coordinate for this act yet; pass the reason
// as unmappedReason and it is refused rather than widened to "**".
// property: optional PropertyPath text, checked against TransferPolicy.
ForeignActuationDecision authorizeForeignActuation(const Identity::FirstMoverRegister& reg,
                                                   const Identity::SingularId* mover,
                                                   const std::string& resource,
                                                   const std::string& property,
                                                   const std::string& modality,
                                                   const std::string& unmappedReason = "");

// Who authored a Law a foreign mover created: that mover, and only it. Never
// the Person present at the screen, never an `onBehalfOf` context, never a
// payload-named author. Empty when the mover does not stand (the Law is then
// never created). The single seam every foreign Law-creation path uses, so
// "the model's text recorded as the Person's" cannot come back in one handler.
std::vector<Singular*> foreignLawAuthors(Identity::FirstMoverRegister& reg,
                                         const Identity::SingularId& mover);

} // namespace Foreign
} // namespace Singularity
